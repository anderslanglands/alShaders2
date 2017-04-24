#include <ai.h>
#include <string>
#include <map>
#include <algorithm>
#include <vector>
#include "filters.h"
#include <cstring>


///////////////////////////////////////////////
//
//    Filter node things
//
///////////////////////////////////////////////

AI_FILTER_NODE_EXPORT_METHODS(cryptomatte_filterMtd)

static const AtString ats_opacity("opacity");

enum cryptomatte_filterParams {
   p_width,
   p_rank,
   p_filter,
   p_mode,
};

struct CryptomatteFilterData {
   float (*filter_func)(AtVector2, float);
   float width;
   int rank;
   int filter;
};

enum modeEnum {
   p_mode_double_rgba,
};

static const char* modeEnumNames[] = {
   "double_rgba",
   NULL
};

node_parameters {
   AiMetaDataSetStr(nentry, NULL, "maya.attr_prefix", "filter_");
   AiMetaDataSetStr(nentry, NULL, "maya.translator", "cryptomatteFilter");
   AiMetaDataSetInt(nentry, NULL, "maya.id", 0x00116420);

   AiParameterFlt("width", 2.0);
   AiParameterInt("rank", 0);
   AiParameterEnum("filter", p_filter_gaussian, filterEnumNames);
   AiParameterEnum("mode", p_mode_double_rgba, modeEnumNames);
}

node_loader {
   if (i>0) return 0;
   node->methods     = cryptomatte_filterMtd;
   node->output_type = AI_TYPE_RGBA;
   node->name        = "cryptomatte_filter";
   node->node_type   = AI_NODE_FILTER;
   strcpy(node->version, AI_VERSION);
   return true;
}

node_initialize {
   // Z is still required despite the values themselves not being used. 
   static const char* necessary_aovs[] = {
      "FLOAT Z",
      "RGB opacity",
      NULL 
   };
    CryptomatteFilterData * data = new CryptomatteFilterData();
    AiNodeSetLocalData(node, data);
    AiFilterInitialize(node, true, necessary_aovs);
}

node_finish {
    CryptomatteFilterData * data = (CryptomatteFilterData*) AiNodeGetLocalData(node);
    delete data;
    AiNodeSetLocalData(node, NULL);
}

node_update {
   AtShaderGlobals shader_globals;
   AtShaderGlobals *sg = &shader_globals;

   CryptomatteFilterData * data = (CryptomatteFilterData*) AiNodeGetLocalData(node);
   data->width = AiNodeGetFlt(node, "width");
   data->rank = AiNodeGetInt(node, "rank");
   data->filter = AiNodeGetInt(node, "filter");

   switch (data->filter) {
      case p_filter_triangle:
         data->filter_func = &triangle;
         break;
      case p_filter_blackman_harris:
         data->filter_func = &blackman_harris;
         break;
      case p_filter_box:
         data->filter_func = &box;
         break;
      case p_filter_disk:
         data->filter_func = &disk;
         break;
      case p_filter_cone:
         data->filter_func = &cone;
         break;
      case p_filter_gaussian:
      default:
         data->filter_func = &gaussian;
         break;
   }

   if (data->filter == p_filter_box) {
      AiFilterUpdate(node, 1.0f);
   } else {
      AiFilterUpdate(node, data->width);
   }
    
}

filter_output_type {
   if (input_type == AI_TYPE_FLOAT)
      return AI_TYPE_RGBA;
   else
      return AI_TYPE_NONE;
}

///////////////////////////////////////////////
//
//    Sample-Weight map Class and type definitions
//
///////////////////////////////////////////////

class compareTail {
public:
   bool operator()(const std::pair<float, float> x, const std::pair<float, float> y) {
      return x.second > y.second;
   }
};

typedef std::map<float,float>           sw_map_t ;
typedef std::map<float,float>::iterator sw_map_iterator_t;

void write_to_samples_map(sw_map_t * vals, float hash, float sample_weight) {
   (*vals)[hash] += sample_weight;
}

///////////////////////////////////////////////
//
//    Filter proper
//
///////////////////////////////////////////////

filter_pixel {
   AtRGBA *out_value = (AtRGBA *)data_out;
   *out_value = AI_RGBA_ZERO;
   CryptomatteFilterData * data = (CryptomatteFilterData*)AiNodeGetLocalData(node);

   ///////////////////////////////////////////////
   //
   //    early out for black pixels
   //
   ///////////////////////////////////////////////
   
   bool early_out = true;

   while (AiAOVSampleIteratorGetNext(iterator)) {
      while (AiAOVSampleIteratorGetNextDepth(iterator)) {
         if (AiAOVSampleIteratorHasValue(iterator))   {
            early_out = false;   
            break;
         }
      }
      if (!early_out)
         break;
   }

   if (early_out) {
      if (data->rank == 0)
         out_value->g = 1.0f;
      return;
   }
   AiAOVSampleIteratorReset(iterator);

   ///////////////////////////////////////////////
   //
   //    Set up sample-weight maps and friends
   //
   ///////////////////////////////////////////////

   sw_map_t vals;
   float total_weight = 0.0f;

   ///////////////////////////////////////////////
   //
   //    Iterate samples
   //
   ///////////////////////////////////////////////
   while (AiAOVSampleIteratorGetNext(iterator)) {
      float sample_weight = data->filter_func(AiAOVSampleIteratorGetOffset(iterator), data->width);
      if (sample_weight == 0.0f)
         continue;

      float iterative_transparency_weight = 1.0f;
      float quota = sample_weight;
      float sample_value = 0.0f;
      total_weight += quota;

      while (AiAOVSampleIteratorGetNextDepth(iterator)) {
         const float sub_sample_opacity = AiColorToGrey(AiAOVSampleIteratorGetAOVRGB(iterator, ats_opacity));
         sample_value = AiAOVSampleIteratorGetFlt(iterator);
         const float sub_sample_weight = sub_sample_opacity * iterative_transparency_weight * sample_weight;

         // so if the current sub sample is 80% opaque, it means 20% of the weight will remain for the next subsample
         iterative_transparency_weight *= (1.0f - sub_sample_opacity); 

         quota -= sub_sample_weight;
         write_to_samples_map(&vals, sample_value, sub_sample_weight);
      }
      
      if (quota > 0.0) {
         // the remaining values gets allocated to the last sample 
         write_to_samples_map(&vals, sample_value, quota);
      }
   }


   ///////////////////////////////////////////////
   //
   //    Rank samples and make pixels
   //
   ///////////////////////////////////////////////

   // rank 0 means if vals.size() does not contain 0, we can stop
   // rank 2 means if vals.size() does not contain 2, we can stop
   if (vals.size() <= data->rank)
      return;

   sw_map_iterator_t vals_iter;
   
   std::vector<std::pair<float, float> > all_vals;
   std::vector<std::pair<float, float> >::iterator all_vals_iter;

   all_vals.reserve(vals.size());
   for (vals_iter = vals.begin(); vals_iter != vals.end(); ++vals_iter)
      all_vals.push_back(*vals_iter);
   
   std::sort(all_vals.begin(), all_vals.end(), compareTail());

   int iter = 0;
   for (all_vals_iter = all_vals.begin(); all_vals_iter != all_vals.end(); ++all_vals_iter) {
      if (iter == data->rank) {
         out_value->r = all_vals_iter->first;
         out_value->g = (all_vals_iter->second / total_weight);
      }
      else if (iter == data->rank + 1) {
         out_value->b = all_vals_iter->first;
         out_value->a = (all_vals_iter->second / total_weight);
         return;
      }

      iter++;
   }
}



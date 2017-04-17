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



enum cryptomatte_filterParams {
   p_width,
   p_rank,
   p_filter,
   p_mode,
};

struct CryptomatteFilterData {
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
   if (data->filter == p_filter_box) {
      AiFilterUpdate(node, 1.0f);
   } else {
      AiFilterUpdate(node, data->width);
   }
    
}

filter_output_type {
   if (input_type == AI_TYPE_VECTOR)
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

typedef std::map<float,float>           sw_map_type ;
typedef std::map<float,float>::iterator sw_map_iterator_type;

void write_to_samples_map(sw_map_type * vals, float hash, float sample_weight) {
   (*vals)[hash] += sample_weight;
}

///////////////////////////////////////////////
//
//    Filter proper
//
///////////////////////////////////////////////

filter_pixel { 
   AtShaderGlobals shader_globals;
   AtShaderGlobals *sg = &shader_globals;
   AtRGBA *out_value = (AtRGBA *)data_out;
   *out_value = AI_RGBA_ZERO;
    
    CryptomatteFilterData * data = (CryptomatteFilterData*)AiNodeGetLocalData(node);

   AtNode * renderOptions = AiUniverseGetOptions();
   int auto_transparency_depth = AiNodeGetInt(renderOptions, "auto_transparency_depth");

   AtNode * camera = AiUniverseGetCamera();
   float camera_far_clip = AiNodeGetFlt(camera, "far_clip");
   
   float (*filter)(AtVector2, float);


   ///////////////////////////////////////////////
   //
   //    early out for black pixels
   //
   ///////////////////////////////////////////////
   
   bool early_out = true;

   while (AiAOVSampleIteratorGetNext(iterator)) {

      if (AiAOVSampleIteratorHasValue(iterator))   {
         early_out = false;   
         break;
      }

   }

   if (early_out) {
      *out_value = AI_RGBA_ZERO;
      if (data->rank == 0) {
         out_value->g = 1.0f;             
      }
      return;
   }
   AiAOVSampleIteratorReset(iterator);


   ///////////////////////////////////////////////
   //
   //    Select Filter
   //
   ///////////////////////////////////////////////

   switch (data->filter) {
      case p_filter_triangle:
         filter = &triangle;
         break;
      case p_filter_blackman_harris:
         filter = &blackman_harris;
         break;
      case p_filter_box:
         filter = &box;
         break;
      case p_filter_disk:
         filter = &disk;
         break;
      case p_filter_cone:
         filter = &cone;
         break;
      case p_filter_gaussian:
      default:
         filter = &gaussian;
         break;
   }


   ///////////////////////////////////////////////
   //
   //    Set up sample-weight maps and friends
   //
   ///////////////////////////////////////////////

   sw_map_type vals;


   int total_samples = 0;
   float total_weight = 0.0f;
   float sample_weight;
   AtVector2 offset;
   static const AtString zAOV("Z");

   ///////////////////////////////////////////////
   //
   //    Iterate samples
   //
   ///////////////////////////////////////////////
   while (AiAOVSampleIteratorGetNext(iterator)) {
      offset = AiAOVSampleIteratorGetOffset(iterator);
      sample_weight = filter(offset, data->width);

      if (sample_weight == 0.0f) {
         continue;
      }
      total_samples++;

      ///////////////////////////////////////////////
      //
      //    Samples with no value.
      //
      ///////////////////////////////////////////////

      if (!AiAOVSampleIteratorHasValue(iterator))  {        
         total_weight += sample_weight;
         write_to_samples_map(&vals, 0.0f, sample_weight);
         continue;
      }


      ///////////////////////////////////////////////
      //
      //    Samples with values, depth or no depth
      //
      ///////////////////////////////////////////////
      
      float quota = sample_weight;
      float iterative_transparency_weight = 1.0f;
      int depth = 0;
      AtVector sample_value = AI_V3_ZERO;

      total_weight += quota;
      while (AiAOVSampleIteratorGetNextDepth(iterator)) {
         // sample.x is the ID
         // sample.y is unused
         // sample.z is the opacity
         sample_value = AiAOVSampleIteratorGetVec(iterator);
         const float sub_sample_opacity = sample_value.z;
         const float sub_sample_weight = sub_sample_opacity * iterative_transparency_weight * sample_weight;

         // so if the current sub sample is 80% opaque, it means 20% of the weight will remain for the next subsample
         iterative_transparency_weight *= (1.0f - sub_sample_opacity); 

         quota -= sub_sample_weight;
         write_to_samples_map(&vals, sample_value.x, sub_sample_weight);
      }
      
      if (quota != 0.0) {
         // the remaining values gets allocated to the last sample 
         write_to_samples_map(&vals, sample_value.x, quota);
      }
   }


   ///////////////////////////////////////////////
   //
   //    Rank samples and make pixels
   //
   ///////////////////////////////////////////////

   if (total_samples == 0) {
      return;
   }

   sw_map_iterator_type vals_iter;
   
   std::vector<std::pair<float, float> > all_vals;
   std::vector<std::pair<float, float> >::iterator all_vals_iter;

   for (vals_iter = vals.begin(); vals_iter != vals.end(); ++vals_iter) {
      all_vals.push_back(*vals_iter);
   }
   
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



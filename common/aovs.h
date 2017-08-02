#pragma once
// aovs.h

#define NUM_LIGHT_GROUPS 16
#define NUM_AOVs 41
#define NUM_AOVs_RGBA 16

#define REGISTER_AOVS \
data->aovs.clear(); \
data->aovs.push_back(params[p_aov_diffuse_color].STR); \
data->aovs.push_back(params[p_aov_direct_diffuse].STR); \
data->aovs.push_back(params[p_aov_direct_diffuse_raw].STR); \
data->aovs.push_back(params[p_aov_indirect_diffuse].STR); \
data->aovs.push_back(params[p_aov_indirect_diffuse_raw].STR); \
data->aovs.push_back(params[p_aov_direct_backlight].STR); \
data->aovs.push_back(params[p_aov_indirect_backlight].STR); \
data->aovs.push_back(params[p_aov_direct_specular].STR); \
data->aovs.push_back(params[p_aov_indirect_specular].STR); \
data->aovs.push_back(params[p_aov_direct_specular_2].STR); \
data->aovs.push_back(params[p_aov_indirect_specular_2].STR); \
data->aovs.push_back(params[p_aov_single_scatter].STR); \
data->aovs.push_back(params[p_aov_sss].STR); \
data->aovs.push_back(params[p_aov_refraction].STR); \
data->aovs.push_back(params[p_aov_emission].STR); \
data->aovs.push_back(params[p_aov_uv].STR); \
data->aovs.push_back(params[p_aov_depth].STR); \
data->aovs.push_back(params[p_aov_light_group_1].STR); \
data->aovs.push_back(params[p_aov_light_group_2].STR); \
data->aovs.push_back(params[p_aov_light_group_3].STR); \
data->aovs.push_back(params[p_aov_light_group_4].STR); \
data->aovs.push_back(params[p_aov_light_group_5].STR); \
data->aovs.push_back(params[p_aov_light_group_6].STR); \
data->aovs.push_back(params[p_aov_light_group_7].STR); \
data->aovs.push_back(params[p_aov_light_group_8].STR); \
data->aovs.push_back(params[p_aov_light_group_9].STR); \
data->aovs.push_back(params[p_aov_light_group_10].STR); \
data->aovs.push_back(params[p_aov_light_group_11].STR); \
data->aovs.push_back(params[p_aov_light_group_12].STR); \
data->aovs.push_back(params[p_aov_light_group_13].STR); \
data->aovs.push_back(params[p_aov_light_group_14].STR); \
data->aovs.push_back(params[p_aov_light_group_15].STR); \
data->aovs.push_back(params[p_aov_light_group_16].STR); \
data->aovs.push_back(params[p_aov_id_1].STR); \
data->aovs.push_back(params[p_aov_id_2].STR); \
data->aovs.push_back(params[p_aov_id_3].STR); \
data->aovs.push_back(params[p_aov_id_4].STR); \
data->aovs.push_back(params[p_aov_id_5].STR); \
data->aovs.push_back(params[p_aov_id_6].STR); \
data->aovs.push_back(params[p_aov_id_7].STR); \
data->aovs.push_back(params[p_aov_id_8].STR); \
assert(NUM_AOVs == data->aovs.size() && "NUM_AOVs does not match size of aovs array!"); \
for (size_t i=0; i < data->aovs.size(); ++i) \
    	AiAOVRegister(data->aovs[i].c_str(), AI_TYPE_RGB, AI_AOV_BLEND_OPACITY); \
data->aovs_rgba.clear(); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_1].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_2].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_3].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_4].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_5].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_6].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_7].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_8].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_9].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_10].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_11].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_12].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_13].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_14].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_15].STR); \
data->aovs_rgba.push_back(params[p_aov_shadow_group_16].STR); \
assert(NUM_AOVs_RGBA == data->aovs_rgba.size() && "NUM_AOVs_RGBA does not match size of aovs_rgba array!"); \
for (size_t i=0; i < data->aovs_rgba.size(); ++i) \
        AiAOVRegister(data->aovs_rgba[i].c_str(), AI_TYPE_RGBA, AI_AOV_BLEND_OPACITY); \

enum AovIndices
{
	k_diffuse_color = 0,
    k_direct_diffuse,
    k_direct_diffuse_raw,
    k_indirect_diffuse,
    k_indirect_diffuse_raw,
    k_direct_backlight,
    k_indirect_backlight,
    k_direct_specular,
    k_indirect_specular,
    k_direct_specular_2,
    k_indirect_specular_2,
    k_single_scatter,
    k_sss,
    k_refraction,
    k_emission,
    k_uv,
    k_depth,
    k_light_group_1,
    k_light_group_2,
    k_light_group_3,
    k_light_group_4,
    k_light_group_5,
    k_light_group_6,
    k_light_group_7,
    k_light_group_8,
    k_light_group_9,
    k_light_group_10,
    k_light_group_11,
    k_light_group_12,
    k_light_group_13,
    k_light_group_14,
    k_light_group_15,
    k_light_group_16,
    k_id_1,
    k_id_2,
    k_id_3,
    k_id_4,
    k_id_5,
    k_id_6,
    k_id_7,
    k_id_8
};

enum AovRGBIndices
{
    k_shadow_group_1=0,
    k_shadow_group_2,
    k_shadow_group_3,
    k_shadow_group_4,
    k_shadow_group_5,
    k_shadow_group_6,
    k_shadow_group_7,
    k_shadow_group_8,
    k_shadow_group_9,
    k_shadow_group_10,
    k_shadow_group_11,
    k_shadow_group_12,
    k_shadow_group_13,
    k_shadow_group_14,
    k_shadow_group_15,
    k_shadow_group_16
};

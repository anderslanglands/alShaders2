#ifndef COMMON_WRITE_IDS_H
#define COMMON_WRITE_IDS_H

#include <ai.h>

namespace WriteIds
{
    const AtString nameParamName("name");
    const AtString shaderParamName("shader");
    const AtString filenameParamName("filename");

    enum AOVsToWrite
    {
        WI_AOV_STR_INDEX = 0,
        WI_AOV_FLT1_INDEX = 1,
        WI_AOV_FLT2_INDEX = 2,

        WI_NUM_STR_AOVS = 1,
        WI_NUM_FLT_AOVS = 2,
        WI_NUM_AOVS = WI_NUM_STR_AOVS + WI_NUM_FLT_AOVS
    };

    const char* aov_paramNameCStrs[WI_NUM_AOVS] = { "aov_string", "aov_idFloat1", "aov_idFloat2" };

    const AtString aov_paramNames[WI_NUM_AOVS] =
    {
        AtString(aov_paramNameCStrs[WI_AOV_STR_INDEX]),
        AtString(aov_paramNameCStrs[WI_AOV_FLT1_INDEX]),
        AtString(aov_paramNameCStrs[WI_AOV_FLT2_INDEX])
    };

    const AtByte aov_types[WI_NUM_AOVS] =
    {
        AI_TYPE_POINTER,
        AI_TYPE_FLOAT,
        AI_TYPE_FLOAT
    };

    // Default AOV name values (Nuke uses "<layer>.<channel>" for all channel names and it doesn't allow dots in the "<layer>" part)
    const char* aov_namesDefaultValues[WI_NUM_AOVS] = {"id.shapeName", "id.shapeName1", "id.shapeName2"};

    enum StringOptions
    {
        WI_SHAPE_NAME = 0,
        WI_PROCEDURAL_NAME = 1,
        WI_SHADER_NAME = 2,
        WI_NUM_STR_OPTIONS = 3
    };

    const char* stringOptionsNames[WI_NUM_STR_OPTIONS+1] =
    {
        "Shape Node Name",
        "Procedural Node Name",
        "Shader Node Name",
        NULL
    };
}

#endif // COMMON_WRITE_IDS_H

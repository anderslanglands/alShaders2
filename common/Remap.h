#pragma once

#include <alUtil.h>
#include <ai.h>

#define REMAP_FLOAT_PARAM_ENUM 	\
	p_RMPinputMin,					\
	p_RMPinputMax,					\
	p_RMPcontrast,					\
	p_RMPcontrastPivot,			\
	p_RMPbiasVal,						\
	p_RMPgainVal,						\
	p_RMPoutputMin,				\
	p_RMPoutputMax,				\
	p_RMPclampEnable,				\
	p_RMPthreshold,				\
	p_RMPclampMin,					\
	p_RMPclampMax					\

#define REMAP_FLOAT_PARAM_DECLARE 				\
	AiParameterFLT("RMPinputMin", 0.0f);			\
	AiParameterFLT("RMPinputMax", 1.0f);			\
	AiParameterFLT("RMPcontrast", 1.0f);			\
	AiParameterFLT("RMPcontrastPivot", 0.5f);		\
	AiParameterFLT("RMPbias", 0.5f);				\
	AiParameterFLT("RMPgain", 0.5f);				\
	AiParameterFLT("RMPoutputMin", 0.0f);			\
	AiParameterFLT("RMPoutputMax", 1.0f);			\
    AiParameterBOOL("RMPclampEnable", true);		\
	AiParameterBOOL("RMPthreshold", false);		\
	AiParameterFLT("RMPclampMin", 0.0f);			\
	AiParameterFLT("RMPclampMax", 1.0f);			\

struct RemapFloat
{
public:
	RemapFloat(float imn, float imx, float ct, float ctp, float bs, float gn, float omn, float omx,
				bool ce, bool t, float cmn, float cmx) :
		inputMin(imn),
		inputMax(imx),
		contrastVal(ct),
		contrastPivot(ctp),
		bias(bs),
		gain(gn),
		outputMin(omn),
		outputMax(omx),
		clampEnable(ce),
		threshold(t),
		clampMin(cmn),
		clampMax(cmx)
	{}

	float remap(float input)
	{
		float f = (input-inputMin)/(inputMax-inputMin);
		f = contrast(f, contrastVal, contrastPivot);
		f = biasandgain(f, bias, gain);
		f = lerp(outputMin, outputMax, f);
		if (clampEnable)
		{
			f = std::min(clampMax, f);
			f = std::max(clampMin, f);
			if (threshold)
			{
				f = (f-clampMin)/(clampMax-clampMin);
			}
		}
		return f;
	}

	float inputMin;
	float inputMax;
	float contrastVal;
	float contrastPivot;
	float bias;
	float gain;
	float outputMin;
	float outputMax;
	bool clampEnable;
	bool threshold;
	float clampMin;
	float clampMax;
};

#define REMAP_FLOAT_CREATE							\
	RemapFloat( 									\
		AiShaderEvalParamFlt(p_RMPinputMin), 			\
		AiShaderEvalParamFlt(p_RMPinputMax),			\
		AiShaderEvalParamFlt(p_RMPcontrast),			\
		AiShaderEvalParamFlt(p_RMPcontrastPivot),		\
		AiShaderEvalParamFlt(p_RMPbiasVal),				\
		AiShaderEvalParamFlt(p_RMPgainVal),				\
		AiShaderEvalParamFlt(p_RMPoutputMin),			\
		AiShaderEvalParamFlt(p_RMPoutputMax),			\
		AiShaderEvalParamBool(p_RMPclampEnable),		\
		AiShaderEvalParamBool(p_RMPthreshold),		\
		AiShaderEvalParamFlt(p_RMPclampMin),			\
		AiShaderEvalParamFlt(p_RMPclampMax)			\
	)

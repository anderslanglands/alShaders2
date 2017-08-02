#pragma once
#include <ai.h>
#include "alUtil.h"

class Fresnel
{
public:
	virtual ~Fresnel(){};
	virtual AtRGB kr(float cos_theta)=0;

	float _eta;
};

class FresnelDielectric : public Fresnel
{
public:
	FresnelDielectric() {_eta = 0.6f;}
	FresnelDielectric(float eta) { _eta = eta; }
	virtual ~FresnelDielectric(){}

	virtual AtRGB kr(float cos_theta)
	{
		return rgb(fresnel(cos_theta, _eta));
	}
};

inline float frcond(float cosi, const float eta, const float k)
{
    float tmp = (eta*eta + k*k) * cosi*cosi;
    float Rparl2 = (tmp - (2.f * eta * cosi) + 1) /
                      (tmp + (2.f * eta * cosi) + 1);
    float tmp_f = eta*eta + k*k;
    float Rperp2 =
        (tmp_f - (2.f * eta * cosi) + cosi*cosi) /
        (tmp_f + (2.f * eta * cosi) + cosi*cosi);
    return (Rparl2 + Rperp2) / 2.f;
}

#define FRCOND_STEPS 90
class FresnelConductor : public Fresnel
{
public:
   FresnelConductor(int material, const AtRGB& r, const AtRGB& g) {setMaterial(material, r, g);}
	virtual ~FresnelConductor(){}
	void setMaterial(int material, const AtRGB& r, const AtRGB& g);
	virtual AtRGB kr(float cos_theta);

	void generateTable(const AtRGB& r, const AtRGB& g);
	AtRGB _r;
   AtRGB _g;
};

enum ConductorMaterial
{
	kAluminium=0,
	kChrome,
	kCopper,
	kGold,
	kSilver,
	kPlatinum,
	kTitanium,
	kTungsten,
	kCustom	
};
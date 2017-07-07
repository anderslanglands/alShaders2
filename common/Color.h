#pragma once

#include <ai.h>
#include <string>

#include "alUtil.h"

/// A structure representing a color coordinate system
struct ColorSystem
{
	ColorSystem(const std::string& name_, const AtPoint2& r, const AtPoint2& g, const AtPoint2& b, const AtPoint2& w):
		name(name_), red(r), green(g), blue(b), white(w)
	{}

	std::string name;	/// name of the system, e.g. rec709, ACES etc
	AtPoint2 red;		/// chromaticity of the red primary
	AtPoint2 green;		/// chromaticity of the green primary
	AtPoint2 blue;		/// chromticity of the blue primary
	AtPoint2 white;		/// chromaticity of the white point
};

/// White points for common illuminants
extern  AtPoint2 illumC;
extern  AtPoint2 illumD65;
extern  AtPoint2 illumE;

/// Color system definitions for common color spaces
extern  ColorSystem CsNTSC;
extern  ColorSystem CsSMPTE;
extern  ColorSystem CsHDTV;
extern  ColorSystem CsCIE;
extern  ColorSystem CsRec709;

/// CIE spectral color matching functions from 380 to 780 nm, in 5nm increments
extern float cieMatch[81][3];

/// Function to convert a spectral intensity to CIE XYZ. The template argument SpecFunc should be a functor that
/// descrbes the desired spectral function. The functor should have the member:
/// float operator(const float wavelength)
/// which should return the spectral intensity of the function at the given wavelength
template <class SpecFunc>
AtColor spectrumToXyz(const SpecFunc& sf)
{
	AtColor result;
	float lambda = 380.0f;
	float X=0.0f, Y=0.0f, Z=0.0f;
	for (int i = 0; lambda < 780.1; i++, lambda += 5)
	{
		double Me;

		Me = sf(lambda);
		X += Me * cieMatch[i][0];
		Y += Me * cieMatch[i][1];
		Z += Me * cieMatch[i][2];
	}
	float XYZ = (X + Y + Z);

	result.r = X / XYZ ;
	result.g = Y / XYZ ;
	result.b = Z / XYZ ;

	return result;
}

/// Function to convert an XYZ color to RGB
AtRGB xyzToRgb(const ColorSystem& cs, const AtColor& xyz);

/// Function to convert sRGB to linear
AtRGB sRgbToLin(const AtRGB& c);

/// Function to convert Cineon to linear
AtRGB cineonToLin(const AtRGB& c);

/// Function to convert ARRI LogC to linear
AtRGB logCToLin(const AtRGB& c);

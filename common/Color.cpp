#include "Color.h"

inline AtPoint2 point2(float x, float y)
{
	AtPoint2 r;
	r.x = x;
	r.y = y;
	return r;
}

AtPoint2 illumC(point2(0.3101, 0.3162));
AtPoint2 illumD65(point2(0.3127, 0.3291));
AtPoint2 illumE(point2(0.33333333, 0.33333333));

ColorSystem CsNTSC("NTSC", point2(0.67, 0.33), point2(0.21, 0.71), point2(0.14, 0.08), illumC);
ColorSystem CsSMPTE("SMPTE", point2(0.630, 0.340), point2(0.31, 0.595), point2(0.155, 0.07), illumD65);
ColorSystem CsHDTV("HDTV", point2(0.670, 0.330), point2(0.21, 0.71), point2(0.150, 0.06), illumD65);
ColorSystem CsCIE("CIE", point2(0.7355, 0.2645), point2(0.2658, 0.7243), point2(0.1669, 0.085), illumE);
ColorSystem CsRec709("Rec709", point2(0.64, 0.33), point2(0.30, 0.60), point2(0.15, 0.06), illumD65);


float cieMatch[81][3] =
{
	{0.0014,0.0000,0.0065}, {0.0022,0.0001,0.0105}, {0.0042,0.0001,0.0201},
	{0.0076,0.0002,0.0362}, {0.0143,0.0004,0.0679}, {0.0232,0.0006,0.1102},
	{0.0435,0.0012,0.2074}, {0.0776,0.0022,0.3713}, {0.1344,0.0040,0.6456},
	{0.2148,0.0073,1.0391}, {0.2839,0.0116,1.3856}, {0.3285,0.0168,1.6230},
	{0.3483,0.0230,1.7471}, {0.3481,0.0298,1.7826}, {0.3362,0.0380,1.7721},
	{0.3187,0.0480,1.7441}, {0.2908,0.0600,1.6692}, {0.2511,0.0739,1.5281},
	{0.1954,0.0910,1.2876}, {0.1421,0.1126,1.0419}, {0.0956,0.1390,0.8130},
	{0.0580,0.1693,0.6162}, {0.0320,0.2080,0.4652}, {0.0147,0.2586,0.3533},
	{0.0049,0.3230,0.2720}, {0.0024,0.4073,0.2123}, {0.0093,0.5030,0.1582},
	{0.0291,0.6082,0.1117}, {0.0633,0.7100,0.0782}, {0.1096,0.7932,0.0573},
	{0.1655,0.8620,0.0422}, {0.2257,0.9149,0.0298}, {0.2904,0.9540,0.0203},
	{0.3597,0.9803,0.0134}, {0.4334,0.9950,0.0087}, {0.5121,1.0000,0.0057},
	{0.5945,0.9950,0.0039}, {0.6784,0.9786,0.0027}, {0.7621,0.9520,0.0021},
	{0.8425,0.9154,0.0018}, {0.9163,0.8700,0.0017}, {0.9786,0.8163,0.0014},
	{1.0263,0.7570,0.0011}, {1.0567,0.6949,0.0010}, {1.0622,0.6310,0.0008},
	{1.0456,0.5668,0.0006}, {1.0026,0.5030,0.0003}, {0.9384,0.4412,0.0002},
	{0.8544,0.3810,0.0002}, {0.7514,0.3210,0.0001}, {0.6424,0.2650,0.0000},
	{0.5419,0.2170,0.0000}, {0.4479,0.1750,0.0000}, {0.3608,0.1382,0.0000},
	{0.2835,0.1070,0.0000}, {0.2187,0.0816,0.0000}, {0.1649,0.0610,0.0000},
	{0.1212,0.0446,0.0000}, {0.0874,0.0320,0.0000}, {0.0636,0.0232,0.0000},
	{0.0468,0.0170,0.0000}, {0.0329,0.0119,0.0000}, {0.0227,0.0082,0.0000},
	{0.0158,0.0057,0.0000}, {0.0114,0.0041,0.0000}, {0.0081,0.0029,0.0000},
	{0.0058,0.0021,0.0000}, {0.0041,0.0015,0.0000}, {0.0029,0.0010,0.0000},
	{0.0020,0.0007,0.0000}, {0.0014,0.0005,0.0000}, {0.0010,0.0004,0.0000},
	{0.0007,0.0002,0.0000}, {0.0005,0.0002,0.0000}, {0.0003,0.0001,0.0000},
	{0.0002,0.0001,0.0000}, {0.0002,0.0001,0.0000}, {0.0001,0.0000,0.0000},
	{0.0001,0.0000,0.0000}, {0.0001,0.0000,0.0000}, {0.0000,0.0000,0.0000}
};

AtRGB xyzToRgb(const ColorSystem& cs, const AtColor& xyz)
{
    float xr, yr, zr, xg, yg, zg, xb, yb, zb;
    float xw, yw, zw;
    float rx, ry, rz, gx, gy, gz, bx, by, bz;
    float rw, gw, bw;
    AtRGB rgb;

    xr = cs.red.x;    yr = cs.red.y;    zr = 1 - (xr + yr);
    xg = cs.green.x;  yg = cs.green.y;  zg = 1 - (xg + yg);
    xb = cs.blue.x;   yb = cs.blue.y;   zb = 1 - (xb + yb);

    xw = cs.white.x;  yw = cs.white.y;  zw = 1 - (xw + yw);

    // xyz -> rgb matrix, before scaling to white
    rx = (yg * zb) - (yb * zg);  ry = (xb * zg) - (xg * zb);  rz = (xg * yb) - (xb * yg);
    gx = (yb * zr) - (yr * zb);  gy = (xr * zb) - (xb * zr);  gz = (xb * yr) - (xr * yb);
    bx = (yr * zg) - (yg * zr);  by = (xg * zr) - (xr * zg);  bz = (xr * yg) - (xg * yr);

    // White scaling factors.
    // Dividing by yw scales the white luminance to unity, as conventional
    rw = ((rx * xw) + (ry * yw) + (rz * zw)) / yw;
    gw = ((gx * xw) + (gy * yw) + (gz * zw)) / yw;
    bw = ((bx * xw) + (by * yw) + (bz * zw)) / yw;

    // xyz -> rgb matrix, correctly scaled to white
    rx = rx / rw;  ry = ry / rw;  rz = rz / rw;
    gx = gx / gw;  gy = gy / gw;  gz = gz / gw;
    bx = bx / bw;  by = by / bw;  bz = bz / bw;

    // rgb of the desired point
    rgb.r = (rx * xyz.r) + (ry * xyz.g) + (rz * xyz.b);
    rgb.g = (gx * xyz.r) + (gy * xyz.g) + (gz * xyz.b);
    rgb.b = (bx * xyz.r) + (by * xyz.g) + (bz * xyz.b);

    return rgb;
}

/// Function to convert sRGB to linear
float sRgbToLin(float c)
{
	if (c > 1.0f) return c;
	else if (c <= 0.04045) return c / 12.92f;
	else return powf((c + 0.055f) / 1.055f, 2.4f);
}

AtRGB sRgbToLin(const AtRGB& c)
{
	return rgb(sRgbToLin(c.r), sRgbToLin(c.g), sRgbToLin(c.b));
}

/// Function to convert Cineon to linear
float cineonToLin(float c)
{
	//return powf(10.0f, (c*1023.0f - 445.0f)*0.002/0.6f) * 0.18f;
	static const float RefWhite = 685.0f;
	static const float RefBlack = 95.f;
	c *= 1023.0f;
	static const float Gain = 1.0f / (1.0f - powf(10.0f, (RefBlack-RefWhite)*0.002/0.6));
	static const float Offset = Gain - 1.0f;
	c = powf(10.0f, (c-RefWhite)*0.002/0.6) * Gain - Offset;
	return std::max(0.0f, c);
}

AtRGB cineonToLin(const AtRGB& c)
{
	return rgb(cineonToLin(c.r), cineonToLin(c.g), cineonToLin(c.b));
}

float logCToLin(float c)
{
	if (c > 0.1496582f) return (powf(10.0f, (c - 0.385537f) / 0.2471896f) - 0.052272f) / 5.555556f;
	else return (c - 0.092809f) / 5.367655f;
}

AtRGB logCToLin(const AtRGB& c)
{
	return rgb(logCToLin(c.r), logCToLin(c.g), logCToLin(c.b));
}
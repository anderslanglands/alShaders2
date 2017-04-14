#include <ai.h>
#include <string>
#include <map>
#include <algorithm>
#include <vector>


enum filterEnum
{
	p_filter_gaussian=0,
	p_filter_blackman_harris,
	p_filter_triangle,
	p_filter_box,
	p_filter_disk,
	p_filter_cone
};

static const char* filterEnumNames[] =
{
	"gaussian",
	"blackman_harris",
	"triangle",
	"box",
	"disk",
	"cone",
	NULL
};


float gaussian(AtPoint2 p, float width) {
	p /= (width * 0.5f);
	float dist_squared = (p.x * p.x + p.y * p.y) ;
	if (dist_squared >  (1.0f)) {
		return 0.0f;
	}
	float weight = expf(-dist_squared * 2.0f); // was: 

	if (weight > 0.0f) {
		return weight;
	} else {
		return 0.0f;
	}
}


float blackman_harris(AtPoint2 p, float width) {
	// Close to matching Arnolds, but not exact. 
	p /= (width * 0.5f);

	float dist_squared = (p.x * p.x + p.y * p.y) ;
	if (dist_squared >=  (1.0f)) {
		return 0.0f;
	}
	float x = sqrt(dist_squared);

	float a0 = 0.35875f;
	float a1 = 0.48829f;
	float a2 = 0.14128f;
	float a3 = 0.01168f;

	float weight  = a0 + a1*cos(1.0f * AI_PI * x) + a2*cos(2.0f * AI_PI * x) + a3*cos(4.0f * AI_PI * x);

	return weight;
}


float box(AtPoint2 p, float width) {
	// The trick with matching arnold's filter here is making sure you give a value of 1.0 in the filter update . 
	return 1.0f;
}

float box_strict(AtPoint2 p, float width) {
	// The trick with matching arnold's filter here is making sure you give a value of 1.0 in the filter update.
	if (std::abs(p.x) > 1.0 || std::abs(p.y) > 1.0)
		return 1.0f;
	else
		return 0.0f;
}


float triangle(AtPoint2 p, float width) {
	// Still does not match arnold's
	p /= (width * 0.5f);
	float weight = std::abs(p.x) + std::abs(p.y);
	return 2.0f - weight;
}


float disk(AtPoint2 p, float width) {
	// Is now extremely close to arnold's

	p /= (width * 0.5f);
	float dist_squared = (p.x * p.x + p.y * p.y) ;
	if (dist_squared > (1.0f)) {
		return 0.0f;
	} else {
		return 1.0f;
	}
}


float cone(AtPoint2 p, float width) {
	// Is now extremely close to arnold's

	p /= (width * 0.5f);
	float distance = sqrt(p.x * p.x + p.y * p.y) ;
	if (distance > (1.0f)) {
		return 0.0f;
	} else {
		return 1.0f - distance;
	}

}



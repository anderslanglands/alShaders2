#pragma once

void writeRGBEXR(const char* filename, const float* pixels, int w, int h);

// pads a 1D lut to be a bit thicker for easier viewing
void writeThickRGBEXR(const char* filename, const float* pixels, int w, int h);

void writeFloatEXR(const char* filename, const float* pixels, int w, int h);

void writeThickFloatEXR(const char* filename, const float* pixels, int w);
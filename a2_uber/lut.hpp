#pragma once

#include "common/util.hpp"
#include <functional>
#include <vector>

namespace a2 {

struct Dimension {
    Dimension(int resolution, float start, float end, bool inclusive)
        : resolution(resolution), start(start), end(end),
          inv_range(1.0f / (end - start)), inclusive(inclusive) {}
    int resolution;
    float start;
    float end;
    float inv_range;
    bool inclusive;
};

template <typename T> class LUT1D {
public:
    std::vector<T> _data;
    Dimension _dim_x;

    LUT1D(const std::vector<T>& data, const Dimension& dim_x)
        : _data(data), _dim_x(dim_x) {}

    LUT1D(std::function<T(int, int)> fun, const Dimension& dim_x)
        : _dim_x(dim_x) {
        _data.resize(_dim_x.resolution);
        for (int i = 0; i < _dim_x.resolution; ++i) {
            _data[i] = fun(i, _dim_x.resolution);
        }
    }

    auto lookup(float x) -> T {
        float xx = clamp((x - _dim_x.start) * _dim_x.inv_range, 0.0f, 1.0f);
        int inc_x = _dim_x.inclusive ? 1 : 0;
        xx *= (_dim_x.resolution - inc_x);
        float fx = floorf(xx);
        float dx = xx - fx;
        int ix0 = int(fx);
        int ix1 = ix0 + 1;
        ix0 = clamp(ix0, 0, _dim_x.resolution - 1);
        ix1 = clamp(ix1, 0, _dim_x.resolution - 1);
        float a = _data[ix0];
        float b = _data[ix1];
        return lerp(a, b, dx);
    }
};

template <typename T> class LUT2D {
public:
    std::vector<T> _data;
    Dimension _dim_x;
    Dimension _dim_y;

    LUT2D(const std::vector<T>& data, const Dimension& dim_x,
          const Dimension& dim_y)
        : _data(data), _dim_x(dim_x), _dim_y(dim_y) {}

    LUT2D(std::function<T(int, int, int, int)> fun, const Dimension& dim_x,
          const Dimension& dim_y)
        : _dim_x(dim_x), _dim_y(dim_y) {
        _data.resize(_dim_x.resolution * _dim_y.resolution);
        for (int j = 0; j < _dim_y.resolution; ++j) {
            for (int i = 0; i < _dim_x.resolution; ++i) {
                int idx = j * _dim_x.resolution + i;
                _data[idx] = fun(i, j, _dim_x.resolution, _dim_y.resolution);
            }
        }
    }

    auto lookup(float x, float y) -> T {
        float xx = clamp((x - _dim_x.start) * _dim_x.inv_range, 0.0f, 1.0f);
        float yy = clamp((y - _dim_y.start) * _dim_y.inv_range, 0.0f, 1.0f);
        int inc_x = _dim_x.inclusive ? 1 : 0;
        int inc_y = _dim_y.inclusive ? 1 : 0;
        xx *= (_dim_x.resolution - inc_x);
        float fx = floorf(xx);
        float dx = xx - fx;
        int ix0 = int(fx);
        int ix1 = ix0 + 1;
        yy *= (_dim_y.resolution - inc_y);
        float fy = floorf(yy);
        float dy = yy - fy;
        int iy0 = int(fy);
        int iy1 = iy0 + 1;
        ix0 = clamp(ix0, 0, _dim_x.resolution - 1);
        iy0 = clamp(iy0, 0, _dim_y.resolution - 1);
        ix1 = clamp(ix1, 0, _dim_x.resolution - 1);
        iy1 = clamp(iy1, 0, _dim_y.resolution - 1);
        float a = _data[iy0 * _dim_x.resolution + ix0];
        float b = _data[iy0 * _dim_x.resolution + ix1];
        float c = _data[iy1 * _dim_x.resolution + ix0];
        float d = _data[iy1 * _dim_x.resolution + ix1];
        float A = lerp(a, b, dx);
        float B = lerp(c, d, dx);
        return lerp(A, B, dy);
    }
};
}

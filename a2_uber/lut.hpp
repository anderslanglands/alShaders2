#pragma once

#include "common/a2_assert.hpp"
#include "common/util.hpp"
#include <functional>
#include <spdlog/fmt/ostr.h>
#include <thread>
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

        auto thread_fun = [=](int j0, int j1) {
            fmt::print("[{}:{}]\n", j0, j1);
            for (int j = j0; j < j1; ++j) {
                for (int i = 0; i < dim_x.resolution; ++i) {
                    int idx = j * dim_x.resolution + i;
                    this->_data[idx] =
                        fun(i, j, dim_x.resolution, dim_y.resolution);
                }
            }
        };
        std::vector<std::thread> thread_grp;
        auto n_threads = std::thread::hardware_concurrency();
        fmt::print("[lut2d] Starting {} threads\n", n_threads);
        int work_size = dim_y.resolution / n_threads;
        for (int t = 0; t < dim_y.resolution; t += work_size) {
            thread_grp.emplace_back(thread_fun, t,
                                    std::min(t + work_size, dim_y.resolution));
        }

        for (auto& t : thread_grp) {
            if (t.joinable()) {
                t.join();
            }
        }

        // thread_fun(0, dim_y.resolution);
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

template <typename T> class LUT3D {
public:
    std::vector<T> _data;
    Dimension _dim_x;
    Dimension _dim_y;
    Dimension _dim_z;

    LUT3D(const std::vector<T>& data, const Dimension& dim_x,
          const Dimension& dim_y, const Dimension& dim_z)
        : _data(data), _dim_x(dim_x), _dim_y(dim_y), _dim_z(dim_z) {}

    LUT3D(std::function<T(int, int, int, int, int, int)> fun,
          const Dimension& dim_x, const Dimension& dim_y,
          const Dimension& dim_z)
        : _dim_x(dim_x), _dim_y(dim_y), _dim_z(dim_z) {
        _data.resize(_dim_x.resolution * _dim_y.resolution * _dim_z.resolution);

        auto thread_fun = [=](int k0, int k1) {
            // fmt::print("[{}:{}]\n", j0, j1);
            for (int k = k0; k < k1; ++k) {
                for (int j = 0; j < _dim_y.resolution; ++j) {
                    for (int i = 0; i < dim_x.resolution; ++i) {
                        this->_data[index(i, j, k)] =
                            fun(i, j, k, dim_x.resolution, dim_y.resolution,
                                dim_z.resolution);
                    }
                }
            }
        };
        std::vector<std::thread> thread_grp;
        auto n_threads = std::thread::hardware_concurrency();
        fmt::print("[lut3d] Starting {} threads\n", n_threads);
        int work_size = dim_z.resolution / n_threads;
        for (int t = 0; t < dim_z.resolution; t += work_size) {
            thread_grp.emplace_back(thread_fun, t,
                                    std::min(t + work_size, dim_z.resolution));
        }

        for (auto& t : thread_grp) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    auto index(int i, int j, int k) const -> int {
        int idx = k * _dim_y.resolution * _dim_x.resolution +
                  j * _dim_x.resolution + i;
        a2assert(idx < _data.size() && idx >= 0,
                 "idx was out of range: {} ({})", idx, _data.size());
        return idx;
    }

    auto lookup(float x, float y, float z) -> T {
        float xx = clamp((x - _dim_x.start) * _dim_x.inv_range, 0.0f, 1.0f);
        float yy = clamp((y - _dim_y.start) * _dim_y.inv_range, 0.0f, 1.0f);
        float zz = clamp((z - _dim_z.start) * _dim_z.inv_range, 0.0f, 1.0f);

        int inc_x = _dim_x.inclusive ? 1 : 0;
        int inc_y = _dim_y.inclusive ? 1 : 0;
        int inc_z = _dim_z.inclusive ? 1 : 0;

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

        zz *= (_dim_z.resolution - inc_z);
        float fz = floorf(zz);
        float dz = zz - fz;
        int iz0 = int(fz);
        int iz1 = iz0 + 1;

        ix0 = clamp(ix0, 0, _dim_x.resolution - 1);
        iy0 = clamp(iy0, 0, _dim_y.resolution - 1);
        iz0 = clamp(iz0, 0, _dim_z.resolution - 1);
        ix1 = clamp(ix1, 0, _dim_x.resolution - 1);
        iy1 = clamp(iy1, 0, _dim_y.resolution - 1);
        iz1 = clamp(iz1, 0, _dim_z.resolution - 1);

        float a = _data[index(ix0, iy0, iz0)];
        float b = _data[index(ix1, iy0, iz0)];
        float c = _data[index(ix0, iy1, iz0)];
        float d = _data[index(ix1, iy1, iz0)];
        float e = _data[index(ix0, iy0, iz1)];
        float f = _data[index(ix1, iy0, iz1)];
        float g = _data[index(ix0, iy1, iz1)];
        float h = _data[index(ix1, iy1, iz1)];

        float A = lerp(a, b, dx);
        float B = lerp(c, d, dx);
        float C = lerp(e, f, dx);
        float D = lerp(g, h, dx);

        float E = lerp(A, B, dy);
        float F = lerp(C, D, dy);

        return lerp(E, F, dz);
    }
};
}

#pragma once

#include "common/util.hpp"
#include <OpenImageIO/imagebuf.h>
#include <fstream>
#include <memory>
#include <spdlog/fmt/ostr.h>
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

template <typename T> class LUT2D {
    std::vector<T> _data;
    Dimension _dim_x;
    Dimension _dim_y;

public:
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
        float xx =
            clamp((x - _dim_x.start) / (_dim_x.end - _dim_x.start), 0.0f, 1.0f);
        float yy =
            clamp((y - _dim_y.start) / (_dim_y.end - _dim_y.start), 0.0f, 1.0f);
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

    void write(const std::string& name) {
        OIIO::ImageSpec spec(_dim_x.resolution, _dim_y.resolution, 1,
                             OIIO::TypeDesc::FLOAT);
        auto output_img = std::make_unique<OIIO::ImageBuf>(
            fmt::format("lut_{}.exr", name), spec);
        output_img->set_write_format(OIIO::TypeDesc::HALF);

        std::fstream fs_src;
        fs_src.open(fmt::format("lut_{}.hpp", name), std::fstream::out);

        fmt::print(fs_src, "#pragma once\n"
                           "#include \"a2_uber/lut.hpp\"\n"
                           "inline auto get_lut_{}() -> "
                           "std::unique_ptr<a2::LUT2D<float>> {{\n",
                   name);

        fmt::print(
            fs_src,
            "return std::make_unique<a2::LUT2D<float>>(std::vector<float>{{\n",
            _dim_x.resolution * _dim_y.resolution);

        for (int j = 0; j < _dim_y.resolution; ++j) {
            for (int i = 0; i < _dim_x.resolution; ++i) {
                auto idx = j * _dim_x.resolution + i;
                fmt::print(fs_src, "{}", _data[idx]);
                output_img->setpixel(i, j, &_data[idx]);
                if (idx != _dim_x.resolution * _dim_y.resolution - 1)
                    fmt::print(fs_src, ", ");
            }
            fmt::print(fs_src, "\n");
        }
        fmt::print(fs_src, "}},\n");
        fmt::print(fs_src, "a2::Dimension({}, {}, {}, {}), ", _dim_x.resolution,
                   _dim_x.start, _dim_x.end, _dim_x.inclusive);
        fmt::print(fs_src, "a2::Dimension({}, {}, {}, {}));\n",
                   _dim_y.resolution, _dim_y.start, _dim_y.end,
                   _dim_y.inclusive);

        fmt::print(fs_src, "}}\n");

        output_img->write(fmt::format("lut_{}.exr", name));
    }
};
}

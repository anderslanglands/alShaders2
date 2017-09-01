#pragma once

#include "a2_uber/lut.hpp"
#include <OpenImageIO/imagebuf.h>
#include <fstream>
#include <memory>
#include <spdlog/fmt/ostr.h>

namespace a2 {

template <typename T>
inline void write(const LUT1D<T>& lut, const std::string& name) {
    OIIO::ImageSpec spec(lut._dim_x.resolution, 1, 1, OIIO::TypeDesc::FLOAT);
    auto output_img =
        std::make_unique<OIIO::ImageBuf>(fmt::format("lut_{}.exr", name), spec);
    output_img->set_write_format(OIIO::TypeDesc::HALF);

    std::fstream fs_src;
    fs_src.open(fmt::format("lut_{}.hpp", name), std::fstream::out);

    fmt::print(fs_src, "#pragma once\n"
                       "#include \"a2_uber/lut.hpp\"\n"
                       "inline auto get_lut_{}() -> "
                       "std::unique_ptr<a2::LUT1D<float>> {{\n",
               name);

    fmt::print(
        fs_src,
        "return std::make_unique<a2::LUT1D<float>>(std::vector<float>{{\n",
        lut._dim_x.resolution);

    for (int i = 0; i < lut._dim_x.resolution; ++i) {
        fmt::print(fs_src, "{}", lut._data[i]);
        float px = lut._data[i];
        output_img->setpixel(i, 0, &px);
        if (i != lut._dim_x.resolution - 1)
            fmt::print(fs_src, ", ");
    }
    fmt::print(fs_src, "\n");
    fmt::print(fs_src, "}},\n");
    fmt::print(fs_src, "a2::Dimension({}, {}, {}, {}));\n",
               lut._dim_x.resolution, lut._dim_x.start, lut._dim_x.end,
               lut._dim_x.inclusive);

    fmt::print(fs_src, "}}\n");

    output_img->write(fmt::format("lut_{}.exr", name));
}

template <typename T>
inline void write(const LUT2D<T>& lut, const std::string& name) {
    OIIO::ImageSpec spec(lut._dim_x.resolution, lut._dim_y.resolution, 1,
                         OIIO::TypeDesc::FLOAT);
    auto output_img =
        std::make_unique<OIIO::ImageBuf>(fmt::format("lut_{}.exr", name), spec);
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
        lut._dim_x.resolution * lut._dim_y.resolution);

    for (int j = 0; j < lut._dim_y.resolution; ++j) {
        for (int i = 0; i < lut._dim_x.resolution; ++i) {
            auto idx = j * lut._dim_x.resolution + i;
            fmt::print(fs_src, "{}", lut._data[idx]);
            float px = lut._data[idx];
            output_img->setpixel(i, j, &px);
            if (idx != lut._dim_x.resolution * lut._dim_y.resolution - 1)
                fmt::print(fs_src, ", ");
        }
        fmt::print(fs_src, "\n");
    }
    fmt::print(fs_src, "}},\n");
    fmt::print(fs_src, "a2::Dimension({}, {}, {}, {}), ", lut._dim_x.resolution,
               lut._dim_x.start, lut._dim_x.end, lut._dim_x.inclusive);
    fmt::print(fs_src, "a2::Dimension({}, {}, {}, {}));\n",
               lut._dim_y.resolution, lut._dim_y.start, lut._dim_y.end,
               lut._dim_y.inclusive);

    fmt::print(fs_src, "}}\n");

    output_img->write(fmt::format("lut_{}.exr", name));
}

template <typename T>
inline void write(const LUT3D<T>& lut, const std::string& name) {

    std::fstream fs_src;
    fs_src.open(fmt::format("lut_{}.hpp", name), std::fstream::out);

    fmt::print(fs_src, "#pragma once\n"
                       "#include \"a2_uber/lut.hpp\"\n"
                       "inline auto get_lut_{}() -> "
                       "std::unique_ptr<a2::LUT3D<float>> {{\n",
               name);

    fmt::print(
        fs_src,
        "return std::make_unique<a2::LUT3D<float>>(std::vector<float>{{\n",
        lut._dim_x.resolution * lut._dim_y.resolution);

    for (int k = 0; k < lut._dim_z.resolution; ++k) {
        OIIO::ImageSpec spec(lut._dim_x.resolution, lut._dim_y.resolution, 1,
                             OIIO::TypeDesc::FLOAT);
        auto output_img = std::make_unique<OIIO::ImageBuf>(
            fmt::format("lut_{}.{:04d}.exr", name, k), spec);
        output_img->set_write_format(OIIO::TypeDesc::HALF);
        for (int j = 0; j < lut._dim_y.resolution; ++j) {
            for (int i = 0; i < lut._dim_x.resolution; ++i) {
                auto idx = lut.index(i, j, k);
                fmt::print(fs_src, "{}", lut._data[idx]);
                float px = lut._data[idx];
                output_img->setpixel(i, j, &px);
                if (idx !=
                    lut._dim_x.resolution * lut._dim_y.resolution *
                            lut._dim_z.resolution -
                        1)
                    fmt::print(fs_src, ", ");
            }
            fmt::print(fs_src, "\n");
        }
        fmt::print(fs_src, "\n");
        output_img->write(fmt::format("lut_{}.{:04d}.exr", name, k));
    }
    fmt::print(fs_src, "}},\n");
    fmt::print(fs_src, "a2::Dimension({}, {}, {}, {}), ", lut._dim_x.resolution,
               lut._dim_x.start, lut._dim_x.end, lut._dim_x.inclusive);
    fmt::print(fs_src, "a2::Dimension({}, {}, {}, {}), ", lut._dim_y.resolution,
               lut._dim_y.start, lut._dim_y.end, lut._dim_y.inclusive);
    fmt::print(fs_src, "a2::Dimension({}, {}, {}, {}));\n",
               lut._dim_z.resolution, lut._dim_z.start, lut._dim_z.end,
               lut._dim_z.inclusive);

    fmt::print(fs_src, "}}\n");
}
}

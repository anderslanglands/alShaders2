#include "bsdf_ggx_multiscatter.hpp"
#include "lut_ggx_E.hpp"
#include "lut_ggx_E_avg.hpp"

namespace a2 {

std::unique_ptr<LUT2D<float>>
    BsdfGGXMultiscatterBase::_lut_ggx_E(get_lut_ggx_E());
std::unique_ptr<LUT1D<float>>
    BsdfGGXMultiscatterBase::_lut_ggx_E_avg(get_lut_ggx_E_avg());
}

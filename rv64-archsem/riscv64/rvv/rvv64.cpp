// MIT License

// Copyright (c) 2024 Meng Chengzhen, in Shandong University

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "riscv64.hpp"

#include "rvf/bitpack.hpp"
#include "softfloat/softfloat.h"

#include "rvv/vint.hpp"
#include "rvv/vfxp.hpp"
#include "rvv/vfp.hpp"
#include "rvv/vmsk.hpp"
#include "rvv/vrd.hpp"
#include "rvv/vperm.hpp"

#include <array>
#include <cstring>

using namespace rvvarchsem;

namespace rv64archsem {

uint32_t get_max_vector_length(const RVVSEW sew, const RVVLMUL lmul) {
    uint32_t sew_bits = 0;
    switch (sew)
    {
    case RVVSEW::e8: sew_bits = 8; break;
    case RVVSEW::e16: sew_bits = 16; break;
    case RVVSEW::e32: sew_bits = 32; break;
    case RVVSEW::e64: sew_bits = 64; break;
    default: return 0;
    }
    switch (lmul)
    {
    case RVVLMUL::m1: return (RVV_VLEN * 1) / sew_bits;
    case RVVLMUL::m2: return (RVV_VLEN * 2) / sew_bits;
    case RVVLMUL::m4: return (RVV_VLEN * 4) / sew_bits;
    case RVVLMUL::m8: return (RVV_VLEN * 8) / sew_bits;
    case RVVLMUL::mf8: return (RVV_VLEN / 8) / sew_bits;
    case RVVLMUL::mf4: return (RVV_VLEN / 4) / sew_bits;
    case RVVLMUL::mf2: return (RVV_VLEN / 2) / sew_bits;
    default: return 0;
    }
    return 0;
}

constexpr uint32_t MAX_VL = RVV_VLEN; // (VLEN / SEW_MIN) * LMUL_MAX, assuming SEW_MIN = 8 bits and LMUL_MAX = 8

template <uint32_t BitWidth>
struct IntType {
    using sintt = std::conditional_t< BitWidth == 8, int8_t,
                          std::conditional_t< BitWidth == 16, int16_t,
                          std::conditional_t< BitWidth == 32, int32_t,
                          int64_t> > > ;
    using uintt = std::conditional_t< BitWidth == 8, uint8_t,
                          std::conditional_t< BitWidth == 16, uint16_t,
                          std::conditional_t< BitWidth == 32, uint32_t,
                          uint64_t> > > ;
    using sint2t = std::conditional_t< BitWidth == 8, int16_t,
                           std::conditional_t< BitWidth == 16, int32_t,
                           std::conditional_t< BitWidth == 32, int64_t,
                           __int128_t> > > ;
    using uint2t = std::conditional_t< BitWidth == 8, uint16_t,
                           std::conditional_t< BitWidth == 16, uint32_t,
                           std::conditional_t< BitWidth == 32, uint64_t,
                           __uint128_t> > > ;
    using sintf2t = std::conditional_t< BitWidth == 16, int8_t,
                            std::conditional_t< BitWidth == 32, int16_t,
                            int32_t> > ;
    using uintf2t = std::conditional_t< BitWidth == 16, uint8_t,
                            std::conditional_t< BitWidth == 32, uint16_t,
                            uint32_t> > ;
    using sintf4t = std::conditional_t< BitWidth == 32, int8_t,
                            int16_t> ;
    using uintf4t = std::conditional_t< BitWidth == 32, uint8_t,
                            uint16_t> ;
    using sintf8t = int8_t;
    using uintf8t = uint8_t;
};

template <uint32_t SEW>
bool _exec_opv(
    const DecodedInst& inst,
    const uint32_t & vl,
    const uint32_t & vstart,
    uint64_t * __restrict vd,
    const uint64_t * __restrict vrs1,
    const uint64_t * __restrict vrs2,
    const ExpdMaskT * __restrict mask,
    const VXRM &rm,
    bool &vxsat,
    const bool nomask = false,
    const uint32_t vlmax = MAX_VL
) {
    static_assert(SEW == 8 || SEW == 16 || SEW == 32 || SEW == 64, "Unsupported SEW");

    using SInt = typename IntType<SEW>::sintt;
    using UInt = typename IntType<SEW>::uintt;
    using SInt2 = typename IntType<SEW>::sint2t;
    using UInt2 = typename IntType<SEW>::uint2t;
    using SIntF2 = typename IntType<SEW>::sintf2t;
    using UIntF2 = typename IntType<SEW>::uintf2t;
    using SIntF4 = typename IntType<SEW>::sintf4t;
    using UIntF4 = typename IntType<SEW>::uintf4t;
    using SIntF8 = typename IntType<SEW>::sintf8t;
    using UIntF8 = typename IntType<SEW>::uintf8t;

    UInt xrs1 = static_cast<UInt>(vrs1[0]);
    SInt xsrs1 = static_cast<SInt>(xrs1);
    uint32_t uxrs132 = static_cast<uint32_t>(vrs1[0]);
    UInt uimm = static_cast<UInt>(static_cast<uint64_t>(inst.imm));
    SInt simm = static_cast<SInt>(uimm);
    uint32_t uimm32 = static_cast<uint32_t>(static_cast<uint64_t>(inst.imm));

    switch (inst.instName)
    {
    // OPIVV
    case InstName::vadd_vv: vadd_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vsub_vv: vsub_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vminu_vv: vmin_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vmin_vv: vmin_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vmaxu_vv: vmax_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vmax_vv: vmax_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vand_vv: vand_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vor_vv: vor_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vxor_vv: vxor_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    // case InstName::vrgather_vv: vrgather_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vrgather_vv: vrgather_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), vlmax, mask); break;
    // case InstName::vrgatherei16_vv: vrgatherei16_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const uint16_t*>(vrs1), mask); break;
    case InstName::vrgatherei16_vv: vrgatherei16_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const uint16_t*>(vrs1), vlmax, mask); break;

    case InstName::vadc_vvm: vadc_vvm<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vmadc_vvm: vmadc_vvm<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vmadc_vv: vmadc_vv<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1)); break;
    case InstName::vsbc_vvm: vsbc_vvm<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vmsbc_vvm: vmsbc_vvm<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vmsbc_vv: vmsbc_vv<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1)); break;
    case InstName::vmerge_vvm: vmerge_vvm<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vmv_v_v:
    for (uint32_t i = vstart; i < vl; i++) reinterpret_cast<UInt*>(vd)[i] = reinterpret_cast<const UInt*>(vrs1)[i];
    break;
    // case InstName::vmv_v_v: memcpy(vd, vrs2, vl * sizeof(UInt)); break;
    case InstName::vmseq_vv: vmseq_vv<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vmsne_vv: vmsne_vv<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vmsltu_vv: vmslt_vv<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vmslt_vv: vmslt_vv<SInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vmsleu_vv: vmsle_vv<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vmsle_vv: vmsle_vv<SInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vsaddu_vv: vsaddu_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, vxsat); break;
    case InstName::vsadd_vv: vsadd_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask, vxsat); break;
    case InstName::vssubu_vv: vssubu_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, vxsat); break;
    case InstName::vssub_vv: vssub_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask, vxsat); break;
    case InstName::vsll_vv: vsl_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vsmul_vv: vsmul_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask, rm, vxsat); break;
    case InstName::vsrl_vv: vsr_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vsra_vv: vsr_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vssrl_vv: vssrl_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, rm, vxsat); break;
    case InstName::vssra_vv: vssra_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask, rm, vxsat); break;
    case InstName::vnsrl_wv:
        vnsr_wv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt*>(vd),
                         reinterpret_cast<const UInt2*>(vrs2),
                         reinterpret_cast<const UInt*>(vrs1), mask);
    break;

    case InstName::vnsra_wv:
        vnsr_wv<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt*>(vd),
                         reinterpret_cast<const SInt2*>(vrs2),
                         reinterpret_cast<const SInt*>(vrs1), mask);
    break;
    case InstName::vnclipu_wv: vnclipu_wv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt2*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, rm, vxsat); break;
    case InstName::vnclip_wv: vnclip_wv<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt2*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask, rm, vxsat); break;
    case InstName::vwredsumu_vs: vwredsumu_vs<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt2*>(vrs1), mask, nomask); break;
    case InstName::vwredsum_vs: vwredsum_vs<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt2*>(vrs1), mask, nomask); break;

    // OPFVV
    case InstName::vfadd_vv: vfadd_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfredusum_vs: vfredusum_vs<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfsub_vv: vfsub_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfredosum_vs: vfredosum_vs<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfmin_vv: vfmin_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfredmin_vs: vfredmin_vs<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfmax_vv: vfmax_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfredmax_vs: vfredmax_vs<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfsgnj_vv: vfsgnj_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfsgnjn_vv: vfsgnjn_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfsgnjx_vv: vfsgnjx_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfmv_f_s: memcpy(vd, vrs2, sizeof(UInt)); break;
    case InstName::vfcvt_xu_f_v: vfcvt_xu_f_v<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), mask, nomask); break;
    case InstName::vfcvt_x_f_v: vfcvt_x_f_v<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), mask, nomask); break;
    case InstName::vfcvt_f_xu_v: vfcvt_f_xu_v<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), mask, nomask); break;
    case InstName::vfcvt_f_x_v: vfcvt_f_x_v<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), mask, nomask); break;
    case InstName::vfcvt_rtz_xu_f_v: vfcvt_rtz_xu_f_v<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), mask, nomask); break;
    case InstName::vfcvt_rtz_x_f_v: vfcvt_rtz_x_f_v<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), mask, nomask); break;
    case InstName::vfwcvt_xu_f_v: vfwcvt_xu_f_v<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), mask, nomask); break;
    case InstName::vfwcvt_x_f_v: vfwcvt_x_f_v<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt*>(vrs2), mask, nomask); break;
    case InstName::vfwcvt_f_xu_v: vfwcvt_f_xu_v<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), mask, nomask); break;
    case InstName::vfwcvt_f_x_v: vfwcvt_f_x_v<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt*>(vrs2), mask, nomask); break;
    case InstName::vfwcvt_f_f_v: vfwcvt_f_f_v<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), mask, nomask); break;
    case InstName::vfwcvt_rtz_xu_f_v: vfwcvt_rtz_xu_f_v<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), mask, nomask); break;
    case InstName::vfwcvt_rtz_x_f_v: vfwcvt_rtz_x_f_v<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt*>(vrs2), mask, nomask); break;
    case InstName::vfncvt_xu_f_w: vfncvt_xu_f_w<UInt2, UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt2*>(vrs2), mask, nomask); break;
    case InstName::vfncvt_x_f_w: vfncvt_x_f_w<SInt2, SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt2*>(vrs2), mask, nomask); break;
    case InstName::vfncvt_f_xu_w: vfncvt_f_xu_w<UInt2, UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt2*>(vrs2), mask, nomask); break;
    case InstName::vfncvt_f_x_w: vfncvt_f_x_w<SInt2, SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt2*>(vrs2), mask, nomask); break;
    case InstName::vfncvt_f_f_w: vfncvt_f_f_w<UInt2, UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt2*>(vrs2), mask, nomask); break;
    case InstName::vfncvt_rod_f_f_w: vfncvt_rod_f_f_w<UInt2, UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt2*>(vrs2), mask, nomask); break;
    case InstName::vfncvt_rtz_xu_f_w: vfncvt_rtz_xu_f_w<UInt2, UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt2*>(vrs2), mask, nomask); break;
    case InstName::vfncvt_rtz_x_f_w: vfncvt_rtz_x_f_w<SInt2, SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt2*>(vrs2), mask, nomask); break;
    case InstName::vfsqrt_v: vfsqrt_v<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), mask, nomask); break;
    case InstName::vfrsqrt7_v: vfrsqrt7_v<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), mask, nomask); break;
    case InstName::vfrec7_v: vfrec7_v<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), mask, nomask); break;
    case InstName::vfclass_v: vfclass_v<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), mask, nomask); break;
    case InstName::vmfeq_vv: vmfeq_vv<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vmfle_vv: vmfle_vv<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vmflt_vv: vmflt_vv<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vmfne_vv: vmfne_vv<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfdiv_vv: vfdiv_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfmul_vv: vfmul_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfmadd_vv: vfmadd_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfnmadd_vv: vfnmadd_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfmsub_vv: vfmsub_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfnmsub_vv: vfnmsub_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfmacc_vv: vfmacc_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfnmacc_vv: vfnmacc_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfmsac_vv: vfmsac_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfnmsac_vv: vfnmsac_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfwadd_vv: vfwadd_vv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfwredusum_vs: vfwredusum_vs<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt2*>(vrs1), mask, nomask); break;
    case InstName::vfwsub_vv: vfwsub_vv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfwredosum_vs: vfwredosum_vs<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt2*>(vrs1), mask, nomask); break;
    case InstName::vfwadd_wv: vfwadd_wv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt2*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfwsub_wv: vfwsub_wv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt2*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfwmul_vv: vfwmul_vv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfwmacc_vv: vfwmacc_vv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfwnmacc_vv: vfwnmacc_vv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfwmsac_vv: vfwmsac_vv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vfwnmsac_vv: vfwnmsac_vv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;

    // OPMVV
    case InstName::vredsum_vs: vredsum_vs<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vredand_vs: vredand_vs<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vredor_vs: vredor_vs<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vredxor_vs: vredxor_vs<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vredminu_vs: vredminu_vs<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vredmin_vs: vredmin_vs<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask, nomask); break;
    case InstName::vredmaxu_vs: vredmaxu_vs<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, nomask); break;
    case InstName::vredmax_vs: vredmax_vs<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask, nomask); break;
    case InstName::vaaddu_vv: vaaddu_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, rm, vxsat); break;
    case InstName::vaadd_vv: vaadd_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask, rm, vxsat); break;
    case InstName::vasubu_vv: vasubu_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask, rm, vxsat); break;
    case InstName::vasub_vv: vasub_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask, rm, vxsat); break;
    case InstName::vmv_x_s: memcpy(vd, vrs2, sizeof(UInt)); break;
    case InstName::vcpop_m: vd[0] = vcpop_m(vl, vstart, reinterpret_cast<const uint8_t*>(vrs2), mask); break;
    case InstName::vfirst_m: vd[0] = vfirst_m(vl, vstart, reinterpret_cast<const uint8_t*>(vrs2), mask); break;
    case InstName::vzext_vf8: vext_vf<UInt, UIntF8>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UIntF8*>(vrs2), mask); break;
    case InstName::vsext_vf8: vext_vf<SInt, SIntF8>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SIntF8*>(vrs2), mask); break;
    case InstName::vzext_vf4: vext_vf<UInt, UIntF4>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UIntF4*>(vrs2), mask); break;
    case InstName::vsext_vf4: vext_vf<SInt, SIntF4>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SIntF4*>(vrs2), mask); break;
    case InstName::vzext_vf2: vext_vf<UInt, UIntF2>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UIntF2*>(vrs2), mask); break;
    case InstName::vsext_vf2: vext_vf<SInt, SIntF2>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SIntF2*>(vrs2), mask); break;
    case InstName::vmsbf_m: vmsbf_m(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const uint8_t*>(vrs2), mask); break;
    case InstName::vmsof_m: vmsof_m(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const uint8_t*>(vrs2), mask); break;
    case InstName::vmsif_m: vmsif_m(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const uint8_t*>(vrs2), mask); break;
    case InstName::viota_m: viota_m<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const uint8_t*>(vrs2), mask); break;
    case InstName::vid_v: vid_v<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), mask); break;
    case InstName::vcompress_vm: vcompress_vm<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const uint8_t*>(vrs1)); break;
    case InstName::vmandn_mm: vmandn_mm(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const uint8_t*>(vrs2), reinterpret_cast<const uint8_t*>(vrs1)); break;
    case InstName::vmand_mm: vmand_mm(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const uint8_t*>(vrs2), reinterpret_cast<const uint8_t*>(vrs1)); break;
    case InstName::vmor_mm: vmor_mm(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const uint8_t*>(vrs2), reinterpret_cast<const uint8_t*>(vrs1)); break;
    case InstName::vmxor_mm: vmxor_mm(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const uint8_t*>(vrs2), reinterpret_cast<const uint8_t*>(vrs1)); break;
    case InstName::vmorn_mm: vmorn_mm(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const uint8_t*>(vrs2), reinterpret_cast<const uint8_t*>(vrs1)); break;
    case InstName::vmnand_mm: vmnand_mm(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const uint8_t*>(vrs2), reinterpret_cast<const uint8_t*>(vrs1)); break;
    case InstName::vmnor_mm: vmnor_mm(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const uint8_t*>(vrs2), reinterpret_cast<const uint8_t*>(vrs1)); break;
    case InstName::vmxnor_mm: vmxnor_mm(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const uint8_t*>(vrs2), reinterpret_cast<const uint8_t*>(vrs1)); break;
    case InstName::vdivu_vv: vdiv_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vdiv_vv: vdiv_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vremu_vv: vrem_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vrem_vv: vrem_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vmulhu_vv: vmulhu_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vmul_vv: vmul_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vmulhsu_vv: vmulhsu_vv<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vmulh_vv: vmulh_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vmadd_vv: vmadd_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vnmsub_vv: vnmsub_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vmacc_vv: vmacc_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vnmsac_vv: vnmsac_vv<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vwaddu_vv: vwadd_vv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vwadd_vv: vwadd_vv<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vwsubu_vv: vwsub_vv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vwsub_vv: vwsub_vv<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vwaddu_wv: vwadd_wv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt2*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vwadd_wv: vwadd_wv<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt2*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vwsubu_wv: vwsub_wv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt2*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vwsub_wv: vwsub_wv<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt2*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vwmulu_vv: vwmulu_vv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vwmulsu_vv: vwmulsu_vv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vwmul_vv: vwmul_vv<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vwmaccu_vv: vwmaccu_vv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    case InstName::vwmacc_vv: vwmacc_vv<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt*>(vrs2), reinterpret_cast<const SInt*>(vrs1), mask); break;
    case InstName::vwmaccsu_vv: vwmaccsu_vv<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), reinterpret_cast<const UInt*>(vrs1), mask); break;
    
    // OPIVI
    case InstName::vadd_vi: vadd_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask); break;
    case InstName::vrsub_vi: vrsub_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask); break;
    case InstName::vand_vi: vand_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask); break;
    case InstName::vor_vi: vor_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask); break;
    case InstName::vxor_vi: vxor_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask); break;
    // case InstName::vrgather_vi: vrgather_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask); break;
    case InstName::vrgather_vi: vrgather_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, vlmax, mask); break;

    case InstName::vslideup_vi: vslideup_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm32, mask); break;
    case InstName::vslidedown_vi: vslidedown_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm32, vlmax, mask); break;
    case InstName::vadc_vim: vadc_vxim<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask); break;
    case InstName::vmadc_vim: vmadc_vxim<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask); break;
    case InstName::vmadc_vi: vmadc_vxi<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm); break;
    case InstName::vmerge_vim: vmerge_vxim<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask); break;
    case InstName::vmv_v_i:
    for (uint32_t i = vstart; i < vl; i++) reinterpret_cast<UInt*>(vd)[i] = uimm;
    break;
    // case InstName::vmv_v_i: for (uint32_t i = 0; i < vl; i++) reinterpret_cast<UInt*>(vd)[i] = uimm; break;
    case InstName::vmseq_vi: vmseq_vxi<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask); break;
    case InstName::vmsne_vi: vmsne_vxi<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask); break;
    case InstName::vmsleu_vi: vmsle_vxi<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask); break;
    case InstName::vmsle_vi: vmsle_vxi<SInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const SInt*>(vrs2), simm, mask); break;
    case InstName::vmsgtu_vi: vmsgt_vxi<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask); break;
    case InstName::vmsgt_vi: vmsgt_vxi<SInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const SInt*>(vrs2), simm, mask); break;
    case InstName::vsaddu_vi: vsaddu_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm, mask, vxsat); break;
    case InstName::vsadd_vi: vsadd_vxi<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), simm, mask, vxsat); break;
    case InstName::vsll_vi: vsl_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm32, mask); break;
    case InstName::vmv1r_v: memcpy(vd, vrs2, RVV_VLEN/8); break;
    case InstName::vmv2r_v: memcpy(vd, vrs2, RVV_VLEN/4); break;
    case InstName::vmv4r_v: memcpy(vd, vrs2, RVV_VLEN/2); break;
    case InstName::vmv8r_v: memcpy(vd, vrs2, RVV_VLEN); break;
    case InstName::vsrl_vi: vsr_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm32, mask); break;
    case InstName::vsra_vi: vsr_vxi<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), uimm32, mask); break;
    case InstName::vssrl_vi: vssrl_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uimm32, mask, rm, vxsat); break;
    case InstName::vssra_vi: vssra_vxi<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), uimm32, mask, rm, vxsat); break;
    case InstName::vnsrl_wi: vnsr_wxi<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt2*>(vrs2), uimm32, mask); break;
    case InstName::vnsra_wi: vnsr_wxi<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt2*>(vrs2), uimm32, mask); break;
    case InstName::vnclipu_wi: vnclipu_wxi<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt2*>(vrs2), uimm32, mask, rm, vxsat); break;
    case InstName::vnclip_wi: vnclip_wxi<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt2*>(vrs2), uimm32, mask, rm, vxsat); break;

    // OPIVX
    case InstName::vadd_vx: vadd_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vsub_vx: vsub_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vrsub_vx: vrsub_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vminu_vx: vmin_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmin_vx: vmin_vx<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vmaxu_vx: vmax_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmax_vx: vmax_vx<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vand_vx: vand_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vor_vx: vor_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vxor_vx: vxor_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    // case InstName::vrgather_vx: vrgather_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vrgather_vx: vrgather_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1,vlmax, mask); break;

    case InstName::vslideup_vx: vslideup_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uxrs132, mask); break;
    case InstName::vslidedown_vx: vslidedown_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uxrs132, vlmax, mask); break;
    case InstName::vadc_vxm: vadc_vxim<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmadc_vxm: vmadc_vxim<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmadc_vx: vmadc_vxi<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1); break;
    case InstName::vsbc_vxm: vsbc_vxim<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmsbc_vxm: vmsbc_vxim<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmsbc_vx: vmsbc_vxi<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1); break;
    case InstName::vmerge_vxm: vmerge_vxim<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmv_v_x:
    for (uint32_t i = vstart; i < vl; i++) reinterpret_cast<UInt*>(vd)[i] = xrs1;
    break;
    // case InstName::vmv_v_x: for (uint32_t i = 0; i < vl; i++) reinterpret_cast<UInt*>(vd)[i] = xrs1; break;
    case InstName::vmseq_vx: vmseq_vxi<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmsne_vx: vmsne_vxi<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmsltu_vx: vmslt_vxi<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmslt_vx: vmslt_vxi<SInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vmsleu_vx: vmsle_vxi<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmsle_vx: vmsle_vxi<SInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vmsgtu_vx: vmsgt_vxi<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmsgt_vx: vmsgt_vxi<SInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vsaddu_vx: vsaddu_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, vxsat); break;
    case InstName::vsadd_vx: vsadd_vxi<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask, vxsat); break;
    case InstName::vssubu_vx: vssubu_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, vxsat); break;
    case InstName::vssub_vx: vssub_vxi<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask, vxsat); break;
    case InstName::vsll_vx: vsl_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uxrs132, mask); break;
    case InstName::vsmul_vx: vsmul_vx<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask, rm, vxsat); break;
    case InstName::vsrl_vx: vsr_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uxrs132, mask); break;
    case InstName::vsra_vx: vsr_vxi<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), uxrs132, mask); break;
    case InstName::vssrl_vx: vssrl_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), uxrs132, mask, rm, vxsat); break;
    case InstName::vssra_vx: vssra_vxi<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), uxrs132, mask, rm, vxsat); break;
    case InstName::vnsrl_wx: vnsr_wxi<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt2*>(vrs2), uxrs132, mask); break;
    case InstName::vnsra_wx: vnsr_wxi<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt2*>(vrs2), uxrs132, mask); break;
    case InstName::vnclipu_wx: vnclipu_wxi<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt2*>(vrs2), uxrs132, mask, rm, vxsat); break;
    case InstName::vnclip_wx: vnclip_wxi<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt2*>(vrs2), uxrs132, mask, rm, vxsat); break;

    // OPFVF
    case InstName::vfadd_vf: vfadd_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfsub_vf: vfsub_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfmin_vf: vfmin_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfmax_vf: vfmax_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfsgnj_vf: vfsgnj_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfsgnjn_vf: vfsgnjn_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfsgnjx_vf: vfsgnjx_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfslide1up_vf: vslide1up_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vfslide1down_vf: vslide1down_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vfmv_s_f: reinterpret_cast<UInt*>(vd)[0] = xrs1; break;
    case InstName::vfmerge_vfm: vmerge_vxim<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vfmv_v_f: for (uint32_t i = 0; i < vl; i++) reinterpret_cast<UInt*>(vd)[i] = xrs1; break;
    case InstName::vmfeq_vf: vmfeq_vf<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vmfle_vf: vmfle_vf<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vmflt_vf: vmflt_vf<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vmfne_vf: vmfne_vf<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vmfgt_vf: vmfgt_vf<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vmfge_vf: vmfge_vf<UInt>(vl, vstart, reinterpret_cast<uint8_t*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfdiv_vf: vfdiv_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfrdiv_vf: vfrdiv_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfmul_vf: vfmul_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfrsub_vf: vfrsub_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfmadd_vf: vfmadd_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfnmadd_vf: vfnmadd_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfmsub_vf: vfmsub_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfnmsub_vf: vfnmsub_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfmacc_vf: vfmacc_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfnmacc_vf: vfnmacc_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfmsac_vf: vfmsac_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfnmsac_vf: vfnmsac_vf<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfwadd_vf: vfwadd_vf<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfwsub_vf: vfwsub_vf<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfwadd_wf: vfwadd_wf<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt2*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfwsub_wf: vfwsub_wf<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt2*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfwmul_vf: vfwmul_vf<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfwmacc_vf: vfwmacc_vf<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfwnmacc_vf: vfwnmacc_vf<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfwmsac_vf: vfwmsac_vf<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;
    case InstName::vfwnmsac_vf: vfwnmsac_vf<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, nomask); break;

    // OPMVX
    case InstName::vaaddu_vx: vaaddu_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, rm, vxsat); break;
    case InstName::vaadd_vx: vaadd_vxi<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask, rm, vxsat); break;
    case InstName::vasubu_vx: vasubu_vxi<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask, rm, vxsat); break;
    case InstName::vasub_vx: vasub_vxi<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask, rm, vxsat); break;
    case InstName::vslide1up_vx: vslide1up_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vslide1down_vx: vslide1down_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmv_s_x:
    if (vl > 0 && vstart == 0) reinterpret_cast<UInt*>(vd)[0] = xrs1;
    break;
    // case InstName::vmv_s_x: reinterpret_cast<UInt*>(vd)[0] = xrs1; break;
    case InstName::vdivu_vx: vdiv_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vdiv_vx: vdiv_vx<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vremu_vx: vrem_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vrem_vx: vrem_vx<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vmulhu_vx: vmulhu_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmul_vx: vmul_vx<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vmulhsu_vx: vmulhsu_vx<UInt>(vl, vstart, reinterpret_cast<UInt*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vmulh_vx: vmulh_vx<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vmadd_vx: vmadd_vx<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vnmsub_vx: vnmsub_vx<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vmacc_vx: vmacc_vx<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vnmsac_vx: vnmsac_vx<SInt>(vl, vstart, reinterpret_cast<SInt*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vwaddu_vx: vwadd_vx<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vwadd_vx: vwadd_vx<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vwsubu_vx: vwsub_vx<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vwsub_vx: vwsub_vx<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vwaddu_wx: vwadd_wx<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt2*>(vrs2), xrs1, mask); break;
    case InstName::vwadd_wx: vwadd_wx<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt2*>(vrs2), xsrs1, mask); break;
    case InstName::vwsubu_wx: vwsub_wx<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt2*>(vrs2), xrs1, mask); break;
    case InstName::vwsub_wx: vwsub_wx<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt2*>(vrs2), xsrs1, mask); break;
    case InstName::vwmulu_vx: vwmulu_vx<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vwmulsu_vx: vwmulsu_vx<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vwmul_vx: vwmul_vx<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vwmaccu_vx: vwmaccu_vx<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;
    case InstName::vwmacc_vx: vwmacc_vx<SInt, SInt2>(vl, vstart, reinterpret_cast<SInt2*>(vd), reinterpret_cast<const SInt*>(vrs2), xsrs1, mask); break;
    case InstName::vwmaccus_vx: vwmaccus_vx<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xsrs1, mask); break;
    case InstName::vwmaccsu_vx: vwmaccsu_vx<UInt, UInt2>(vl, vstart, reinterpret_cast<UInt2*>(vd), reinterpret_cast<const UInt*>(vrs2), xrs1, mask); break;

    default: return false;
    }

    return true;
}

static inline uint32_t get_element_bytes(const VecContext &vctx, VRegView vregview)
{
    uint32_t sew_bytes = 1;
    switch (vctx.sew)
    {
    case RVVSEW::e8: sew_bytes = 1; break;
    case RVVSEW::e16: sew_bytes = 2; break;
    case RVVSEW::e32: sew_bytes = 4; break;
    case RVVSEW::e64: sew_bytes = 8; break;
    }
    switch (vregview)
    {
    case VRegView::none: return sew_bytes;
    case VRegView::m2: return sew_bytes * 2;
    case VRegView::f2: return sew_bytes / 2;
    case VRegView::f4: return sew_bytes / 4;
    case VRegView::f8: return sew_bytes / 8;
    case VRegView::ei8: return 1;
    case VRegView::ei16: return 2;
    case VRegView::ei32: return 4;
    case VRegView::ei64: return 8;
    case VRegView::mask: return 1;
    case VRegView::e0: return sew_bytes;
    case VRegView::e0m2: return sew_bytes * 2;
    case VRegView::reg1: return RVV_VLEN / 8;
    case VRegView::reg2: return RVV_VLEN / 4;
    case VRegView::reg4: return RVV_VLEN / 2;
    case VRegView::reg8: return RVV_VLEN;
    default: return 0;
    }
}

static inline uint32_t request_reg_bytes(const VecContext &vctx, RegType regtype, VRegView vregview)
{
    uint32_t sew_bytes = 1;
    switch (vctx.sew)
    {
    case RVVSEW::e8: sew_bytes = 1; break;
    case RVVSEW::e16: sew_bytes = 2; break;
    case RVVSEW::e32: sew_bytes = 4; break;
    case RVVSEW::e64: sew_bytes = 8; break;
    }
    switch (regtype)
    {
    case RegType::gpreg: return 8;
    case RegType::freg: return 8;
    case RegType::vreg:
    {
        switch (vregview)
        {
        case VRegView::none: return sew_bytes * vctx.vl;
        case VRegView::m2: return sew_bytes * vctx.vl * 2;
        case VRegView::f2: return sew_bytes * vctx.vl / 2;
        case VRegView::f4: return sew_bytes * vctx.vl / 4;
        case VRegView::f8: return sew_bytes * vctx.vl / 8;
        case VRegView::ei8: return 1 * vctx.vl;
        case VRegView::ei16: return 2 * vctx.vl;
        case VRegView::ei32: return 4 * vctx.vl;
        case VRegView::ei64: return 8 * vctx.vl;
        case VRegView::mask: return (vctx.vl + 7) / 8;
        case VRegView::e0: return sew_bytes;
        case VRegView::e0m2: return sew_bytes * 2;
        case VRegView::reg1: return RVV_VLEN / 8;
        case VRegView::reg2: return RVV_VLEN / 4;
        case VRegView::reg4: return RVV_VLEN / 2;
        case VRegView::reg8: return RVV_VLEN;
        default: return 0;
        }
        return 0;
    }
    default: return 0;
    }
}

VecEXResult exec_opv(
    const DecodedInst& inst,
    VecContext& vctx,
    vector<uint64_t> &vdraw,
    const vector<uint64_t>& vrs1raw,
    const vector<uint64_t>& vrs2raw,
    uint64_t &fcsr
){

    static std::array<ExpdMaskT, MAX_VL> AllTrueExpdMask{1};

    const RVVSEW &sew = vctx.sew;
    const uint32_t &vl = vctx.vl;

    uint32_t rd_need_bytes = request_reg_bytes(vctx, inst.rdType, inst.vrdView);
    uint32_t rs1_need_bytes = request_reg_bytes(vctx, inst.rs1Type, inst.vrs1View);
    uint32_t rs2_need_bytes = request_reg_bytes(vctx, inst.rs2Type, inst.vrs2View);
    uint32_t vmaks_need_bytes = request_reg_bytes(vctx, RegType::vreg, VRegView::mask);

    if (vdraw.size() * 8 < rd_need_bytes) return VecEXResult::insufficient_vd;
    if (vrs1raw.size() * 8 < rs1_need_bytes) return VecEXResult::insufficient_vs1;
    if (vrs2raw.size() * 8 < rs2_need_bytes) return VecEXResult::insufficient_vs2;
    if (!vctx.vmask.empty() && vctx.vmask.size() * 8 < vmaks_need_bytes) return VecEXResult::insufficient_vmask;

    bool no_mask = (inst.funct7 & 1);
    const ExpdMaskT* maskptr = AllTrueExpdMask.data();
    vector<ExpdMaskT> expdmask;
    if (!no_mask && !vctx.vmask.empty()) {
        expdmask.resize(vl);
        expand_mask(vl, expdmask.data(), vctx.vmask.data());
        maskptr = expdmask.data();
    }

    VXRM rm = static_cast<VXRM>( (vctx.vcsr >> 1) & 0x3 );
    bool vxsat = false;

    uint8_t rmmod = getFRM(fcsr);
    if (rmmod == FRM_DYN) {
        rmmod = inst.funct3;
    }
    if (rmmod > 4) {
        rmmod = FRM_RNE;
    }
    softfloat_roundingMode = rmmod;
    softfloat_exceptionFlags = 0;

    bool res = false;
    switch (sew)
    {
    case RVVSEW::e8:
        if (
            inst.vrdView == VRegView::f8 || inst.vrdView == VRegView::f4 || inst.vrdView == VRegView::f2 ||
            inst.vrs1View == VRegView::f8 || inst.vrs1View == VRegView::f4 || inst.vrs1View == VRegView::f2 ||
            inst.vrs2View == VRegView::f8 || inst.vrs2View == VRegView::f4 || inst.vrs2View == VRegView::f2
        ) {
            return VecEXResult::invalid_widen_narrow;
        }
        res = _exec_opv<8U>(inst, vl, vctx.vstart, vdraw.data(), vrs1raw.data(), vrs2raw.data(), maskptr, rm, vxsat, no_mask, get_max_vector_length(sew, vctx.lmul));
        break;
    case RVVSEW::e16:
        if (
            inst.vrdView == VRegView::f8 || inst.vrdView == VRegView::f4 ||
            inst.vrs1View == VRegView::f8 || inst.vrs1View == VRegView::f4 ||
            inst.vrs2View == VRegView::f8 || inst.vrs2View == VRegView::f4
        ) {
            return VecEXResult::invalid_widen_narrow;
        }
        res = _exec_opv<16U>(inst, vl, vctx.vstart, vdraw.data(), vrs1raw.data(), vrs2raw.data(), maskptr, rm, vxsat, no_mask, get_max_vector_length(sew, vctx.lmul));
        break;
    case RVVSEW::e32:
        if (
            inst.vrdView == VRegView::f8 || 
            inst.vrs1View == VRegView::f8 ||
            inst.vrs2View == VRegView::f8
        ) {
            return VecEXResult::invalid_widen_narrow;
        }
        res = _exec_opv<32U>(inst, vl, vctx.vstart, vdraw.data(), vrs1raw.data(), vrs2raw.data(), maskptr, rm, vxsat, no_mask, get_max_vector_length(sew, vctx.lmul));
        break;
    case RVVSEW::e64:
        if (
            inst.vrdView == VRegView::m2 || 
            inst.vrs1View == VRegView::m2 || 
            inst.vrs2View == VRegView::m2 
        ) {
            return VecEXResult::invalid_widen_narrow;
        }
        res = _exec_opv<64U>(inst, vl, vctx.vstart, vdraw.data(), vrs1raw.data(), vrs2raw.data(), maskptr, rm, vxsat, no_mask, get_max_vector_length(sew, vctx.lmul));
        break;
    default:
        return VecEXResult::unsupported_sew;
    }
    if (!res) {
        return VecEXResult::illegal_instruction;
    }
    vctx.vstart = 0;
    fcsr |= (static_cast<uint64_t>(softfloat_exceptionFlags) & 0x1f);
    if (vxsat) {
        vctx.vcsr |= 0x1;
    }
    return VecEXResult::success;
}


static inline uint32_t get_field_stride_in_vreg(const RVVLMUL lmul) {
    uint32_t field_stride_in_vreg = RVV_VLEN / 8;
    switch (lmul)
    {
    case RVVLMUL::m2: field_stride_in_vreg *= 2; break;
    case RVVLMUL::m4: field_stride_in_vreg *= 4; break;
    case RVVLMUL::m8: field_stride_in_vreg *= 8; break;
    case RVVLMUL::mf8: field_stride_in_vreg /= 8; break;
    case RVVLMUL::mf4: field_stride_in_vreg /= 4; break;
    case RVVLMUL::mf2: field_stride_in_vreg /= 2; break;
    default: break;
    }
    return field_stride_in_vreg;
}


MemParseResult parse_load_vector(
    const DecodedInst& inst,
    VecContext& vctx,
    const uint64_t rs1val,
    const vector<uint64_t>& vs2raw,
    const uint8_t boundary_bytes,
    const bool misalign_error,
    vector<MemLoadDescriptor>& desc
) {
    vector<MemLoadDescriptor> temp_desc_pre_element;
    vector<ExpdMaskT> expdmask;
    if ((inst.funct7 & 1) || vctx.vmask.empty()) {
        expdmask.assign(vctx.vl, ExpdMaskT{1});
    } else {
        uint32_t need_vmask_bytes = request_reg_bytes(vctx, RegType::vreg, VRegView::mask);
        if (!vctx.vmask.empty() && vctx.vmask.size() < need_vmask_bytes) {
            return MemParseResult::insufficient_vmask;
        }
        expdmask.resize(vctx.vl);
        expand_mask(vctx.vl, expdmask.data(), vctx.vmask.data());
    }

    uint32_t need_vs2_bytes = request_reg_bytes(vctx, inst.rs2Type, inst.vrs2View);
    if (vs2raw.size() * 8 < need_vs2_bytes) {
        return MemParseResult::insufficient_vs2;
    }

    uint32_t field_stride_in_vreg = get_field_stride_in_vreg(vctx.lmul);
    uint32_t nf = ((inst.funct7 >> 4) & 0x7) + 1;
    uint32_t ele_width = get_element_bytes(vctx, inst.vrdView);
    int64_t stride = static_cast<int64_t>(rs1val);

    switch (inst.instName)
    {
    case InstName::vleXff_v:
        [[fallthrough]];
    case InstName::vleX_v:
        stride = 0;
        [[fallthrough]];
    case InstName::vlseX_v:
    {
        for (uint32_t i = vctx.vstart; i < vctx.vl; i++) {
            if (!expdmask[i]) {
                continue;
            }
            uint32_t reg_offset_base = (i * ele_width);
            for (uint32_t f = 0; f < nf; f++) {
                MemLoadDescriptor tmpdesc;
                tmpdesc.addr = static_cast<uint64_t>(static_cast<int64_t>(rs1val) + i * stride + static_cast<int64_t>(f * ele_width));
                tmpdesc.size = static_cast<uint16_t>(ele_width);
                tmpdesc.reg_offset = static_cast<uint16_t>(reg_offset_base + f * field_stride_in_vreg);
                tmpdesc.first_element_idx = static_cast<uint16_t>(i);
                temp_desc_pre_element.push_back(tmpdesc);
            }
        }
        break;
    }
    case InstName::vluxeiX_v:
        [[fallthrough]];
    case InstName::vloxeiX_v:
    {
        vector<int64_t> offset(vctx.vl, 0);
        switch (inst.vrs2View)
        {
        case VRegView::ei16:
            for (uint32_t i = 0; i < vctx.vl; i++) {
                offset[i] = static_cast<int64_t>(reinterpret_cast<const int16_t*>(vs2raw.data())[i]);
            }
            break;
        case VRegView::ei32:
            for (uint32_t i = 0; i < vctx.vl; i++) {
                offset[i] = static_cast<int64_t>(reinterpret_cast<const int32_t*>(vs2raw.data())[i]);
            }
            break;
        case VRegView::ei64:
            for (uint32_t i = 0; i < vctx.vl; i++) {
                offset[i] = static_cast<int64_t>(reinterpret_cast<const int64_t*>(vs2raw.data())[i]);
            }
            break;
        default:
            for (uint32_t i = 0; i < vctx.vl; i++) {
                offset[i] = static_cast<int64_t>(reinterpret_cast<const int8_t*>(vs2raw.data())[i]);
            }
            break;
        }
        for (uint32_t i = vctx.vstart; i < vctx.vl; i++) {
            if (!expdmask[i]) {
                continue;
            }
            uint32_t reg_offset_base = (i * ele_width);
            for (uint32_t f = 0; f < nf; f++) {
                MemLoadDescriptor tmpdesc;
                tmpdesc.addr = static_cast<uint64_t>(static_cast<int64_t>(rs1val) + offset[i] + static_cast<int64_t>(f * ele_width));
                tmpdesc.size = static_cast<uint16_t>(ele_width);
                tmpdesc.reg_offset = static_cast<uint16_t>(reg_offset_base + f * field_stride_in_vreg);
                tmpdesc.first_element_idx = static_cast<uint16_t>(i);
                temp_desc_pre_element.push_back(tmpdesc);
            }
        }
        break;
    }
    case InstName::vlm_v:
    {
        uint32_t length = (vctx.vl + 7) / 8;
        for (uint32_t i = vctx.vstart; i < length; i++) {
            MemLoadDescriptor tmpdesc;
            tmpdesc.addr = static_cast<uint64_t>(rs1val + i);
            tmpdesc.size = 1;
            tmpdesc.reg_offset = static_cast<uint16_t>(i);
            tmpdesc.first_element_idx = static_cast<uint16_t>(i);
            temp_desc_pre_element.push_back(tmpdesc);
        }
        break;
    }
    case InstName::vlNreX_v:
    {
        uint32_t len = RVV_VLEN * nf / 8;
        MemLoadDescriptor tmpdesc;
        tmpdesc.addr = static_cast<uint64_t>(rs1val);
        tmpdesc.size = static_cast<uint16_t>(len);
        tmpdesc.reg_offset = 0;
        tmpdesc.first_element_idx = 0;
        temp_desc_pre_element.push_back(tmpdesc);
        break;
    }
    default:
        return MemParseResult::illegal_instruction;
    }

    if (temp_desc_pre_element.empty()) {
        vctx.vstart = 0;
        return MemParseResult::success;
    }

    if (misalign_error) {
        for (auto iter = temp_desc_pre_element.begin(); iter != temp_desc_pre_element.end(); ++iter) {
            const auto &d = *iter;
            uint64_t align_to = (d.size > 8UL) ? 8UL : d.size;
            if (d.addr % align_to != 0) {
                desc.insert(desc.end(), temp_desc_pre_element.begin(), iter);
                vctx.vstart = d.first_element_idx;
                return MemParseResult::misaligned_address;
            }
        }
    }
    
    // merge and split desc
    // split
    vector<MemLoadDescriptor> spilted_desc;
    if (boundary_bytes == 0) {
        spilted_desc = std::move(temp_desc_pre_element);
        temp_desc_pre_element.clear();
    } else {
        for (const auto &d : temp_desc_pre_element) {
            uint64_t addr = d.addr;
            uint32_t size = d.size;
            uint64_t offset = 0;
            while (size > 0) {
                uint32_t chunk_size = size;
                uint64_t align_remain = boundary_bytes - (addr % boundary_bytes);
                if (chunk_size > align_remain) {
                    chunk_size = static_cast<uint32_t>(align_remain);
                }
                MemLoadDescriptor sd;
                sd.addr = addr;
                sd.size = static_cast<uint16_t>(chunk_size);
                sd.reg_offset = static_cast<uint16_t>(d.reg_offset + offset);
                sd.first_element_idx = d.first_element_idx;
                spilted_desc.push_back(sd);
                addr += chunk_size;
                size -= chunk_size;
                offset += chunk_size;
            }
        }
    }
    // merge
    MemLoadDescriptor merge_workspace = spilted_desc[0];
    for (size_t i = 1; i < spilted_desc.size(); i++) {
        const auto &d = spilted_desc[i];
        if (
            merge_workspace.addr / boundary_bytes == d.addr / boundary_bytes &&
            merge_workspace.addr + merge_workspace.size == d.addr &&
            merge_workspace.reg_offset + merge_workspace.size == d.reg_offset
        ) {
            merge_workspace.size += d.size;
        } else {
            desc.push_back(merge_workspace);
            merge_workspace = d;
        }
    }
    desc.push_back(merge_workspace);

    vctx.vstart = 0;
    return MemParseResult::success;
}

MemParseResult parse_store_vector(
    const DecodedInst& inst,
    VecContext& vctx,
    const uint64_t rs1val,
    const vector<uint64_t>& vs2raw,
    const vector<uint64_t>& vs3raw,
    const uint8_t boundary_bytes,
    const bool misalign_error,
    vector<MemStoreDescriptor>& desc
) {
    vector<MemStoreDescriptor> temp_desc_pre_element;
    vector<ExpdMaskT> expdmask;
    if ((inst.funct7 & 1) || vctx.vmask.empty()) {
        expdmask.assign(vctx.vl, ExpdMaskT{1});
    } else {
        uint32_t need_vmask_bytes = request_reg_bytes(vctx, RegType::vreg, VRegView::mask);
        if (!vctx.vmask.empty() && vctx.vmask.size() < need_vmask_bytes) {
            return MemParseResult::insufficient_vmask;
        }
        expdmask.resize(vctx.vl);
        expand_mask(vctx.vl, expdmask.data(), vctx.vmask.data());
    }

    uint32_t need_vs2_bytes = request_reg_bytes(vctx, inst.rs2Type, inst.vrs2View);
    if (vs2raw.size() * 8 < need_vs2_bytes) {
        return MemParseResult::insufficient_vs2;
    }
    uint32_t need_vs3_bytes = request_reg_bytes(vctx, inst.rs3Type, inst.vrs3View);
    if (vs3raw.size() * 8 < need_vs3_bytes) {
        return MemParseResult::insufficient_vs3;
    }
    const uint8_t* vs3ptr = reinterpret_cast<const uint8_t*>(vs3raw.data());

    uint32_t field_stride_in_vreg = get_field_stride_in_vreg(vctx.lmul);
    uint32_t nf = ((inst.funct7 >> 4) & 0x7) + 1;
    uint32_t ele_width = get_element_bytes(vctx, inst.vrdView);
    int64_t stride = static_cast<int64_t>(rs1val);

    switch (inst.instName)
    {
    case InstName::vseX_v:
        stride = 0;
        [[fallthrough]];
    case InstName::vsseX_v:
    {
        for (uint32_t i = vctx.vstart; i < vctx.vl; i++) {
            if (!expdmask[i]) {
                continue;
            }
            uint32_t reg_offset_base = i * ele_width;
            for (uint32_t f = 0; f < nf; f++) {
                MemStoreDescriptor tmpdesc;
                tmpdesc.addr = static_cast<uint64_t>(static_cast<int64_t>(rs1val) + i * stride + static_cast<int64_t>(f * ele_width));
                tmpdesc.raw_data.resize(ele_width);
                memcpy(tmpdesc.raw_data.data(), vs3ptr + reg_offset_base + f * field_stride_in_vreg, ele_width);
                temp_desc_pre_element.push_back(std::move(tmpdesc));
            }
        }
        break;
    }
    case InstName::vsuxeiX_v:
        [[fallthrough]];
    case InstName::vsoxeiX_v:
    {
        vector<int64_t> offset(vctx.vl, 0);
        switch (inst.vrs2View)
        {
        case VRegView::ei16:
            for (uint32_t i = 0; i < vctx.vl; i++) {
                offset[i] = static_cast<int64_t>(reinterpret_cast<const int16_t*>(vs2raw.data())[i]);
            }
            break;
        case VRegView::ei32:
            for (uint32_t i = 0; i < vctx.vl; i++) {
                offset[i] = static_cast<int64_t>(reinterpret_cast<const int32_t*>(vs2raw.data())[i]);
            }
            break;
        case VRegView::ei64:
            for (uint32_t i = 0; i < vctx.vl; i++) {
                offset[i] = static_cast<int64_t>(reinterpret_cast<const int64_t*>(vs2raw.data())[i]);
            }
            break;
        default:
            for (uint32_t i = 0; i < vctx.vl; i++) {
                offset[i] = static_cast<int64_t>(reinterpret_cast<const int8_t*>(vs2raw.data())[i]);
            }
            break;
        }
        for (uint32_t i = vctx.vstart; i < vctx.vl; i++) {
            if (!expdmask[i]) {
                continue;
            }
            uint32_t reg_offset_base = i * ele_width;
            for (uint32_t f = 0; f < nf; f++) {
                MemStoreDescriptor tmpdesc;
                tmpdesc.addr = static_cast<uint64_t>(static_cast<int64_t>(rs1val) + offset[i] + static_cast<int64_t>(f * ele_width));
                tmpdesc.raw_data.resize(ele_width);
                memcpy(tmpdesc.raw_data.data(), vs3ptr + reg_offset_base + f * field_stride_in_vreg, ele_width);
                temp_desc_pre_element.push_back(std::move(tmpdesc));
            }
        }
        break;
    }
    case InstName::vsm_v:
    {
        uint32_t length = (vctx.vl + 7) / 8;
        for (uint32_t i = vctx.vstart; i < length; i++) {
            MemStoreDescriptor tmpdesc;
            tmpdesc.addr = static_cast<uint64_t>(rs1val + i);
            tmpdesc.raw_data.resize(1);
            memcpy(tmpdesc.raw_data.data(), vs3ptr + i, 1);
            temp_desc_pre_element.push_back(std::move(tmpdesc));
        }
        break;
    }
    case InstName::vsNr_v:
    {
        uint32_t len = RVV_VLEN * nf / 8;
        MemStoreDescriptor tmpdesc;
        tmpdesc.addr = static_cast<uint64_t>(rs1val);
        tmpdesc.raw_data.resize(len);
        memcpy(tmpdesc.raw_data.data(), vs3ptr, len);
        temp_desc_pre_element.push_back(std::move(tmpdesc));
        break;
    }
    default:
        return MemParseResult::illegal_instruction;
    }

    if (temp_desc_pre_element.empty()) {
        vctx.vstart = 0;
        return MemParseResult::success;
    }

    if (misalign_error) {
        for (auto iter = temp_desc_pre_element.begin(); iter != temp_desc_pre_element.end(); ++iter) {
            const auto &d = *iter;
            uint64_t align_to = (d.raw_data.size() > 8UL) ? 8UL : d.raw_data.size();
            if (d.addr % align_to != 0) {
                desc.insert(desc.end(), temp_desc_pre_element.begin(), iter);
                vctx.vstart = d.first_element_idx;
                return MemParseResult::misaligned_address;
            }
        }
    }
    
    // merge and split desc
    // split
    vector<MemStoreDescriptor> spilted_desc;
    if (boundary_bytes == 0) {
        spilted_desc = std::move(temp_desc_pre_element);
        temp_desc_pre_element.clear();
    } else {
        for (const auto &d : temp_desc_pre_element) {
            uint64_t addr = d.addr;
            uint32_t size = static_cast<uint32_t>(d.raw_data.size());
            uint64_t offset = 0;
            while (size > 0) {
                uint32_t chunk_size = size;
                uint64_t align_remain = boundary_bytes - (addr % boundary_bytes);
                if (chunk_size > align_remain) {
                    chunk_size = static_cast<uint32_t>(align_remain);
                }
                MemStoreDescriptor sd;
                sd.addr = addr;
                sd.raw_data.resize(chunk_size);
                memcpy(sd.raw_data.data(), d.raw_data.data() + offset, chunk_size);
                sd.first_element_idx = d.first_element_idx;
                spilted_desc.push_back(sd);
                addr += chunk_size;
                size -= chunk_size;
                offset += chunk_size;
            }
        }
    }
    // merge
    MemStoreDescriptor merge_workspace = std::move(spilted_desc[0]);
    for (size_t i = 1; i < spilted_desc.size(); i++) {
        const auto &d = spilted_desc[i];
        if (
            merge_workspace.addr / boundary_bytes == d.addr / boundary_bytes &&
            merge_workspace.addr + merge_workspace.raw_data.size() == d.addr
        ) {
            merge_workspace.raw_data.insert(merge_workspace.raw_data.end(), d.raw_data.begin(), d.raw_data.end());
        } else {
            desc.push_back(std::move(merge_workspace));
            merge_workspace = std::move(d);
        }
    }
    desc.push_back(std::move(merge_workspace));
    
    vctx.vstart = 0;
    return MemParseResult::success;
}



} // namespace rv64archsem

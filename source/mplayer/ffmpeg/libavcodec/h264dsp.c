/*
 * H.26L/H.264/AVC/JVT/14496-10/... encoder/decoder
 * Copyright (c) 2003-2010 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of Libav.
 *
 * Libav is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Libav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libav; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * H.264 / AVC / MPEG4 part10 DSP functions.
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#include <stdint.h>
#include "avcodec.h"
#include "h264dsp.h"

#define BIT_DEPTH 8
#include "h264dsp_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 9
#include "h264dsp_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 10
#include "h264dsp_template.c"
#undef BIT_DEPTH

void ff_h264dsp_init(H264DSPContext *c, const int bit_depth)
{
#undef FUNC
#define FUNC(a, depth) a ## _ ## depth ## _c

#define H264_DSP(depth) \
    c->h264_idct_add= FUNC(ff_h264_idct_add, depth);\
    c->h264_idct8_add= FUNC(ff_h264_idct8_add, depth);\
    c->h264_idct_dc_add= FUNC(ff_h264_idct_dc_add, depth);\
    c->h264_idct8_dc_add= FUNC(ff_h264_idct8_dc_add, depth);\
    c->h264_idct_add16     = FUNC(ff_h264_idct_add16, depth);\
    c->h264_idct8_add4     = FUNC(ff_h264_idct8_add4, depth);\
    c->h264_idct_add8      = FUNC(ff_h264_idct_add8, depth);\
    c->h264_idct_add16intra= FUNC(ff_h264_idct_add16intra, depth);\
    c->h264_luma_dc_dequant_idct= FUNC(ff_h264_luma_dc_dequant_idct, depth);\
    c->h264_chroma_dc_dequant_idct= FUNC(ff_h264_chroma_dc_dequant_idct, depth);\
\
    c->weight_h264_pixels_tab[0]= FUNC(weight_h264_pixels16x16, depth);\
    c->weight_h264_pixels_tab[1]= FUNC(weight_h264_pixels16x8, depth);\
    c->weight_h264_pixels_tab[2]= FUNC(weight_h264_pixels8x16, depth);\
    c->weight_h264_pixels_tab[3]= FUNC(weight_h264_pixels8x8, depth);\
    c->weight_h264_pixels_tab[4]= FUNC(weight_h264_pixels8x4, depth);\
    c->weight_h264_pixels_tab[5]= FUNC(weight_h264_pixels4x8, depth);\
    c->weight_h264_pixels_tab[6]= FUNC(weight_h264_pixels4x4, depth);\
    c->weight_h264_pixels_tab[7]= FUNC(weight_h264_pixels4x2, depth);\
    c->weight_h264_pixels_tab[8]= FUNC(weight_h264_pixels2x4, depth);\
    c->weight_h264_pixels_tab[9]= FUNC(weight_h264_pixels2x2, depth);\
    c->biweight_h264_pixels_tab[0]= FUNC(biweight_h264_pixels16x16, depth);\
    c->biweight_h264_pixels_tab[1]= FUNC(biweight_h264_pixels16x8, depth);\
    c->biweight_h264_pixels_tab[2]= FUNC(biweight_h264_pixels8x16, depth);\
    c->biweight_h264_pixels_tab[3]= FUNC(biweight_h264_pixels8x8, depth);\
    c->biweight_h264_pixels_tab[4]= FUNC(biweight_h264_pixels8x4, depth);\
    c->biweight_h264_pixels_tab[5]= FUNC(biweight_h264_pixels4x8, depth);\
    c->biweight_h264_pixels_tab[6]= FUNC(biweight_h264_pixels4x4, depth);\
    c->biweight_h264_pixels_tab[7]= FUNC(biweight_h264_pixels4x2, depth);\
    c->biweight_h264_pixels_tab[8]= FUNC(biweight_h264_pixels2x4, depth);\
    c->biweight_h264_pixels_tab[9]= FUNC(biweight_h264_pixels2x2, depth);\
\
    c->h264_v_loop_filter_luma= FUNC(h264_v_loop_filter_luma, depth);\
    c->h264_h_loop_filter_luma= FUNC(h264_h_loop_filter_luma, depth);\
    c->h264_h_loop_filter_luma_mbaff= FUNC(h264_h_loop_filter_luma_mbaff, depth);\
    c->h264_v_loop_filter_luma_intra= FUNC(h264_v_loop_filter_luma_intra, depth);\
    c->h264_h_loop_filter_luma_intra= FUNC(h264_h_loop_filter_luma_intra, depth);\
    c->h264_h_loop_filter_luma_mbaff_intra= FUNC(h264_h_loop_filter_luma_mbaff_intra, depth);\
    c->h264_v_loop_filter_chroma= FUNC(h264_v_loop_filter_chroma, depth);\
    c->h264_h_loop_filter_chroma= FUNC(h264_h_loop_filter_chroma, depth);\
    c->h264_h_loop_filter_chroma_mbaff= FUNC(h264_h_loop_filter_chroma_mbaff, depth);\
    c->h264_v_loop_filter_chroma_intra= FUNC(h264_v_loop_filter_chroma_intra, depth);\
    c->h264_h_loop_filter_chroma_intra= FUNC(h264_h_loop_filter_chroma_intra, depth);\
    c->h264_h_loop_filter_chroma_mbaff_intra= FUNC(h264_h_loop_filter_chroma_mbaff_intra, depth);\
    c->h264_loop_filter_strength= NULL;

    switch (bit_depth) {
    case 9:
        H264_DSP(9);
        break;
    case 10:
        H264_DSP(10);
        break;
    default:
        H264_DSP(8);
        break;
    }

    if (ARCH_ARM) ff_h264dsp_init_arm(c, bit_depth);
    if (ARCH_PPC) ff_h264dsp_init_ppc(c, bit_depth);
    if (HAVE_MMX) ff_h264dsp_init_x86(c, bit_depth);
}

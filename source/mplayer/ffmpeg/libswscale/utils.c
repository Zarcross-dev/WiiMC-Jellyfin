/*
 * Copyright (C) 2001-2003 Michael Niedermayer <michaelni@gmx.at>
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

#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "config.h"
#include <assert.h>
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif
#if HAVE_VIRTUALALLOC
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "swscale.h"
#include "swscale_internal.h"
#include "rgb2rgb.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/x86_cpu.h"
#include "libavutil/cpu.h"
#include "libavutil/avutil.h"
#include "libavutil/bswap.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"

unsigned swscale_version(void)
{
    return LIBSWSCALE_VERSION_INT;
}

const char *swscale_configuration(void)
{
    return LIBAV_CONFIGURATION;
}

const char *swscale_license(void)
{
#define LICENSE_PREFIX "libswscale license: "
    return LICENSE_PREFIX LIBAV_LICENSE + sizeof(LICENSE_PREFIX) - 1;
}

#define RET 0xC3 //near return opcode for x86

typedef struct FormatEntry {
    int is_supported_in, is_supported_out;
} FormatEntry;

const static FormatEntry format_entries[PIX_FMT_NB] = {
    [PIX_FMT_YUV420P]     = { 1 , 1 },
    [PIX_FMT_YUYV422]     = { 1 , 1 },
    [PIX_FMT_RGB24]       = { 1 , 1 },
    [PIX_FMT_BGR24]       = { 1 , 1 },
    [PIX_FMT_YUV422P]     = { 1 , 1 },
    [PIX_FMT_YUV444P]     = { 1 , 1 },
    [PIX_FMT_YUV410P]     = { 1 , 1 },
    [PIX_FMT_YUV411P]     = { 1 , 1 },
    [PIX_FMT_GRAY8]       = { 1 , 1 },
    [PIX_FMT_MONOWHITE]   = { 1 , 1 },
    [PIX_FMT_MONOBLACK]   = { 1 , 1 },
    [PIX_FMT_PAL8]        = { 1 , 0 },
    [PIX_FMT_YUVJ420P]    = { 1 , 1 },
    [PIX_FMT_YUVJ422P]    = { 1 , 1 },
    [PIX_FMT_YUVJ444P]    = { 1 , 1 },
    [PIX_FMT_UYVY422]     = { 1 , 1 },
    [PIX_FMT_UYYVYY411]   = { 0 , 0 },
    [PIX_FMT_BGR8]        = { 1 , 1 },
    [PIX_FMT_BGR4]        = { 0 , 1 },
    [PIX_FMT_BGR4_BYTE]   = { 1 , 1 },
    [PIX_FMT_RGB8]        = { 1 , 1 },
    [PIX_FMT_RGB4]        = { 0 , 1 },
    [PIX_FMT_RGB4_BYTE]   = { 1 , 1 },
    [PIX_FMT_NV12]        = { 1 , 1 },
    [PIX_FMT_NV21]        = { 1 , 1 },
    [PIX_FMT_ARGB]        = { 1 , 1 },
    [PIX_FMT_RGBA]        = { 1 , 1 },
    [PIX_FMT_ABGR]        = { 1 , 1 },
    [PIX_FMT_BGRA]        = { 1 , 1 },
    [PIX_FMT_GRAY16BE]    = { 1 , 1 },
    [PIX_FMT_GRAY16LE]    = { 1 , 1 },
    [PIX_FMT_YUV440P]     = { 1 , 1 },
    [PIX_FMT_YUVJ440P]    = { 1 , 1 },
    [PIX_FMT_YUVA420P]    = { 1 , 1 },
    [PIX_FMT_RGB48BE]     = { 1 , 1 },
    [PIX_FMT_RGB48LE]     = { 1 , 1 },
    [PIX_FMT_RGB565BE]    = { 1 , 1 },
    [PIX_FMT_RGB565LE]    = { 1 , 1 },
    [PIX_FMT_RGB555BE]    = { 1 , 1 },
    [PIX_FMT_RGB555LE]    = { 1 , 1 },
    [PIX_FMT_BGR565BE]    = { 1 , 1 },
    [PIX_FMT_BGR565LE]    = { 1 , 1 },
    [PIX_FMT_BGR555BE]    = { 1 , 1 },
    [PIX_FMT_BGR555LE]    = { 1 , 1 },
    [PIX_FMT_YUV420P16LE] = { 1 , 1 },
    [PIX_FMT_YUV420P16BE] = { 1 , 1 },
    [PIX_FMT_YUV422P16LE] = { 1 , 1 },
    [PIX_FMT_YUV422P16BE] = { 1 , 1 },
    [PIX_FMT_YUV444P16LE] = { 1 , 1 },
    [PIX_FMT_YUV444P16BE] = { 1 , 1 },
    [PIX_FMT_RGB444LE]    = { 0 , 1 },
    [PIX_FMT_RGB444BE]    = { 0 , 1 },
    [PIX_FMT_BGR444LE]    = { 0 , 1 },
    [PIX_FMT_BGR444BE]    = { 0 , 1 },
    [PIX_FMT_Y400A]       = { 1 , 0 },
    [PIX_FMT_BGR48BE]     = { 1 , 1 },
    [PIX_FMT_BGR48LE]     = { 1 , 1 },
    [PIX_FMT_YUV420P9BE]  = { 1 , 1 },
    [PIX_FMT_YUV420P9LE]  = { 1 , 1 },
    [PIX_FMT_YUV420P10BE] = { 1 , 1 },
    [PIX_FMT_YUV420P10LE] = { 1 , 1 },
    [PIX_FMT_YUV422P10BE] = { 1 , 1 },
    [PIX_FMT_YUV422P10LE] = { 1 , 1 },
    [PIX_FMT_YUV444P9BE]  = { 1 , 0 },
    [PIX_FMT_YUV444P9LE]  = { 1 , 0 },
    [PIX_FMT_YUV444P10BE] = { 1 , 0 },
    [PIX_FMT_YUV444P10LE] = { 1 , 0 },
};

int sws_isSupportedInput(enum PixelFormat pix_fmt)
{
    return (unsigned)pix_fmt < PIX_FMT_NB ?
        format_entries[pix_fmt].is_supported_in : 0;
}

int sws_isSupportedOutput(enum PixelFormat pix_fmt)
{
    return (unsigned)pix_fmt < PIX_FMT_NB ?
        format_entries[pix_fmt].is_supported_out : 0;
}

extern const int32_t ff_yuv2rgb_coeffs[8][4];

const char *sws_format_name(enum PixelFormat format)
{
    if ((unsigned)format < PIX_FMT_NB && av_pix_fmt_descriptors[format].name)
        return av_pix_fmt_descriptors[format].name;
    else
        return "Unknown format";
}

static double getSplineCoeff(double a, double b, double c, double d, double dist)
{
    if (dist<=1.0) return ((d*dist + c)*dist + b)*dist +a;
    else           return getSplineCoeff(        0.0,
                                          b+ 2.0*c + 3.0*d,
                                                 c + 3.0*d,
                                         -b- 3.0*c - 6.0*d,
                                         dist-1.0);
}

static int initFilter(int16_t **outFilter, int16_t **filterPos, int *outFilterSize, int xInc,
                      int srcW, int dstW, int filterAlign, int one, int flags, int cpu_flags,
                      SwsVector *srcFilter, SwsVector *dstFilter, double param[2])
{
    int i;
    int filterSize;
    int filter2Size;
    int minFilterSize;
    int64_t *filter=NULL;
    int64_t *filter2=NULL;
    const int64_t fone= 1LL<<54;
    int ret= -1;

    emms_c(); //FIXME this should not be required but it IS (even for non-MMX versions)

    // NOTE: the +1 is for the MMX scaler which reads over the end
    FF_ALLOC_OR_GOTO(NULL, *filterPos, (dstW+1)*sizeof(int16_t), fail);

    if (FFABS(xInc - 0x10000) <10) { // unscaled
        int i;
        filterSize= 1;
        FF_ALLOCZ_OR_GOTO(NULL, filter, dstW*sizeof(*filter)*filterSize, fail);

        for (i=0; i<dstW; i++) {
            filter[i*filterSize]= fone;
            (*filterPos)[i]=i;
        }

    } else if (flags&SWS_POINT) { // lame looking point sampling mode
        int i;
        int xDstInSrc;
        filterSize= 1;
        FF_ALLOC_OR_GOTO(NULL, filter, dstW*sizeof(*filter)*filterSize, fail);

        xDstInSrc= xInc/2 - 0x8000;
        for (i=0; i<dstW; i++) {
            int xx= (xDstInSrc - ((filterSize-1)<<15) + (1<<15))>>16;

            (*filterPos)[i]= xx;
            filter[i]= fone;
            xDstInSrc+= xInc;
        }
    } else if ((xInc <= (1<<16) && (flags&SWS_AREA)) || (flags&SWS_FAST_BILINEAR)) { // bilinear upscale
        int i;
        int xDstInSrc;
        filterSize= 2;
        FF_ALLOC_OR_GOTO(NULL, filter, dstW*sizeof(*filter)*filterSize, fail);

        xDstInSrc= xInc/2 - 0x8000;
        for (i=0; i<dstW; i++) {
            int xx= (xDstInSrc - ((filterSize-1)<<15) + (1<<15))>>16;
            int j;

            (*filterPos)[i]= xx;
            //bilinear upscale / linear interpolate / area averaging
            for (j=0; j<filterSize; j++) {
                int64_t coeff= fone - FFABS((xx<<16) - xDstInSrc)*(fone>>16);
                if (coeff<0) coeff=0;
                filter[i*filterSize + j]= coeff;
                xx++;
            }
            xDstInSrc+= xInc;
        }
    } else {
        int xDstInSrc;
        int sizeFactor;

        if      (flags&SWS_BICUBIC)      sizeFactor=  4;
        else if (flags&SWS_X)            sizeFactor=  8;
        else if (flags&SWS_AREA)         sizeFactor=  1; //downscale only, for upscale it is bilinear
        else if (flags&SWS_GAUSS)        sizeFactor=  8;   // infinite ;)
        else if (flags&SWS_LANCZOS)      sizeFactor= param[0] != SWS_PARAM_DEFAULT ? ceil(2*param[0]) : 6;
        else if (flags&SWS_SINC)         sizeFactor= 20; // infinite ;)
        else if (flags&SWS_SPLINE)       sizeFactor= 20;  // infinite ;)
        else if (flags&SWS_BILINEAR)     sizeFactor=  2;
        else {
            sizeFactor= 0; //GCC warning killer
            assert(0);
        }

        if (xInc <= 1<<16)      filterSize= 1 + sizeFactor; // upscale
        else                    filterSize= 1 + (sizeFactor*srcW + dstW - 1)/ dstW;

        if (filterSize > srcW-2) filterSize=srcW-2;

        FF_ALLOC_OR_GOTO(NULL, filter, dstW*sizeof(*filter)*filterSize, fail);

        xDstInSrc= xInc - 0x10000;
        for (i=0; i<dstW; i++) {
            int xx= (xDstInSrc - ((filterSize-2)<<16)) / (1<<17);
            int j;
            (*filterPos)[i]= xx;
            for (j=0; j<filterSize; j++) {
                int64_t d= ((int64_t)FFABS((xx<<17) - xDstInSrc))<<13;
                double floatd;
                int64_t coeff;

                if (xInc > 1<<16)
                    d= d*dstW/srcW;
                floatd= d * (1.0/(1<<30));

                if (flags & SWS_BICUBIC) {
                    int64_t B= (param[0] != SWS_PARAM_DEFAULT ? param[0] :   0) * (1<<24);
                    int64_t C= (param[1] != SWS_PARAM_DEFAULT ? param[1] : 0.6) * (1<<24);
                    int64_t dd = ( d*d)>>30;
                    int64_t ddd= (dd*d)>>30;

                    if      (d < 1LL<<30)
                        coeff = (12*(1<<24)-9*B-6*C)*ddd + (-18*(1<<24)+12*B+6*C)*dd + (6*(1<<24)-2*B)*(1<<30);
                    else if (d < 1LL<<31)
                        coeff = (-B-6*C)*ddd + (6*B+30*C)*dd + (-12*B-48*C)*d + (8*B+24*C)*(1<<30);
                    else
                        coeff=0.0;
                    coeff *= fone>>(30+24);
                }
/*                else if (flags & SWS_X) {
                    double p= param ? param*0.01 : 0.3;
                    coeff = d ? sin(d*M_PI)/(d*M_PI) : 1.0;
                    coeff*= pow(2.0, - p*d*d);
                }*/
                else if (flags & SWS_X) {
                    double A= param[0] != SWS_PARAM_DEFAULT ? param[0] : 1.0;
                    double c;

                    if (floatd<1.0)
                        c = cos(floatd*M_PI);
                    else
                        c=-1.0;
                    if (c<0.0)      c= -pow(-c, A);
                    else            c=  pow( c, A);
                    coeff= (c*0.5 + 0.5)*fone;
                } else if (flags & SWS_AREA) {
                    int64_t d2= d - (1<<29);
                    if      (d2*xInc < -(1LL<<(29+16))) coeff= 1.0 * (1LL<<(30+16));
                    else if (d2*xInc <  (1LL<<(29+16))) coeff= -d2*xInc + (1LL<<(29+16));
                    else coeff=0.0;
                    coeff *= fone>>(30+16);
                } else if (flags & SWS_GAUSS) {
                    double p= param[0] != SWS_PARAM_DEFAULT ? param[0] : 3.0;
                    coeff = (pow(2.0, - p*floatd*floatd))*fone;
                } else if (flags & SWS_SINC) {
                    coeff = (d ? sin(floatd*M_PI)/(floatd*M_PI) : 1.0)*fone;
                } else if (flags & SWS_LANCZOS) {
                    double p= param[0] != SWS_PARAM_DEFAULT ? param[0] : 3.0;
                    coeff = (d ? sin(floatd*M_PI)*sin(floatd*M_PI/p)/(floatd*floatd*M_PI*M_PI/p) : 1.0)*fone;
                    if (floatd>p) coeff=0;
                } else if (flags & SWS_BILINEAR) {
                    coeff= (1<<30) - d;
                    if (coeff<0) coeff=0;
                    coeff *= fone >> 30;
                } else if (flags & SWS_SPLINE) {
                    double p=-2.196152422706632;
                    coeff = getSplineCoeff(1.0, 0.0, p, -p-1.0, floatd) * fone;
                } else {
                    coeff= 0.0; //GCC warning killer
                    assert(0);
                }

                filter[i*filterSize + j]= coeff;
                xx++;
            }
            xDstInSrc+= 2*xInc;
        }
    }

    /* apply src & dst Filter to filter -> filter2
       av_free(filter);
    */
    assert(filterSize>0);
    filter2Size= filterSize;
    if (srcFilter) filter2Size+= srcFilter->length - 1;
    if (dstFilter) filter2Size+= dstFilter->length - 1;
    assert(filter2Size>0);
    FF_ALLOCZ_OR_GOTO(NULL, filter2, filter2Size*dstW*sizeof(*filter2), fail);

    for (i=0; i<dstW; i++) {
        int j, k;

        if(srcFilter) {
            for (k=0; k<srcFilter->length; k++) {
                for (j=0; j<filterSize; j++)
                    filter2[i*filter2Size + k + j] += srcFilter->coeff[k]*filter[i*filterSize + j];
            }
        } else {
            for (j=0; j<filterSize; j++)
                filter2[i*filter2Size + j]= filter[i*filterSize + j];
        }
        //FIXME dstFilter

        (*filterPos)[i]+= (filterSize-1)/2 - (filter2Size-1)/2;
    }
    av_freep(&filter);

    /* try to reduce the filter-size (step1 find size and shift left) */
    // Assume it is near normalized (*0.5 or *2.0 is OK but * 0.001 is not).
    minFilterSize= 0;
    for (i=dstW-1; i>=0; i--) {
        int min= filter2Size;
        int j;
        int64_t cutOff=0.0;

        /* get rid of near zero elements on the left by shifting left */
        for (j=0; j<filter2Size; j++) {
            int k;
            cutOff += FFABS(filter2[i*filter2Size]);

            if (cutOff > SWS_MAX_REDUCE_CUTOFF*fone) break;

            /* preserve monotonicity because the core can't handle the filter otherwise */
            if (i<dstW-1 && (*filterPos)[i] >= (*filterPos)[i+1]) break;

            // move filter coefficients left
            for (k=1; k<filter2Size; k++)
                filter2[i*filter2Size + k - 1]= filter2[i*filter2Size + k];
            filter2[i*filter2Size + k - 1]= 0;
            (*filterPos)[i]++;
        }

        cutOff=0;
        /* count near zeros on the right */
        for (j=filter2Size-1; j>0; j--) {
            cutOff += FFABS(filter2[i*filter2Size + j]);

            if (cutOff > SWS_MAX_REDUCE_CUTOFF*fone) break;
            min--;
        }

        if (min>minFilterSize) minFilterSize= min;
    }

    if (HAVE_ALTIVEC && cpu_flags & AV_CPU_FLAG_ALTIVEC) {
        // we can handle the special case 4,
        // so we don't want to go to the full 8
        if (minFilterSize < 5)
            filterAlign = 4;

        // We really don't want to waste our time
        // doing useless computation, so fall back on
        // the scalar C code for very small filters.
        // Vectorizing is worth it only if you have a
        // decent-sized vector.
        if (minFilterSize < 3)
            filterAlign = 1;
    }

    if (HAVE_MMX && cpu_flags & AV_CPU_FLAG_MMX) {
        // special case for unscaled vertical filtering
        if (minFilterSize == 1 && filterAlign == 2)
            filterAlign= 1;
    }

    assert(minFilterSize > 0);
    filterSize= (minFilterSize +(filterAlign-1)) & (~(filterAlign-1));
    assert(filterSize > 0);
    filter= av_malloc(filterSize*dstW*sizeof(*filter));
    if (filterSize >= MAX_FILTER_SIZE*16/((flags&SWS_ACCURATE_RND) ? APCK_SIZE : 16) || !filter)
        goto fail;
    *outFilterSize= filterSize;

    if (flags&SWS_PRINT_INFO)
        av_log(NULL, AV_LOG_VERBOSE, "SwScaler: reducing / aligning filtersize %d -> %d\n", filter2Size, filterSize);
    /* try to reduce the filter-size (step2 reduce it) */
    for (i=0; i<dstW; i++) {
        int j;

        for (j=0; j<filterSize; j++) {
            if (j>=filter2Size) filter[i*filterSize + j]= 0;
            else               filter[i*filterSize + j]= filter2[i*filter2Size + j];
            if((flags & SWS_BITEXACT) && j>=minFilterSize)
                filter[i*filterSize + j]= 0;
        }
    }

    //FIXME try to align filterPos if possible

    //fix borders
    for (i=0; i<dstW; i++) {
        int j;
        if ((*filterPos)[i] < 0) {
            // move filter coefficients left to compensate for filterPos
            for (j=1; j<filterSize; j++) {
                int left= FFMAX(j + (*filterPos)[i], 0);
                filter[i*filterSize + left] += filter[i*filterSize + j];
                filter[i*filterSize + j]=0;
            }
            (*filterPos)[i]= 0;
        }

        if ((*filterPos)[i] + filterSize > srcW) {
            int shift= (*filterPos)[i] + filterSize - srcW;
            // move filter coefficients right to compensate for filterPos
            for (j=filterSize-2; j>=0; j--) {
                int right= FFMIN(j + shift, filterSize-1);
                filter[i*filterSize +right] += filter[i*filterSize +j];
                filter[i*filterSize +j]=0;
            }
            (*filterPos)[i]= srcW - filterSize;
        }
    }

    // Note the +1 is for the MMX scaler which reads over the end
    /* align at 16 for AltiVec (needed by hScale_altivec_real) */
    FF_ALLOCZ_OR_GOTO(NULL, *outFilter, *outFilterSize*(dstW+1)*sizeof(int16_t), fail);

    /* normalize & store in outFilter */
    for (i=0; i<dstW; i++) {
        int j;
        int64_t error=0;
        int64_t sum=0;

        for (j=0; j<filterSize; j++) {
            sum+= filter[i*filterSize + j];
        }
        sum= (sum + one/2)/ one;
        for (j=0; j<*outFilterSize; j++) {
            int64_t v= filter[i*filterSize + j] + error;
            int intV= ROUNDED_DIV(v, sum);
            (*outFilter)[i*(*outFilterSize) + j]= intV;
            error= v - intV*sum;
        }
    }

    (*filterPos)[dstW]= (*filterPos)[dstW-1]; // the MMX scaler will read over the end
    for (i=0; i<*outFilterSize; i++) {
        int j= dstW*(*outFilterSize);
        (*outFilter)[j + i]= (*outFilter)[j + i - (*outFilterSize)];
    }

    ret=0;
fail:
    av_free(filter);
    av_free(filter2);
    return ret;
}

#if HAVE_MMX2
static int initMMX2HScaler(int dstW, int xInc, uint8_t *filterCode, int16_t *filter, int32_t *filterPos, int numSplits)
{
    uint8_t *fragmentA;
    x86_reg imm8OfPShufW1A;
    x86_reg imm8OfPShufW2A;
    x86_reg fragmentLengthA;
    uint8_t *fragmentB;
    x86_reg imm8OfPShufW1B;
    x86_reg imm8OfPShufW2B;
    x86_reg fragmentLengthB;
    int fragmentPos;

    int xpos, i;

    // create an optimized horizontal scaling routine
    /* This scaler is made of runtime-generated MMX2 code using specially
     * tuned pshufw instructions. For every four output pixels, if four
     * input pixels are enough for the fast bilinear scaling, then a chunk
     * of fragmentB is used. If five input pixels are needed, then a chunk
     * of fragmentA is used.
     */

    //code fragment

    __asm__ volatile(
        "jmp                         9f                 \n\t"
    // Begin
        "0:                                             \n\t"
        "movq    (%%"REG_d", %%"REG_a"), %%mm3          \n\t"
        "movd    (%%"REG_c", %%"REG_S"), %%mm0          \n\t"
        "movd   1(%%"REG_c", %%"REG_S"), %%mm1          \n\t"
        "punpcklbw                %%mm7, %%mm1          \n\t"
        "punpcklbw                %%mm7, %%mm0          \n\t"
        "pshufw                   $0xFF, %%mm1, %%mm1   \n\t"
        "1:                                             \n\t"
        "pshufw                   $0xFF, %%mm0, %%mm0   \n\t"
        "2:                                             \n\t"
        "psubw                    %%mm1, %%mm0          \n\t"
        "movl   8(%%"REG_b", %%"REG_a"), %%esi          \n\t"
        "pmullw                   %%mm3, %%mm0          \n\t"
        "psllw                       $7, %%mm1          \n\t"
        "paddw                    %%mm1, %%mm0          \n\t"

        "movq                     %%mm0, (%%"REG_D", %%"REG_a") \n\t"

        "add                         $8, %%"REG_a"      \n\t"
    // End
        "9:                                             \n\t"
//        "int $3                                         \n\t"
        "lea                 " LOCAL_MANGLE(0b) ", %0   \n\t"
        "lea                 " LOCAL_MANGLE(1b) ", %1   \n\t"
        "lea                 " LOCAL_MANGLE(2b) ", %2   \n\t"
        "dec                         %1                 \n\t"
        "dec                         %2                 \n\t"
        "sub                         %0, %1             \n\t"
        "sub                         %0, %2             \n\t"
        "lea                 " LOCAL_MANGLE(9b) ", %3   \n\t"
        "sub                         %0, %3             \n\t"


        :"=r" (fragmentA), "=r" (imm8OfPShufW1A), "=r" (imm8OfPShufW2A),
        "=r" (fragmentLengthA)
    );

    __asm__ volatile(
        "jmp                         9f                 \n\t"
    // Begin
        "0:                                             \n\t"
        "movq    (%%"REG_d", %%"REG_a"), %%mm3          \n\t"
        "movd    (%%"REG_c", %%"REG_S"), %%mm0          \n\t"
        "punpcklbw                %%mm7, %%mm0          \n\t"
        "pshufw                   $0xFF, %%mm0, %%mm1   \n\t"
        "1:                                             \n\t"
        "pshufw                   $0xFF, %%mm0, %%mm0   \n\t"
        "2:                                             \n\t"
        "psubw                    %%mm1, %%mm0          \n\t"
        "movl   8(%%"REG_b", %%"REG_a"), %%esi          \n\t"
        "pmullw                   %%mm3, %%mm0          \n\t"
        "psllw                       $7, %%mm1          \n\t"
        "paddw                    %%mm1, %%mm0          \n\t"

        "movq                     %%mm0, (%%"REG_D", %%"REG_a") \n\t"

        "add                         $8, %%"REG_a"      \n\t"
    // End
        "9:                                             \n\t"
//        "int                       $3                   \n\t"
        "lea                 " LOCAL_MANGLE(0b) ", %0   \n\t"
        "lea                 " LOCAL_MANGLE(1b) ", %1   \n\t"
        "lea                 " LOCAL_MANGLE(2b) ", %2   \n\t"
        "dec                         %1                 \n\t"
        "dec                         %2                 \n\t"
        "sub                         %0, %1             \n\t"
        "sub                         %0, %2             \n\t"
        "lea                 " LOCAL_MANGLE(9b) ", %3   \n\t"
        "sub                         %0, %3             \n\t"


        :"=r" (fragmentB), "=r" (imm8OfPShufW1B), "=r" (imm8OfPShufW2B),
        "=r" (fragmentLengthB)
    );

    xpos= 0; //lumXInc/2 - 0x8000; // difference between pixel centers
    fragmentPos=0;

    for (i=0; i<dstW/numSplits; i++) {
        int xx=xpos>>16;

        if ((i&3) == 0) {
            int a=0;
            int b=((xpos+xInc)>>16) - xx;
            int c=((xpos+xInc*2)>>16) - xx;
            int d=((xpos+xInc*3)>>16) - xx;
            int inc                = (d+1<4);
            uint8_t *fragment      = (d+1<4) ? fragmentB       : fragmentA;
            x86_reg imm8OfPShufW1  = (d+1<4) ? imm8OfPShufW1B  : imm8OfPShufW1A;
            x86_reg imm8OfPShufW2  = (d+1<4) ? imm8OfPShufW2B  : imm8OfPShufW2A;
            x86_reg fragmentLength = (d+1<4) ? fragmentLengthB : fragmentLengthA;
            int maxShift= 3-(d+inc);
            int shift=0;

            if (filterCode) {
                filter[i  ] = (( xpos         & 0xFFFF) ^ 0xFFFF)>>9;
                filter[i+1] = (((xpos+xInc  ) & 0xFFFF) ^ 0xFFFF)>>9;
                filter[i+2] = (((xpos+xInc*2) & 0xFFFF) ^ 0xFFFF)>>9;
                filter[i+3] = (((xpos+xInc*3) & 0xFFFF) ^ 0xFFFF)>>9;
                filterPos[i/2]= xx;

                memcpy(filterCode + fragmentPos, fragment, fragmentLength);

                filterCode[fragmentPos + imm8OfPShufW1]=
                    (a+inc) | ((b+inc)<<2) | ((c+inc)<<4) | ((d+inc)<<6);
                filterCode[fragmentPos + imm8OfPShufW2]=
                    a | (b<<2) | (c<<4) | (d<<6);

                if (i+4-inc>=dstW) shift=maxShift; //avoid overread
                else if ((filterPos[i/2]&3) <= maxShift) shift=filterPos[i/2]&3; //Align

                if (shift && i>=shift) {
                    filterCode[fragmentPos + imm8OfPShufW1]+= 0x55*shift;
                    filterCode[fragmentPos + imm8OfPShufW2]+= 0x55*shift;
                    filterPos[i/2]-=shift;
                }
            }

            fragmentPos+= fragmentLength;

            if (filterCode)
                filterCode[fragmentPos]= RET;
        }
        xpos+=xInc;
    }
    if (filterCode)
        filterPos[((i/2)+1)&(~1)]= xpos>>16; // needed to jump to the next part

    return fragmentPos + 1;
}
#endif /* HAVE_MMX2 */

static void getSubSampleFactors(int *h, int *v, enum PixelFormat format)
{
    *h = av_pix_fmt_descriptors[format].log2_chroma_w;
    *v = av_pix_fmt_descriptors[format].log2_chroma_h;
}

int sws_setColorspaceDetails(struct SwsContext *c, const int inv_table[4],
                             int srcRange, const int table[4], int dstRange,
                             int brightness, int contrast, int saturation)
{
    memcpy(c->srcColorspaceTable, inv_table, sizeof(int)*4);
    memcpy(c->dstColorspaceTable,     table, sizeof(int)*4);

    c->brightness= brightness;
    c->contrast  = contrast;
    c->saturation= saturation;
    c->srcRange  = srcRange;
    c->dstRange  = dstRange;
    if (isYUV(c->dstFormat) || isGray(c->dstFormat)) return -1;

    c->dstFormatBpp = av_get_bits_per_pixel(&av_pix_fmt_descriptors[c->dstFormat]);
    c->srcFormatBpp = av_get_bits_per_pixel(&av_pix_fmt_descriptors[c->srcFormat]);

    ff_yuv2rgb_c_init_tables(c, inv_table, srcRange, brightness, contrast, saturation);
    //FIXME factorize

    if (HAVE_ALTIVEC && av_get_cpu_flags() & AV_CPU_FLAG_ALTIVEC)
        ff_yuv2rgb_init_tables_altivec(c, inv_table, brightness, contrast, saturation);
    return 0;
}

int sws_getColorspaceDetails(struct SwsContext *c, int **inv_table,
                             int *srcRange, int **table, int *dstRange,
                             int *brightness, int *contrast, int *saturation)
{
    if (isYUV(c->dstFormat) || isGray(c->dstFormat)) return -1;

    *inv_table = c->srcColorspaceTable;
    *table     = c->dstColorspaceTable;
    *srcRange  = c->srcRange;
    *dstRange  = c->dstRange;
    *brightness= c->brightness;
    *contrast  = c->contrast;
    *saturation= c->saturation;

    return 0;
}

static int handle_jpeg(enum PixelFormat *format)
{
    switch (*format) {
    case PIX_FMT_YUVJ420P: *format = PIX_FMT_YUV420P; return 1;
    case PIX_FMT_YUVJ422P: *format = PIX_FMT_YUV422P; return 1;
    case PIX_FMT_YUVJ444P: *format = PIX_FMT_YUV444P; return 1;
    case PIX_FMT_YUVJ440P: *format = PIX_FMT_YUV440P; return 1;
    default:                                          return 0;
    }
}

SwsContext *sws_alloc_context(void)
{
    SwsContext *c= av_mallocz(sizeof(SwsContext));

    c->av_class = &sws_context_class;
    av_opt_set_defaults(c);

    return c;
}

int sws_init_context(SwsContext *c, SwsFilter *srcFilter, SwsFilter *dstFilter)
{
    int i;
    int usesVFilter, usesHFilter;
    int unscaled;
    SwsFilter dummyFilter= {NULL, NULL, NULL, NULL};
    int srcW= c->srcW;
    int srcH= c->srcH;
    int dstW= c->dstW;
    int dstH= c->dstH;
    int dst_stride = FFALIGN(dstW * sizeof(int16_t) + 16, 16), dst_stride_px = dst_stride >> 1;
    int flags, cpu_flags;
    enum PixelFormat srcFormat= c->srcFormat;
    enum PixelFormat dstFormat= c->dstFormat;
#ifdef GEKKO
	cpu_flags = 0;
#else
    cpu_flags = av_get_cpu_flags();
#endif
    flags     = c->flags;
    emms_c();
    if (!rgb15to16) sws_rgb2rgb_init();

    unscaled = (srcW == dstW && srcH == dstH);

    if (!sws_isSupportedInput(srcFormat)) {
        av_log(c, AV_LOG_ERROR, "%s is not supported as input pixel format\n", sws_format_name(srcFormat));
        return AVERROR(EINVAL);
    }
    if (!sws_isSupportedOutput(dstFormat)) {
        av_log(c, AV_LOG_ERROR, "%s is not supported as output pixel format\n", sws_format_name(dstFormat));
        return AVERROR(EINVAL);
    }

    i= flags & ( SWS_POINT
                |SWS_AREA
                |SWS_BILINEAR
                |SWS_FAST_BILINEAR
                |SWS_BICUBIC
                |SWS_X
                |SWS_GAUSS
                |SWS_LANCZOS
                |SWS_SINC
                |SWS_SPLINE
                |SWS_BICUBLIN);
    if(!i || (i & (i-1))) {
        av_log(c, AV_LOG_ERROR, "Exactly one scaler algorithm must be chosen\n");
        return AVERROR(EINVAL);
    }
    /* sanity check */
    if (srcW<4 || srcH<1 || dstW<8 || dstH<1) { //FIXME check if these are enough and try to lowwer them after fixing the relevant parts of the code
        av_log(c, AV_LOG_ERROR, "%dx%d -> %dx%d is invalid scaling dimension\n",
               srcW, srcH, dstW, dstH);
        return AVERROR(EINVAL);
    }

    if (!dstFilter) dstFilter= &dummyFilter;
    if (!srcFilter) srcFilter= &dummyFilter;

    c->lumXInc= ((srcW<<16) + (dstW>>1))/dstW;
    c->lumYInc= ((srcH<<16) + (dstH>>1))/dstH;
    c->dstFormatBpp = av_get_bits_per_pixel(&av_pix_fmt_descriptors[dstFormat]);
    c->srcFormatBpp = av_get_bits_per_pixel(&av_pix_fmt_descriptors[srcFormat]);
    c->vRounder= 4* 0x0001000100010001ULL;

    usesVFilter = (srcFilter->lumV && srcFilter->lumV->length>1) ||
                  (srcFilter->chrV && srcFilter->chrV->length>1) ||
                  (dstFilter->lumV && dstFilter->lumV->length>1) ||
                  (dstFilter->chrV && dstFilter->chrV->length>1);
    usesHFilter = (srcFilter->lumH && srcFilter->lumH->length>1) ||
                  (srcFilter->chrH && srcFilter->chrH->length>1) ||
                  (dstFilter->lumH && dstFilter->lumH->length>1) ||
                  (dstFilter->chrH && dstFilter->chrH->length>1);

    getSubSampleFactors(&c->chrSrcHSubSample, &c->chrSrcVSubSample, srcFormat);
    getSubSampleFactors(&c->chrDstHSubSample, &c->chrDstVSubSample, dstFormat);

    // reuse chroma for 2 pixels RGB/BGR unless user wants full chroma interpolation
    if (flags & SWS_FULL_CHR_H_INT &&
        dstFormat != PIX_FMT_RGBA &&
        dstFormat != PIX_FMT_ARGB &&
        dstFormat != PIX_FMT_BGRA &&
        dstFormat != PIX_FMT_ABGR &&
        dstFormat != PIX_FMT_RGB24 &&
        dstFormat != PIX_FMT_BGR24) {
        av_log(c, AV_LOG_ERROR,
               "full chroma interpolation for destination format '%s' not yet implemented\n",
               sws_format_name(dstFormat));
        flags &= ~SWS_FULL_CHR_H_INT;
        c->flags = flags;
    }
    if (isAnyRGB(dstFormat) && !(flags&SWS_FULL_CHR_H_INT)) c->chrDstHSubSample=1;

    // drop some chroma lines if the user wants it
    c->vChrDrop= (flags&SWS_SRC_V_CHR_DROP_MASK)>>SWS_SRC_V_CHR_DROP_SHIFT;
    c->chrSrcVSubSample+= c->vChrDrop;

    // drop every other pixel for chroma calculation unless user wants full chroma
    if (isAnyRGB(srcFormat) && !(flags&SWS_FULL_CHR_H_INP)
      && srcFormat!=PIX_FMT_RGB8      && srcFormat!=PIX_FMT_BGR8
      && srcFormat!=PIX_FMT_RGB4      && srcFormat!=PIX_FMT_BGR4
      && srcFormat!=PIX_FMT_RGB4_BYTE && srcFormat!=PIX_FMT_BGR4_BYTE
      && ((dstW>>c->chrDstHSubSample) <= (srcW>>1) || (flags&SWS_FAST_BILINEAR)))
        c->chrSrcHSubSample=1;

    // Note the -((-x)>>y) is so that we always round toward +inf.
    c->chrSrcW= -((-srcW) >> c->chrSrcHSubSample);
    c->chrSrcH= -((-srcH) >> c->chrSrcVSubSample);
    c->chrDstW= -((-dstW) >> c->chrDstHSubSample);
    c->chrDstH= -((-dstH) >> c->chrDstVSubSample);

    /* unscaled special cases */
    if (unscaled && !usesHFilter && !usesVFilter && (c->srcRange == c->dstRange || isAnyRGB(dstFormat))) {
        ff_get_unscaled_swscale(c);

        if (c->swScale) {
            if (flags&SWS_PRINT_INFO)
                av_log(c, AV_LOG_INFO, "using unscaled %s -> %s special converter\n",
                       sws_format_name(srcFormat), sws_format_name(dstFormat));
            return 0;
        }
    }

    c->srcBpc = 1 + av_pix_fmt_descriptors[srcFormat].comp[0].depth_minus1;
    if (c->srcBpc < 8)
        c->srcBpc = 8;
    c->dstBpc = 1 + av_pix_fmt_descriptors[dstFormat].comp[0].depth_minus1;
    if (c->dstBpc < 8)
        c->dstBpc = 8;
    if (c->dstBpc == 16)
        dst_stride <<= 1;
    FF_ALLOC_OR_GOTO(c, c->formatConvBuffer,
                     FFALIGN(srcW, 16) * 2 * FFALIGN(c->srcBpc, 8) >> 3,
                     fail);
    if (HAVE_MMX2 && cpu_flags & AV_CPU_FLAG_MMX2 && c->srcBpc == 8 && c->dstBpc <= 10) {
        c->canMMX2BeUsed= (dstW >=srcW && (dstW&31)==0 && (srcW&15)==0) ? 1 : 0;
        if (!c->canMMX2BeUsed && dstW >=srcW && (srcW&15)==0 && (flags&SWS_FAST_BILINEAR)) {
            if (flags&SWS_PRINT_INFO)
                av_log(c, AV_LOG_INFO, "output width is not a multiple of 32 -> no MMX2 scaler\n");
        }
        if (usesHFilter) c->canMMX2BeUsed=0;
    }
    else
        c->canMMX2BeUsed=0;

    c->chrXInc= ((c->chrSrcW<<16) + (c->chrDstW>>1))/c->chrDstW;
    c->chrYInc= ((c->chrSrcH<<16) + (c->chrDstH>>1))/c->chrDstH;

    // match pixel 0 of the src to pixel 0 of dst and match pixel n-2 of src to pixel n-2 of dst
    // but only for the FAST_BILINEAR mode otherwise do correct scaling
    // n-2 is the last chrominance sample available
    // this is not perfect, but no one should notice the difference, the more correct variant
    // would be like the vertical one, but that would require some special code for the
    // first and last pixel
    if (flags&SWS_FAST_BILINEAR) {
        if (c->canMMX2BeUsed) {
            c->lumXInc+= 20;
            c->chrXInc+= 20;
        }
        //we don't use the x86 asm scaler if MMX is available
        else if (HAVE_MMX && cpu_flags & AV_CPU_FLAG_MMX) {
            c->lumXInc = ((srcW-2)<<16)/(dstW-2) - 20;
            c->chrXInc = ((c->chrSrcW-2)<<16)/(c->chrDstW-2) - 20;
        }
    }

    /* precalculate horizontal scaler filter coefficients */
    {
#if HAVE_MMX2
// can't downscale !!!
        if (c->canMMX2BeUsed && (flags & SWS_FAST_BILINEAR)) {
            c->lumMmx2FilterCodeSize = initMMX2HScaler(      dstW, c->lumXInc, NULL, NULL, NULL, 8);
            c->chrMmx2FilterCodeSize = initMMX2HScaler(c->chrDstW, c->chrXInc, NULL, NULL, NULL, 4);

#ifdef MAP_ANONYMOUS
            c->lumMmx2FilterCode = mmap(NULL, c->lumMmx2FilterCodeSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            c->chrMmx2FilterCode = mmap(NULL, c->chrMmx2FilterCodeSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#elif HAVE_VIRTUALALLOC
            c->lumMmx2FilterCode = VirtualAlloc(NULL, c->lumMmx2FilterCodeSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
            c->chrMmx2FilterCode = VirtualAlloc(NULL, c->chrMmx2FilterCodeSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#else
            c->lumMmx2FilterCode = av_malloc(c->lumMmx2FilterCodeSize);
            c->chrMmx2FilterCode = av_malloc(c->chrMmx2FilterCodeSize);
#endif

            if (!c->lumMmx2FilterCode || !c->chrMmx2FilterCode)
                return AVERROR(ENOMEM);
            FF_ALLOCZ_OR_GOTO(c, c->hLumFilter   , (dstW        /8+8)*sizeof(int16_t), fail);
            FF_ALLOCZ_OR_GOTO(c, c->hChrFilter   , (c->chrDstW  /4+8)*sizeof(int16_t), fail);
            FF_ALLOCZ_OR_GOTO(c, c->hLumFilterPos, (dstW      /2/8+8)*sizeof(int32_t), fail);
            FF_ALLOCZ_OR_GOTO(c, c->hChrFilterPos, (c->chrDstW/2/4+8)*sizeof(int32_t), fail);

            initMMX2HScaler(      dstW, c->lumXInc, c->lumMmx2FilterCode, c->hLumFilter, c->hLumFilterPos, 8);
            initMMX2HScaler(c->chrDstW, c->chrXInc, c->chrMmx2FilterCode, c->hChrFilter, c->hChrFilterPos, 4);

#ifdef MAP_ANONYMOUS
            mprotect(c->lumMmx2FilterCode, c->lumMmx2FilterCodeSize, PROT_EXEC | PROT_READ);
            mprotect(c->chrMmx2FilterCode, c->chrMmx2FilterCodeSize, PROT_EXEC | PROT_READ);
#endif
        } else
#endif /* HAVE_MMX2 */
        {
            const int filterAlign=
                (HAVE_MMX     && cpu_flags & AV_CPU_FLAG_MMX) ? 4 :
                (HAVE_ALTIVEC && cpu_flags & AV_CPU_FLAG_ALTIVEC) ? 8 :
                1;

            if (initFilter(&c->hLumFilter, &c->hLumFilterPos, &c->hLumFilterSize, c->lumXInc,
                           srcW      ,       dstW, filterAlign, 1<<14,
                           (flags&SWS_BICUBLIN) ? (flags|SWS_BICUBIC)  : flags, cpu_flags,
                           srcFilter->lumH, dstFilter->lumH, c->param) < 0)
                goto fail;
            if (initFilter(&c->hChrFilter, &c->hChrFilterPos, &c->hChrFilterSize, c->chrXInc,
                           c->chrSrcW, c->chrDstW, filterAlign, 1<<14,
                           (flags&SWS_BICUBLIN) ? (flags|SWS_BILINEAR) : flags, cpu_flags,
                           srcFilter->chrH, dstFilter->chrH, c->param) < 0)
                goto fail;
        }
    } // initialize horizontal stuff

    /* precalculate vertical scaler filter coefficients */
    {
        const int filterAlign=
            (HAVE_MMX     && cpu_flags & AV_CPU_FLAG_MMX) && (flags & SWS_ACCURATE_RND) ? 2 :
            (HAVE_ALTIVEC && cpu_flags & AV_CPU_FLAG_ALTIVEC) ? 8 :
            1;

        if (initFilter(&c->vLumFilter, &c->vLumFilterPos, &c->vLumFilterSize, c->lumYInc,
                       srcH      ,        dstH, filterAlign, (1<<12),
                       (flags&SWS_BICUBLIN) ? (flags|SWS_BICUBIC)  : flags, cpu_flags,
                       srcFilter->lumV, dstFilter->lumV, c->param) < 0)
            goto fail;
        if (initFilter(&c->vChrFilter, &c->vChrFilterPos, &c->vChrFilterSize, c->chrYInc,
                       c->chrSrcH, c->chrDstH, filterAlign, (1<<12),
                       (flags&SWS_BICUBLIN) ? (flags|SWS_BILINEAR) : flags, cpu_flags,
                       srcFilter->chrV, dstFilter->chrV, c->param) < 0)
            goto fail;

#if HAVE_ALTIVEC
        FF_ALLOC_OR_GOTO(c, c->vYCoeffsBank, sizeof (vector signed short)*c->vLumFilterSize*c->dstH, fail);
        FF_ALLOC_OR_GOTO(c, c->vCCoeffsBank, sizeof (vector signed short)*c->vChrFilterSize*c->chrDstH, fail);

        for (i=0;i<c->vLumFilterSize*c->dstH;i++) {
            int j;
            short *p = (short *)&c->vYCoeffsBank[i];
            for (j=0;j<8;j++)
                p[j] = c->vLumFilter[i];
        }

        for (i=0;i<c->vChrFilterSize*c->chrDstH;i++) {
            int j;
            short *p = (short *)&c->vCCoeffsBank[i];
            for (j=0;j<8;j++)
                p[j] = c->vChrFilter[i];
        }
#endif
    }

    // calculate buffer sizes so that they won't run out while handling these damn slices
    c->vLumBufSize= c->vLumFilterSize;
    c->vChrBufSize= c->vChrFilterSize;
    for (i=0; i<dstH; i++) {
        int chrI= i*c->chrDstH / dstH;
        int nextSlice= FFMAX(c->vLumFilterPos[i   ] + c->vLumFilterSize - 1,
                           ((c->vChrFilterPos[chrI] + c->vChrFilterSize - 1)<<c->chrSrcVSubSample));

        nextSlice>>= c->chrSrcVSubSample;
        nextSlice<<= c->chrSrcVSubSample;
        if (c->vLumFilterPos[i   ] + c->vLumBufSize < nextSlice)
            c->vLumBufSize= nextSlice - c->vLumFilterPos[i];
        if (c->vChrFilterPos[chrI] + c->vChrBufSize < (nextSlice>>c->chrSrcVSubSample))
            c->vChrBufSize= (nextSlice>>c->chrSrcVSubSample) - c->vChrFilterPos[chrI];
    }

    // allocate pixbufs (we use dynamic allocation because otherwise we would need to
    // allocate several megabytes to handle all possible cases)
    FF_ALLOC_OR_GOTO(c, c->lumPixBuf, c->vLumBufSize*2*sizeof(int16_t*), fail);
    FF_ALLOC_OR_GOTO(c, c->chrUPixBuf, c->vChrBufSize*2*sizeof(int16_t*), fail);
    FF_ALLOC_OR_GOTO(c, c->chrVPixBuf, c->vChrBufSize*2*sizeof(int16_t*), fail);
    if (CONFIG_SWSCALE_ALPHA && isALPHA(c->srcFormat) && isALPHA(c->dstFormat))
        FF_ALLOCZ_OR_GOTO(c, c->alpPixBuf, c->vLumBufSize*2*sizeof(int16_t*), fail);
    //Note we need at least one pixel more at the end because of the MMX code (just in case someone wanna replace the 4000/8000)
    /* align at 16 bytes for AltiVec */
    for (i=0; i<c->vLumBufSize; i++) {
        FF_ALLOCZ_OR_GOTO(c, c->lumPixBuf[i+c->vLumBufSize], dst_stride+16, fail);
        c->lumPixBuf[i] = c->lumPixBuf[i+c->vLumBufSize];
    }
    // 64 / (c->dstBpc & ~7) is the same as 16 / sizeof(scaling_intermediate)
    c->uv_off_px   = dst_stride_px + 64 / (c->dstBpc &~ 7);
    c->uv_off_byte = dst_stride + 16;
    for (i=0; i<c->vChrBufSize; i++) {
        FF_ALLOC_OR_GOTO(c, c->chrUPixBuf[i+c->vChrBufSize], dst_stride*2+32, fail);
        c->chrUPixBuf[i] = c->chrUPixBuf[i+c->vChrBufSize];
        c->chrVPixBuf[i] = c->chrVPixBuf[i+c->vChrBufSize] = c->chrUPixBuf[i] + (dst_stride >> 1) + 8;
    }
    if (CONFIG_SWSCALE_ALPHA && c->alpPixBuf)
        for (i=0; i<c->vLumBufSize; i++) {
            FF_ALLOCZ_OR_GOTO(c, c->alpPixBuf[i+c->vLumBufSize], dst_stride+16, fail);
            c->alpPixBuf[i] = c->alpPixBuf[i+c->vLumBufSize];
        }

    //try to avoid drawing green stuff between the right end and the stride end
    for (i=0; i<c->vChrBufSize; i++)
        memset(c->chrUPixBuf[i], 64, dst_stride*2+1);

    assert(c->chrDstH <= dstH);

    if (flags&SWS_PRINT_INFO) {
        if      (flags&SWS_FAST_BILINEAR) av_log(c, AV_LOG_INFO, "FAST_BILINEAR scaler, ");
        else if (flags&SWS_BILINEAR)      av_log(c, AV_LOG_INFO, "BILINEAR scaler, ");
        else if (flags&SWS_BICUBIC)       av_log(c, AV_LOG_INFO, "BICUBIC scaler, ");
        else if (flags&SWS_X)             av_log(c, AV_LOG_INFO, "Experimental scaler, ");
        else if (flags&SWS_POINT)         av_log(c, AV_LOG_INFO, "Nearest Neighbor / POINT scaler, ");
        else if (flags&SWS_AREA)          av_log(c, AV_LOG_INFO, "Area Averaging scaler, ");
        else if (flags&SWS_BICUBLIN)      av_log(c, AV_LOG_INFO, "luma BICUBIC / chroma BILINEAR scaler, ");
        else if (flags&SWS_GAUSS)         av_log(c, AV_LOG_INFO, "Gaussian scaler, ");
        else if (flags&SWS_SINC)          av_log(c, AV_LOG_INFO, "Sinc scaler, ");
        else if (flags&SWS_LANCZOS)       av_log(c, AV_LOG_INFO, "Lanczos scaler, ");
        else if (flags&SWS_SPLINE)        av_log(c, AV_LOG_INFO, "Bicubic spline scaler, ");
        else                              av_log(c, AV_LOG_INFO, "ehh flags invalid?! ");

        av_log(c, AV_LOG_INFO, "from %s to %s%s ",
               sws_format_name(srcFormat),
#ifdef DITHER1XBPP
               dstFormat == PIX_FMT_BGR555 || dstFormat == PIX_FMT_BGR565 ||
               dstFormat == PIX_FMT_RGB444BE || dstFormat == PIX_FMT_RGB444LE ||
               dstFormat == PIX_FMT_BGR444BE || dstFormat == PIX_FMT_BGR444LE ? "dithered " : "",
#else
               "",
#endif
               sws_format_name(dstFormat));

        if      (HAVE_MMX2     && cpu_flags & AV_CPU_FLAG_MMX2)    av_log(c, AV_LOG_INFO, "using MMX2\n");
        else if (HAVE_AMD3DNOW && cpu_flags & AV_CPU_FLAG_3DNOW)   av_log(c, AV_LOG_INFO, "using 3DNOW\n");
        else if (HAVE_MMX      && cpu_flags & AV_CPU_FLAG_MMX)     av_log(c, AV_LOG_INFO, "using MMX\n");
        else if (HAVE_ALTIVEC  && cpu_flags & AV_CPU_FLAG_ALTIVEC) av_log(c, AV_LOG_INFO, "using AltiVec\n");
        else                                   av_log(c, AV_LOG_INFO, "using C\n");

        av_log(c, AV_LOG_VERBOSE, "%dx%d -> %dx%d\n", srcW, srcH, dstW, dstH);
        av_log(c, AV_LOG_DEBUG, "lum srcW=%d srcH=%d dstW=%d dstH=%d xInc=%d yInc=%d\n",
               c->srcW, c->srcH, c->dstW, c->dstH, c->lumXInc, c->lumYInc);
        av_log(c, AV_LOG_DEBUG, "chr srcW=%d srcH=%d dstW=%d dstH=%d xInc=%d yInc=%d\n",
               c->chrSrcW, c->chrSrcH, c->chrDstW, c->chrDstH, c->chrXInc, c->chrYInc);
    }

    c->swScale= ff_getSwsFunc(c);
    return 0;
fail: //FIXME replace things by appropriate error codes
    return -1;
}

#if FF_API_SWS_GETCONTEXT
SwsContext *sws_getContext(int srcW, int srcH, enum PixelFormat srcFormat,
                           int dstW, int dstH, enum PixelFormat dstFormat, int flags,
                           SwsFilter *srcFilter, SwsFilter *dstFilter, const double *param)
{
    SwsContext *c;

    if(!(c=sws_alloc_context()))
        return NULL;

    c->flags= flags;
    c->srcW= srcW;
    c->srcH= srcH;
    c->dstW= dstW;
    c->dstH= dstH;
    c->srcRange = handle_jpeg(&srcFormat);
    c->dstRange = handle_jpeg(&dstFormat);
    c->srcFormat= srcFormat;
    c->dstFormat= dstFormat;

    if (param) {
        c->param[0] = param[0];
        c->param[1] = param[1];
    }
    sws_setColorspaceDetails(c, ff_yuv2rgb_coeffs[SWS_CS_DEFAULT], c->srcRange, ff_yuv2rgb_coeffs[SWS_CS_DEFAULT] /* FIXME*/, c->dstRange, 0, 1<<16, 1<<16);

    if(sws_init_context(c, srcFilter, dstFilter) < 0){
        sws_freeContext(c);
        return NULL;
    }

    return c;
}
#endif

SwsFilter *sws_getDefaultFilter(float lumaGBlur, float chromaGBlur,
                                float lumaSharpen, float chromaSharpen,
                                float chromaHShift, float chromaVShift,
                                int verbose)
{
    SwsFilter *filter= av_malloc(sizeof(SwsFilter));
    if (!filter)
        return NULL;

    if (lumaGBlur!=0.0) {
        filter->lumH= sws_getGaussianVec(lumaGBlur, 3.0);
        filter->lumV= sws_getGaussianVec(lumaGBlur, 3.0);
    } else {
        filter->lumH= sws_getIdentityVec();
        filter->lumV= sws_getIdentityVec();
    }

    if (chromaGBlur!=0.0) {
        filter->chrH= sws_getGaussianVec(chromaGBlur, 3.0);
        filter->chrV= sws_getGaussianVec(chromaGBlur, 3.0);
    } else {
        filter->chrH= sws_getIdentityVec();
        filter->chrV= sws_getIdentityVec();
    }

    if (chromaSharpen!=0.0) {
        SwsVector *id= sws_getIdentityVec();
        sws_scaleVec(filter->chrH, -chromaSharpen);
        sws_scaleVec(filter->chrV, -chromaSharpen);
        sws_addVec(filter->chrH, id);
        sws_addVec(filter->chrV, id);
        sws_freeVec(id);
    }

    if (lumaSharpen!=0.0) {
        SwsVector *id= sws_getIdentityVec();
        sws_scaleVec(filter->lumH, -lumaSharpen);
        sws_scaleVec(filter->lumV, -lumaSharpen);
        sws_addVec(filter->lumH, id);
        sws_addVec(filter->lumV, id);
        sws_freeVec(id);
    }

    if (chromaHShift != 0.0)
        sws_shiftVec(filter->chrH, (int)(chromaHShift+0.5));

    if (chromaVShift != 0.0)
        sws_shiftVec(filter->chrV, (int)(chromaVShift+0.5));

    sws_normalizeVec(filter->chrH, 1.0);
    sws_normalizeVec(filter->chrV, 1.0);
    sws_normalizeVec(filter->lumH, 1.0);
    sws_normalizeVec(filter->lumV, 1.0);

    if (verbose) sws_printVec2(filter->chrH, NULL, AV_LOG_DEBUG);
    if (verbose) sws_printVec2(filter->lumH, NULL, AV_LOG_DEBUG);

    return filter;
}

SwsVector *sws_allocVec(int length)
{
    SwsVector *vec = av_malloc(sizeof(SwsVector));
    if (!vec)
        return NULL;
    vec->length = length;
    vec->coeff  = av_malloc(sizeof(double) * length);
    if (!vec->coeff)
        av_freep(&vec);
    return vec;
}

SwsVector *sws_getGaussianVec(double variance, double quality)
{
    const int length= (int)(variance*quality + 0.5) | 1;
    int i;
    double middle= (length-1)*0.5;
    SwsVector *vec= sws_allocVec(length);

    if (!vec)
        return NULL;

    for (i=0; i<length; i++) {
        double dist= i-middle;
        vec->coeff[i]= exp(-dist*dist/(2*variance*variance)) / sqrt(2*variance*M_PI);
    }

    sws_normalizeVec(vec, 1.0);

    return vec;
}

SwsVector *sws_getConstVec(double c, int length)
{
    int i;
    SwsVector *vec= sws_allocVec(length);

    if (!vec)
        return NULL;

    for (i=0; i<length; i++)
        vec->coeff[i]= c;

    return vec;
}

SwsVector *sws_getIdentityVec(void)
{
    return sws_getConstVec(1.0, 1);
}

static double sws_dcVec(SwsVector *a)
{
    int i;
    double sum=0;

    for (i=0; i<a->length; i++)
        sum+= a->coeff[i];

    return sum;
}

void sws_scaleVec(SwsVector *a, double scalar)
{
    int i;

    for (i=0; i<a->length; i++)
        a->coeff[i]*= scalar;
}

void sws_normalizeVec(SwsVector *a, double height)
{
    sws_scaleVec(a, height/sws_dcVec(a));
}

static SwsVector *sws_getConvVec(SwsVector *a, SwsVector *b)
{
    int length= a->length + b->length - 1;
    int i, j;
    SwsVector *vec= sws_getConstVec(0.0, length);

    if (!vec)
        return NULL;

    for (i=0; i<a->length; i++) {
        for (j=0; j<b->length; j++) {
            vec->coeff[i+j]+= a->coeff[i]*b->coeff[j];
        }
    }

    return vec;
}

static SwsVector *sws_sumVec(SwsVector *a, SwsVector *b)
{
    int length= FFMAX(a->length, b->length);
    int i;
    SwsVector *vec= sws_getConstVec(0.0, length);

    if (!vec)
        return NULL;

    for (i=0; i<a->length; i++) vec->coeff[i + (length-1)/2 - (a->length-1)/2]+= a->coeff[i];
    for (i=0; i<b->length; i++) vec->coeff[i + (length-1)/2 - (b->length-1)/2]+= b->coeff[i];

    return vec;
}

static SwsVector *sws_diffVec(SwsVector *a, SwsVector *b)
{
    int length= FFMAX(a->length, b->length);
    int i;
    SwsVector *vec= sws_getConstVec(0.0, length);

    if (!vec)
        return NULL;

    for (i=0; i<a->length; i++) vec->coeff[i + (length-1)/2 - (a->length-1)/2]+= a->coeff[i];
    for (i=0; i<b->length; i++) vec->coeff[i + (length-1)/2 - (b->length-1)/2]-= b->coeff[i];

    return vec;
}

/* shift left / or right if "shift" is negative */
static SwsVector *sws_getShiftedVec(SwsVector *a, int shift)
{
    int length= a->length + FFABS(shift)*2;
    int i;
    SwsVector *vec= sws_getConstVec(0.0, length);

    if (!vec)
        return NULL;

    for (i=0; i<a->length; i++) {
        vec->coeff[i + (length-1)/2 - (a->length-1)/2 - shift]= a->coeff[i];
    }

    return vec;
}

void sws_shiftVec(SwsVector *a, int shift)
{
    SwsVector *shifted= sws_getShiftedVec(a, shift);
    av_free(a->coeff);
    a->coeff= shifted->coeff;
    a->length= shifted->length;
    av_free(shifted);
}

void sws_addVec(SwsVector *a, SwsVector *b)
{
    SwsVector *sum= sws_sumVec(a, b);
    av_free(a->coeff);
    a->coeff= sum->coeff;
    a->length= sum->length;
    av_free(sum);
}

void sws_subVec(SwsVector *a, SwsVector *b)
{
    SwsVector *diff= sws_diffVec(a, b);
    av_free(a->coeff);
    a->coeff= diff->coeff;
    a->length= diff->length;
    av_free(diff);
}

void sws_convVec(SwsVector *a, SwsVector *b)
{
    SwsVector *conv= sws_getConvVec(a, b);
    av_free(a->coeff);
    a->coeff= conv->coeff;
    a->length= conv->length;
    av_free(conv);
}

SwsVector *sws_cloneVec(SwsVector *a)
{
    int i;
    SwsVector *vec= sws_allocVec(a->length);

    if (!vec)
        return NULL;

    for (i=0; i<a->length; i++) vec->coeff[i]= a->coeff[i];

    return vec;
}

void sws_printVec2(SwsVector *a, AVClass *log_ctx, int log_level)
{
    int i;
    double max=0;
    double min=0;
    double range;

    for (i=0; i<a->length; i++)
        if (a->coeff[i]>max) max= a->coeff[i];

    for (i=0; i<a->length; i++)
        if (a->coeff[i]<min) min= a->coeff[i];

    range= max - min;

    for (i=0; i<a->length; i++) {
        int x= (int)((a->coeff[i]-min)*60.0/range +0.5);
        av_log(log_ctx, log_level, "%1.3f ", a->coeff[i]);
        for (;x>0; x--) av_log(log_ctx, log_level, " ");
        av_log(log_ctx, log_level, "|\n");
    }
}

void sws_freeVec(SwsVector *a)
{
    if (!a) return;
    av_freep(&a->coeff);
    a->length=0;
    av_free(a);
}

void sws_freeFilter(SwsFilter *filter)
{
    if (!filter) return;

    if (filter->lumH) sws_freeVec(filter->lumH);
    if (filter->lumV) sws_freeVec(filter->lumV);
    if (filter->chrH) sws_freeVec(filter->chrH);
    if (filter->chrV) sws_freeVec(filter->chrV);
    av_free(filter);
}

void sws_freeContext(SwsContext *c)
{
    int i;
    if (!c) return;

    if (c->lumPixBuf) {
        for (i=0; i<c->vLumBufSize; i++)
            av_freep(&c->lumPixBuf[i]);
        av_freep(&c->lumPixBuf);
    }

    if (c->chrUPixBuf) {
        for (i=0; i<c->vChrBufSize; i++)
            av_freep(&c->chrUPixBuf[i]);
        av_freep(&c->chrUPixBuf);
        av_freep(&c->chrVPixBuf);
    }

    if (CONFIG_SWSCALE_ALPHA && c->alpPixBuf) {
        for (i=0; i<c->vLumBufSize; i++)
            av_freep(&c->alpPixBuf[i]);
        av_freep(&c->alpPixBuf);
    }

    av_freep(&c->vLumFilter);
    av_freep(&c->vChrFilter);
    av_freep(&c->hLumFilter);
    av_freep(&c->hChrFilter);
#if HAVE_ALTIVEC
    av_freep(&c->vYCoeffsBank);
    av_freep(&c->vCCoeffsBank);
#endif

    av_freep(&c->vLumFilterPos);
    av_freep(&c->vChrFilterPos);
    av_freep(&c->hLumFilterPos);
    av_freep(&c->hChrFilterPos);

#if HAVE_MMX
#ifdef MAP_ANONYMOUS
    if (c->lumMmx2FilterCode) munmap(c->lumMmx2FilterCode, c->lumMmx2FilterCodeSize);
    if (c->chrMmx2FilterCode) munmap(c->chrMmx2FilterCode, c->chrMmx2FilterCodeSize);
#elif HAVE_VIRTUALALLOC
    if (c->lumMmx2FilterCode) VirtualFree(c->lumMmx2FilterCode, 0, MEM_RELEASE);
    if (c->chrMmx2FilterCode) VirtualFree(c->chrMmx2FilterCode, 0, MEM_RELEASE);
#else
    av_free(c->lumMmx2FilterCode);
    av_free(c->chrMmx2FilterCode);
#endif
    c->lumMmx2FilterCode=NULL;
    c->chrMmx2FilterCode=NULL;
#endif /* HAVE_MMX */

    av_freep(&c->yuvTable);
    av_free(c->formatConvBuffer);

    av_free(c);
}

struct SwsContext *sws_getCachedContext(struct SwsContext *context,
                                        int srcW, int srcH, enum PixelFormat srcFormat,
                                        int dstW, int dstH, enum PixelFormat dstFormat, int flags,
                                        SwsFilter *srcFilter, SwsFilter *dstFilter, const double *param)
{
    static const double default_param[2] = {SWS_PARAM_DEFAULT, SWS_PARAM_DEFAULT};

    if (!param)
        param = default_param;

    if (context &&
        (context->srcW      != srcW      ||
         context->srcH      != srcH      ||
         context->srcFormat != srcFormat ||
         context->dstW      != dstW      ||
         context->dstH      != dstH      ||
         context->dstFormat != dstFormat ||
         context->flags     != flags     ||
         context->param[0]  != param[0]  ||
         context->param[1]  != param[1])) {
        sws_freeContext(context);
        context = NULL;
    }

    if (!context) {
        if (!(context = sws_alloc_context()))
            return NULL;
        context->srcW      = srcW;
        context->srcH      = srcH;
        context->srcRange  = handle_jpeg(&srcFormat);
        context->srcFormat = srcFormat;
        context->dstW      = dstW;
        context->dstH      = dstH;
        context->dstRange  = handle_jpeg(&dstFormat);
        context->dstFormat = dstFormat;
        context->flags     = flags;
        context->param[0]  = param[0];
        context->param[1]  = param[1];
        sws_setColorspaceDetails(context, ff_yuv2rgb_coeffs[SWS_CS_DEFAULT], context->srcRange, ff_yuv2rgb_coeffs[SWS_CS_DEFAULT] /* FIXME*/, context->dstRange, 0, 1<<16, 1<<16);
        if (sws_init_context(context, srcFilter, dstFilter) < 0) {
            sws_freeContext(context);
            return NULL;
        }
    }
    return context;
}


/*
 * Copyright (c) 2002 Brian Foley
 * Copyright (c) 2002 Dieter Shirley
 * Copyright (c) 2003-2004 Romain Dolbeau <romain@dolbeau.org>
 * Copyright (c) 2010-2011 Extrems <metaradil@gmail.com>
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

#include "libavutil/cpu.h"
#include "libavcodec/dsputil.h"
#include "dsputil_altivec.h"
#include "dsputil_paired.h"

/* ***** WARNING ***** WARNING ***** WARNING ***** */
/*
clear_blocks_dcbz32_ppc will not work properly on PowerPC processors with a
cache line size not equal to 32 bytes.
Fortunately all processor used by Apple up to at least the 7450 (aka second
generation G4) use 32 bytes cache line.
This is due to the use of the 'dcbz' instruction. It simply clear to zero a
single cache line, so you need to know the cache line size to use it !
It's absurd, but it's fast...

update 24/06/2003 : Apple released yesterday the G5, with a PPC970. cache line
size: 128 bytes. Oups.
The semantic of dcbz was changed, it always clear 32 bytes. so the function
below will work, but will be slow. So I fixed check_dcbz_effect to use dcbzl,
which is defined to clear a cache line (as dcbz before). So we still can
distinguish, and use dcbz (32 bytes) or dcbzl (one cache line) as required.

see <http://developer.apple.com/technotes/tn/tn2087.html>
and <http://developer.apple.com/technotes/tn/tn2086.html>
*/
static void clear_blocks_dcbz32_ppc(DCTELEM *blocks)
{
    register int misal = ((unsigned long)blocks & 0x00000010);
    register int i = 0;
    if (misal) {
        ((unsigned long*)blocks)[0] = 0L;
        ((unsigned long*)blocks)[1] = 0L;
        ((unsigned long*)blocks)[2] = 0L;
        ((unsigned long*)blocks)[3] = 0L;
        i += 16;
    }
    for ( ; i < sizeof(DCTELEM)*6*64-31 ; i += 32) {
        __asm__ volatile("dcbz %0,%1" : : "b" (blocks), "r" (i) : "memory");
    }
    if (misal) {
        ((unsigned long*)blocks)[188] = 0L;
        ((unsigned long*)blocks)[189] = 0L;
        ((unsigned long*)blocks)[190] = 0L;
        ((unsigned long*)blocks)[191] = 0L;
        i += 16;
    }
}

/* same as above, when dcbzl clear a whole 128B cache line
   i.e. the PPC970 aka G5 */
#if HAVE_DCBZL
static void clear_blocks_dcbz128_ppc(DCTELEM *blocks)
{
    register int misal = ((unsigned long)blocks & 0x0000007f);
    register int i = 0;
    if (misal) {
        // we could probably also optimize this case,
        // but there's not much point as the machines
        // aren't available yet (2003-06-26)
        memset(blocks, 0, sizeof(DCTELEM)*6*64);
    }
    else
        for ( ; i < sizeof(DCTELEM)*6*64 ; i += 128) {
            __asm__ volatile("dcbzl %0,%1" : : "b" (blocks), "r" (i) : "memory");
        }
}
#else
static void clear_blocks_dcbz128_ppc(DCTELEM *blocks)
{
    memset(blocks, 0, sizeof(DCTELEM)*6*64);
}
#endif

#if HAVE_DCBZL
/* check dcbz report how many bytes are set to 0 by dcbz */
/* update 24/06/2003 : replace dcbz by dcbzl to get
   the intended effect (Apple "fixed" dcbz)
   unfortunately this cannot be used unless the assembler
   knows about dcbzl ... */
static long check_dcbzl_effect(void)
{
    register char *fakedata = av_malloc(1024);
    register char *fakedata_middle;
    register long zero = 0;
    register long i = 0;
    long count = 0;

    if (!fakedata) {
        return 0L;
    }

    fakedata_middle = (fakedata + 512);

    memset(fakedata, 0xFF, 1024);

    /* below the constraint "b" seems to mean "Address base register"
       in gcc-3.3 / RS/6000 speaks. seems to avoid using r0, so.... */
    __asm__ volatile("dcbzl %0, %1" : : "b" (fakedata_middle), "r" (zero));

    for (i = 0; i < 1024 ; i ++) {
        if (fakedata[i] == (char)0)
            count++;
    }

    av_free(fakedata);

    return count;
}
#else
static long check_dcbzl_effect(void)
{
  return 0;
}
#endif

static void prefetch_ppc(void *mem, int stride, int h)
{
    register const uint8_t *p = mem;
    do {
        __asm__ volatile ("dcbt 0,%0" : : "r" (p));
        p+= stride;
    } while(--h);
}

#if defined(GEKKO)
static void fill_block16_gekko(uint8_t *block, uint8_t value, int line_size, int h)
{
	uint32_t word = value;
	word |= (word << 8);
	word |= (word << 16);
	
	block -= line_size;
	
	do {
		__asm__ volatile (
			"stwux	%2,%0,%1\n"
			"stw	%2,4(%0)\n"
			"stw	%2,8(%0)\n"
			"stw	%2,12(%0)\n"
			: "+b"(block)
			: "r"(line_size), "r"(word)
		);
	} while (--h);
}

static void fill_block8_gekko(uint8_t *block, uint8_t value, int line_size, int h)
{
	uint32_t word = value;
	word |= (word << 8);
	word |= (word << 16);
	
	block -= line_size;
	
	do {
		__asm__ volatile (
			"stwux	%2,%0,%1\n"
			"stw	%2,4(%0)\n"
			: "+b"(block)
			: "r"(line_size), "r"(word)
		);
	} while (--h);
}
#endif /* GEKKO */

void dsputil_init_ppc(DSPContext* c, AVCodecContext *avctx)
{
    const int high_bit_depth = avctx->bits_per_raw_sample > 8;

    // Common optimizations whether AltiVec is available or not
    c->prefetch = prefetch_ppc;
    if (!high_bit_depth) {
    switch (check_dcbzl_effect()) {
        case 32:
            c->clear_blocks = clear_blocks_dcbz32_ppc;
            break;
        case 128:
            c->clear_blocks = clear_blocks_dcbz128_ppc;
            break;
        default:
            break;
    }
    }

#if defined(GEKKO)
    c->fill_block_tab[0] = fill_block16_gekko;
    c->fill_block_tab[1] = fill_block8_gekko;
#endif

#if HAVE_ALTIVEC
    if(CONFIG_H264_DECODER) dsputil_h264_init_ppc(c, avctx);

    if (av_get_cpu_flags() & AV_CPU_FLAG_ALTIVEC) {
        dsputil_init_altivec(c, avctx);
        float_init_altivec(c, avctx);
        int_init_altivec(c, avctx);
        c->gmc1 = gmc1_altivec;

#if CONFIG_ENCODERS
        if (avctx->bits_per_raw_sample <= 8 &&
            (avctx->dct_algo == FF_DCT_AUTO ||
             avctx->dct_algo == FF_DCT_ALTIVEC)) {
            c->fdct = fdct_altivec;
        }
#endif //CONFIG_ENCODERS

        if (avctx->lowres == 0 && avctx->bits_per_raw_sample <= 8) {
            if ((avctx->idct_algo == FF_IDCT_AUTO) ||
                (avctx->idct_algo == FF_IDCT_ALTIVEC)) {
                c->idct_put = idct_put_altivec;
                c->idct_add = idct_add_altivec;
                c->idct_permutation_type = FF_TRANSPOSE_IDCT_PERM;
            }else if((CONFIG_VP3_DECODER || CONFIG_VP5_DECODER || CONFIG_VP6_DECODER) &&
                     avctx->idct_algo==FF_IDCT_VP3){
                c->idct_put = ff_vp3_idct_put_altivec;
                c->idct_add = ff_vp3_idct_add_altivec;
                c->idct     = ff_vp3_idct_altivec;
                c->idct_permutation_type = FF_TRANSPOSE_IDCT_PERM;
            }
        }

    }
#endif /* HAVE_ALTIVEC */

#if HAVE_PAIRED
    dsputil_init_paired(c, avctx);
    float_init_paired(c, avctx);

    if (CONFIG_H264_DECODER)
        dsputil_h264_init_ppc(c, avctx);
#endif /* HAVE_PAIRED */
}

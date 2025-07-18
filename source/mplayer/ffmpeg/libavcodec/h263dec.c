/*
 * H.263 decoder
 * Copyright (c) 2001 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
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
 * H.263 decoder.
 */

#include "libavutil/cpu.h"
#include "internal.h"
#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "h263.h"
#include "h263_parser.h"
#include "mpeg4video_parser.h"
#include "msmpeg4.h"
#include "vdpau_internal.h"
#include "thread.h"
#include "flv.h"
#include "mpeg4video.h"

//#define DEBUG
//#define PRINT_FRAME_TIME

av_cold int ff_h263_decode_init(AVCodecContext *avctx)
{
    MpegEncContext *s = avctx->priv_data;

    s->avctx = avctx;
    s->out_format = FMT_H263;

    s->width  = avctx->coded_width;
    s->height = avctx->coded_height;
    s->workaround_bugs= avctx->workaround_bugs;

    // set defaults
    MPV_decode_defaults(s);
    s->quant_precision=5;
    s->decode_mb= ff_h263_decode_mb;
    s->low_delay= 1;
    avctx->pix_fmt= avctx->get_format(avctx, avctx->codec->pix_fmts);
    s->unrestricted_mv= 1;

    /* select sub codec */
    switch(avctx->codec->id) {
    case CODEC_ID_H263:
        s->unrestricted_mv= 0;
        avctx->chroma_sample_location = AVCHROMA_LOC_CENTER;
        break;
    case CODEC_ID_MPEG4:
        break;
    case CODEC_ID_MSMPEG4V1:
        s->h263_pred = 1;
        s->msmpeg4_version=1;
        break;
    case CODEC_ID_MSMPEG4V2:
        s->h263_pred = 1;
        s->msmpeg4_version=2;
        break;
    case CODEC_ID_MSMPEG4V3:
        s->h263_pred = 1;
        s->msmpeg4_version=3;
        break;
    case CODEC_ID_WMV1:
        s->h263_pred = 1;
        s->msmpeg4_version=4;
        break;
    case CODEC_ID_WMV2:
        s->h263_pred = 1;
        s->msmpeg4_version=5;
        break;
    case CODEC_ID_VC1:
    case CODEC_ID_WMV3:
        s->h263_pred = 1;
        s->msmpeg4_version=6;
        avctx->chroma_sample_location = AVCHROMA_LOC_LEFT;
        break;
    case CODEC_ID_H263I:
        break;
    case CODEC_ID_FLV1:
        s->h263_flv = 1;
        break;
    default:
        return -1;
    }
    s->codec_id= avctx->codec->id;
    avctx->hwaccel= ff_find_hwaccel(avctx->codec->id, avctx->pix_fmt);

    /* for h263, we allocate the images after having read the header */
    if (avctx->codec->id != CODEC_ID_H263 && avctx->codec->id != CODEC_ID_MPEG4)
        if (MPV_common_init(s) < 0)
            return -1;

        h263_decode_init_vlc(s);

    return 0;
}

av_cold int ff_h263_decode_end(AVCodecContext *avctx)
{
    MpegEncContext *s = avctx->priv_data;

    MPV_common_end(s);
    return 0;
}

/**
 * returns the number of bytes consumed for building the current frame
 */
static int get_consumed_bytes(MpegEncContext *s, int buf_size){
    int pos= (get_bits_count(&s->gb)+7)>>3;

    if(s->divx_packed || s->avctx->hwaccel){
        //we would have to scan through the whole buf to handle the weird reordering ...
        return buf_size;
    }else if(s->flags&CODEC_FLAG_TRUNCATED){
        pos -= s->parse_context.last_index;
        if(pos<0) pos=0; // padding is not really read so this might be -1
        return pos;
    }else{
        if(pos==0) pos=1; //avoid infinite loops (i doubt that is needed but ...)
        if(pos+10>buf_size) pos=buf_size; // oops ;)

        return pos;
    }
}

static int decode_slice(MpegEncContext *s){
    const int part_mask= s->partitioned_frame ? (AC_END|AC_ERROR) : 0x7F;
    const int mb_size= 16>>s->avctx->lowres;
    s->last_resync_gb= s->gb;
    s->first_slice_line= 1;

    s->resync_mb_x= s->mb_x;
    s->resync_mb_y= s->mb_y;

    ff_set_qscale(s, s->qscale);

    if (s->avctx->hwaccel) {
        const uint8_t *start= s->gb.buffer + get_bits_count(&s->gb)/8;
        const uint8_t *end  = ff_h263_find_resync_marker(start + 1, s->gb.buffer_end);
        skip_bits_long(&s->gb, 8*(end - start));
        return s->avctx->hwaccel->decode_slice(s->avctx, start, end - start);
    }

    if(s->partitioned_frame){
        const int qscale= s->qscale;

        if(CONFIG_MPEG4_DECODER && s->codec_id==CODEC_ID_MPEG4){
            if(ff_mpeg4_decode_partitions(s) < 0)
                return -1;
        }

        /* restore variables which were modified */
        s->first_slice_line=1;
        s->mb_x= s->resync_mb_x;
        s->mb_y= s->resync_mb_y;
        ff_set_qscale(s, qscale);
    }

    for(; s->mb_y < s->mb_height; s->mb_y++) {
        /* per-row end of slice checks */
        if(s->msmpeg4_version){
            if(s->resync_mb_y + s->slice_height == s->mb_y){
                ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_END|DC_END|MV_END);

                return 0;
            }
        }

        if(s->msmpeg4_version==1){
            s->last_dc[0]=
            s->last_dc[1]=
            s->last_dc[2]= 128;
        }

        ff_init_block_index(s);
        for(; s->mb_x < s->mb_width; s->mb_x++) {
            int ret;

            ff_update_block_index(s);

            if(s->resync_mb_x == s->mb_x && s->resync_mb_y+1 == s->mb_y){
                s->first_slice_line=0;
            }

            /* DCT & quantize */

            s->mv_dir = MV_DIR_FORWARD;
            s->mv_type = MV_TYPE_16X16;
//            s->mb_skipped = 0;
//printf("%d %d %06X\n", ret, get_bits_count(&s->gb), show_bits(&s->gb, 24));
            ret= s->decode_mb(s, s->block);

            if (s->pict_type!=AV_PICTURE_TYPE_B)
                ff_h263_update_motion_val(s);

            if(ret<0){
                const int xy= s->mb_x + s->mb_y*s->mb_stride;
                if(ret==SLICE_END){
                    MPV_decode_mb(s, s->block);
                    if(s->loop_filter)
                        ff_h263_loop_filter(s);

//printf("%d %d %d %06X\n", s->mb_x, s->mb_y, s->gb.size*8 - get_bits_count(&s->gb), show_bits(&s->gb, 24));
                    ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_END|DC_END|MV_END)&part_mask);

                    s->padding_bug_score--;

                    if(++s->mb_x >= s->mb_width){
                        s->mb_x=0;
                        ff_draw_horiz_band(s, s->mb_y*mb_size, mb_size);
                        MPV_report_decode_progress(s);
                        s->mb_y++;
                    }
                    return 0;
                }else if(ret==SLICE_NOEND){
                    av_log(s->avctx, AV_LOG_ERROR, "Slice mismatch at MB: %d\n", xy);
                    ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x+1, s->mb_y, (AC_END|DC_END|MV_END)&part_mask);
                    return -1;
                }
                av_log(s->avctx, AV_LOG_ERROR, "Error at MB: %d\n", xy);
                ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_ERROR|DC_ERROR|MV_ERROR)&part_mask);

                return -1;
            }

            MPV_decode_mb(s, s->block);
            if(s->loop_filter)
                ff_h263_loop_filter(s);
        }

        ff_draw_horiz_band(s, s->mb_y*mb_size, mb_size);
        MPV_report_decode_progress(s);

        s->mb_x= 0;
    }

    assert(s->mb_x==0 && s->mb_y==s->mb_height);

    if(s->codec_id==CODEC_ID_MPEG4
       && (s->workaround_bugs&FF_BUG_AUTODETECT)
       && get_bits_left(&s->gb) >= 48
       && show_bits(&s->gb, 24)==0x4010
       && !s->data_partitioning)
        s->padding_bug_score+=32;

    /* try to detect the padding bug */
    if(      s->codec_id==CODEC_ID_MPEG4
       &&   (s->workaround_bugs&FF_BUG_AUTODETECT)
       &&    get_bits_left(&s->gb) >=0
       &&    get_bits_left(&s->gb) < 48
//       &&   !s->resync_marker
       &&   !s->data_partitioning){

        const int bits_count= get_bits_count(&s->gb);
        const int bits_left = s->gb.size_in_bits - bits_count;

        if(bits_left==0){
            s->padding_bug_score+=16;
        } else if(bits_left != 1){
            int v= show_bits(&s->gb, 8);
            v|= 0x7F >> (7-(bits_count&7));

            if(v==0x7F && bits_left<=8)
                s->padding_bug_score--;
            else if(v==0x7F && ((get_bits_count(&s->gb)+8)&8) && bits_left<=16)
                s->padding_bug_score+= 4;
            else
                s->padding_bug_score++;
        }
    }

    if(s->workaround_bugs&FF_BUG_AUTODETECT){
        if(s->padding_bug_score > -2 && !s->data_partitioning /*&& (s->divx_version>=0 || !s->resync_marker)*/)
            s->workaround_bugs |=  FF_BUG_NO_PADDING;
        else
            s->workaround_bugs &= ~FF_BUG_NO_PADDING;
    }

    // handle formats which don't have unique end markers
    if(s->msmpeg4_version || (s->workaround_bugs&FF_BUG_NO_PADDING)){ //FIXME perhaps solve this more cleanly
        int left= get_bits_left(&s->gb);
        int max_extra=7;

        /* no markers in M$ crap */
        if(s->msmpeg4_version && s->pict_type==AV_PICTURE_TYPE_I)
            max_extra+= 17;

        /* buggy padding but the frame should still end approximately at the bitstream end */
        if((s->workaround_bugs&FF_BUG_NO_PADDING) && s->error_recognition>=3)
            max_extra+= 48;
        else if((s->workaround_bugs&FF_BUG_NO_PADDING))
            max_extra+= 256*256*256*64;

        if(left>max_extra){
            av_log(s->avctx, AV_LOG_ERROR, "discarding %d junk bits at end, next would be %X\n", left, show_bits(&s->gb, 24));
        }
        else if(left<0){
            av_log(s->avctx, AV_LOG_ERROR, "overreading %d bits\n", -left);
        }else
            ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_END|DC_END|MV_END);

        return 0;
    }

    av_log(s->avctx, AV_LOG_ERROR, "slice end not reached but screenspace end (%d left %06X, score= %d)\n",
            get_bits_left(&s->gb),
            show_bits(&s->gb, 24), s->padding_bug_score);

    ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_END|DC_END|MV_END)&part_mask);

    return -1;
}

int ff_h263_decode_frame(AVCodecContext *avctx,
                             void *data, int *data_size,
                             AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    MpegEncContext *s = avctx->priv_data;
    int ret;
    AVFrame *pict = data;

#ifdef PRINT_FRAME_TIME
uint64_t time= rdtsc();
#endif
    s->flags= avctx->flags;
    s->flags2= avctx->flags2;

    /* no supplementary picture */
    if (buf_size == 0) {
        /* special case for last picture */
        if (s->low_delay==0 && s->next_picture_ptr) {
            *pict= *(AVFrame*)s->next_picture_ptr;
            s->next_picture_ptr= NULL;

            *data_size = sizeof(AVFrame);
        }

        return 0;
    }

    if(s->flags&CODEC_FLAG_TRUNCATED){
        int next;

        if(CONFIG_MPEG4_DECODER && s->codec_id==CODEC_ID_MPEG4){
            next= ff_mpeg4_find_frame_end(&s->parse_context, buf, buf_size);
        }else if(CONFIG_H263_DECODER && s->codec_id==CODEC_ID_H263){
            next= ff_h263_find_frame_end(&s->parse_context, buf, buf_size);
        }else{
            av_log(s->avctx, AV_LOG_ERROR, "this codec does not support truncated bitstreams\n");
            return -1;
        }

        if( ff_combine_frame(&s->parse_context, next, (const uint8_t **)&buf, &buf_size) < 0 )
            return buf_size;
    }


retry:

    if(s->bitstream_buffer_size && (s->divx_packed || buf_size<20)){ //divx 5.01+/xvid frame reorder
        init_get_bits(&s->gb, s->bitstream_buffer, s->bitstream_buffer_size*8);
    }else
        init_get_bits(&s->gb, buf, buf_size*8);
    s->bitstream_buffer_size=0;

    if (!s->context_initialized) {
        if (MPV_common_init(s) < 0) //we need the idct permutaton for reading a custom matrix
            return -1;
    }

    /* We need to set current_picture_ptr before reading the header,
     * otherwise we cannot store anyting in there */
    if (s->current_picture_ptr == NULL || s->current_picture_ptr->f.data[0]) {
        int i= ff_find_unused_picture(s, 0);
        s->current_picture_ptr= &s->picture[i];
    }

    /* let's go :-) */
    if (CONFIG_WMV2_DECODER && s->msmpeg4_version==5) {
        ret= ff_wmv2_decode_picture_header(s);
    } else if (CONFIG_MSMPEG4_DECODER && s->msmpeg4_version) {
        ret = msmpeg4_decode_picture_header(s);
    } else if (CONFIG_MPEG4_DECODER && s->h263_pred) {
        if(s->avctx->extradata_size && s->picture_number==0){
            GetBitContext gb;

            init_get_bits(&gb, s->avctx->extradata, s->avctx->extradata_size*8);
            ret = ff_mpeg4_decode_picture_header(s, &gb);
        }
        ret = ff_mpeg4_decode_picture_header(s, &s->gb);
    } else if (CONFIG_H263I_DECODER && s->codec_id == CODEC_ID_H263I) {
        ret = ff_intel_h263_decode_picture_header(s);
    } else if (CONFIG_FLV_DECODER && s->h263_flv) {
        ret = ff_flv_decode_picture_header(s);
    } else {
        ret = h263_decode_picture_header(s);
    }

    if(ret==FRAME_SKIPPED) return get_consumed_bytes(s, buf_size);

    /* skip if the header was thrashed */
    if (ret < 0){
        av_log(s->avctx, AV_LOG_ERROR, "header damaged\n");
        return -1;
    }

    avctx->has_b_frames= !s->low_delay;

    if(s->xvid_build==-1 && s->divx_version==-1 && s->lavc_build==-1){
        if(s->stream_codec_tag == AV_RL32("XVID") ||
           s->codec_tag == AV_RL32("XVID") || s->codec_tag == AV_RL32("XVIX") ||
           s->codec_tag == AV_RL32("RMP4") ||
           s->codec_tag == AV_RL32("SIPP")
           )
            s->xvid_build= 0;
#if 0
        if(s->codec_tag == AV_RL32("DIVX") && s->vo_type==0 && s->vol_control_parameters==1
           && s->padding_bug_score > 0 && s->low_delay) // XVID with modified fourcc
            s->xvid_build= 0;
#endif
    }

    if(s->xvid_build==-1 && s->divx_version==-1 && s->lavc_build==-1){
        if(s->codec_tag == AV_RL32("DIVX") && s->vo_type==0 && s->vol_control_parameters==0)
            s->divx_version= 400; //divx 4
    }

    if(s->xvid_build>=0 && s->divx_version>=0){
        s->divx_version=
        s->divx_build= -1;
    }

    if(s->workaround_bugs&FF_BUG_AUTODETECT){
        if(s->codec_tag == AV_RL32("XVIX"))
            s->workaround_bugs|= FF_BUG_XVID_ILACE;

        if(s->codec_tag == AV_RL32("UMP4")){
            s->workaround_bugs|= FF_BUG_UMP4;
        }

        if(s->divx_version>=500 && s->divx_build<1814){
            s->workaround_bugs|= FF_BUG_QPEL_CHROMA;
        }

        if(s->divx_version>502 && s->divx_build<1814){
            s->workaround_bugs|= FF_BUG_QPEL_CHROMA2;
        }

        if(s->xvid_build<=3U)
            s->padding_bug_score= 256*256*256*64;

        if(s->xvid_build<=1U)
            s->workaround_bugs|= FF_BUG_QPEL_CHROMA;

        if(s->xvid_build<=12U)
            s->workaround_bugs|= FF_BUG_EDGE;

        if(s->xvid_build<=32U)
            s->workaround_bugs|= FF_BUG_DC_CLIP;

#define SET_QPEL_FUNC(postfix1, postfix2) \
    s->dsp.put_ ## postfix1 = ff_put_ ## postfix2;\
    s->dsp.put_no_rnd_ ## postfix1 = ff_put_no_rnd_ ## postfix2;\
    s->dsp.avg_ ## postfix1 = ff_avg_ ## postfix2;

        if(s->lavc_build<4653U)
            s->workaround_bugs|= FF_BUG_STD_QPEL;

        if(s->lavc_build<4655U)
            s->workaround_bugs|= FF_BUG_DIRECT_BLOCKSIZE;

        if(s->lavc_build<4670U){
            s->workaround_bugs|= FF_BUG_EDGE;
        }

        if(s->lavc_build<=4712U)
            s->workaround_bugs|= FF_BUG_DC_CLIP;

        if(s->divx_version>=0)
            s->workaround_bugs|= FF_BUG_DIRECT_BLOCKSIZE;
//printf("padding_bug_score: %d\n", s->padding_bug_score);
        if(s->divx_version==501 && s->divx_build==20020416)
            s->padding_bug_score= 256*256*256*64;

        if(s->divx_version<500U){
            s->workaround_bugs|= FF_BUG_EDGE;
        }

        if(s->divx_version>=0)
            s->workaround_bugs|= FF_BUG_HPEL_CHROMA;
#if 0
        if(s->divx_version==500)
            s->padding_bug_score= 256*256*256*64;

        /* very ugly XVID padding bug detection FIXME/XXX solve this differently
         * Let us hope this at least works.
         */
        if(   s->resync_marker==0 && s->data_partitioning==0 && s->divx_version==-1
           && s->codec_id==CODEC_ID_MPEG4 && s->vo_type==0)
            s->workaround_bugs|= FF_BUG_NO_PADDING;

        if(s->lavc_build<4609U) //FIXME not sure about the version num but a 4609 file seems ok
            s->workaround_bugs|= FF_BUG_NO_PADDING;
#endif
    }

    if(s->workaround_bugs& FF_BUG_STD_QPEL){
        SET_QPEL_FUNC(qpel_pixels_tab[0][ 5], qpel16_mc11_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[0][ 7], qpel16_mc31_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[0][ 9], qpel16_mc12_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[0][11], qpel16_mc32_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[0][13], qpel16_mc13_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[0][15], qpel16_mc33_old_c)

        SET_QPEL_FUNC(qpel_pixels_tab[1][ 5], qpel8_mc11_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[1][ 7], qpel8_mc31_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[1][ 9], qpel8_mc12_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[1][11], qpel8_mc32_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[1][13], qpel8_mc13_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[1][15], qpel8_mc33_old_c)
    }

    if(avctx->debug & FF_DEBUG_BUGS)
        av_log(s->avctx, AV_LOG_DEBUG, "bugs: %X lavc_build:%d xvid_build:%d divx_version:%d divx_build:%d %s\n",
               s->workaround_bugs, s->lavc_build, s->xvid_build, s->divx_version, s->divx_build,
               s->divx_packed ? "p" : "");

#if HAVE_MMX
    if (s->codec_id == CODEC_ID_MPEG4 && s->xvid_build>=0 && avctx->idct_algo == FF_IDCT_AUTO && (av_get_cpu_flags() & AV_CPU_FLAG_MMX)) {
        avctx->idct_algo= FF_IDCT_XVIDMMX;
        avctx->coded_width= 0; // force reinit
//        dsputil_init(&s->dsp, avctx);
        s->picture_number=0;
    }
#endif

        /* After H263 & mpeg4 header decode we have the height, width,*/
        /* and other parameters. So then we could init the picture   */
        /* FIXME: By the way H263 decoder is evolving it should have */
        /* an H263EncContext                                         */

    if (   s->width  != avctx->coded_width
        || s->height != avctx->coded_height) {
        /* H.263 could change picture size any time */
        ParseContext pc= s->parse_context; //FIXME move these demuxng hack to avformat
        s->parse_context.buffer=0;
        MPV_common_end(s);
        s->parse_context= pc;
    }
    if (!s->context_initialized) {
        avcodec_set_dimensions(avctx, s->width, s->height);

        goto retry;
    }

    if((s->codec_id==CODEC_ID_H263 || s->codec_id==CODEC_ID_H263P || s->codec_id == CODEC_ID_H263I))
        s->gob_index = ff_h263_get_gob_height(s);

    // for skipping the frame
    s->current_picture.f.pict_type = s->pict_type;
    s->current_picture.f.key_frame = s->pict_type == AV_PICTURE_TYPE_I;

    /* skip B-frames if we don't have reference frames */
    if(s->last_picture_ptr==NULL && (s->pict_type==AV_PICTURE_TYPE_B || s->dropable)) return get_consumed_bytes(s, buf_size);
    if(   (avctx->skip_frame >= AVDISCARD_NONREF && s->pict_type==AV_PICTURE_TYPE_B)
       || (avctx->skip_frame >= AVDISCARD_NONKEY && s->pict_type!=AV_PICTURE_TYPE_I)
       ||  avctx->skip_frame >= AVDISCARD_ALL)
        return get_consumed_bytes(s, buf_size);

    if(s->next_p_frame_damaged){
        if(s->pict_type==AV_PICTURE_TYPE_B)
            return get_consumed_bytes(s, buf_size);
        else
            s->next_p_frame_damaged=0;
    }

    if((s->avctx->flags2 & CODEC_FLAG2_FAST) && s->pict_type==AV_PICTURE_TYPE_B){
        s->me.qpel_put= s->dsp.put_2tap_qpel_pixels_tab;
        s->me.qpel_avg= s->dsp.avg_2tap_qpel_pixels_tab;
    }else if((!s->no_rounding) || s->pict_type==AV_PICTURE_TYPE_B){
        s->me.qpel_put= s->dsp.put_qpel_pixels_tab;
        s->me.qpel_avg= s->dsp.avg_qpel_pixels_tab;
    }else{
        s->me.qpel_put= s->dsp.put_no_rnd_qpel_pixels_tab;
        s->me.qpel_avg= s->dsp.avg_qpel_pixels_tab;
    }

    if(MPV_frame_start(s, avctx) < 0)
        return -1;

    if (!s->divx_packed) ff_thread_finish_setup(avctx);

    if (CONFIG_MPEG4_VDPAU_DECODER && (s->avctx->codec->capabilities & CODEC_CAP_HWACCEL_VDPAU)) {
        ff_vdpau_mpeg4_decode_picture(s, s->gb.buffer, s->gb.buffer_end - s->gb.buffer);
        goto frame_end;
    }

    if (avctx->hwaccel) {
        if (avctx->hwaccel->start_frame(avctx, s->gb.buffer, s->gb.buffer_end - s->gb.buffer) < 0)
            return -1;
    }

    ff_er_frame_start(s);

    //the second part of the wmv2 header contains the MB skip bits which are stored in current_picture->mb_type
    //which is not available before MPV_frame_start()
    if (CONFIG_WMV2_DECODER && s->msmpeg4_version==5){
        ret = ff_wmv2_decode_secondary_picture_header(s);
        if(ret<0) return ret;
        if(ret==1) goto intrax8_decoded;
    }

    /* decode each macroblock */
    s->mb_x=0;
    s->mb_y=0;

    ret = decode_slice(s);
    while(s->mb_y<s->mb_height){
        if(s->msmpeg4_version){
            if(s->slice_height==0 || s->mb_x!=0 || (s->mb_y%s->slice_height)!=0 || get_bits_count(&s->gb) > s->gb.size_in_bits)
                break;
        }else{
            int prev_x=s->mb_x, prev_y=s->mb_y;
            if(ff_h263_resync(s)<0)
                break;
            if (prev_y * s->mb_width + prev_x < s->mb_y * s->mb_width + s->mb_x)
                s->error_occurred = 1;
        }

        if(s->msmpeg4_version<4 && s->h263_pred)
            ff_mpeg4_clean_buffers(s);

        if (decode_slice(s) < 0) ret = AVERROR_INVALIDDATA;
    }

    if (s->msmpeg4_version && s->msmpeg4_version<4 && s->pict_type==AV_PICTURE_TYPE_I)
        if(!CONFIG_MSMPEG4_DECODER || msmpeg4_decode_ext_header(s, buf_size) < 0){
            s->error_status_table[s->mb_num-1]= AC_ERROR|DC_ERROR|MV_ERROR;
        }

    assert(s->bitstream_buffer_size==0);
frame_end:
    /* divx 5.01+ bistream reorder stuff */
    if(s->codec_id==CODEC_ID_MPEG4 && s->divx_packed){
        int current_pos= get_bits_count(&s->gb)>>3;
        int startcode_found=0;

        if(buf_size - current_pos > 5){
            int i;
            for(i=current_pos; i<buf_size-3; i++){
                if(buf[i]==0 && buf[i+1]==0 && buf[i+2]==1 && buf[i+3]==0xB6){
                    startcode_found=1;
                    break;
                }
            }
        }
        if(s->gb.buffer == s->bitstream_buffer && buf_size>7 && s->xvid_build>=0){ //xvid style
            startcode_found=1;
            current_pos=0;
        }

        if(startcode_found){
            av_fast_malloc(
                &s->bitstream_buffer,
                &s->allocated_bitstream_buffer_size,
                buf_size - current_pos + FF_INPUT_BUFFER_PADDING_SIZE);
            if (!s->bitstream_buffer)
                return AVERROR(ENOMEM);
            memcpy(s->bitstream_buffer, buf + current_pos, buf_size - current_pos);
            s->bitstream_buffer_size= buf_size - current_pos;
        }
    }

intrax8_decoded:
    ff_er_frame_end(s);

    if (avctx->hwaccel) {
        if (avctx->hwaccel->end_frame(avctx) < 0)
            return -1;
    }

    MPV_frame_end(s);

    assert(s->current_picture.f.pict_type == s->current_picture_ptr->f.pict_type);
    assert(s->current_picture.f.pict_type == s->pict_type);
    if (s->pict_type == AV_PICTURE_TYPE_B || s->low_delay) {
        *pict= *(AVFrame*)s->current_picture_ptr;
    } else if (s->last_picture_ptr != NULL) {
        *pict= *(AVFrame*)s->last_picture_ptr;
    }

    if(s->last_picture_ptr || s->low_delay){
        *data_size = sizeof(AVFrame);
        ff_print_debug_info(s, pict);
    }

#ifdef PRINT_FRAME_TIME
av_log(avctx, AV_LOG_DEBUG, "%"PRId64"\n", rdtsc()-time);
#endif

    return (ret && avctx->error_recognition >= FF_ER_EXPLODE)?ret:get_consumed_bytes(s, buf_size);
}

AVCodec ff_h263_decoder = {
    .name           = "h263",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = CODEC_ID_H263,
    .priv_data_size = sizeof(MpegEncContext),
    .init           = ff_h263_decode_init,
    .close          = ff_h263_decode_end,
    .decode         = ff_h263_decode_frame,
    .capabilities   = CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_TRUNCATED | CODEC_CAP_DELAY,
    .flush= ff_mpeg_flush,
    .max_lowres= 3,
    .long_name= NULL_IF_CONFIG_SMALL("H.263 / H.263-1996, H.263+ / H.263-1998 / H.263 version 2"),
    .pix_fmts= ff_hwaccel_pixfmt_list_420,
};

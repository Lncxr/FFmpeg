/*
 * Sony ADS decoder
 * Copyright (c) 2025 Laniel Riddick
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

 /**
 * @file
 * Sony ADS decoder, PCM format found in PlayStation 2 and PSP video streams
 */


#include "codec_internal.h"
#include "decode.h"

static av_cold int pcm_ads_init(AVCodecContext *avctx) {
    avctx->sample_fmt = AV_SAMPLE_FMT_S16;
    return 0;
}

static av_cold int pcm_ads_close(AVCodecContext *avctx) {
    return 0;
}

// https://archive.org/details/ps2str_v1.08_2001

static int pcm_ads_decode_frame(AVCodecContext *avctx, AVFrame *frame,
    int *got_frame_ptr, AVPacket *avpkt) {
    /* TODO */
    return 0;   
}

const FFCodec ff_pcm_ads_decoder = {
    .p.name           = "ads",
    CODEC_LONG_NAME("Sony ADS (PS2)"),
    .p.type           = AVMEDIA_TYPE_AUDIO,
    .p.id             = AV_CODEC_ID_ADS,
    FF_CODEC_DECODE_CB(pcm_ads_decode_frame),
    .p.capabilities   = AV_CODEC_CAP_DR1,
    CODEC_SAMPLEFMTS(AV_SAMPLE_FMT_S16),
};


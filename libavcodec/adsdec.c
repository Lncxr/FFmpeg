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
 * Sony ADS decoder, PCM format found in PlayStation 2, PSP, and Xbox video streams
 */


#include "codec_internal.h"
#include "decode.h"

typedef struct ADSContext {
    int sample_rate;
    int channels;
    int interleave;
    uint32_t codec;
    int coding_type;

    uint64_t start_offset;
    uint64_t stream_size;
    uint64_t body_size;
    uint64_t file_size;

    uint8_t loop_flag;
    uint8_t is_loop_samples;
    uint32_t loop_start_sample;
    uint32_t loop_end_sample;
    uint32_t loop_start_offset;
    uint32_t loop_end_offset;

    uint8_t ignore_silent_frame_cavia;
    uint8_t ignore_silent_frame_capcom;

    uint64_t current_offset; 
    uint64_t samples_decoded; 

    uint8_t is_container;
    uint64_t container_offset;
    uint64_t container_size;
} ADSContext;


static av_cold int ads_decode_init(AVCodecContext *avctx)
{
    return 0;
}

static av_cold int ads_decode_close(AVCodecContext *avctx) {
    return 0;
}

// https://archive.org/details/ps2str_v1.08_2001

static int ads_decode_frame(AVCodecContext *avctx, AVFrame *frame,
    int *got_frame_ptr, AVPacket *avpkt) {
    /* TODO */
    return 0;   
}

const FFCodec ff_ads_decoder = {
    .p.name           = "ads",
    CODEC_LONG_NAME("Sony ADS (PS2)"),
    .p.type           = AVMEDIA_TYPE_AUDIO,
    .p.id             = AV_CODEC_ID_ADS,
    .priv_data_size = sizeof(ADSContext),
    .init             = ads_decode_init,
    .close            = ads_decode_close,
    FF_CODEC_DECODE_CB(ads_decode_frame),
    .p.capabilities = AV_CODEC_CAP_CHANNEL_CONF |
                      AV_CODEC_CAP_DR1,
    CODEC_SAMPLEFMTS(AV_SAMPLE_FMT_S16),
};


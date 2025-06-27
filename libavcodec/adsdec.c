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
 * Sony ADS decoder, PCM format found in PlayStation 2, PSP, and Xbox video
 * streams
 */

#include "codec_internal.h"
#include "decode.h"
#include "libavutil/intreadwrite.h"

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

static av_cold int ads_decode_init(AVCodecContext *avctx) { return 0; }

static av_cold int ads_decode_close(AVCodecContext *avctx) { return 0; }

// https://archive.org/details/ps2str_v1.08_2001

static int ads_decode_frame(AVCodecContext *avctx, AVFrame *frame,
                            int *got_frame_ptr, AVPacket *avpkt) {
  ADSContext *s = avctx->priv_data;
  const uint8_t *src = avpkt->data;
  int size = avpkt->size;
  int consumed = 0;

  *got_frame_ptr = 0;

  /* ads_decode_header */
  if (!s->sample_rate && size >= 0x28) {
    int found = 0;
    for (int i = 0; i <= FFMIN(size - 0x28, 0x40); i++) {
      if (!memcmp(src + i, "SShd", 4) && !memcmp(src + i + 0x20, "SSbd", 4)) {
        const uint8_t *hdr = src + i;
        s->codec = AV_RL32(hdr + 0x08);
        s->sample_rate = AV_RL32(hdr + 0x0C);
        s->channels = AV_RL32(hdr + 0x10);
        s->interleave = AV_RL32(hdr + 0x14);
        src += i + 0x28;
        size -= i + 0x28;
        consumed += i + 0x28;
        found = 1;
        break;
      }
    }
    if (!found || s->sample_rate <= 0 || s->channels <= 0)
      return AVERROR_INVALIDDATA;

    avctx->sample_rate = s->sample_rate;
    av_channel_layout_default(&avctx->ch_layout, s->channels);
    avctx->sample_fmt = AV_SAMPLE_FMT_S16;
  }

  if (!s->sample_rate || !s->channels)
    return AVERROR_INVALIDDATA;

  int block_size = s->interleave > 0 ? s->interleave : 2 * s->channels;
  int block_samples = block_size / 2;
  int blocks = size / (block_size * s->channels);
  int samples = blocks * block_samples;
  int bytes_used = blocks * block_size * s->channels;

  if (!samples)
    return consumed + size;

  frame->nb_samples = samples;
  if (ff_get_buffer(avctx, frame, 0) < 0)
    return AVERROR(ENOMEM);

  if (s->channels == 1 || s->interleave <= 2) {
    memcpy(frame->data[0], src, samples * 2 * s->channels);
  } else {
    int16_t *dst = (int16_t *)frame->data[0];
    for (int b = 0; b < blocks; b++) {
      for (int i = 0; i < block_samples; i++) {
        for (int ch = 0; ch < s->channels; ch++) {
          const int16_t *block =
              (const int16_t *)(src + b * block_size * s->channels +
                                ch * block_size);
          dst[(b * block_samples + i) * s->channels + ch] = block[i];
        }
      }
    }
  }

  *got_frame_ptr = 1;
  return consumed + bytes_used;
}

const FFCodec ff_ads_decoder = {
    .p.name = "ads",
    CODEC_LONG_NAME("Sony ADS (PS2)"),
    .p.type = AVMEDIA_TYPE_AUDIO,
    .p.id = AV_CODEC_ID_ADS,
    .priv_data_size = sizeof(ADSContext),
    .init = ads_decode_init,
    .close = ads_decode_close,
    FF_CODEC_DECODE_CB(ads_decode_frame),
    .p.capabilities = AV_CODEC_CAP_CHANNEL_CONF | AV_CODEC_CAP_DR1,
    CODEC_SAMPLEFMTS(AV_SAMPLE_FMT_S16),
};
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
 * Sony ADS decoder, (AD)PCM format found in PlayStation 2, PSP, and Xbox video
 * streams
 */

#include "codec_internal.h"
#include "decode.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/mem.h"

#include <iostream>
#include <iomanip>

// https://archive.org/details/ps2str_v1.08_2001

typedef struct ADSContext {
  uint8_t codec;
  uint32_t frequency;
  uint8_t channels;
  uint32_t interleave;
  uint32_t size;
} ADSContext;

void dump_block(const uint8_t* block, int size) {
    for (int i = 0; i < size; i++) {
        if (i % 16 == 0)
            std::cout << std::setw(4) << std::setfill('0') << std::hex << i << ": ";
        std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)block[i] << " ";
        if ((i + 1) % 16 == 0 || i == size - 1)
            std::cout << std::endl;
    }
    std::cout << std::dec;
}

static int ads_decode_frame(AVCodecContext *avctx, AVFrame *frame,
                            int *got_frame_ptr, AVPacket *avpkt) {
  ADSContext *s = avctx->priv_data;
  const uint8_t *src = avpkt->data;
  int size = avpkt->size;
  int consumed = 0;

  *got_frame_ptr = 0;

  if (!s->codec && size >= 0x28) {
    for (int i = 0; i <= FFMIN(size - 0x28, 0x40); i++) {
      if (!memcmp(src + i, "SShd", 4) && !memcmp(src + i + 0x20, "SSbd", 4)) {
        const uint8_t *hdr = src + i;

        s->codec = AV_RL32(hdr + 0x08);
        s->frequency = AV_RL32(hdr + 0x0C);
        s->channels = AV_RL32(hdr + 0x10);
        s->interleave = AV_RL32(hdr + 0x14);
        s->size = AV_RL32(hdr + 0x24);

        src += i + 0x28;
        size -= i + 0x28;
        consumed += i + 0x28;
        break;
      }
    }

    if (s->frequency <= 0 || s->channels <= 0)
      return AVERROR_INVALIDDATA;

    avctx->sample_rate = s->frequency;
    avctx->sample_fmt = AV_SAMPLE_FMT_S16P;
    av_channel_layout_default(&avctx->ch_layout, s->channels);
  }

  if (!s->codec)
    return AVERROR_INVALIDDATA;

  int bytes_per_block = s->interleave * s->channels;
  int samples_per_block_ch = s->interleave / 2;
  int blocks_in_packet = size / bytes_per_block;

  frame->nb_samples = blocks_in_packet * samples_per_block_ch;

  if (frame->nb_samples <= 0)
    return AVERROR_INVALIDDATA;

  if (ff_get_buffer(avctx, frame, 0) < 0)
    return AVERROR(ENOMEM);

  const uint8_t *src_ptr = src;

  dump_block(src_ptr, bytes_per_block);

  if (avctx->sample_fmt == AV_SAMPLE_FMT_S16P) {
    for (int b = 0; b < blocks_in_packet; b++) {
      for (int ch = 0; ch < s->channels; ch++) {
        int16_t *dst_ch = (int16_t *)frame->data[ch];
        for (int i = 0; i < samples_per_block_ch; i++) {
          const uint8_t *sample_ptr = src_ptr + ch * s->interleave + i * 2;
          dst_ch[i + b * samples_per_block_ch] = AV_RL16(sample_ptr);
        }
      }
      src_ptr += bytes_per_block;
    }
  } else if (avctx->sample_fmt == AV_SAMPLE_FMT_S16) {
    int16_t *dst = (int16_t *)frame->data[0];
    for (int b = 0; b < blocks_in_packet; b++) {
      for (int i = 0; i < samples_per_block_ch; i++) {
        for (int ch = 0; ch < s->channels; ch++) {
          const uint8_t *sample_ptr = src_ptr + ch * s->interleave + i * 2;
          *dst++ = AV_RL16(sample_ptr);
        }
      }
      src_ptr += bytes_per_block;
    }

  } else {
    return AVERROR(EINVAL);
  }

  *got_frame_ptr = 1;
  return size;
}

const FFCodec ff_ads_decoder = {
    .p.name = "ads",
    CODEC_LONG_NAME("Sony ADS (PS2)"),
    .p.type = AVMEDIA_TYPE_AUDIO,
    .p.id = AV_CODEC_ID_ADS,
    .priv_data_size = sizeof(ADSContext),
    FF_CODEC_DECODE_CB(ads_decode_frame),
    .p.capabilities = AV_CODEC_CAP_CHANNEL_CONF | AV_CODEC_CAP_DR1,
    CODEC_SAMPLEFMTS(AV_SAMPLE_FMT_S16P),
};
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

#include <stdio.h>

// https://archive.org/details/ps2str_v1.08_2001

typedef struct ADSContext {
  uint8_t codec;
  uint32_t frequency;
  uint8_t channels;
  uint32_t interleave;
  uint32_t size;

  uint8_t *leftover;
  int leftover_size;
} ADSContext;

static void dump_block(const uint8_t *block, int size, int block_size) {
  for (int i = 0; i < size; i++) {
    printf("%02x ", block[i]);

    if ((i + 1) % 16 == 0 || i == size - 1)
      printf("\n");

    if ((i + 1) % (block_size / 2) == 0)
      printf("\n\n");
  }
  printf("\n--- END ---\n");
}

static int ads_parse_header(AVCodecContext *avctx, ADSContext *s,
                            const uint8_t *src, int size) {
  if (!memcmp(src + 3, "SShd", 4) && !memcmp(src + 0x23, "SSbd", 4)) {
    const uint8_t *hdr = src + 3;

    s->codec = AV_RL32(hdr + 0x08);
    s->frequency = AV_RL32(hdr + 0x0C);
    s->channels = AV_RL32(hdr + 0x10);
    s->interleave = AV_RL32(hdr + 0x14);
    s->size = AV_RL32(hdr + 0x24);

    if (s->frequency <= 0 || s->channels <= 0)
      return AVERROR_INVALIDDATA;

    avctx->sample_rate = s->frequency;
    avctx->sample_fmt = AV_SAMPLE_FMT_S16P;

    av_channel_layout_default(&avctx->ch_layout, s->channels);
    avctx->block_align = s->channels * 16 / 8;

    return 0x2B;
  }
  return AVERROR_INVALIDDATA;
}

static int ads_decode_init(AVCodecContext *avctx) {
  ADSContext *s = avctx->priv_data;
  memset(s, 0, sizeof(*s));
  s->leftover = NULL;
  s->leftover_size = 0;
  return 0;
}

static int ads_decode_close(AVCodecContext *avctx) {
  ADSContext *s = avctx->priv_data;
  if (s->leftover) {
    av_free(s->leftover);
    s->leftover = NULL;
  }
  return 0;
}

static int ads_decode_frame(AVCodecContext *avctx, AVFrame *frame,
                            int *got_frame_ptr, AVPacket *avpkt) {
  ADSContext *s = avctx->priv_data;
  const uint8_t *src = avpkt->data;
  int size = avpkt->size;
  int pkt_size = avpkt->size;
  *got_frame_ptr = 0;

  if (!s->codec && size >= 0x28) {
    int skip = ads_parse_header(avctx, s, src, size);
    if (skip < 0)
      return skip;
    src += skip;
    size -= skip;
  }

  if (!s->codec)
    return AVERROR_INVALIDDATA;

  int bytes_per_block = s->interleave * s->channels;
  int samples_per_block_ch = s->interleave / 2;

  uint8_t *buffer = NULL;
  int buffer_size = 0;
  if (s->leftover_size > 0) {
    buffer_size = s->leftover_size + size;
    buffer = av_malloc(buffer_size);
    if (!buffer)
      return AVERROR(ENOMEM);
    memcpy(buffer, s->leftover, s->leftover_size);
    memcpy(buffer + s->leftover_size, src, size);
    src = buffer;
    size = buffer_size;
    av_free(s->leftover);
    s->leftover = NULL;
    s->leftover_size = 0;
  }

  int blocks_in_packet = size / bytes_per_block;
  int bytes_to_process = blocks_in_packet * bytes_per_block;

  frame->nb_samples = blocks_in_packet * samples_per_block_ch;

  if (frame->nb_samples <= 0) {
    if (buffer)
      av_free(buffer);
    return AVERROR_INVALIDDATA;
  }

  if (ff_get_buffer(avctx, frame, 0) < 0) {
    if (buffer)
      av_free(buffer);
    return AVERROR(ENOMEM);
  }

  const uint8_t *src_ptr = src;

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

  int leftover_bytes = size - bytes_to_process;
  if (leftover_bytes > 0) {
    if (s->leftover)
      av_free(s->leftover);
    s->leftover = av_malloc(leftover_bytes);
    if (!s->leftover) {
      if (buffer)
        av_free(buffer);
      return AVERROR(ENOMEM);
    }
    memcpy(s->leftover, src + bytes_to_process, leftover_bytes);
    s->leftover_size = leftover_bytes;
  } else {
    if (s->leftover)
      av_free(s->leftover);
    s->leftover = NULL;
    s->leftover_size = 0;
  }

  if (buffer)
    av_free(buffer);

  *got_frame_ptr = 1;
  return pkt_size;
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
    .caps_internal = FF_CODEC_CAP_INIT_CLEANUP |
                     FF_CODEC_CAP_SKIP_FRAME_FILL_PARAM |
                     FF_CODEC_CAP_SETS_FRAME_PROPS,
    CODEC_SAMPLEFMTS(AV_SAMPLE_FMT_S16P)};

// avframe->pts =
// FF_CODEC_CAP_SETS_PKT_DTS
// avpriv_set_pts_info(st, 64, 1, st->codecpar->sample_rate);
// dump_block(src_ptr, bytes_to_process, bytes_per_block);

/*
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
*/
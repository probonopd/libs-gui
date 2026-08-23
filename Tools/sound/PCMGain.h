/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause OR GPL-3.0-or-later
 */

#ifndef PCMGain_h
#define PCMGain_h

/*
 * Software-gain helpers for NSSound sink plug-ins.
 *
 * NSSound volume must change the amplitude of the samples themselves,
 * never a mixer or device-level volume, so fades stay local to the app
 * playing the sound.
 */

#include <stdint.h>
#include <string.h>

/* Read one signed little- or big-endian PCM sample of n bytes. */
static inline int32_t PCMGainReadSample(const uint8_t *p, int n, BOOL be)
{
  uint32_t u = 0;
  int i;
  for (i = 0; i < n; i++)
    {
      u = (u << 8) | (be ? p[i] : p[n - 1 - i]);
    }
  /* Sign-extend from the top bit */
  {
    uint32_t sign = 1u << (n * 8 - 1);
    return (int32_t)((u ^ sign) - sign);
  }
}

/* Write one signed PCM sample of n bytes in the given byte order. */
static inline void PCMGainWriteSample(uint8_t *p, int n, BOOL be, int32_t v)
{
  uint32_t u = (uint32_t)v;
  int64_t lo = -(int64_t)1 << (n * 8 - 1);
  int64_t hi = (int64_t)1 << (n * 8 - 1);
  int i;

  if ((int64_t)v < lo)
    {
      v = (int32_t)lo;
    }
  else if ((int64_t)v > hi - 1)
    {
      v = (int32_t)(hi - 1);
    }
  u = (uint32_t)v;
  for (i = 0; i < n; i++)
    {
      int shift = 8 * (be ? (n - 1 - i) : i);
      p[i] = (uint8_t)((u >> shift) & 0xff);
    }
}

/*
 * Scale signed PCM data (bits = 8/16/24/32, byteFormat big-endian flag)
 * by vol into out. out may equal bytes for in-place scaling; otherwise
 * it must have room for length bytes.
 */
static inline void PCMGainApply(void *bytes, NSUInteger length, int bits,
                                BOOL be, float vol, void *out)
{
  const int n = bits / 8;
  const NSUInteger count = length / n;
  const uint8_t *src = bytes;
  uint8_t *dst = out;
  NSUInteger i;

  if (vol >= 1.0f || n < 1)
    {
      if (out != bytes && vol >= 1.0f)
        {
          memcpy(out, bytes, length);
        }
      return;
    }

  for (i = 0; i < count; i++)
    {
      int32_t s = PCMGainReadSample(src + i * n, n, be);
      float scaled = (float)s * vol;

      if (scaled > 2147483647.0f)
        {
          scaled = 2147483647.0f;
        }
      else if (scaled < -2147483648.0f)
        {
          scaled = -2147483648.0f;
        }
      PCMGainWriteSample(dst + i * n, n, be, (int32_t)scaled);
    }

  /* Trailing odd byte (should not happen for valid PCM frames) */
  if (length % n)
    {
      memcpy(dst + count * n, src + count * n, length % n);
    }
}

#endif /* PCMGain_h */

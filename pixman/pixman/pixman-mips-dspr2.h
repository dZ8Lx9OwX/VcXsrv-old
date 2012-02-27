/*
 * Copyright (c) 2012
 *      MIPS Technologies, Inc., California.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the MIPS Technologies, Inc., nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE MIPS TECHNOLOGIES, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE MIPS TECHNOLOGIES, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author:  Nemanja Lukic (nlukic@mips.com)
 */

#ifndef PIXMAN_MIPS_DSPR2_H
#define PIXMAN_MIPS_DSPR2_H

#include "pixman-private.h"
#include "pixman-inlines.h"

#define SKIP_ZERO_SRC  1
#define SKIP_ZERO_MASK 2
#define DO_FAST_MEMCPY 3

void
pixman_mips_fast_memcpy (void *dst, void *src, uint32_t n_bytes);

/****************************************************************/

#define PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST(flags, name,          \
                                           src_type, src_cnt,    \
                                           dst_type, dst_cnt)    \
void                                                             \
pixman_composite_##name##_asm_mips (dst_type *dst,               \
                                    src_type *src,               \
                                    int32_t   w);                \
                                                                 \
static void                                                      \
mips_composite_##name (pixman_implementation_t *imp,             \
                       pixman_composite_info_t *info)            \
{                                                                \
    PIXMAN_COMPOSITE_ARGS (info);                                \
    dst_type *dst_line, *dst;                                    \
    src_type *src_line, *src;                                    \
    int32_t dst_stride, src_stride;                              \
    int bpp = PIXMAN_FORMAT_BPP (dest_image->bits.format) / 8;   \
                                                                 \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, src_type,    \
                           src_stride, src_line, src_cnt);       \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type, \
                           dst_stride, dst_line, dst_cnt);       \
                                                                 \
    while (height--)                                             \
    {                                                            \
      dst = dst_line;                                            \
      dst_line += dst_stride;                                    \
      src = src_line;                                            \
      src_line += src_stride;                                    \
                                                                 \
      if (flags == DO_FAST_MEMCPY)                               \
        pixman_mips_fast_memcpy (dst, src, width * bpp);         \
      else                                                       \
        pixman_composite_##name##_asm_mips (dst, src, width);    \
    }                                                            \
}

#endif //PIXMAN_MIPS_DSPR2_H

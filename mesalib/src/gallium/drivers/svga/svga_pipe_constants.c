/*
 * Copyright (c) 2008-2024 Broadcom. All Rights Reserved.
 * The term “Broadcom” refers to Broadcom Inc.
 * and/or its subsidiaries.
 * SPDX-License-Identifier: MIT
 */

#include "util/u_inlines.h"
#include "pipe/p_defines.h"
#include "util/u_math.h"

#include "svga_context.h"
#include "svga_resource_buffer.h"


struct svga_constbuf
{
   unsigned type;
   float (*data)[4];
   unsigned count;
};



static void
svga_set_constant_buffer(struct pipe_context *pipe,
                         enum pipe_shader_type shader, uint index,
                         bool take_ownership,
                         const struct pipe_constant_buffer *cb)
{
   struct svga_screen *svgascreen = svga_screen(pipe->screen);
   struct svga_context *svga = svga_context(pipe);
   struct pipe_resource *buf = cb ? cb->buffer : NULL;
   unsigned buffer_size = 0;

   if (cb) {
      buffer_size = cb->buffer_size;

      if (cb->user_buffer) {
         buf = svga_user_buffer_create(pipe->screen,
                                       (void *) cb->user_buffer,
                                       cb->buffer_size,
                                       PIPE_BIND_CONSTANT_BUFFER);
      }
   }

   assert(shader < PIPE_SHADER_TYPES);
   assert(index < ARRAY_SIZE(svga->curr.constbufs[shader]));
   assert(index < svgascreen->max_const_buffers);
   (void) svgascreen;

   if (take_ownership) {
      pipe_resource_reference(&svga->curr.constbufs[shader][index].buffer, NULL);
      svga->curr.constbufs[shader][index].buffer = buf;
   } else {
      pipe_resource_reference(&svga->curr.constbufs[shader][index].buffer, buf);
   }

   /* Make sure the constant buffer size to be updated is within the
    * limit supported by the device.
    */
   svga->curr.constbufs[shader][index].buffer_size =
      MIN2(buffer_size, SVGA_MAX_CONST_BUF_SIZE);

   svga->curr.constbufs[shader][index].buffer_offset = cb ? cb->buffer_offset : 0;
   svga->curr.constbufs[shader][index].user_buffer = NULL; /* not used */

   if (index == 0) {
      if (shader == PIPE_SHADER_FRAGMENT)
         svga->dirty |= SVGA_NEW_FS_CONSTS;
      else if (shader == PIPE_SHADER_VERTEX)
         svga->dirty |= SVGA_NEW_VS_CONSTS;
      else if (shader == PIPE_SHADER_GEOMETRY)
         svga->dirty |= SVGA_NEW_GS_CONSTS;
      else if (shader == PIPE_SHADER_TESS_CTRL)
         svga->dirty |= SVGA_NEW_TCS_CONSTS;
      else if (shader == PIPE_SHADER_TESS_EVAL)
         svga->dirty |= SVGA_NEW_TES_CONSTS;
      else if (shader == PIPE_SHADER_COMPUTE)
         svga->dirty |= SVGA_NEW_CS_CONSTS;
   } else {
      if (shader == PIPE_SHADER_FRAGMENT)
         svga->dirty |= SVGA_NEW_FS_CONST_BUFFER;
      else if (shader == PIPE_SHADER_VERTEX)
         svga->dirty |= SVGA_NEW_VS_CONST_BUFFER;
      else if (shader == PIPE_SHADER_GEOMETRY)
         svga->dirty |= SVGA_NEW_GS_CONST_BUFFER;
      else if (shader == PIPE_SHADER_TESS_CTRL)
         svga->dirty |= SVGA_NEW_TCS_CONST_BUFFER;
      else if (shader == PIPE_SHADER_TESS_EVAL)
         svga->dirty |= SVGA_NEW_TES_CONST_BUFFER;
      else if (shader == PIPE_SHADER_COMPUTE)
         svga->dirty |= SVGA_NEW_CS_CONST_BUFFER;

      /* update bitmask of dirty const buffers */
      svga->state.dirty_constbufs[shader] |= (1 << index);

      /* purge any stale rawbuf srv */
      svga_destroy_rawbuf_srv(svga);
   }

   if (cb && cb->user_buffer) {
      pipe_resource_reference(&buf, NULL);
   }
}


void
svga_init_constbuffer_functions(struct svga_context *svga)
{
   svga->pipe.set_constant_buffer = svga_set_constant_buffer;
}


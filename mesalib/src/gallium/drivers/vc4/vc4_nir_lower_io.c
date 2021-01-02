/*
 * Copyright © 2015 Broadcom
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "vc4_qir.h"
#include "compiler/nir/nir_builder.h"
#include "util/format/u_format.h"
#include "util/u_helpers.h"

/**
 * Walks the NIR generated by TGSI-to-NIR or GLSL-to-NIR to lower its io
 * intrinsics into something amenable to the VC4 architecture.
 *
 * Currently, it splits VS inputs and uniforms into scalars, drops any
 * non-position outputs in coordinate shaders, and fixes up the addressing on
 * indirect uniform loads.  FS input and VS output scalarization is handled by
 * nir_lower_io_to_scalar().
 */

static void
replace_intrinsic_with_vec(nir_builder *b, nir_intrinsic_instr *intr,
                           nir_ssa_def **comps)
{

        /* Batch things back together into a vector.  This will get split by
         * the later ALU scalarization pass.
         */
        nir_ssa_def *vec = nir_vec(b, comps, intr->num_components);

        /* Replace the old intrinsic with a reference to our reconstructed
         * vector.
         */
        nir_ssa_def_rewrite_uses(&intr->dest.ssa, nir_src_for_ssa(vec));
        nir_instr_remove(&intr->instr);
}

static nir_ssa_def *
vc4_nir_unpack_8i(nir_builder *b, nir_ssa_def *src, unsigned chan)
{
        return nir_ubitfield_extract(b,
                                     src,
                                     nir_imm_int(b, 8 * chan),
                                     nir_imm_int(b, 8));
}

/** Returns the 16 bit field as a sign-extended 32-bit value. */
static nir_ssa_def *
vc4_nir_unpack_16i(nir_builder *b, nir_ssa_def *src, unsigned chan)
{
        return nir_ibitfield_extract(b,
                                     src,
                                     nir_imm_int(b, 16 * chan),
                                     nir_imm_int(b, 16));
}

/** Returns the 16 bit field as an unsigned 32 bit value. */
static nir_ssa_def *
vc4_nir_unpack_16u(nir_builder *b, nir_ssa_def *src, unsigned chan)
{
        if (chan == 0) {
                return nir_iand(b, src, nir_imm_int(b, 0xffff));
        } else {
                return nir_ushr(b, src, nir_imm_int(b, 16));
        }
}

static nir_ssa_def *
vc4_nir_unpack_8f(nir_builder *b, nir_ssa_def *src, unsigned chan)
{
        return nir_channel(b, nir_unpack_unorm_4x8(b, src), chan);
}

static nir_ssa_def *
vc4_nir_get_vattr_channel_vpm(struct vc4_compile *c,
                              nir_builder *b,
                              nir_ssa_def **vpm_reads,
                              uint8_t swiz,
                              const struct util_format_description *desc)
{
        const struct util_format_channel_description *chan =
                &desc->channel[swiz];
        nir_ssa_def *temp;

        if (swiz > PIPE_SWIZZLE_W) {
                return vc4_nir_get_swizzled_channel(b, vpm_reads, swiz);
        } else if (chan->size == 32 && chan->type == UTIL_FORMAT_TYPE_FLOAT) {
                return vc4_nir_get_swizzled_channel(b, vpm_reads, swiz);
        } else if (chan->size == 32 && chan->type == UTIL_FORMAT_TYPE_SIGNED) {
                if (chan->normalized) {
                        return nir_fmul(b,
                                        nir_i2f32(b, vpm_reads[swiz]),
                                        nir_imm_float(b,
                                                      1.0 / 0x7fffffff));
                } else {
                        return nir_i2f32(b, vpm_reads[swiz]);
                }
        } else if (chan->size == 8 &&
                   (chan->type == UTIL_FORMAT_TYPE_UNSIGNED ||
                    chan->type == UTIL_FORMAT_TYPE_SIGNED)) {
                nir_ssa_def *vpm = vpm_reads[0];
                if (chan->type == UTIL_FORMAT_TYPE_SIGNED) {
                        temp = nir_ixor(b, vpm, nir_imm_int(b, 0x80808080));
                        if (chan->normalized) {
                                return nir_fsub(b, nir_fmul(b,
                                                            vc4_nir_unpack_8f(b, temp, swiz),
                                                            nir_imm_float(b, 2.0)),
                                                nir_imm_float(b, 1.0));
                        } else {
                                return nir_fadd(b,
                                                nir_i2f32(b,
                                                          vc4_nir_unpack_8i(b, temp,
                                                                            swiz)),
                                                nir_imm_float(b, -128.0));
                        }
                } else {
                        if (chan->normalized) {
                                return vc4_nir_unpack_8f(b, vpm, swiz);
                        } else {
                                return nir_i2f32(b, vc4_nir_unpack_8i(b, vpm, swiz));
                        }
                }
        } else if (chan->size == 16 &&
                   (chan->type == UTIL_FORMAT_TYPE_UNSIGNED ||
                    chan->type == UTIL_FORMAT_TYPE_SIGNED)) {
                nir_ssa_def *vpm = vpm_reads[swiz / 2];

                /* Note that UNPACK_16F eats a half float, not ints, so we use
                 * UNPACK_16_I for all of these.
                 */
                if (chan->type == UTIL_FORMAT_TYPE_SIGNED) {
                        temp = nir_i2f32(b, vc4_nir_unpack_16i(b, vpm, swiz & 1));
                        if (chan->normalized) {
                                return nir_fmul(b, temp,
                                                nir_imm_float(b, 1/32768.0f));
                        } else {
                                return temp;
                        }
                } else {
                        temp = nir_i2f32(b, vc4_nir_unpack_16u(b, vpm, swiz & 1));
                        if (chan->normalized) {
                                return nir_fmul(b, temp,
                                                nir_imm_float(b, 1 / 65535.0));
                        } else {
                                return temp;
                        }
                }
        } else {
                return NULL;
        }
}

static void
vc4_nir_lower_vertex_attr(struct vc4_compile *c, nir_builder *b,
                          nir_intrinsic_instr *intr)
{
        b->cursor = nir_before_instr(&intr->instr);

        int attr = nir_intrinsic_base(intr);
        enum pipe_format format = c->vs_key->attr_formats[attr];
        uint32_t attr_size = util_format_get_blocksize(format);

        /* We only accept direct outputs and TGSI only ever gives them to us
         * with an offset value of 0.
         */
        assert(nir_src_as_uint(intr->src[0]) == 0);

        /* Generate dword loads for the VPM values (Since these intrinsics may
         * be reordered, the actual reads will be generated at the top of the
         * shader by ntq_setup_inputs().
         */
        nir_ssa_def *vpm_reads[4];
        for (int i = 0; i < align(attr_size, 4) / 4; i++) {
                nir_intrinsic_instr *intr_comp =
                        nir_intrinsic_instr_create(c->s,
                                                   nir_intrinsic_load_input);
                intr_comp->num_components = 1;
                nir_intrinsic_set_base(intr_comp, nir_intrinsic_base(intr));
                nir_intrinsic_set_component(intr_comp, i);
                intr_comp->src[0] = nir_src_for_ssa(nir_imm_int(b, 0));
                nir_ssa_dest_init(&intr_comp->instr, &intr_comp->dest, 1, 32, NULL);
                nir_builder_instr_insert(b, &intr_comp->instr);

                vpm_reads[i] = &intr_comp->dest.ssa;
        }

        bool format_warned = false;
        const struct util_format_description *desc =
                util_format_description(format);

        nir_ssa_def *dests[4];
        for (int i = 0; i < intr->num_components; i++) {
                uint8_t swiz = desc->swizzle[i];
                dests[i] = vc4_nir_get_vattr_channel_vpm(c, b, vpm_reads, swiz,
                                                         desc);

                if (!dests[i]) {
                        if (!format_warned) {
                                fprintf(stderr,
                                        "vtx element %d unsupported type: %s\n",
                                        attr, util_format_name(format));
                                format_warned = true;
                        }
                        dests[i] = nir_imm_float(b, 0.0);
                }
        }

        replace_intrinsic_with_vec(b, intr, dests);
}

static void
vc4_nir_lower_fs_input(struct vc4_compile *c, nir_builder *b,
                       nir_intrinsic_instr *intr)
{
        b->cursor = nir_after_instr(&intr->instr);

        if (nir_intrinsic_base(intr) >= VC4_NIR_TLB_COLOR_READ_INPUT &&
            nir_intrinsic_base(intr) < (VC4_NIR_TLB_COLOR_READ_INPUT +
                                        VC4_MAX_SAMPLES)) {
                /* This doesn't need any lowering. */
                return;
        }

        nir_variable *input_var =
                nir_find_variable_with_driver_location(c->s, nir_var_shader_in,
                                                       nir_intrinsic_base(intr));
        assert(input_var);

        int comp = nir_intrinsic_component(intr);

        /* Lower away point coordinates, and fix up PNTC. */
        if (util_varying_is_point_coord(input_var->data.location,
                                        c->fs_key->point_sprite_mask)) {
                assert(intr->num_components == 1);

                nir_ssa_def *result = &intr->dest.ssa;

                switch (comp) {
                case 0:
                case 1:
                        /* If we're not rendering points, we need to set a
                         * defined value for the input that would come from
                         * PNTC.
                         */
                        if (!c->fs_key->is_points)
                                result = nir_imm_float(b, 0.0);
                        break;
                case 2:
                        result = nir_imm_float(b, 0.0);
                        break;
                case 3:
                        result = nir_imm_float(b, 1.0);
                        break;
                }

                if (c->fs_key->point_coord_upper_left && comp == 1)
                        result = nir_fsub(b, nir_imm_float(b, 1.0), result);

                if (result != &intr->dest.ssa) {
                        nir_ssa_def_rewrite_uses_after(&intr->dest.ssa,
                                                       nir_src_for_ssa(result),
                                                       result->parent_instr);
                }
        }
}

static void
vc4_nir_lower_output(struct vc4_compile *c, nir_builder *b,
                     nir_intrinsic_instr *intr)
{
        nir_variable *output_var =
                nir_find_variable_with_driver_location(c->s, nir_var_shader_out,
                                                       nir_intrinsic_base(intr));
        assert(output_var);

        if (c->stage == QSTAGE_COORD &&
            output_var->data.location != VARYING_SLOT_POS &&
            output_var->data.location != VARYING_SLOT_PSIZ) {
                nir_instr_remove(&intr->instr);
                return;
        }
}

static void
vc4_nir_lower_uniform(struct vc4_compile *c, nir_builder *b,
                      nir_intrinsic_instr *intr)
{
        b->cursor = nir_before_instr(&intr->instr);

        /* Generate scalar loads equivalent to the original vector. */
        nir_ssa_def *dests[4];
        for (unsigned i = 0; i < intr->num_components; i++) {
                nir_intrinsic_instr *intr_comp =
                        nir_intrinsic_instr_create(c->s, intr->intrinsic);
                intr_comp->num_components = 1;
                nir_ssa_dest_init(&intr_comp->instr, &intr_comp->dest, 1,
                                  intr->dest.ssa.bit_size, NULL);

                /* Convert the uniform offset to bytes.  If it happens
                 * to be a constant, constant-folding will clean up
                 * the shift for us.
                 */
                nir_intrinsic_set_base(intr_comp,
                                       nir_intrinsic_base(intr) * 16 +
                                       i * 4);
                nir_intrinsic_set_range(intr_comp,
                                        nir_intrinsic_range(intr) * 16 - i * 4);

                intr_comp->src[0] =
                        nir_src_for_ssa(nir_ishl(b, intr->src[0].ssa,
                                                 nir_imm_int(b, 4)));

                dests[i] = &intr_comp->dest.ssa;

                nir_builder_instr_insert(b, &intr_comp->instr);
        }

        replace_intrinsic_with_vec(b, intr, dests);
}

static void
vc4_nir_lower_io_instr(struct vc4_compile *c, nir_builder *b,
                       struct nir_instr *instr)
{
        if (instr->type != nir_instr_type_intrinsic)
                return;
        nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);

        switch (intr->intrinsic) {
        case nir_intrinsic_load_input:
                if (c->stage == QSTAGE_FRAG)
                        vc4_nir_lower_fs_input(c, b, intr);
                else
                        vc4_nir_lower_vertex_attr(c, b, intr);
                break;

        case nir_intrinsic_store_output:
                vc4_nir_lower_output(c, b, intr);
                break;

        case nir_intrinsic_load_uniform:
                vc4_nir_lower_uniform(c, b, intr);
                break;

        case nir_intrinsic_load_user_clip_plane:
        default:
                break;
        }
}

static bool
vc4_nir_lower_io_impl(struct vc4_compile *c, nir_function_impl *impl)
{
        nir_builder b;
        nir_builder_init(&b, impl);

        nir_foreach_block(block, impl) {
                nir_foreach_instr_safe(instr, block)
                        vc4_nir_lower_io_instr(c, &b, instr);
        }

        nir_metadata_preserve(impl, nir_metadata_block_index |
                              nir_metadata_dominance);

        return true;
}

void
vc4_nir_lower_io(nir_shader *s, struct vc4_compile *c)
{
        nir_foreach_function(function, s) {
                if (function->impl)
                        vc4_nir_lower_io_impl(c, function->impl);
        }
}

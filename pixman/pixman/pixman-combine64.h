/* WARNING: This file is generated by combine.pl from combine.inc.
   Please edit one of those files rather than this one. */

#line 1 "pixman-combine.c.template"

#define COMPONENT_SIZE 16
#define MASK 0xffffULL
#define ONE_HALF 0x8000ULL

#define A_SHIFT 16 * 3
#define R_SHIFT 16 * 2
#define G_SHIFT 16
#define A_MASK 0xffff000000000000ULL
#define R_MASK 0xffff00000000ULL
#define G_MASK 0xffff0000ULL

#define RB_MASK 0xffff0000ffffULL
#define AG_MASK 0xffff0000ffff0000ULL
#define RB_ONE_HALF 0x800000008000ULL
#define RB_MASK_PLUS_ONE 0x10000000010000ULL

#define ALPHA_16(x) ((x) >> A_SHIFT)
#define RED_16(x) (((x) >> R_SHIFT) & MASK)
#define GREEN_16(x) (((x) >> G_SHIFT) & MASK)
#define BLUE_16(x) ((x) & MASK)

/*
 * Helper macros.
 */

#define MUL_UN16(a, b, t)						\
    ((t) = (a) * (b) + ONE_HALF, ((((t) >> G_SHIFT ) + (t) ) >> G_SHIFT ))

#define DIV_UN16(a, b)							\
    (((uint32_t) (a) * MASK) / (b))

#define ADD_UN16(x, y, t)				     \
    ((t) = x + y,					     \
     (uint64_t) (uint16_t) ((t) | (0 - ((t) >> G_SHIFT))))

#define DIV_ONE_UN16(x)							\
    (((x) + ONE_HALF + (((x) + ONE_HALF) >> G_SHIFT)) >> G_SHIFT)

/*
 * The methods below use some tricks to be able to do two color
 * components at the same time.
 */

/*
 * x_c = (x_c * a) / 255
 */
#define UN16x4_MUL_UN16(x, a)						\
    do									\
    {									\
	uint64_t t = ((x & RB_MASK) * a) + RB_ONE_HALF;                  \
	t = (t + ((t >> COMPONENT_SIZE) & RB_MASK)) >> COMPONENT_SIZE;  \
	t &= RB_MASK;                                                   \
                                                                        \
	x = (((x >> COMPONENT_SIZE) & RB_MASK) * a) + RB_ONE_HALF;      \
	x = (x + ((x >> COMPONENT_SIZE) & RB_MASK));                    \
	x &= RB_MASK << COMPONENT_SIZE;                                 \
	x += t;                                                         \
    } while (0)

/*
 * x_c = (x_c * a) / 255 + y_c
 */
#define UN16x4_MUL_UN16_ADD_UN16x4(x, a, y)				\
    do									\
    {									\
	/* multiply and divide: trunc((i + 128)*257/65536) */           \
	uint64_t t = ((x & RB_MASK) * a) + RB_ONE_HALF;                  \
	t = (t + ((t >> COMPONENT_SIZE) & RB_MASK)) >> COMPONENT_SIZE;  \
	t &= RB_MASK;                                                   \
                                                                        \
	/* add */                                                       \
	t += y & RB_MASK;                                               \
                                                                        \
	/* saturate */                                                  \
	t |= RB_MASK_PLUS_ONE - ((t >> COMPONENT_SIZE) & RB_MASK);      \
	t &= RB_MASK;                                                   \
                                                                        \
	/* multiply and divide */                                       \
	x = (((x >> COMPONENT_SIZE) & RB_MASK) * a) + RB_ONE_HALF;      \
	x = (x + ((x >> COMPONENT_SIZE) & RB_MASK)) >> COMPONENT_SIZE;  \
	x &= RB_MASK;                                                   \
                                                                        \
	/* add */                                                       \
	x += (y >> COMPONENT_SIZE) & RB_MASK;                           \
                                                                        \
	/* saturate */                                                  \
	x |= RB_MASK_PLUS_ONE - ((x >> COMPONENT_SIZE) & RB_MASK);      \
	x &= RB_MASK;                                                   \
                                                                        \
	/* recombine */                                                 \
	x <<= COMPONENT_SIZE;                                           \
	x += t;                                                         \
    } while (0)

/*
 * x_c = (x_c * a + y_c * b) / 255
 */
#define UN16x4_MUL_UN16_ADD_UN16x4_MUL_UN16(x, a, y, b)			\
    do									\
    {									\
	uint64_t t;                                                      \
	uint64_t r = (x >> A_SHIFT) * a + (y >> A_SHIFT) * b + ONE_HALF; \
	r += (r >> G_SHIFT);                                            \
	r >>= G_SHIFT;                                                  \
                                                                        \
	t = (x & G_MASK) * a + (y & G_MASK) * b;                        \
	t += (t >> G_SHIFT) + (ONE_HALF << G_SHIFT);                    \
	t >>= R_SHIFT;                                                  \
                                                                        \
	t |= r << R_SHIFT;                                              \
	t |= RB_MASK_PLUS_ONE - ((t >> G_SHIFT) & RB_MASK);             \
	t &= RB_MASK;                                                   \
	t <<= G_SHIFT;                                                  \
                                                                        \
	r = ((x >> R_SHIFT) & MASK) * a +                               \
	    ((y >> R_SHIFT) & MASK) * b + ONE_HALF;                     \
	r += (r >> G_SHIFT);                                            \
	r >>= G_SHIFT;                                                  \
                                                                        \
	x = (x & MASK) * a + (y & MASK) * b + ONE_HALF;                 \
	x += (x >> G_SHIFT);                                            \
	x >>= G_SHIFT;                                                  \
	x |= r << R_SHIFT;                                              \
	x |= RB_MASK_PLUS_ONE - ((x >> G_SHIFT) & RB_MASK);             \
	x &= RB_MASK;                                                   \
	x |= t;                                                         \
    } while (0)

/*
 * x_c = (x_c * a_c) / 255
 */
#define UN16x4_MUL_UN16x4(x, a)						\
    do									\
    {									\
	uint64_t t;                                                      \
	uint64_t r = (x & MASK) * (a & MASK);                            \
	r |= (x & R_MASK) * ((a >> R_SHIFT) & MASK);                    \
	r += RB_ONE_HALF;                                               \
	r = (r + ((r >> G_SHIFT) & RB_MASK)) >> G_SHIFT;                \
	r &= RB_MASK;                                                   \
                                                                        \
	x >>= G_SHIFT;                                                  \
	t = (x & MASK) * ((a >> G_SHIFT) & MASK);                       \
	t |= (x & R_MASK) * (a >> A_SHIFT);                             \
	t += RB_ONE_HALF;                                               \
	t = t + ((t >> G_SHIFT) & RB_MASK);                             \
	x = r | (t & AG_MASK);                                          \
    } while (0)

/*
 * x_c = (x_c * a_c) / 255 + y_c
 */
#define UN16x4_MUL_UN16x4_ADD_UN16x4(x, a, y)				\
    do									\
    {									\
	uint64_t t;                                                      \
	uint64_t r = (x & MASK) * (a & MASK);                            \
	r |= (x & R_MASK) * ((a >> R_SHIFT) & MASK);                    \
	r += RB_ONE_HALF;                                               \
	r = (r + ((r >> G_SHIFT) & RB_MASK)) >> G_SHIFT;                \
	r &= RB_MASK;                                                   \
	r += y & RB_MASK;                                               \
	r |= RB_MASK_PLUS_ONE - ((r >> G_SHIFT) & RB_MASK);             \
	r &= RB_MASK;                                                   \
                                                                        \
	x >>= G_SHIFT;                                                  \
	t = (x & MASK) * ((a >> G_SHIFT) & MASK);                       \
	t |= (x & R_MASK) * (a >> A_SHIFT);                             \
	t += RB_ONE_HALF;                                               \
	t = (t + ((t >> G_SHIFT) & RB_MASK)) >> G_SHIFT;                \
	t &= RB_MASK;                                                   \
	t += (y >> G_SHIFT) & RB_MASK;                                  \
	t |= RB_MASK_PLUS_ONE - ((t >> G_SHIFT) & RB_MASK);             \
	t &= RB_MASK;                                                   \
	x = r | (t << G_SHIFT);                                         \
    } while (0)

/*
 * x_c = (x_c * a_c + y_c * b) / 255
 */
#define UN16x4_MUL_UN16x4_ADD_UN16x4_MUL_UN16(x, a, y, b)			\
    do									\
    {									\
	uint64_t t;                                                      \
	uint64_t r = (x >> A_SHIFT) * (a >> A_SHIFT) +                   \
	    (y >> A_SHIFT) * b;						\
	r += (r >> G_SHIFT) + ONE_HALF;                                 \
	r >>= G_SHIFT;                                                  \
        								\
	t = (x & G_MASK) * ((a >> G_SHIFT) & MASK) + (y & G_MASK) * b;  \
	t += (t >> G_SHIFT) + (ONE_HALF << G_SHIFT);                    \
	t >>= R_SHIFT;                                                  \
        								\
	t |= r << R_SHIFT;                                              \
	t |= RB_MASK_PLUS_ONE - ((t >> G_SHIFT) & RB_MASK);             \
	t &= RB_MASK;                                                   \
	t <<= G_SHIFT;                                                  \
									\
	r = ((x >> R_SHIFT) & MASK) * ((a >> R_SHIFT) & MASK) +         \
	    ((y >> R_SHIFT) & MASK) * b + ONE_HALF;                     \
	r += (r >> G_SHIFT);                                            \
	r >>= G_SHIFT;                                                  \
        								\
	x = (x & MASK) * (a & MASK) + (y & MASK) * b + ONE_HALF;        \
	x += (x >> G_SHIFT);                                            \
	x >>= G_SHIFT;                                                  \
	x |= r << R_SHIFT;                                              \
	x |= RB_MASK_PLUS_ONE - ((x >> G_SHIFT) & RB_MASK);             \
	x &= RB_MASK;                                                   \
	x |= t;                                                         \
    } while (0)

/*
   x_c = min(x_c + y_c, 255)
 */
#define UN16x4_ADD_UN16x4(x, y)						\
    do									\
    {									\
	uint64_t t;                                                      \
	uint64_t r = (x & RB_MASK) + (y & RB_MASK);                      \
	r |= RB_MASK_PLUS_ONE - ((r >> G_SHIFT) & RB_MASK);             \
	r &= RB_MASK;                                                   \
        								\
	t = ((x >> G_SHIFT) & RB_MASK) + ((y >> G_SHIFT) & RB_MASK);    \
	t |= RB_MASK_PLUS_ONE - ((t >> G_SHIFT) & RB_MASK);             \
	r |= (t & RB_MASK) << G_SHIFT;                                  \
	x = r;                                                          \
    } while (0)

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


static const int min_locations = 4;  /* stop after finding this many */
static const int max_radius = 1024;  /* but not past this many blocks away */


/* ref MC 1.20.2 */

static const int64_t rng_poly = INT64_C(0x0005deece66d);
static const int64_t rng_mask = INT64_C(0xffffffffffff);
static const int64_t rng_adv = 0xb;

/* LegacyRandomSource.m_188584_ */
static inline int64_t
rng_init (int64_t seed)
{
    return (seed ^ rng_poly) & rng_mask;
}

/* LegacyRandomSource.m_64707_(32) */
static inline int64_t
rng_next32 (int64_t *rng)
{
    *rng = *rng*rng_poly + rng_adv & rng_mask;
    return *rng >> 48-32;
}

/* BitRandomSource.m_188505_ */
static inline int64_t
rng_next64 (int64_t *rng)
{
    int64_t hi = rng_next32(rng);
    int32_t lo = rng_next32(rng);
    return (hi<<32) + lo;
}

/* BitRandomSource.m_188501_ */
static inline float
rng_nextfloat (int64_t *rng)
{
    return (rng_next32(rng) >> 8) * 5.9604645e-8;
}

static inline int64_t
gen_bypos_seed (int64_t src_seed)
{
    /* WorldgenRandom.Algorithm.m_224687_ */
    int64_t rng = rng_init(src_seed);
    /* LegacyRandomSource.m_188582_ */
    return rng_next64(&rng);
}

/* LegacyPositionalRandomFactory.m_213715_ */
static inline int64_t
rng_forpos (int64_t seed, int x, int y, int z)
{
    /* Mth.m_14130_ */
    const int32_t mix_x = 0x002fc20f;  // NB 32 bit
    const int64_t mix_z = INT64_C(0x06ebfff5);
    const int64_t mix2 = INT64_C(0x0285b825);
    const int64_t mix1 = 0xb;

    int64_t h = x*mix_x ^ z*mix_z ^ y;
    return rng_init(h*h*mix2 + h*mix1 >> 16 ^ seed);
}

/* String.hashCode */
static inline int32_t
ascii_hash (const char *str)
{
    int32_t h = str[0];
    for(int i = 1; str[i]; i++)
        h = 31*h + str[i];
    return h;
}


/* SurfaceRules.VerticalGradientConditionSource.f_189829_ */
#define ceiling_top 127
/* SurfaceRules.VerticalGradientConditionSource.f_189830_ */
#define ceiling_bottom (127 - 5)

// Mth.m_144914_((double)y, (double)true_at_and_below, (double)false_at_and_above, 1.0D, 0.0D)
#define ceiling_height (double)(ceiling_top - ceiling_bottom)
static const double gradient[] = {
#define GRAD(_y) (1 - (_y - ceiling_bottom) / ceiling_height)
    GRAD(122), GRAD(123), GRAD(124), GRAD(125), GRAD(126), GRAD(127),
#undef GRAD
};

/* SurfaceSystem.m_224648_ */
static inline uint64_t
gen_ceiling_row64 (int64_t seed, int x0, int y, int z)
{
    assert(ceiling_bottom < y && y < ceiling_top);
    const double thr = gradient[y - ceiling_bottom];
    uint64_t row = 0;
    for(int dx = -31; dx <= 32; dx++) {
        // VerticalGradientCondition.m_183479_
        int64_t rng = rng_forpos(seed, x0+dx, y, z);
        row = row<<1 | !(rng_nextfloat(&rng) < thr);
    }
    return row;
}

static inline void
dump_ceiling_around (int64_t seed, int x0, int z0)
{
    char x0str[64];
    int n = snprintf(x0str, sizeof(x0str)-1, "%16s%d%16s", "", x0, "");
    x0str[n/2 + 12] = 0;
    char *x0cen = x0str + (n/2 - 8);
    printf("%14s%s%s", "x=", x0cen, x0cen);
    x0str[n-16] = 0;
    printf("%s\n", x0cen);
    printf("%14s[%3d]   |%11s[%3d]   |%11s[%3d]   |\n",
           "y=", 123, "", 124, "", 125);

    for(int dz = -3; dz <= 3; dz++) {
        if(dz)
            printf("%14s ", (dz != -1) ? "" : "z=");
        else
            printf("%13d -", z0);

        for(int y = 123; y <= 125; y++) {
            uint64_t row = gen_ceiling_row64(seed, x0, y, z0+dz);
            for(int dx = -3; dx <= 3; dx++)
                printf(" %c", (row>>32-dx & 1) ? '#' : '.');
            if(y < 125)
                printf("%6s", "");
        }
        printf("\n");
    }
}


static int quiet;

static inline uint64_t
erosionx (uint64_t src)
{
    return src>>1 & src<<1;
}

static inline uint64_t
erodex (uint64_t src)
{
    return src & erosionx(src);
}

static inline int
check_around (int64_t seed, int x0, int z0)
{
    int nfound = 0;

    uint64_t layer7c[2], layer7d[2];
    for(int z = z0-31; z <= z0-30; z++) {
        layer7c[~z&1] = erodex(gen_ceiling_row64(seed, x0, 124, z));
        layer7d[~z&1] = gen_ceiling_row64(seed, x0, 125, z);
    }

    for(int z = z0-30; z <= z0+31; z++) {
        int i = z&1;
        uint64_t mask = ~gen_ceiling_row64(seed, x0, 123, z);
        mask &= layer7c[0] & layer7c[1];
        mask &= layer7d[i];

        uint64_t row2_7c = erodex(gen_ceiling_row64(seed, x0, 124, z+1));
        layer7c[i] = row2_7c;
        mask &= row2_7c;

        uint64_t row2_7d = gen_ceiling_row64(seed, x0, 125, z+1);
        mask &= row2_7d & erosionx(row2_7d & layer7d[0] & layer7d[1]);
        layer7d[i] = row2_7d;

        mask = mask >> 1 << 2;
        for(int dx = -30; mask && dx <= 31; dx++, mask <<= 1)
            if(mask>>63 & 1) {
                nfound++;
                int x = x0 + dx;
                if(!quiet) {
                    printf("found @ (%d,, %d)\n", x, z);
                    dump_ceiling_around(seed, x, z);
                } else
                    printf("%d %d\n", x, z);
            }
    }
    return nfound;
}

static inline int
search_ceiling (int64_t seed, int x0, int z0)
{
    const int step = 62;
    x0 &= ~1, z0 &= ~1;
    int nfound = check_around(seed, x0, z0);
    int r = step;
    for(; r < max_radius && nfound < min_locations; r += step) {
        if(!quiet)
            printf("search radius [%d:%d]\n", r - (step-1)/2, r + step/2);

        for(int i = -r; i < r; i += step) {
            nfound += check_around(seed, x0 + i, z0 - r);
            nfound += check_around(seed, x0 + r, z0 + i);
            nfound += check_around(seed, x0 - i, z0 + r);
            nfound += check_around(seed, x0 - r, z0 - i);
        }
    }

    if(!quiet)
        printf("found %d locations within %d blocks\n", nfound, r + step/2);
    return nfound;
}


int main (int argc, char **argv)
{
    assert(gradient[0] == 1);
    assert(gradient[127-ceiling_bottom] == 0);

    quiet = argc > 1 && !strcmp(argv[1], "-q");
    int iarg0 = 1 + quiet;
    int nargs = argc - iarg0;
    if(nargs < 1 || nargs == 2 || nargs > 3) {
        printf("usage: %s [-q] world_seed [start_x start_z]\n", argv[0]);
        return argc != 1;
    }

    int64_t world_seed = strtoll(argv[iarg0], NULL, 0);
    int64_t pos_seed = gen_bypos_seed(world_seed);
    int64_t ceil_seed = gen_bypos_seed(
        pos_seed ^ ascii_hash("minecraft:bedrock_roof")
    );

    int x0 = 0, z0 = 0;
    if(nargs > 1) {
        x0 = strtol(argv[iarg0+1], NULL, 0);
        z0 = strtol(argv[iarg0+2], NULL, 0);
    }

    if(!quiet)
        printf("seed=%"PRId64" around (%d,, %d)\n", world_seed, x0, z0);

    return !search_ceiling(ceil_seed, x0, z0);
}

#include <stdint.h>


/* RNG - ref MC 1.20.2 */

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
static inline int32_t
rng_next32 (int64_t *rng)
{
    *rng = *rng*rng_poly + rng_adv & rng_mask;
    return *rng >> 48-32;
}

/* BitRandomSource.m_188505_ */
static inline int64_t
rng_next64 (int64_t *rng)
{
    uint64_t hi = rng_next32(rng);
    int64_t lo = rng_next32(rng);
    return (hi<<32) + lo;
}

/* BitRandomSource.m_188501_ */
static inline float
rng_nextfloat (int64_t *rng)
{
    return ((uint32_t)rng_next32(rng) >> 8) * 5.9604645e-8f;  // (1.f/(1<<24))
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
    const int64_t mix_x = 0x002fc20f;
    const int64_t mix_z = INT64_C(0x06ebfff5);
    const uint64_t mix2 = INT64_C(0x0285b825);
    const uint64_t mix1 = 0xb;

    int32_t hx = mix_x * x;  // NB 32 bit
    int64_t hz = mix_z * z;
    uint64_t h = (int64_t)hx ^ hz ^ y;
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
    const double thr = gradient[y - ceiling_bottom];
    uint64_t row = 0;
    for(int dx = -32; dx < 32; dx++) {
        // VerticalGradientCondition.m_183479_
        int64_t rng = rng_forpos(seed, x0+dx, y, z);
        row = row<<1 | !(rng_nextfloat(&rng) < thr);
    }
    return row;
}

static inline uint64_t
gen_ceiling_col64 (int64_t seed, int x, int y, int z0)
{
    const double thr = gradient[y - ceiling_bottom];
    uint64_t col = 0;
    for(int dz = -32; dz < 32; dz++) {
        int64_t rng = rng_forpos(seed, x, y, z0+dz);
        col = col<<1 | !(rng_nextfloat(&rng) < thr);
    }
    return col;
}


/*--------------------------------------------------------------------------*/
/* core search library */

#define STEP 60			/* 64-bits -2 on each size for 5 wide */

typedef struct n9cfg {
    int (*onsearch)(int);
    int (*onresult)(int, int);
    int64_t seed;		/* *position* seed for minecraft:bedrock_roof */
    uint8_t sel[4/*y-layers*/][5/*z-blks*/ * 5/*x-blks*/];
} n9cfg_t;


static inline uint64_t
match_slice (const uint64_t *masks, const uint8_t *sel, int di)
{
    return masks[sel[0]] >> 2
        & masks[sel[di]] >> 1
        & masks[sel[2*di]]
        & masks[sel[3*di]] << 1
        & masks[sel[4*di]] << 2;
}

static inline int
searchz (const n9cfg_t *cfg, int x0, int z0, int z1)
{
    uint64_t match[8], masks[3];
    for(int i = 0; i < 8; i++)
        match[i] = UINT64_C(-1);
    masks[2] = UINT64_C(-1);

    for(int dz = 0; dz < 4; dz++) {
        const int z = z0 - 2 + dz;
        for(int dy = 0; dy < 4; dy++) {
            const int y = ceiling_bottom+1 + dy;
            const uint64_t row = gen_ceiling_row64(cfg->seed, x0, y, z);
            masks[0] = ~row, masks[1] = row;
            for(int k = 0; k <= dz; k++)
                match[z+2-k & 7] &= match_slice(masks, cfg->sel[dy] + 5*k, 1);
        }
    }

    for(int z = z0; z < z1; z++) {
        match[z+4 & 7] = UINT64_C(-1);
        for(int dy = 0; dy < 4; dy++) {
            const int y = ceiling_bottom+1 + dy;
            const uint64_t row = gen_ceiling_row64(cfg->seed, x0, y, z+2);
            masks[0] = ~row, masks[1] = row;
            for(int k = 0; k < 5; k++)
                match[z+4-k & 7] &= match_slice(masks, cfg->sel[dy] + 5*k, 1);
        }

        uint64_t mask = match[z & 7] >> 2 << 4;
        for(int dx = -STEP/2; mask && dx < STEP/2; dx++, mask <<= 1)
            if(mask & (UINT64_C(1)<<63) && cfg->onresult(x0+dx, z))
                return 1;
    }
    return 0;
}

static inline int
searchx (const n9cfg_t *cfg, int z0, int x0, int x1)
{
    uint64_t match[8], masks[3];
    for(int i = 0; i < 8; i++)
        match[i] = UINT64_C(-1);
    masks[2] = UINT64_C(-1);

    for(int dx = 0; dx < 4; dx++) {
        const int x = x0 - 2 + dx;
        for(int dy = 0; dy < 4; dy++) {
            const int y = ceiling_bottom+1 + dy;
            const uint64_t col = gen_ceiling_col64(cfg->seed, x, y, z0);
            masks[0] = ~col, masks[1] = col;
            for(int k = 0; k <= dx; k++)
                match[x+2-k & 7] &= match_slice(masks, cfg->sel[dy] + k, 5);
        }
    }

    for(int x = x0; x < x1; x++) {
        match[x+4 & 7] = UINT64_C(-1);
        for(int dy = 0; dy < 4; dy++) {
            const int y = ceiling_bottom+1 + dy;
            const uint64_t col = gen_ceiling_col64(cfg->seed, x+2, y, z0);
            masks[0] = ~col, masks[1] = col;
            for(int k = 0; k < 5; k++)
                match[x+4-k & 7] &= match_slice(masks, cfg->sel[dy] + k, 5);
        }

        uint64_t mask = match[x & 7] >> 2 << 4;
        for(int dz = -STEP/2; mask && dz < STEP/2; dz++, mask <<= 1)
            if(mask & (UINT64_C(1)<<63) && cfg->onresult(x, z0+dz))
                return 1;
    }
    return 0;
}

int64_t
n9_gen_seed (int64_t world_seed)
{
    int64_t pos_seed = gen_bypos_seed(world_seed);
    return gen_bypos_seed(pos_seed ^ ascii_hash("minecraft:bedrock_roof"));
}

void
n9_gen_ceiling_around (const n9cfg_t *cfg, int x0, int z0, uint8_t pat[2*4*8])
{
    x0 += 3 - 31;
    for(int dy = 0; dy < 4; dy++) {
        int y = ceiling_bottom+1 + dy;
        for(int dz = 0; dz < 8; dz++) {
            int i = 8*dy + dz;
            pat[i] = gen_ceiling_row64(cfg->seed, x0, y, z0+dz-4) & 0xff;
            pat[i + 4*8] = 0;
        }
    }
}

void
n9_build_pattern (n9cfg_t *cfg, const uint8_t pat[2*4*8])
{
    for(int dy = 0; dy < 4; dy++)
        for(int dz = -2; dz <= 2; dz++) {
            int i = 8*dy + dz+4;
            for(int dx = -2; dx <= 2; dx++)
                cfg->sel[dy][5*(dz+2) + dx+2] =
                    (pat[i + 4*8] >> 3-dx & 1) ? 2 : (pat[i] >> 3-dx & 1);
        }
}

void
n9_search_ceiling (const n9cfg_t *cfg, int x0, int z0)
{
    for(int r = 0; ; r += STEP)
        if(searchz(cfg, x0 + (r + STEP/2), z0 - (r + STEP), z0 + r)
           | searchx(cfg, z0 + (r + STEP/2), x0 - r, x0 + (r + STEP))
           | searchz(cfg, x0 - (r + STEP/2), z0 - r, z0 + (r + STEP))
           | searchx(cfg, z0 - (r + STEP/2), x0 - (r + STEP), x0 + r)
           | (cfg->onsearch && cfg->onsearch(r + STEP)))
            break;
}


/*--------------------------------------------------------------------------*/
/* global interface for easier wasm */

#ifndef N9_NOGLOBALS
# define EXPORT __attribute__((visibility("default")))

extern int onsearch(int radius);
extern int onresult(int x, int z);

static n9cfg_t n9cfg;

EXPORT uint8_t pattern[2*4*8];

EXPORT const uint8_t default_pattern[2*4*8] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
    0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
    0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,

    0xff, 0xff, 0xff, 0xff, 0xf7, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xe3, 0xe3, 0xe3, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xe3, 0xeb, 0xe3, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};


static inline void
init_cfg (int64_t seed, const uint8_t *pat)
{
    n9cfg.onsearch = onsearch;
    n9cfg.onresult = onresult;
    n9cfg.seed = n9_gen_seed(seed);
    n9_build_pattern(&n9cfg, pat);
}

EXPORT void
gen_ceiling_around (int64_t seed, int x, int z)
{
    n9cfg.seed = n9_gen_seed(seed);
    n9_gen_ceiling_around(&n9cfg, x, z, pattern);
}

EXPORT void
search_ceiling (int64_t seed, int x0, int z0)
{
    init_cfg(seed, pattern);
    n9_search_ceiling(&n9cfg, x0, z0);
}
#endif


/*--------------------------------------------------------------------------*/
/* command line interface */

#ifndef N9_NOMAIN
# include <inttypes.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <errno.h>
# include <assert.h>

static int min_matches = 4;	/* stop after finding this many */
static int max_radius = 1024;	/* but not past this many blocks away */
static int nfound, radius;
static int quiet;


static inline void
dump_pattern (int x0, int z0)
{
    char x0str[64];
    int n = snprintf(x0str, sizeof(x0str)-1, "%16s%d%16s", "", x0, "");
    x0str[n/2 + 10] = 0;
    char *x0cen = x0str + (n/2 - 8);
    printf("%9s%s%s%s", "x=", x0cen, x0cen, x0cen);
    x0str[n-16] = 0;
    printf("%s\n", x0cen);
    printf("%9s[%3d]   |%9s[%3d]   |%9s[%3d]   |%9s[%3d]   |\n",
           "y=", 123, "", 124, "", 125, "", 126);

    for(int dz = -3; dz <= 3; dz++) {
        if(dz)
            printf("%9s ", (dz != -1) ? "" : "z=");
        else
            printf("%8d -", z0);

        for(int dy = 0; dy < 4; dy++) {
            uint8_t row = pattern[8*dy + dz+4];
            for(int dx = -3; dx <= 3; dx++)
                printf(" %c", (row>>3-dx & 1) ? '#' : '.');
            if(dy < 3)
                printf("%4s", "");
        }
        printf("\n");
    }
}

int
onsearch (int r)
{
    radius = r;
    if(!quiet)
        printf("search radius %d\n", r);
    return r > max_radius || nfound >= min_matches;
}

int
onresult (int x, int z)
{
    nfound++;
    if(!quiet) {
        printf("found @ (%d,, %d)\n", x, z);
        n9_gen_ceiling_around(&n9cfg, x, z, pattern);
        dump_pattern(x, z);
    } else
        printf("%d %d\n", x, z);
    return 0;
}

static inline int
usage (int rc, const char *cmd, const char *msg)
{
    if(msg)
        fprintf(stderr, "%s", msg);
    fprintf((rc) ? stderr : stdout,
        "usage: %s [-q] world_seed [start_x start_z]\n", cmd);
    return (rc) ? 2 : 0;
}

int main (int argc, char **argv)
{
    assert(INT64_C(-1)>>1 == INT64_C(-1));
    assert(gradient[0] == 1);
    assert(gradient[127-ceiling_bottom] == 0);

    quiet = argc > 1 && !strcmp(argv[1], "-q");
    int iarg0 = 1 + quiet;
    int nargs = argc - iarg0;
    if(!(nargs == 1 || nargs == 3))
        return usage(argc != 1, argv[0], NULL);

    char *end = NULL;
    errno = 0;
    int64_t seed = strtoll(argv[iarg0], &end, 0);
    if(!end || *end || errno)
        return usage(2, argv[0], "ERROR: invalid world seed\n");

    int x0 = 0, z0 = 0;
    if(nargs > 1) {
        end = NULL;
        x0 = strtol(argv[iarg0+1], &end, 0);
        if(!end || *end)
            return usage(2, argv[0], "ERROR: invalid x location\n");

        end = NULL;
        z0 = strtol(argv[iarg0+2], &end, 0);
        if(!end || *end)
            return usage(2, argv[0], "ERROR: invalid z location\n");
    }

    if(!quiet)
        printf("seed=%"PRId64" around (%d,, %d)\n", seed, x0, z0);

    init_cfg(seed, default_pattern);
    n9_search_ceiling(&n9cfg, x0, z0);

    if(!quiet)
        printf("found %d locations within %d blocks\n", nfound, radius);
    return !nfound;
}

#endif

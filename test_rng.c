#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define N9_NOMAIN
#define N9_NOGLOBALS
#include "nether9.c"


static inline void
verify_row (int64_t bypos, int x, int z, const uint64_t exp[4])
{
    for(int dy = 0; dy < 4; dy++) {
        uint64_t row = gen_ceiling_row64(bypos, x, 123+dy, z);
        printf("row(%d,%d,%d) = %016"PRIx64"\n", x, 123+dy, z, row);
        assert(row == exp[dy]);
    }
}

static inline void
verify_col (int64_t bypos, int x, int z, const uint64_t exp[4])
{
    for(int dy = 0; dy < 4; dy++) {
        uint64_t col = gen_ceiling_col64(bypos, x, 123+dy, z);
        printf("col(%d,%d,%d) = %016"PRIx64"\n", x, 123+dy, z, col);
        assert(col == exp[dy]);
    }
}

static inline void
verify_pattern (int64_t bypos, int x, int z, const uint8_t exp[4*8])
{
    n9cfg_t cfg;
    cfg.seed = bypos;

    uint8_t pattern[2*4*8];
    memset(pattern, 0xaa, sizeof(pattern));

    n9_gen_ceiling_around(&cfg, x, z, pattern);

    for(int dy = 0; dy < 4; dy++)
        for(int dz = 0; dz < 8; dz++) {
            int i = 8*dy + dz;
            assert((pattern[i]) == exp[8*dy + dz]);
            assert(!pattern[i + 4*8]);
        }
}


int main ()
{
    // long seed = 3260359575603091685L;
    int64_t seed = INT64_C(3260359575603091685);
    printf("seed = %"PRId64"\n", seed);

    // LegacyRandomSource src = new LegacyRandomSource(seed); (.setSeed)
    int64_t src = rng_init(seed);
    printf("src = %"PRId64"\n", src); // src.f_188575_ (.state)
    assert(src == INT64_C(34941055368840));

    // LegacyPositionalRandomFactory base = src.m_188582_();
    int64_t base = gen_bypos_seed(seed);  // base.f_188586_ (.state)
    printf("base = %"PRId64"\n", base);  // src.f_188575_ (.seed)
    assert(base == INT64_C(4613382707593292652));

    // int hash = "minecraft:bedrock_roof".hashCode()
    int32_t hash = ascii_hash("minecraft:bedrock_roof");
    printf("hash = %"PRId32"\n", hash);
    assert(hash == INT32_C(343340730));

    // LegacyPositionalRandomFactory bypos
    //     = base.m_214111_("minecraft:bedrock_roof").m_188582_(); // (.derive)
    int64_t bypos = n9_gen_seed(seed);
    printf("bypos = %"PRId64"\n", bypos);  // bypos.f_188586_ (.state)
    assert(bypos == INT64_C(9021939305897899040));

    // LegacyRandomSource for_0_0 = bypos.m_213715_(0, 123, 0);
    int64_t for_0_0 = rng_forpos(bypos, 0, 123, 0);
    printf("for_0_0 = %"PRId64"\n", for_0_0);  // for_0_0.f_188575_ (.state)
    assert(for_0_0 == INT64_C(103329519911474));

    // float val = for_0_0.m_188501_(); (.nextFloat)
    float val = rng_nextfloat(&for_0_0);
    printf("val = %.8f\n", val);
    assert(val == 0.99103475f);

    // SurfaceSystem.m_224648_ ... VerticalGradientCondition.m_183479_
    verify_row(bypos, 0, 0, (uint64_t[]){
        UINT64_C(0x0801361280620481),  // 123
        UINT64_C(0x21a87cc1610104ee),
        UINT64_C(0x184fa9d2cff7eafc),
        UINT64_C(0xfaf9fdfb9bf3fdfa),  // 126
    });
    verify_col(bypos, 0, 0, (uint64_t[]){
        UINT64_C(0x03024b00c0202408),  // 123
        UINT64_C(0x012289134835bb8e),
        UINT64_C(0xae7779c8dd9dd35e),
        UINT64_C(0xfdfdb93fdf6fff2f),  // 126
    });

    verify_row(bypos, 1234, -5678, (uint64_t[]){
        UINT64_C(0xc040102a6c344277),  // 123
        UINT64_C(0x3d3250105877990b),
        UINT64_C(0x3dc7c97565cd5fdf),
        UINT64_C(0x9feff7fff777fef7),  // 126
    });
    verify_col(bypos, 1234, -5678, (uint64_t[]){
        UINT64_C(0x022010100002408c),  // 123
        UINT64_C(0x6340bb0513d84831),
        UINT64_C(0x09073ee2557e95de),
        UINT64_C(0xffffaf7def4ff5ff),  // 126
    });

    verify_pattern(bypos, 0, 0, (uint8_t[]){
        0x00, 0x00, 0x05, 0x10, 0x28, 0x0c, 0x80, 0x00,  // 123
        0x55, 0xc1, 0x69, 0x59, 0x16, 0xcf, 0xd2, 0xc3,
        0x78, 0x73, 0xc7, 0xc4, 0x2c, 0x9b, 0x75, 0x6d,
        0xbe, 0xff, 0xff, 0xbb, 0xb9, 0xce, 0xf3, 0xfd,  // 126
    });

    verify_pattern(bypos, 1234, -5678, (uint8_t[]){
        0x00, 0x04, 0x46, 0x10, 0xa6, 0x00, 0x04, 0x02,  // 123
        0x04, 0x2e, 0x73, 0xae, 0x05, 0xa5, 0x04, 0xbc,
        0x87, 0xe1, 0x79, 0x26, 0x56, 0x1d, 0xf7, 0xbf,
        0xff, 0xff, 0x76, 0x7f, 0xff, 0x5f, 0xee, 0xe6,  // 126
    });

    printf("OK\n");
    return 0;
}

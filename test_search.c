#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define N9_NOMAIN
#include "nether9.c"


static const int64_t seed = INT64_C(-3641086242640639169);
static int max_radius, nexpects, iexpect;
static int expects[8][2];

static inline void
expect (int r, int n, int (*exp)[2])
{
    max_radius = r;
    nexpects = n;
    iexpect = 0;
    memset(expects, 0xaa, sizeof(expects));
    memcpy(expects, exp, n*sizeof(*exp));
}

static inline void
expect1 (int r, int x, int z)
{
    expect(r, 1, &(int[2]){ x, z });
}


int
onsearch (int r)
{
    printf("search(%d)\n", r);
    assert(r <= max_radius);
    return 0;
}

int
onresult (int x, int z)
{
    printf("result[%d](%d, %d)\n", iexpect, x, z);
    assert(iexpect < nexpects);
    assert((size_t)nexpects < sizeof(expects)/sizeof(*expects));
    assert(x == expects[iexpect][0]);
    assert(z == expects[iexpect][1]);
    return ++iexpect >= nexpects;
}


static void
setup_strip (int x, int z)
{
    expect1(0, x, z);
    nexpects++;  // force full search
    gen_ceiling_around(seed, x, z);
    init_cfg(seed, pattern);
}

static inline void
verify_searchz (int x0, int z0, int z1, int xr, int zr)
{
    setup_strip(xr, zr);
    searchz(&n9cfg, x0, z0, z1);
    assert(iexpect == 1);
}

static inline void
verify_searchx (int x0, int z0, int z1, int xr, int zr)
{
    setup_strip(xr, zr);
    searchx(&n9cfg, x0, z0, z1);
    assert(iexpect == 1);
}


static inline void
verify_search (int x0, int z0, int r, int xr, int zr)
{
    expect1(r, xr, zr);
    gen_ceiling_around(seed, xr, zr);
    search_ceiling(seed, x0, z0);
    assert(iexpect == nexpects);
}


int main ()
{
    verify_searchz(123, 448, 464, 123, 456);
    verify_searchz(123, 448, 464, 123-30, 448);
    verify_searchz(123, 448, 464, 123+29, 448);
    verify_searchz(123, 448, 464, 123+29, 463);
    verify_searchz(123, 448, 464, 123-30, 463);

    verify_searchx(456, 115, 131, 123, 456);
    verify_searchx(456, 115, 131, 115, 456-30);
    verify_searchx(456, 115, 131, 130, 456-30);
    verify_searchx(456, 115, 131, 130, 456+29);
    verify_searchx(456, 115, 131, 115, 456+29);

    verify_search(-456, -123, 60, -456, -123);
    verify_search(-456, -123, 60, -456-60, -123-60);
    verify_search(-456, -123, 60, -456+59, -123-60);
    verify_search(-456, -123, 60, -456+59, -123+59);
    verify_search(-456, -123, 60, -456-60, -123+59);

    expect(360, 4, (int[][2]){
        { 91, -80 }, { -63, 25 }, { -181, -334 }, { -181, -333 },
    });
    init_cfg(seed, default_pattern);
    n9_search_ceiling(&n9cfg, 0, 0);
    assert(iexpect == nexpects);

    printf("OK\n");
    return 0;
}

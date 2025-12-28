// CUDA Kernel main function
// Compute SecpK1 keys and calculate RIPEMD160(SHA256(key)) then check prefix
// For the kernel, we use a 16 bits prefix lookup table which correspond to ~3 Base58 characters
// A second level lookup table contains 32 bits prefix (if used)
// (The CPU computes the full address and check the full prefix)
// 
// We use affine coordinates for elliptic curve point (ie Z=1)

__device__ __noinline__ void CheckPoint(uint32_t *_h,
                                        int32_t incr,
                                        int32_t endo,
                                        int32_t mode,
                                        const prefix_t *__restrict__ prefix,
                                        const uint32_t *__restrict__ lookup32,
                                        uint32_t maxFound,
                                        uint32_t *__restrict__ out,
                                        int type) {

  uint32_t   off;
  prefixl_t  l32;
  prefix_t   pr0;
  prefix_t   hit;
  uint32_t   pos;
  uint32_t   st;
  uint32_t   ed;
  uint32_t   mi;
  uint32_t   lmi;
  uint32_t   tid = (blockIdx.x*blockDim.x) + threadIdx.x;
  char       add[48];
  
  if (prefix == NULL) {

    // No lookup compute address and return
    const char *pattern = (const char *)lookup32;
    _GetAddress(type, _h, add);
    if (_Match(add, pattern)) {
      // found
      goto addItem;
    }
 
  } else {
    
    // Lookup table
    pr0 = *(prefix_t *)(_h);
    hit = prefix[pr0];

    if (hit) {

      if (lookup32) {
        off = lookup32[pr0];
        l32 = _h[0];
        st = off;
        ed = off + hit - 1;
        while (st <= ed) {
          mi = (st + ed) / 2;
          lmi = lookup32[mi];
          if (l32 < lmi) {
            ed = mi - 1;
          } else if (l32 == lmi) {
            // found
            goto addItem;
          } else {
            st = mi + 1;
          }
        }
        return;
      }

    addItem:

      pos = atomicAdd(out, 1);
      if (pos < maxFound) {
        out[pos*ITEM_SIZE32 + 1] = tid;
        out[pos*ITEM_SIZE32 + 2] = (uint32_t)(incr << 16) | (uint32_t)(mode << 15) | (uint32_t)(endo);
        out[pos*ITEM_SIZE32 + 3] = _h[0];
        out[pos*ITEM_SIZE32 + 4] = _h[1];
        out[pos*ITEM_SIZE32 + 5] = _h[2];
        out[pos*ITEM_SIZE32 + 6] = _h[3];
        out[pos*ITEM_SIZE32 + 7] = _h[4];
      }

    }

  }

}

// -----------------------------------------------------------------------------------------

#define CHECK_POINT(_h,incr,endo,mode)  CheckPoint(_h,incr,endo,mode,prefix,lookup32,maxFound,out,P2PKH)
#define CHECK_POINT_P2SH(_h,incr,endo,mode)  CheckPoint(_h,incr,endo,mode,prefix,lookup32,maxFound,out,P2SH)

__device__ __noinline__ void CheckHashComp(const prefix_t *__restrict__ prefix,
                                           uint64_t *px,
                                           uint8_t isOdd,
                                           int32_t incr,
                                           const uint32_t *__restrict__ lookup32,
                                           uint32_t maxFound,
                                           uint32_t *__restrict__ out) {

  uint32_t   h[5];
  //uint64_t   pe1x[4];
  //uint64_t   pe2x[4];

  _GetHash160Comp(px, isOdd, (uint8_t *)h);
  CHECK_POINT(h, incr, 0, true);
  /*
  _ModMult(pe1x, px, _beta);
  _GetHash160Comp(pe1x, isOdd, (uint8_t *)h);
  CHECK_POINT(h, incr, 1, true);
  _ModMult(pe2x, px, _beta2);
  _GetHash160Comp(pe2x, isOdd, (uint8_t *)h);
  CHECK_POINT(h, incr, 2, true);

  _GetHash160Comp(px, !isOdd, (uint8_t *)h);
  CHECK_POINT(h, -incr, 0, true);
  _GetHash160Comp(pe1x, !isOdd, (uint8_t *)h);
  CHECK_POINT(h, -incr, 1, true);
  _GetHash160Comp(pe2x, !isOdd, (uint8_t *)h);
  CHECK_POINT(h, -incr, 2, true);
  */

}

__device__ __noinline__ void CheckHashP2SHComp(const prefix_t *__restrict__ prefix,
                                               uint64_t *px,
                                               uint8_t isOdd,
                                               int32_t incr,
                                               const uint32_t *__restrict__ lookup32,
                                               uint32_t maxFound,
                                               uint32_t *__restrict__ out) {

  uint32_t   h[5];
  //uint64_t   pe1x[4];
  //uint64_t   pe2x[4];

  _GetHash160P2SHComp(px, isOdd, (uint8_t *)h);
  CHECK_POINT_P2SH(h, incr, 0, true);
  /*
  _ModMult(pe1x, px, _beta);
  _GetHash160P2SHComp(pe1x, isOdd, (uint8_t *)h);
  CHECK_POINT_P2SH(h, incr, 1, true);
  _ModMult(pe2x, px, _beta2);
  _GetHash160P2SHComp(pe2x, isOdd, (uint8_t *)h);
  CHECK_POINT_P2SH(h, incr, 2, true);

  _GetHash160P2SHComp(px, !isOdd, (uint8_t *)h);
  CHECK_POINT_P2SH(h, -incr, 0, true);
  _GetHash160P2SHComp(pe1x, !isOdd, (uint8_t *)h);
  CHECK_POINT_P2SH(h, -incr, 1, true);
  _GetHash160P2SHComp(pe2x, !isOdd, (uint8_t *)h);
  CHECK_POINT_P2SH(h, -incr, 2, true);
  */

}

// -----------------------------------------------------------------------------------------

__device__ __noinline__ void CheckHashUncomp(const prefix_t *__restrict__ prefix,
                                             uint64_t *px,
                                             uint64_t *py,
                                             int32_t incr,
                                             const uint32_t *__restrict__ lookup32,
                                             uint32_t maxFound,
                                             uint32_t *__restrict__ out) {

  uint32_t   h[5];
  //uint64_t   pe1x[4];
  //uint64_t   pe2x[4];
  //uint64_t   pyn[4];

  _GetHash160(px, py, (uint8_t *)h);
  CHECK_POINT(h, incr, 0, false);
  /*
  _ModMult(pe1x, px, _beta);
  _GetHash160(pe1x, py, (uint8_t *)h);
  CHECK_POINT(h, incr, 1, false);
  _ModMult(pe2x, px, _beta2);
  _GetHash160(pe2x, py, (uint8_t *)h);
  CHECK_POINT(h, incr, 2, false);

  ModNeg256(pyn,py);

  _GetHash160(px, pyn, (uint8_t *)h);
  CHECK_POINT(h, -incr, 0, false);
  _GetHash160(pe1x, pyn, (uint8_t *)h);
  CHECK_POINT(h, -incr, 1, false);
  _GetHash160(pe2x, pyn, (uint8_t *)h);
  CHECK_POINT(h, -incr, 2, false);
  */
}

__device__ __noinline__ void CheckHashP2SHUncomp(const prefix_t *__restrict__ prefix,
                                                 uint64_t *px,
                                                 uint64_t *py,
                                                 int32_t incr,
                                                 const uint32_t *__restrict__ lookup32,
                                                 uint32_t maxFound,
                                                 uint32_t *__restrict__ out) {

  uint32_t   h[5];
  //uint64_t   pe1x[4];
  //uint64_t   pe2x[4];
  //uint64_t   pyn[4];

  _GetHash160P2SHUncomp(px, py, (uint8_t *)h);
  CHECK_POINT_P2SH(h, incr, 0, false);
  /*
  _ModMult(pe1x, px, _beta);
  _GetHash160P2SHUncomp(pe1x, py, (uint8_t *)h);
  CHECK_POINT_P2SH(h, incr, 1, false);
  _ModMult(pe2x, px, _beta2);
  _GetHash160P2SHUncomp(pe2x, py, (uint8_t *)h);
  CHECK_POINT_P2SH(h, incr, 2, false);

  ModNeg256(pyn, py);

  _GetHash160P2SHUncomp(px, pyn, (uint8_t *)h);
  CHECK_POINT_P2SH(h, -incr, 0, false);
  _GetHash160P2SHUncomp(pe1x, pyn, (uint8_t *)h);
  CHECK_POINT_P2SH(h, -incr, 1, false);
  _GetHash160P2SHUncomp(pe2x, pyn, (uint8_t *)h);
  CHECK_POINT_P2SH(h, -incr, 2, false);
  */
}

// -----------------------------------------------------------------------------------------

__device__ __noinline__ void CheckHash(uint32_t mode,
                                       const prefix_t *__restrict__ prefix,
                                       uint64_t *px,
                                       uint64_t *py,
                                       int32_t incr,
                                       const uint32_t *__restrict__ lookup32,
                                       uint32_t maxFound,
                                       uint32_t *__restrict__ out) {

  switch (mode) {
  case SEARCH_COMPRESSED:
    CheckHashComp(prefix, px, (uint8_t)(py[0] & 1), incr, lookup32, maxFound, out);
    break;
  case SEARCH_UNCOMPRESSED:
    CheckHashUncomp(prefix, px, py, incr, lookup32, maxFound, out);
    break;
  case SEARCH_BOTH:
    CheckHashComp(prefix, px, (uint8_t)(py[0] & 1), incr, lookup32, maxFound, out);
    CheckHashUncomp(prefix, px, py, incr, lookup32, maxFound, out);
    break;
  }

}

__device__ __noinline__ void CheckP2SHHash(uint32_t mode,
                                           const prefix_t *__restrict__ prefix,
                                           uint64_t *px,
                                           uint64_t *py,
                                           int32_t incr,
                                           const uint32_t *__restrict__ lookup32,
                                           uint32_t maxFound,
                                           uint32_t *__restrict__ out) {

  switch (mode) {
  case SEARCH_COMPRESSED:
    CheckHashP2SHComp(prefix, px, (uint8_t)(py[0] & 1), incr, lookup32, maxFound, out);
    break;
  case SEARCH_UNCOMPRESSED:
    CheckHashP2SHUncomp(prefix, px, py, incr, lookup32, maxFound, out);
    break;
  case SEARCH_BOTH:
    CheckHashP2SHComp(prefix, px, (uint8_t)(py[0] & 1), incr, lookup32, maxFound, out);
    CheckHashP2SHUncomp(prefix, px, py, incr, lookup32, maxFound, out);
    break;
  }

}

#define CHECK_PREFIX(incr) CheckHash(mode, sPrefix, px, py, j*GRP_SIZE + (incr), lookup32, maxFound, out)

// -----------------------------------------------------------------------------------------

#ifndef INV_TILE
// Tiling for batched inversions (reduces local memory pressure massively).
// 32 gives a good balance between extra muls and stack usage.
#define INV_TILE 32
#endif

__device__ void ComputeKeys(uint32_t mode,
                            uint64_t *startx,
                            uint64_t *starty,
                            const prefix_t *__restrict__ sPrefix,
                            const uint32_t *__restrict__ lookup32,
                            uint32_t maxFound,
                            uint32_t *__restrict__ out) {

  // Local working variables
  uint64_t px[4];
  uint64_t py[4];
  uint64_t pyn[4];
  uint64_t sx[4];
  uint64_t sy[4];
  uint64_t sxNext[4];
  uint64_t syNext[4];
  uint64_t dy[4];
  uint64_t _s[4];
  uint64_t _p2[4];
  char pattern[48];

  // Load starting key (no need for __syncthreads here)
  Load256A(sx, startx);
  Load256A(sy, starty);
  Load256(px, sx);
  Load256(py, sy);

  if (sPrefix == NULL) {
    memcpy(pattern, lookup32, 48);
    lookup32 = (const uint32_t *)pattern;
  }

  // Batched inversion over delta-x values without allocating huge per-thread arrays.
  // N = GRP_SIZE/2 + 1 (G..(GRP_SIZE/2)G plus _2Gn)
  constexpr uint32_t N = (GRP_SIZE / 2 + 1);
  constexpr uint32_t TILE = (uint32_t)INV_TILE;
  constexpr uint32_t NTILE = (N + TILE - 1) / TILE;

  uint64_t tileProd[NTILE][4];
  uint64_t tileCum[NTILE][4];

  for (uint32_t j = 0; j < STEP_SIZE / GRP_SIZE; j++) {

    // Center of the group
    CHECK_PREFIX(GRP_SIZE / 2);

    // Precompute -StartPoint.y
    Load256(py, sy);
    ModNeg256(pyn, py);

    // --- Build products per tile and cumulative products ---
    for (uint32_t t = 0; t < NTILE; t++) {

      uint32_t base = t * TILE;
      uint32_t end = base + TILE;
      if (end > N) end = N;

      uint64_t prod[4] = { 1ULL,0ULL,0ULL,0ULL };

      for (uint32_t idx = base; idx < end; idx++) {
        uint64_t dxv[4];
        if (idx < (GRP_SIZE / 2)) {
          ModSub256(dxv, Gx[idx], sx);
        } else {
          ModSub256(dxv, _2Gnx, sx);
        }
        _ModMult(prod, dxv);
      }

      Load256(tileProd[t], prod);

      if (t == 0) {
        Load256(tileCum[t], tileProd[t]);
      } else {
        _ModMult(tileCum[t], tileCum[t - 1], tileProd[t]);
      }

    }

    // invAcc = inverse(totalProduct)
    uint64_t invAcc5[5];
    uint64_t invAcc[4];
    Load256(invAcc5, tileCum[NTILE - 1]);
    invAcc5[4] = 0;
    _ModInv(invAcc5);
    invAcc[0] = invAcc5[0];
    invAcc[1] = invAcc5[1];
    invAcc[2] = invAcc5[2];
    invAcc[3] = invAcc5[3];

    // We'll compute the next start point when we see idx == GRP_SIZE/2.
    // (This is dx index N-1.)
    bool haveNext = false;

    // --- Process tiles in reverse order; compute element inverses inside each tile ---
    for (int32_t t = (int32_t)NTILE - 1; t >= 0; t--) {

      uint32_t base = (uint32_t)t * TILE;
      uint32_t end = base + TILE;
      if (end > N) end = N;
      uint32_t len = end - base;

      // invTileProd = 1 / (product of dx in this tile)
      uint64_t invTileProd[4];
      if (t == 0) {
        Load256(invTileProd, invAcc);
      } else {
        _ModMult(invTileProd, tileCum[t - 1], invAcc);
      }

      // invAcc = invAcc * tileProd[t] (prepare for next tile)
      _ModMult(invAcc, tileProd[t]);

      // Build dx[] and prefix products for this tile
      uint64_t dxv[INV_TILE][4];
      uint64_t pref[INV_TILE][4];
      for (uint32_t k = 0; k < len; k++) {
        uint32_t idx = base + k;
        if (idx < (GRP_SIZE / 2)) {
          ModSub256(dxv[k], Gx[idx], sx);
        } else {
          ModSub256(dxv[k], _2Gnx, sx);
        }
        if (k == 0) {
          Load256(pref[k], dxv[k]);
        } else {
          _ModMult(pref[k], pref[k - 1], dxv[k]);
        }
      }

      // Compute element inverses in reverse within the tile
      uint64_t invElemAcc[4];
      Load256(invElemAcc, invTileProd);

      for (int32_t kk = (int32_t)len - 1; kk >= 0; kk--) {

        uint32_t idx = base + (uint32_t)kk;
        uint64_t invDx[4];

        if (kk == 0) {
          Load256(invDx, invElemAcc);
        } else {
          _ModMult(invDx, pref[kk - 1], invElemAcc);
        }

        // invElemAcc *= dxv[kk]
        _ModMult(invElemAcc, dxv[kk]);

        // Process this index
        if (idx == (GRP_SIZE / 2)) {

          // Next start point (startP + GRP_SIZE*G)
          // Uses point addition with P2 = _2Gn
          Load256(px, sx);
          Load256(py, sy);
          ModSub256(dy, _2Gny, py);

          _ModMult(_s, dy, invDx);
          _ModSqr(_p2, _s);

          ModSub256(px, _p2, px);
          ModSub256(px, _2Gnx);

          ModSub256(py, _2Gnx, px);
          _ModMult(py, _s);
          ModSub256(py, _2Gny);

          Load256(sxNext, px);
          Load256(syNext, py);
          haveNext = true;

        } else if (idx == HSIZE) {

          // First point (startP - (GRP_SIZE/2)*G)
          Load256(px, sx);
          Load256(py, sy);
          ModNeg256(dy, Gy[idx]);
          ModSub256(dy, py);

          _ModMult(_s, dy, invDx);
          _ModSqr(_p2, _s);

          ModSub256(px, _p2, px);
          ModSub256(px, Gx[idx]);

          ModSub256(py, Gx[idx], px);
          _ModMult(py, _s);
          ModAdd256(py, Gy[idx]);

          CHECK_PREFIX(0);

        } else {

          // idx in [0,HSIZE-1] => +/- (idx+1) around the center
          uint32_t i = idx;

          // P = StartPoint + i*G
          Load256(px, sx);
          Load256(py, sy);
          ModSub256(dy, Gy[i], py);

          _ModMult(_s, dy, invDx);
          _ModSqr(_p2, _s);

          ModSub256(px, _p2, px);
          ModSub256(px, Gx[i]);

          ModSub256(py, Gx[i], px);
          _ModMult(py, _s);
          ModSub256(py, Gy[i]);

          CHECK_PREFIX(GRP_SIZE / 2 + (i + 1));

          // P = StartPoint - i*G
          Load256(px, sx);
          ModSub256(dy, pyn, Gy[i]);

          _ModMult(_s, dy, invDx);
          _ModSqr(_p2, _s);

          ModSub256(px, _p2, px);
          ModSub256(px, Gx[i]);

          ModSub256(py, Gx[i], px);
          _ModMult(py, _s);
          ModAdd256(py, Gy[i]);

          CHECK_PREFIX(GRP_SIZE / 2 - (i + 1));

        }

      }

    }

    // Advance to next group center
    if (haveNext) {
      Load256(sx, sxNext);
      Load256(sy, syNext);
    }

  }

  // Update starting point (no need for __syncthreads here)
  Store256A(startx, sx);
  Store256A(starty, sy);

}

// -----------------------------------------------------------------------------------------

#define CHECK_PREFIX_P2SH(incr) CheckP2SHHash(mode, sPrefix, px, py, j*GRP_SIZE + (incr), lookup32, maxFound, out)

__device__ void ComputeKeysP2SH(uint32_t mode,
                                uint64_t *startx,
                                uint64_t *starty,
                                const prefix_t *__restrict__ sPrefix,
                                const uint32_t *__restrict__ lookup32,
                                uint32_t maxFound,
                                uint32_t *__restrict__ out) {

  // Local working variables
  uint64_t px[4];
  uint64_t py[4];
  uint64_t pyn[4];
  uint64_t sx[4];
  uint64_t sy[4];
  uint64_t sxNext[4];
  uint64_t syNext[4];
  uint64_t dy[4];
  uint64_t _s[4];
  uint64_t _p2[4];
  char pattern[48];

  // Load starting key
  Load256A(sx, startx);
  Load256A(sy, starty);
  Load256(px, sx);
  Load256(py, sy);

  if (sPrefix == NULL) {
    memcpy(pattern, lookup32, 48);
    lookup32 = (const uint32_t *)pattern;
  }

  constexpr uint32_t N = (GRP_SIZE / 2 + 1);
  constexpr uint32_t TILE = (uint32_t)INV_TILE;
  constexpr uint32_t NTILE = (N + TILE - 1) / TILE;

  uint64_t tileProd[NTILE][4];
  uint64_t tileCum[NTILE][4];

  for (uint32_t j = 0; j < STEP_SIZE / GRP_SIZE; j++) {

    // Center
    CHECK_PREFIX_P2SH(GRP_SIZE / 2);

    Load256(py, sy);
    ModNeg256(pyn, py);

    // Tile products and cumulative products
    for (uint32_t t = 0; t < NTILE; t++) {

      uint32_t base = t * TILE;
      uint32_t end = base + TILE;
      if (end > N) end = N;

      uint64_t prod[4] = { 1ULL,0ULL,0ULL,0ULL };

      for (uint32_t idx = base; idx < end; idx++) {
        uint64_t dxv[4];
        if (idx < (GRP_SIZE / 2)) {
          ModSub256(dxv, Gx[idx], sx);
        } else {
          ModSub256(dxv, _2Gnx, sx);
        }
        _ModMult(prod, dxv);
      }

      Load256(tileProd[t], prod);

      if (t == 0) {
        Load256(tileCum[t], tileProd[t]);
      } else {
        _ModMult(tileCum[t], tileCum[t - 1], tileProd[t]);
      }
    }

    uint64_t invAcc5[5];
    uint64_t invAcc[4];
    Load256(invAcc5, tileCum[NTILE - 1]);
    invAcc5[4] = 0;
    _ModInv(invAcc5);
    invAcc[0] = invAcc5[0];
    invAcc[1] = invAcc5[1];
    invAcc[2] = invAcc5[2];
    invAcc[3] = invAcc5[3];

    bool haveNext = false;

    for (int32_t t = (int32_t)NTILE - 1; t >= 0; t--) {

      uint32_t base = (uint32_t)t * TILE;
      uint32_t end = base + TILE;
      if (end > N) end = N;
      uint32_t len = end - base;

      uint64_t invTileProd[4];
      if (t == 0) {
        Load256(invTileProd, invAcc);
      } else {
        _ModMult(invTileProd, tileCum[t - 1], invAcc);
      }

      _ModMult(invAcc, tileProd[t]);

      uint64_t dxv[INV_TILE][4];
      uint64_t pref[INV_TILE][4];
      for (uint32_t k = 0; k < len; k++) {
        uint32_t idx = base + k;
        if (idx < (GRP_SIZE / 2)) {
          ModSub256(dxv[k], Gx[idx], sx);
        } else {
          ModSub256(dxv[k], _2Gnx, sx);
        }
        if (k == 0) {
          Load256(pref[k], dxv[k]);
        } else {
          _ModMult(pref[k], pref[k - 1], dxv[k]);
        }
      }

      uint64_t invElemAcc[4];
      Load256(invElemAcc, invTileProd);

      for (int32_t kk = (int32_t)len - 1; kk >= 0; kk--) {

        uint32_t idx = base + (uint32_t)kk;
        uint64_t invDx[4];

        if (kk == 0) {
          Load256(invDx, invElemAcc);
        } else {
          _ModMult(invDx, pref[kk - 1], invElemAcc);
        }

        _ModMult(invElemAcc, dxv[kk]);

        if (idx == (GRP_SIZE / 2)) {

          // Next start point
          Load256(px, sx);
          Load256(py, sy);
          ModSub256(dy, _2Gny, py);

          _ModMult(_s, dy, invDx);
          _ModSqr(_p2, _s);

          ModSub256(px, _p2, px);
          ModSub256(px, _2Gnx);

          ModSub256(py, _2Gnx, px);
          _ModMult(py, _s);
          ModSub256(py, _2Gny);

          Load256(sxNext, px);
          Load256(syNext, py);
          haveNext = true;

        } else if (idx == HSIZE) {

          // First point
          Load256(px, sx);
          Load256(py, sy);
          ModNeg256(dy, Gy[idx]);
          ModSub256(dy, py);

          _ModMult(_s, dy, invDx);
          _ModSqr(_p2, _s);

          ModSub256(px, _p2, px);
          ModSub256(px, Gx[idx]);

          ModSub256(py, Gx[idx], px);
          _ModMult(py, _s);
          ModAdd256(py, Gy[idx]);

          CHECK_PREFIX_P2SH(0);

        } else {

          uint32_t i = idx;

          // +
          Load256(px, sx);
          Load256(py, sy);
          ModSub256(dy, Gy[i], py);

          _ModMult(_s, dy, invDx);
          _ModSqr(_p2, _s);

          ModSub256(px, _p2, px);
          ModSub256(px, Gx[i]);

          ModSub256(py, Gx[i], px);
          _ModMult(py, _s);
          ModSub256(py, Gy[i]);

          CHECK_PREFIX_P2SH(GRP_SIZE / 2 + (i + 1));

          // -
          Load256(px, sx);
          ModSub256(dy, pyn, Gy[i]);

          _ModMult(_s, dy, invDx);
          _ModSqr(_p2, _s);

          ModSub256(px, _p2, px);
          ModSub256(px, Gx[i]);

          ModSub256(py, Gx[i], px);
          _ModMult(py, _s);
          ModAdd256(py, Gy[i]);

          CHECK_PREFIX_P2SH(GRP_SIZE / 2 - (i + 1));

        }

      }

    }

    if (haveNext) {
      Load256(sx, sxNext);
      Load256(sy, syNext);
    }

  }

  Store256A(startx, sx);
  Store256A(starty, sy);

}

// -----------------------------------------------------------------------------------------
// Optimized kernel for compressed P2PKH address only

__device__ void ComputeKeysComp(uint64_t *startx,
                                uint64_t *starty,
                                const prefix_t *__restrict__ sPrefix,
                                const uint32_t *__restrict__ lookup32,
                                uint32_t maxFound,
                                uint32_t *__restrict__ out) {

  // Keep legacy entry point while using the optimized ComputeKeys() implementation.
  // (This preserves correctness for compressed P2PKH without maintaining a separate path.)
  ComputeKeys(SEARCH_COMPRESSED, startx, starty, sPrefix, lookup32, maxFound, out);

}

#pragma once
#if defined(_MSC_VER) && (_MSC_VER < 1600)

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

#define FORCE_INLINE __forceinline

#include <stdlib.h>

#define ROTL64(x, y) _rotl64(x, y)

#define BIG_CONSTANT(x) (x)

// Other compilers、

#else // defined(_MSC_VER)

#include <stdint.h>

#define FORCE_INLINE inline __attribute__((always_inline))

inline uint64_t rotl64(uint64_t x, int8_t r) {
  return (x << r) | (x >> (64 - r));
}

#define ROTL64(x, y) rotl64(x, y)

#define BIG_CONSTANT(x) (x##LLU)

#endif // !defined(_MSC_VER)

FORCE_INLINE uint64_t getblock64(const uint64_t *p, int i) { return p[i]; }

FORCE_INLINE uint64_t fmix64(uint64_t k) {
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;

  return k;
}

/**
* Murmur hash function
* @param key hash target.
* @param len byte number of key.
* @param seed use 1.
* @param out 128bit, use as 4 unsigned int.
* Example
  long long key = 103122;
  unsigned int hash[4] = {0};
  MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
*/
void MurmurHash3_x64_128(const void *key, const int len, const uint32_t seed,
                         void *out);

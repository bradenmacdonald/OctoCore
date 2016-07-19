/**
 * OctoCore field name hashing
 *
 * Uses the FNV-1a hash to compute a 32-bit integer hash from a string at compile time.
 *
 * Part of OctoCore by Braden MacDonald
 */
#pragma once
#include <cstdint>
#include <cstddef>

constexpr uint32_t operator"" _octo_field_name_hash(const char* chars, size_t len) {
    const uint32_t FNV_offset = 2166136261; // The offset basis, i.e. the initial value of the hash
    const int64_t FNV_prime_32 = 16777619;
    // Above: 16777619 is the FNV prime for 32-bit hashes.
    // We have to multiply in 64-bit though, to avoid an int32 overflow
    // that causes the compiler to treat the result as non-constexpr

    uint32_t hash = FNV_offset;
    while (len-- > 0)  {
        hash = hash ^ (*chars++);
        hash = (uint32_t)(hash * FNV_prime_32);
    }
    return hash;
}

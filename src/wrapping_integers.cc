#include "wrapping_integers.hh"

#include <algorithm>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint32_t const low_bits = raw_value_ - zero_point.raw_value_;
  uint64_t const high_bits = checkpoint >> 32;

  uint64_t const r0 = high_bits << 32 | low_bits;
  uint64_t const distance = std::min( r0 - checkpoint, checkpoint - r0 );

  if ( high_bits != UINT32_MAX && ( r0 + ( 1ULL << 32 ) ) - checkpoint < distance ) {
    return r0 + ( 1ULL << 32 );
  }
  if ( high_bits != 0 && checkpoint - ( r0 - ( 1ULL << 32 ) ) < distance ) {
    return r0 - ( 1ULL << 32 );
  }
  return r0;
}

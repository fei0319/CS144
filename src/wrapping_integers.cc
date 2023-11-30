#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint32_t const low_bits = raw_value_ - zero_point.raw_value_;
  uint64_t const high_bits = checkpoint >> 32;

  auto distance = []( uint64_t a, uint64_t b ) { return a > b ? a - b : b - a; };

  uint64_t const r = high_bits << 32 | low_bits;
  uint64_t const d = distance( checkpoint, r );

  if ( distance( checkpoint, r + ( 1ULL << 32 ) ) < d ) {
    return r + ( 1ULL << 32 );
  }
  if ( distance( checkpoint, r - ( 1ULL << 32 ) ) < d ) {
    return r - ( 1ULL << 32 );
  }
  return r;
}

#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + n;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint32_t raw = raw_value_ - zero_point.raw_value_;
  uint64_t sma = ( ( checkpoint - raw ) >> 32 << 32 ) | raw;
  uint64_t lar = sma + ( 1ull << 32 );
  if ( lar < checkpoint )
    return sma;
  if ( sma > checkpoint )
    return lar;
  if ( lar - checkpoint >= checkpoint - sma )
    return sma;
  else
    return lar;
}

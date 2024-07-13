#include "wrapping_integers.hh"
#include <iostream>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  n = ( n + zero_point.raw_value_ ) & UINT32_MAX;
  return Wrap32 { static_cast<uint32_t>( n ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  constexpr uint64_t UINT32_LEN { 1UL << 32 };
  uint64_t offset = 0;
  if ( raw_value_ < zero_point.raw_value_ ) {
    offset = UINT32_LEN - static_cast<uint64_t>( zero_point.raw_value_ ) + static_cast<uint64_t>( raw_value_ );
  } else {
    offset = static_cast<uint64_t>( raw_value_ ) - static_cast<uint64_t>( zero_point.raw_value_ );
  }

  uint64_t nwrap = checkpoint >> 32;
  // cerr << offset << " " << nwrap << " " << offset + nwrap * UINT32_LEN << " " << checkpoint << endl;
  offset = offset + nwrap * UINT32_LEN;
  if ( offset < checkpoint && ( offset + UINT32_LEN ) - checkpoint < checkpoint - offset ) {
    return offset + UINT32_LEN;
  } else if ( offset > checkpoint && offset > UINT32_LEN
              && checkpoint - ( offset - UINT32_LEN ) < offset - checkpoint ) {
    return offset - UINT32_LEN;
  }
  return offset;
}

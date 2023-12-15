#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  if ( is_last_substring ) {
    last_substring_inserted = true;
  }

  if ( first_index < front ) {
    const uint64_t to_pop = front - first_index;
    if ( to_pop >= data.size() ) {
      return;
    }
    data = data.substr( to_pop );
    first_index = 0;
  } else {
    first_index -= front;
    first_index = std::min( first_index, output.available_capacity() );
  }
  // Trim bytes that exceed capacity
  data.resize( std::min( data.size(), output.available_capacity() - first_index ) );
  buffer.resize( std::max( buffer.size(), first_index + data.size() ) );

  for ( uint64_t i = 0; i < data.size(); ++i ) {
    auto& [validity, byte] = buffer[i + first_index];
    byte = data[i];
    bytes_valid -= validity;
    validity = true;
    bytes_valid += validity;
  }

  data.clear();
  while ( !buffer.empty() && buffer.front().first ) {
    front += 1;
    bytes_valid -= 1;

    data.push_back( buffer.front().second );
    buffer.pop_front();
  }
  if ( !data.empty() ) {
    output.push( data );
  }

  if ( last_substring_inserted && buffer.empty() ) {
    output.close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_valid;
}

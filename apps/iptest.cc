#include "ipv4_datagram.hh"
#include "tcp_segment.hh"
#include "tun.hh"

#include <cstdlib>
#include <iostream>

using namespace std;

void program_body( const string& dev )
{
  TunFD tun { dev };

  TCPSegment seg;
  seg.sender_message = TCPSenderMessage { Wrap32 { 12345 }, true, "Hello"s, true };
  seg.receiver_message = TCPReceiverMessage { nullopt, 9987 };
  seg.udinfo = { 54321, 8080, 0 };
  seg.reset = false;

  InternetDatagram dgram;
  dgram.header.src = ( 127U << 24 ) | 1U;
  dgram.header.dst = ( 198U << 24 ) | ( 41U << 16 ) | 4U;
  dgram.header.len = 45;
  dgram.header.compute_checksum();

  Serializer s1;
  seg.serialize( s1 );
  seg.compute_checksum( dgram.header.pseudo_checksum() );
  dgram.payload = s1.output();

  Serializer s;
  dgram.serialize( s );
  tun.write( s.output() );
}

int main( int argc __attribute( ( unused ) ), char* argv[] __attribute( ( unused ) ) )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    if ( argc != 2 ) {
      cerr << "Usage: " << args.front() << " tun_device\n";
      return EXIT_FAILURE;
    }

    program_body( args[1] );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

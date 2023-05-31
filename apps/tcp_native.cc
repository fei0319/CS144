#include "bidirectional_stream_copy.hh"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>

using namespace std;

void show_usage( const char* argv0 )
{
  cerr << "Usage: " << argv0 << " [-l] <host> <port>\n\n"
       << "  -l specifies listen mode; <host>:<port> is the listening address." << endl;
}

int main( int argc, char** argv )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    bool server_mode = false;
    // NOLINTNEXTLINE(bugprone-assignment-*)
    if ( argc < 3 || ( ( server_mode = ( strncmp( "-l", args[1], 3 ) == 0 ) ) && argc < 4 ) ) {
      show_usage( args[0] );
      return EXIT_FAILURE;
    }

    // in client mode, connect; in server mode, accept exactly one connection
    auto socket = [&] {
      if ( server_mode ) {
        TCPSocket listening_socket;                    // create a TCP socket
        listening_socket.set_reuseaddr();              // reuse the server's address as soon as the program quits
        listening_socket.bind( { args[2], args[3] } ); // bind to specified address
        listening_socket.listen();                     // mark the socket as listening for incoming connections
        return listening_socket.accept();              // accept exactly one connection
      }
      TCPSocket connecting_socket;
      connecting_socket.connect( { args[1], args[2] } );
      return connecting_socket;
    }();

    bidirectional_stream_copy( socket );
  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  std::shared_ptr<Node> node = root;
  for ( size_t i = 0; i < prefix_length; ++i ) {
    unsigned int bit = static_cast<bool>( route_prefix & ( 1U << ( 31 - i ) ) );
    if ( node->next.at( bit ) == nullptr ) {
      node->next.at( bit ) = std::make_shared<Node>();
    }
    node = node->next.at( bit );
  }

  node->value = { next_hop, interface_num };
}

std::optional<std::pair<std::optional<Address>, size_t>> Router::match( uint32_t raw_address ) const
{
  std::optional<std::pair<std::optional<Address>, size_t>> result {};

  std::shared_ptr<Node> node = root;
  for ( size_t i = 0; i < 32; ++i ) {
    if ( node->value.has_value() ) {
      result = node->value;
    }

    unsigned int bit = static_cast<bool>( raw_address & ( 1U << ( 31 - i ) ) );
    if ( node->next.at( bit ) == nullptr ) {
      break;
    }
    node = node->next.at( bit );
  }

  if ( node->value.has_value() ) {
    result = node->value;
  }
  return result;
}

void Router::send_datagram( InternetDatagram&& dgram )
{
  InternetDatagram datagram = std::move( dgram );
  if ( datagram.header.ttl <= 1U ) {
    return;
  }
  datagram.header.ttl -= 1;

  auto matching = match( dgram.header.dst );
  if ( matching.has_value() ) {
    const auto& [next_hop, interface_num] = matching.value();
    Address address = next_hop.has_value() ? next_hop.value() : Address::from_ipv4_numeric( datagram.header.dst );

    interface( interface_num ).send_datagram( datagram, address );
  }
}

void Router::route() {}

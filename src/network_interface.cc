#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  auto ethernet_address = look_for_mapping( next_hop );
  if ( ethernet_address.has_value() ) {
    buffer_for_sending( dgram, ethernet_address.value() );
  } else {
    buffer_datagrams[next_hop].push( dgram );
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return {};
  }
  InternetDatagram dgram;
  switch ( frame.header.type ) {
    case EthernetHeader::TYPE_IPv4:
      if ( parse( dgram, frame.payload ) ) {
        return dgram;
      }
      break;
    case EthernetHeader::TYPE_ARP:
      handle_ARP_payload( frame.payload );
      break;
    default:
      cerr << "Unknown ethernet frame type\n";
  }
  return {};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  timestamp += ms_since_last_tick;
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if ( buffer_frames.empty() ) {
    return {};
  }
  auto frame = buffer_frames.front();
  buffer_frames.pop();
  return frame;
}
std::optional<EthernetAddress> NetworkInterface::look_for_mapping( const Address& address )
{
  if ( mappings.contains( address ) ) {
    auto& [ethernet_address, added_time] = mappings[address];
    if ( timestamp - added_time <= EXPIRE_TIME_IN_MS ) {
      return ethernet_address;
    }
    mappings.erase( address );
  }
  send_ARP_request_for( address );
  return {};
}

void NetworkInterface::buffer_for_sending( const EthernetFrame& frame )
{
  buffer_frames.push( frame );
}

void NetworkInterface::buffer_for_sending( const InternetDatagram& dgram, const EthernetAddress& ethernet_address )
{
  EthernetFrame frame;
  frame.header.dst = ethernet_address;
  frame.header.src = ethernet_address_;
  frame.header.type = EthernetHeader::TYPE_IPv4;
  frame.payload = serialize( dgram );

  buffer_for_sending( frame );
}

void NetworkInterface::send_ARP_request_for( const Address& address )
{
  if ( in_flight_ARP.contains( address ) && timestamp - in_flight_ARP[address] <= ARP_REQUEST_INTERVAL ) {
    return;
  }
  in_flight_ARP[address] = timestamp;

  ARPMessage message;
  message.opcode = ARPMessage::OPCODE_REQUEST;
  message.sender_ethernet_address = ethernet_address_;
  message.sender_ip_address = ip_address_.ipv4_numeric();
  message.target_ip_address = address.ipv4_numeric();

  EthernetFrame frame;
  frame.header.dst = ETHERNET_BROADCAST;
  frame.header.src = ethernet_address_;
  frame.header.type = EthernetHeader::TYPE_ARP;
  frame.payload = serialize( message );

  buffer_for_sending( frame );
}

void NetworkInterface::send_ARP_reply_to( const EthernetAddress& ethernet_address, const Address& address )
{
  ARPMessage message;
  message.opcode = ARPMessage::OPCODE_REPLY;
  message.sender_ethernet_address = ethernet_address_;
  message.sender_ip_address = ip_address_.ipv4_numeric();
  message.target_ethernet_address = ethernet_address;
  message.target_ip_address = address.ipv4_numeric();

  EthernetFrame frame;
  frame.header.dst = ethernet_address;
  frame.header.src = ethernet_address_;
  frame.header.type = EthernetHeader::TYPE_ARP;
  frame.payload = serialize( message );

  buffer_for_sending( frame );
}

void NetworkInterface::add_mapping( const Address& address, const EthernetAddress& ethernet_address )
{
  mappings[address] = std::make_pair( ethernet_address, timestamp );
  if ( buffer_datagrams.contains( address ) ) {
    auto& q = buffer_datagrams[address];
    while ( !q.empty() ) {
      buffer_for_sending( q.front(), ethernet_address );
      q.pop();
    }
    buffer_datagrams.erase( address );
  }
}

void NetworkInterface::handle_ARP_payload( const vector<Buffer>& payload )
{
  ARPMessage message;
  parse( message, payload );

  add_mapping( Address::from_ipv4_numeric( message.sender_ip_address ), message.sender_ethernet_address );

  if ( message.opcode == ARPMessage::OPCODE_REQUEST && message.target_ip_address == ip_address_.ipv4_numeric() ) {
    send_ARP_reply_to( message.sender_ethernet_address, Address::from_ipv4_numeric( message.sender_ip_address ) );
  }
}

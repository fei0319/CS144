#include "tuntap_adapter.hh"
#include "parser.hh"

using namespace std;

optional<TCPSegment> TCPOverIPv4OverTunFdAdapter::read()
{
  vector<string> strs( 2 );
  strs.front().resize( IPv4Header::LENGTH );
  _tun.read( strs );

  InternetDatagram ip_dgram;
  const vector<Buffer> buffers = { strs.at( 0 ), strs.at( 1 ) };
  if ( parse( ip_dgram, buffers ) ) {
    return unwrap_tcp_in_ip( ip_dgram );
  }
  return {};
}

//! \param[in] tap Raw network device that will be owned by the adapter
//! \param[in] eth_address Ethernet address (local address) of the adapter
//! \param[in] ip_address IP address (local address) of the adapter
//! \param[in] next_hop IP address of the next hop (typically a router or default gateway)
TCPOverIPv4OverEthernetAdapter::TCPOverIPv4OverEthernetAdapter(
  TapFD&& tap,
  const EthernetAddress& eth_address,
  const Address& ip_address, // NOLINT(*-easily-swappable-*)
  const Address& next_hop )
  : _tap( move( tap ) ), _interface( eth_address, ip_address ), _next_hop( next_hop )
{
  // Linux seems to ignore the first frame sent on a TAP device, so send a dummy frame to prime the pump :-(
  const EthernetFrame dummy_frame;
  _tap.write( serialize( dummy_frame ) );
}

optional<TCPSegment> TCPOverIPv4OverEthernetAdapter::read()
{
  // Read Ethernet frame from the raw device
  vector<string> strs( 3 );
  strs.at( 0 ).resize( EthernetHeader::LENGTH );
  strs.at( 1 ).resize( IPv4Header::LENGTH );
  _tap.read( strs );

  EthernetFrame frame;
  vector<Buffer> buffers;
  ranges::transform( strs, back_inserter( buffers ), identity() );
  if ( not parse( frame, buffers ) ) {
    return {};
  }

  // Give the frame to the NetworkInterface. Get back an Internet datagram if frame was carrying one.
  optional<InternetDatagram> ip_dgram = _interface.recv_frame( frame );

  // The incoming frame may have caused the NetworkInterface to send a frame.
  send_pending();

  // Try to interpret IPv4 datagram as TCP
  if ( ip_dgram ) {
    return unwrap_tcp_in_ip( ip_dgram.value() );
  }
  return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPOverIPv4OverEthernetAdapter::tick( const size_t ms_since_last_tick )
{
  _interface.tick( ms_since_last_tick );
  send_pending();
}

//! \param[in] seg the TCPSegment to send
void TCPOverIPv4OverEthernetAdapter::write( TCPSegment& seg )
{
  _interface.send_datagram( wrap_tcp_in_ip( seg ), _next_hop );
  send_pending();
}

void TCPOverIPv4OverEthernetAdapter::send_pending()
{
  while ( auto frame = _interface.maybe_send() ) {
    _tap.write( serialize( frame.value() ) );
  }
}

//! Specialize LossyFdAdapter to TCPOverIPv4OverTunFdAdapter
template class LossyFdAdapter<TCPOverIPv4OverTunFdAdapter>;

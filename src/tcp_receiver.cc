#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  auto& [seqno, SYN, payload, FIN] = message;
  if ( SYN ) {
    synced = true;
    initial_seqno = seqno;
  }
  uint64_t const absolute_index = seqno.unwrap( initial_seqno, inbound_stream.bytes_pushed() ) - !SYN;
  reassembler.insert( absolute_index, payload, FIN, inbound_stream );
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  TCPReceiverMessage message;
  if ( synced ) {
    message.ackno
      = Wrap32::wrap( inbound_stream.bytes_pushed(), initial_seqno ) + synced + inbound_stream.is_closed();
  }
  if ( inbound_stream.available_capacity() < UINT16_MAX ) {
    message.window_size = inbound_stream.available_capacity();
  } else {
    message.window_size = UINT16_MAX;
  }
  return message;
}

#pragma once

#include "tcp_config.hh"
#include "tcp_receiver.hh"
#include "tcp_receiver_message.hh"
#include "tcp_segment.hh"
#include "tcp_sender.hh"
#include "tcp_sender_message.hh"

#include <optional>

class TCPPeer
{
  TCPConfig cfg_;
  TCPSender sender_ { cfg_.rt_timeout, cfg_.fixed_isn };
  TCPReceiver receiver_ {};
  Reassembler reassembler_ {};

  ByteStream outbound_stream_ { cfg_.send_capacity }, inbound_stream_ { cfg_.recv_capacity };

  bool need_send_ {};

public:
  explicit TCPPeer( const TCPConfig& cfg ) : cfg_( cfg ) {}

  Writer& outbound_writer() { return outbound_stream_.writer(); }
  Reader& inbound_reader() { return inbound_stream_.reader(); }

  void push() { sender_.push( outbound_stream_.reader() ); };
  void tick( uint64_t ms_since_last_tick ) { sender_.tick( ms_since_last_tick ); }

  bool has_ackno() const { return receiver_.send( inbound_stream_.writer() ).ackno.has_value(); }

  bool active() const
  {
    if ( inbound_stream_.reader().has_error() ) {
      return false;
    }
    return ( not outbound_stream_.reader().is_finished() ) or ( not inbound_stream_.writer().is_closed() )
           or sender_.sequence_numbers_in_flight();
  }

  void receive( TCPSegment seg )
  {
    if ( seg.reset or inbound_reader().has_error() ) {
      inbound_stream_.writer().set_error();
      return;
    }

    // Give incoming TCPReceiverMessage to sender.
    sender_.receive( seg.receiver_message );

    // Give incoming TCPSenderMessage to receiver.
    // If SenderMessage is non-empty or a keep-alive, make sure to reply.
    need_send_ |= ( seg.sender_message.sequence_length() > 0 );
    const auto our_ackno = receiver_.send( inbound_stream_.writer() ).ackno;
    need_send_ |= ( our_ackno.has_value() and seg.sender_message.seqno + 1 == our_ackno.value() );

    receiver_.receive( std::move( seg.sender_message ), reassembler_, inbound_stream_.writer() );
  }

  std::optional<TCPSegment> maybe_send()
  {
    // Get outgoing TCPReceiverMessage from receiver.
    auto receiver_msg = receiver_.send( inbound_stream_.writer() );

    // If connection is alive, push stream to TCPSender.
    if ( receiver_msg.ackno.has_value() ) {
      push();
    }

    // Get (possible) outgoing TCPSenderMessage, using empty message if we need to send something.
    auto sender_msg = sender_.maybe_send();

    if ( need_send_ and not sender_msg.has_value() ) {
      sender_msg = sender_.send_empty_message();
    }

    need_send_ = false;

    // Send the segment
    if ( sender_msg.has_value() ) {
      return TCPSegment {
        sender_msg.value(), receiver_msg, outbound_stream_.reader().has_error() or inbound_reader().has_error() };
    }

    return {};
  }

  // Testing interface
  const TCPReceiver& receiver() const { return receiver_; }
  const TCPSender& sender() const { return sender_; }
  const Reassembler& reassembler() const { return reassembler_; }
};

#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

Timer::Timer( uint64_t init_RTO )
  : timer( init_RTO ), initial_RTO( init_RTO ), RTO( init_RTO ), running( false )
{}

void Timer::elapse( uint64_t time_elapsed )
{
  if ( running ) {
    timer -= std::min( timer, time_elapsed );
  }
}

void Timer::double_RTO()
{
  if ( RTO & ( 1ULL << 63 ) ) {
    RTO = UINT64_MAX;
  } else {
    RTO <<= 1;
  }
}

void Timer::reset()
{
  timer = RTO;
}

void Timer::start()
{
  running = true;
}

void Timer::stop()
{
  running = false;
}

bool Timer::expired() const
{
  return timer == 0;
}

void Timer::restore_RTO()
{
  RTO = initial_RTO;
}

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) )
  , initial_RTO_ms_( initial_RTO_ms )
  , timer( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return pushed_no - ack_no;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmissions;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  if ( !messages_to_be_sent.empty() ) {
    timer.start();

    auto msg = messages_to_be_sent.front();
    messages_to_be_sent.pop();
    return *msg;
  }
  return std::nullopt;
}

void TCPSender::push( Reader& outbound_stream )
{
  if (pushed_no == 0) {
    std::shared_ptr<TCPSenderMessage> message = std::make_shared<TCPSenderMessage>(send_empty_message());
    pushed_no += message->sequence_length();
    messages_to_be_sent.push( message );
    outstanding_messages.push( message );
  }

  uint64_t allowed_no = received_ack_no + std::max( window_size, uint16_t { 1 } ) - 1;
  while ( outbound_stream.bytes_buffered() > 0 && pushed_no < allowed_no ) {
    uint64_t msg_len = std::min( allowed_no - pushed_no, outbound_stream.bytes_buffered() );

    Buffer buffer { std::string { outbound_stream.peek().substr( 0, msg_len ) } };
    outbound_stream.pop( msg_len );

    std::shared_ptr<TCPSenderMessage> message = std::make_shared<TCPSenderMessage>();
    message->seqno = Wrap32::wrap( pushed_no, isn_ );
    message->SYN = ( pushed_no == 0 );
    message->payload = buffer;
    message->FIN = outbound_stream.is_finished();

    pushed_no += message->sequence_length();
    messages_to_be_sent.push( message );
    outstanding_messages.push( message );
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  TCPSenderMessage message;
  message.seqno = Wrap32::wrap( pushed_no, isn_ );
  message.SYN = ( pushed_no == 0 );
  message.payload = {};
  message.FIN = false;

  return message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.ackno.has_value() ) {
    uint64_t received_ackno = msg.ackno.value().unwrap( isn_, ack_no );
    while ( !outstanding_messages.empty()
            && received_ackno >= ack_no + outstanding_messages.front()->sequence_length() ) {
      ack_no += outstanding_messages.front()->sequence_length();
      outstanding_messages.pop();

      timer.restore_RTO();
      retransmissions = 0;
      if ( !outstanding_messages.empty() ) {
        timer.start();
      }
    }
    received_ack_no = std::max( received_ack_no, received_ackno );

    if ( ack_no > pushed_no ) {
      timer.stop();
    }
  }
  window_size = msg.window_size;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  timer.elapse( ms_since_last_tick );
  if ( timer.expired() ) {
    messages_to_be_sent.push( outstanding_messages.front() );

    if ( window_size ) {
      timer.double_RTO();
      retransmissions += 1;
    }

    timer.reset();
    timer.start();
  }
}

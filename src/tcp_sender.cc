#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

Timer::Timer( uint64_t initial_RTO )
  : timer( initial_RTO ), initial_RTO( initial_RTO ), RTO( initial_RTO ), running( false )
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
  return outstanding_messages.size();
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmissions;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  if ( !messages_to_be_sent.empty() ) {
    auto msg = messages_to_be_sent.front();
    messages_to_be_sent.pop();
    outstanding_messages.push( msg );
    return *msg;
  }
  return std::nullopt;
}

void TCPSender::push( Reader& outbound_stream )
{
  while ( !outbound_stream.is_finished() && pushed_no < ack_no + window_size - 1 ) {
    uint64_t msg_len = std::min( ack_no + window_size - 1 - pushed_no, outbound_stream.bytes_buffered() );

    Buffer buffer { std::string { outbound_stream.peek().substr( 0, msg_len ) } };
    outbound_stream.pop( msg_len );

    std::shared_ptr<TCPSenderMessage> message = std::make_shared<TCPSenderMessage>();
    message->seqno = Wrap32::wrap( pushed_no, isn_ );
    message->SYN = ( pushed_no == 0 );
    message->payload = buffer;
    message->FIN = outbound_stream.is_finished();

    pushed_no += message->sequence_length();
    messages_to_be_sent.push( message );
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
  if (msg.ackno.has_value()) {
    ack_no = std::max(ack_no, msg.ackno.value().unwrap(isn_, ack_no));
  }
  window_size = msg.window_size;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  (void)ms_since_last_tick;
}

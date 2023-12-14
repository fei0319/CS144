#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return {};
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return {};
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  return {};
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
  // Your code here.
  return {};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  (void)msg;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  (void)ms_since_last_tick;
}

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

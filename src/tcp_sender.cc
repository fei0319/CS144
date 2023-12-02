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
  // Your code here.
  (void)outbound_stream;
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

Timer::Timer( uint64_t& initial_RTO ) : timer( initial_RTO ), RTO( initial_RTO ), running( false ) {}

void Timer::elapse( uint64_t time_elapsed )
{
  if ( running ) {
    if ( time_elapsed >= timer ) {
      timer = 0;
    } else {
      timer -= time_elapsed;
    }
  }
}

void Timer::double_RTO() const
{
  RTO *= 2;
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

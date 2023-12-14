#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class Timer
{
  uint64_t timer;
  uint64_t initial_RTO;
  uint64_t RTO;
  bool running;

public:
  explicit Timer( uint64_t initial_RTO );
  void elapse( uint64_t time_elapsed );
  void double_RTO();
  void reset();
  void start();
  void stop();
  bool expired() const;
  void restore_RTO();
};

class TCPSender
{
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  uint16_t window_size { 1 };
  uint64_t ack_no { 1 };
  uint64_t pushed_no { 0 };
  uint64_t retransmissions { 0 };
  std::queue<std::shared_ptr<TCPSenderMessage>> messages_to_be_sent;
  std::queue<std::shared_ptr<TCPSenderMessage>> outstanding_messages;

  Timer timer;

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};

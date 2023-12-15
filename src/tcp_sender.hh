#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

/*
 * The Timer class used for determining when to resend an outstanding message.
 */
class Timer
{
  uint64_t timer;
  uint64_t initial_RTO;
  uint64_t RTO;
  bool running;

  /* It turns out that `reset` and `start` are always used together, so I
   * made them private and created `restart` as their replacement */
  void reset();
  void start();

public:
  explicit Timer( uint64_t init_RTO );
  void elapse( uint64_t time_elapsed );
  void double_RTO();
  void stop();
  bool expired() const;

  /* A timer is considered stopped if it was manually stopped or expired */
  bool is_stopped() const;
  void restore_RTO();
  void restart();
};

class TCPSender
{
  Wrap32 isn_;

  uint16_t window_size { 1 };

  /* The greatest valid ackno ever received */
  uint64_t received_ack_no { 0 };

  /*
   * The actual ackno the sender use. It differs from `received_ack_no` because we
   * only consider bytes in a completely acknowledged segment acknowledged. Those
   * bytes within `received_ack_no` but not in an acknowledged segment are not acked
   * and may be retransmitted if needed.
   */
  uint64_t ack_no { 0 };

  /* Number of bytes (including SYN and FIN) pushed to be sent */
  uint64_t pushed_no { 0 };

  /* Number of consecutive retransmissions */
  uint64_t retransmissions { 0 };

  /*
   * The purpose of using `shared_ptr` rather than directly storing `TCPSenderMessage`
   * instances is to avoid wasteful copying.
   * This can also be achieved by overloading move assignment operator for
   * `TCPSenderMessage`, but that can be even more cumbersome.
   * It is notable that the payload inside the `TCPSenderMessage` is already implemented
   * with `shared_ptr`, so even if `shared_ptr` is not used here the payload data will not
   * be copied and stored multiple times.
   */

  /* A buffer storing messages to be sent */
  std::queue<std::shared_ptr<TCPSenderMessage>> messages_to_be_sent {};
  std::queue<std::shared_ptr<TCPSenderMessage>> outstanding_messages {};

  Timer timer;

  /* Whether a FIN has already been sent */
  bool FIN_sent { false };

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

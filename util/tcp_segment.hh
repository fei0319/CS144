#pragma once

#include "parser.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include "udinfo.hh"

struct TCPSegment
{
  TCPSenderMessage sender_message {};
  TCPReceiverMessage receiver_message {};
  bool reset {}; // Connection experienced an abnormal error and should be shut down
  UserDatagramInfo udinfo {};

  void parse( Parser& parser, uint32_t datagram_layer_pseudo_checksum );
  void serialize( Serializer& serializer ) const;

  void compute_checksum( uint32_t datagram_layer_pseudo_checksum );
};

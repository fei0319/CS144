#pragma once

#include "address.hh"
#include "ethernet_frame.hh"
#include "ipv4_datagram.hh"

#include <iostream>
#include <list>
#include <optional>
#include <queue>
#include <unordered_map>
#include <utility>

template <>
struct std::hash<Address> {
  size_t operator()(const Address &b) const {
    return std::hash<std::string>()(b.to_string());
  }
};

// A "network interface" that connects IP (the internet layer, or network layer)
// with Ethernet (the network access layer, or link layer).

// This module is the lowest layer of a TCP/IP stack
// (connecting IP with the lower-layer network protocol,
// e.g. Ethernet). But the same module is also used repeatedly
// as part of a router: a router generally has many network
// interfaces, and the router's job is to route Internet datagrams
// between the different interfaces.

// The network interface translates datagrams (coming from the
// "customer," e.g. a TCP/IP stack or router) into Ethernet
// frames. To fill in the Ethernet destination address, it looks up
// the Ethernet address of the next IP hop of each datagram, making
// requests with the [Address Resolution Protocol](\ref rfc::rfc826).
// In the opposite direction, the network interface accepts Ethernet
// frames, checks if they are intended for it, and if so, processes
// the the payload depending on its type. If it's an IPv4 datagram,
// the network interface passes it up the stack. If it's an ARP
// request or reply, the network interface processes the frame
// and learns or replies as necessary.
class NetworkInterface
{
private:
  // Time that a mapping is kept in cache
  static const size_t EXPIRE_TIME_IN_MS = 30000;
  static const size_t ARP_REQUEST_INTERVAL = 5000;

  // Timestamp in ms of the `NetworkInterface`.
  // It is incremented when `tick` is called and used for determining when a mapping was set up.
  size_t timestamp {0};

  // Mappings from IP addresses to ethernet addresses and their timestamps when they were established
  std::unordered_map<Address, std::pair<EthernetAddress, size_t>> mappings {};

  // A buffer storing unsent datagrams, which will be sent immediately when an ethernet address for an
  // IP address of a certain datagram is known.
  std::unordered_map<Address, std::queue<InternetDatagram>> buffer_datagrams {};

  // A buffer storing unsent ethernet frames, which come from `buffer_datagrams` when their ethernet
  // addresses are determined. The frames will be sent when `maybe_send` is called.
  std::queue<EthernetFrame> buffer_frames {};

  // All ARP requests in flight and the timestamps when they are sent
  std::unordered_map<Address, size_t> in_flight_ARP {};

  // Ethernet (known as hardware, network-access, or link-layer) address of the interface
  EthernetAddress ethernet_address_;

  // IP (known as Internet-layer or network-layer) address of the interface
  Address ip_address_;

  // Buffer an ethernet frame for sending
  void buffer_for_sending(const EthernetFrame &);
  // Buffer an internet datagram for sending
  void buffer_for_sending(const InternetDatagram &, const EthernetAddress &);

  // Look up an existing mapping for a certain address.
  // If it does not exist or has expired, and no previous ARP request sent within `ARP_REQUEST_INTERVAL`ms
  // is for this address, an ARP request will be sent.
  std::optional<EthernetAddress> look_for_mapping(const Address& address );

  // Send an ARP request for an address
  void send_ARP_request_for( const Address&);

public:
  // Construct a network interface with given Ethernet (network-access-layer) and IP (internet-layer)
  // addresses
  NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address );

  // Access queue of Ethernet frames awaiting transmission
  std::optional<EthernetFrame> maybe_send();

  // Sends an IPv4 datagram, encapsulated in an Ethernet frame (if it knows the Ethernet destination
  // address). Will need to use [ARP](\ref rfc::rfc826) to look up the Ethernet destination address
  // for the next hop.
  // ("Sending" is accomplished by making sure maybe_send() will release the frame when next called,
  // but please consider the frame sent as soon as it is generated.)
  void send_datagram( const InternetDatagram& dgram, const Address& next_hop );

  // Receives an Ethernet frame and responds appropriately.
  // If type is IPv4, returns the datagram.
  // If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
  // If type is ARP reply, learn a mapping from the "sender" fields.
  std::optional<InternetDatagram> recv_frame( const EthernetFrame& frame );

  // Called periodically when time elapses
  void tick( size_t ms_since_last_tick );
};

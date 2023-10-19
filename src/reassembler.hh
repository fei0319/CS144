#pragma once

#include "byte_stream.hh"

#include <algorithm>
#include <string>
#include <cassert>
#include <iostream>

class Reassembler
{
private:
  // The index of the first byte stored in the Reassembler.
  uint64_t front{0};
  // Number of set bytes in data.
  uint64_t bytes_valid{0};
  // Whether the last substring has been inserted.
  bool last_substring_inserted {false};
  // A pair represents validity and data of a byte.
  std::deque<std::pair<bool, char>> buffer{};
public:
  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring, Writer& output );

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;
};

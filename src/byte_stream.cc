#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity ), pushed_( 0 ), popped_( 0 ), has_error_( false ), is_closed_( false ), data_()
{}

uint64_t ByteStream::StringQueue::size() const
{
  return data_.size() - popped_;
}

void ByteStream::StringQueue::push( char c )
{
  data_.push_back( c );
}

uint64_t ByteStream::StringQueue::pop( uint64_t len )
{
  const uint64_t bytes_to_pop = std::min( len, data_.size() - popped_ );
  popped_ += bytes_to_pop;

  if ( popped_ >= data_.size() / 2 ) {
    data_ = data_.substr( popped_ );
    popped_ = 0;
  }

  return bytes_to_pop;
}

std::string_view ByteStream::StringQueue::peek() const
{
  return std::string_view { data_ }.substr( popped_ );
}

void Writer::push( string data )
{
  for ( auto c : data ) {
    if ( data_.size() >= capacity_ ) {
      break;
    }
    data_.push( c );
    ++pushed_;
  }
}

void Writer::close()
{
  is_closed_ = true;
}

void Writer::set_error()
{
  has_error_ = true;
}

bool Writer::is_closed() const
{
  return is_closed_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - data_.size();
}

uint64_t Writer::bytes_pushed() const
{
  return pushed_;
}

string_view Reader::peek() const
{
  return data_.peek();
}

bool Reader::is_finished() const
{
  return is_closed_ && data_.size() == 0;
}

bool Reader::has_error() const
{
  return has_error_;
}

void Reader::pop( uint64_t len )
{
  popped_ += data_.pop( len );
}

uint64_t Reader::bytes_buffered() const
{
  return data_.size();
}

uint64_t Reader::bytes_popped() const
{
  return popped_;
}

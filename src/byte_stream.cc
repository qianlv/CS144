#include "byte_stream.hh"

#include <iostream>

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity ), buffer_ { std::string( capacity_, '\0' ), std::string( capacity_, '\0' ) }
{
  // std::cerr << "capacity_ " << capacity_ << std::endl;
}

bool Writer::is_closed() const
{
  return eof_;
}

void Writer::push( string data )
{
  if ( is_closed() ) {
    return;
  }

  uint64_t size = std::min( data.size(), available_capacity() );

  total_pushed_bytes += size;

  uint64_t first_size = 0;
  if ( end_ < capacity_ ) {
    first_size = std::min( capacity_ - end_, size );
  }

  if ( first_size > 0 ) {
    std::copy( data.data(), data.data() + first_size, buffer_[0].data() + end_ );
    end_ += first_size;
  }

  if ( size > first_size ) {
    std::copy( data.data() + first_size, data.data() + size, buffer_[1].data() + ( end_ - capacity_ ) );
    end_ += size - first_size;
  }
}

void Writer::close()
{
  eof_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - ( end_ - start_ );
}

uint64_t Writer::bytes_pushed() const
{
  return total_pushed_bytes;
}

bool Reader::is_finished() const
{
  return eof_ && bytes_buffered() == 0;
}

uint64_t Reader::bytes_popped() const
{
  return total_poped_bytes;
}

string_view Reader::peek() const
{
  uint64_t size = bytes_buffered();
  if ( size == 0 ) {
    return "";
  }
  return std::string_view( buffer_[0].begin() + start_,
                           buffer_[0].begin() + start_ + std::min( size, capacity_ - start_ ) );
}

void Reader::pop( uint64_t len )
{
  len = std::min( len, bytes_buffered() );
  start_ += len;
  if ( start_ >= capacity_ ) {
    end_ -= capacity_;
    start_ -= capacity_;
    std::swap( buffer_[0], buffer_[1] );
  }
  total_poped_bytes += len;
}

uint64_t Reader::bytes_buffered() const
{
  return end_ - start_;
}

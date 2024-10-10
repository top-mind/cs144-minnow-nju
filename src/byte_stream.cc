#include "byte_stream.hh"
// #include <iostream>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return close_;
}

void Writer::push( string data )
{
  if ( is_closed() )
    return;
  size_t push = min( data.size(), available_capacity() );
  if ( push == 0 )
    return;

  datas_.push( push == data.size() ? std::move( data ) : ( data.substr( 0, push ) ) );

  if ( size_ == 0 )
    peek_view_ = datas_.back();

  size_ += push;
  push_ += push;

  // cerr << "push " << datas_.back() << ", size = " << size_ << ", push = " << push_ << "\n";
}

void Writer::close()
{
  close_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - size_;
}

uint64_t Writer::bytes_pushed() const
{
  return push_;
}

bool Reader::is_finished() const
{
  return close_ && bytes_buffered() == 0;
}

uint64_t Reader::bytes_popped() const
{
  return push_ - size_;
}

string_view Reader::peek() const
{
  // cerr << "peek " << peek_view_ << ", size = " << size_ << ", push = " << push_ << "\n";
  return peek_view_;
}

void Reader::pop( uint64_t len )
{
  if ( len > size_ )
    len = size_;
  size_ -= len;
  while ( len ) {
    auto vsize = peek_view_.size();
    if ( vsize <= len ) {
      len -= vsize;
      datas_.pop();
      peek_view_ = datas_.front();
    } else {
      peek_view_ = peek_view_.substr( len, vsize );
      break;
    }
  }
  if ( size_ == 0 )
    peek_view_ = {};
}

uint64_t Reader::bytes_buffered() const
{
  return size_;
}

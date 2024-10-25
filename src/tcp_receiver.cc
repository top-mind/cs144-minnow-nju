#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reader().set_error();
    return;
  }

  uint64_t index = 0;

  if ( message.SYN ) {
    isn_ = message.seqno + 1; // index 0 starts at seqno ISN + 1
  } else {
    if ( !isn_.has_value() )
      return;
    uint64_t checkpoint = writer().bytes_pushed();
    index = message.seqno.unwrap( isn_.value(), checkpoint );
  }

  reassembler_.insert( index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  optional<Wrap32> ackno = {};
  if ( isn_.has_value() )
    ackno = Wrap32::wrap( writer().bytes_pushed(), isn_.value() ) + writer().is_closed();

  uint64_t available_capacity = writer().available_capacity();

  uint16_t window_size = available_capacity > UINT16_MAX ? UINT16_MAX : (uint16_t)available_capacity;

  return { ackno, window_size, writer().has_error() };
}

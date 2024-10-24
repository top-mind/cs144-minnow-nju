#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if (message.RST) {
    // XXX who's error?
    reassembler_.reader().set_error();
    return;
  }

  uint64_t index = 0;

  if (message.SYN) {
    isn_ = message.seqno + 1; // index 0 starts at seqno ISN + 1
  } else {
    if (!isn_.has_value())
      return;
    uint64_t checkpoint = reassembler_.writer().bytes_pushed();
    index = message.seqno.unwrap(isn_.value(), checkpoint);
  }

  reassembler_.insert(index, message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  optional<Wrap32> op = {};
  if (isn_.has_value()) {
    op = Wrap32::wrap(reassembler_.writer().bytes_pushed(), isn_.value()) + reassembler_.writer().is_closed();
  }
  uint16_t window_size = UINT16_MAX;
  uint64_t available_capacity = reassembler_.writer().available_capacity();
  if (window_size > available_capacity) window_size = available_capacity;

  TCPReceiverMessage ret;
  ret.ackno = op;
  ret.window_size = window_size;
  ret.RST = reassembler_.writer().has_error();

  return ret;
}

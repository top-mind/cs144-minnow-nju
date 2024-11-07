#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return nr_seq_in_flight;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return nr_consecutive_retrans_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if (input_.has_error()) {
    transmit(make_empty_message());
    return;
  }

  if (nr_seq_in_flight == 0)
    time_retrans_ = time_ + initial_RTO_ms_;

  uint32_t nr_trans = window_size_ > 0 ? window_size_ : 1;

  while ( nr_seq_in_flight < nr_trans ) {
    TCPSenderMessage message = make_message(nr_trans - nr_seq_in_flight);
    if (message.sequence_length() == 0)
      break;

    outstanding_.push(message);
    nr_seq_in_flight += message.sequence_length();
    transmit(message);
    isn_ = isn_ + message.sequence_length();
  }

}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return {.seqno = isn_, .RST = input_.has_error()};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if (msg.RST) {
    input_.set_error();
    return;
  }

  if (msg.ackno.has_value()) {
    Wrap32 ack = msg.ackno.value();
    bool acked = false;
    if (!(ack <= isn_)) {
      // the test covers this case, just do dirty work
      // XXX why
      return;
    }
    while (!outstanding_.empty() && outstanding_.front().seqno + outstanding_.front().sequence_length() <= ack) {
      nr_seq_in_flight -= outstanding_.front().sequence_length();
      outstanding_.pop();
      acked = true;
    }
    if (acked) {
      nr_consecutive_retrans_ = 0;
      time_retrans_ = time_ + initial_RTO_ms_;
    }
  }

  window_size_ = msg.window_size;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if (input_.has_error())
    return;

  time_ += ms_since_last_tick;
  if (time_ >= time_retrans_ && !outstanding_.empty()) {
    transmit(outstanding_.front());

    if (window_size_ > 0) {
      nr_consecutive_retrans_ ++;
    }

    time_retrans_ = time_ + (initial_RTO_ms_ << nr_consecutive_retrans_);
  }
}

TCPSenderMessage TCPSender::make_message(size_t size) {
  // size > 0
  bool SYN = false;
  bool FIN = false;
  string payload;

  if (!SYN_) {
    SYN_ = SYN = true;
  }

  size_t payload_size = std::min(size - SYN, TCPConfig::MAX_PAYLOAD_SIZE);

  if (FIN_) goto empty_message;

  while (payload.size() < payload_size) {
    string_view view = input_.reader().peek();
    if (view.size() == 0) {
      break;
    }
    view = view.substr(0, payload_size - payload.size());
    payload.append(view);
    input_.reader().pop(view.size());
  }

  FIN_ = FIN = input_.reader().is_finished() && input_.reader().peek().size() == 0 && SYN + payload.size() < size;

empty_message:

  return {.seqno = isn_, .SYN = SYN, .payload = move(payload), .FIN = FIN};
}

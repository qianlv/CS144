#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <iostream>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return nextsequnum_ - ackno_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return total_consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // std::cerr << "\nwindow_size_ = " << window_size_
  //           << ", sequence_numbers_in_flight() = " << sequence_numbers_in_flight() << std::endl;

  uint64_t wsize = ( window_size_ == 0 ) ? 1 : window_size_;
  while ( wsize > sequence_numbers_in_flight() ) {
    if ( nextsequnum_ > writer().bytes_pushed() + 1 ) { // this case, with fin msg has sent, so return
      return;
    }

    TCPSenderMessage msg = make_empty_message();

    if ( nextsequnum_ == 0 ) {
      msg.SYN = true;
    }

    uint64_t len = writer().bytes_pushed() + 1 - nextsequnum_; // payload length
    if ( msg.SYN ) {                                           // make syn occupy one window space
      len -= 1;
    }

    len = std::min( std::min( wsize - sequence_numbers_in_flight(), TCPConfig::MAX_PAYLOAD_SIZE ),
                    len ); // min(really window size, MAX_PAYLOAD_SIZE, payload len)
    read( input_.reader(), len, msg.payload );

    if ( len < (wsize - sequence_numbers_in_flight()) && reader().is_finished() ) { // make fin occupy one window space
      msg.FIN = true;
    }

    nextsequnum_ += msg.sequence_length();
    if ( msg.sequence_length() == 0 ) {
      return;
    }

    transmit( msg );
    outstanding_msg_.emplace_back( std::move( msg ) );
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return {
    Wrap32::wrap( nextsequnum_, isn_ ),
    false,
    "",
    false,
    input_.has_error(),
  };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.RST ) {
    input_.set_error();
    return;
  }

  uint64_t new_ackno = msg.ackno->unwrap( isn_, ackno_ );
  if ( new_ackno > nextsequnum_ ) {
    return;
  }

  while ( !outstanding_msg_.empty() ) {
    const TCPSenderMessage& sender_msg = outstanding_msg_.front();
    if ( new_ackno > ackno_ ) {
      RTO_ms_ = initial_RTO_ms_;
      total_consecutive_retransmissions_ = 0;
      timer_ = 0;
    }
    // std::cerr << "new_ackno " << new_ackno << std::endl;
    // std::cerr << "ackno_ " << ackno_ << std::endl;
    // std::cerr << "sequence_length_ " << sender_msg.sequence_length() << std::endl;
    if ( ackno_ + sender_msg.sequence_length() <= new_ackno ) {
      ackno_ += sender_msg.sequence_length();
      outstanding_msg_.pop_front();
    } else {
      break;
    }
  }
  window_size_ = msg.window_size;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  timer_ += ms_since_last_tick;
  // std::cerr << "timer_ " << timer_ << " RTO_ms_ = " << RTO_ms_ << std::endl;
  if ( timer_ >= RTO_ms_ && !outstanding_msg_.empty() ) {
    // std::cerr << "tick window_size_ " << window_size_ << std::endl;
    transmit( outstanding_msg_.front() );
    if ( window_size_ > 0 ) {
      total_consecutive_retransmissions_ += 1;
      RTO_ms_ *= 2;
    }

    timer_ = 0;
  }
}

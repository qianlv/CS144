#include "tcp_receiver.hh"
#include "wrapping_integers.hh"
#include <iostream>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST || reader().has_error() ) {
    reader().set_error();
    return;
  }

  auto seqno = message.seqno;
  if ( message.SYN ) {
    zero_point.emplace( message.seqno );
    seqno = seqno + 1;
  }

  if ( !zero_point.has_value() ) {
    return;
  }

  auto first_index = seqno.unwrap( *zero_point, writer().bytes_pushed() + 1 ) - 1;
  reassembler_.insert( first_index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  uint16_t window_size = UINT16_MAX;
  if ( writer().available_capacity() < UINT16_MAX ) {
    window_size = writer().available_capacity();
  }

  if ( !zero_point.has_value() ) {
    return TCPReceiverMessage { std::nullopt, window_size, reader().has_error() };
  }

  TCPReceiverMessage ret;
  ret.ackno = Wrap32::wrap( writer().bytes_pushed() + 1 + ( writer().is_closed() ? 1 : 0 ), *zero_point );
  ret.window_size = window_size;
  ret.RST = reader().has_error();

  return ret;
}

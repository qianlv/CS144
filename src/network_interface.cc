#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

static ARPMessage make_arp( const uint16_t opcode,
                            const EthernetAddress sender_ethernet_address,
                            uint32_t sender_ip_address,
                            const EthernetAddress target_ethernet_address,
                            uint32_t target_ip_address )
{
  ARPMessage arp;
  arp.opcode = opcode;
  arp.sender_ethernet_address = sender_ethernet_address;
  arp.sender_ip_address = sender_ip_address;
  arp.target_ethernet_address = target_ethernet_address;
  arp.target_ip_address = target_ip_address;
  return arp;
}

static EthernetFrame make_frame( const EthernetAddress& src,
                                 const EthernetAddress& dst,
                                 const uint16_t type,
                                 vector<string> payload )
{
  EthernetFrame frame;
  frame.header.src = src;
  frame.header.dst = dst;
  frame.header.type = type;
  frame.payload = std::move( payload );
  return frame;
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  const auto& it = ip2eth_.find( next_hop.ipv4_numeric() );
  EthernetFrame frame;
  if ( it == ip2eth_.cend() ) {
    if ( arp_request_cache_.count( next_hop.ipv4_numeric() ) > 0 ) {
      return;
    }

    frame = make_frame( ethernet_address_,
                        ETHERNET_BROADCAST,
                        EthernetHeader::TYPE_ARP,
                        serialize( make_arp( ARPMessage::OPCODE_REQUEST,
                                             ethernet_address_,
                                             ip_address_.ipv4_numeric(),
                                             {},
                                             next_hop.ipv4_numeric() ) ) );
    arp_request_cache_[next_hop.ipv4_numeric()] = 0;
    waiting_queue_[next_hop.ipv4_numeric()].emplace( dgram );
  } else {
    frame = make_frame( ethernet_address_, it->second.eth, EthernetHeader::TYPE_IPv4, serialize( dgram ) );
  }
  transmit( frame );
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_ ) {
    return;
  }

  if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp;
    auto success = parse( arp, frame.payload );
    if ( not success ) {
      return;
    }

    if ( arp.opcode == ARPMessage::OPCODE_REQUEST ) {
      if ( arp.target_ip_address != ip_address_.ipv4_numeric() ) {
        return;
      }

      ip2eth_[arp.sender_ip_address] = {
        arp.sender_ethernet_address,
        0,
      };
      EthernetFrame reply = make_frame( ethernet_address_,
                                        frame.header.src,
                                        EthernetHeader::TYPE_ARP,
                                        serialize( make_arp( ARPMessage::OPCODE_REPLY,
                                                             ethernet_address_,
                                                             ip_address_.ipv4_numeric(),
                                                             arp.sender_ethernet_address,
                                                             arp.sender_ip_address ) ) );
      transmit( reply );
    }
    if ( arp.opcode == ARPMessage::OPCODE_REPLY ) {
      ip2eth_[arp.sender_ip_address] = {
        arp.sender_ethernet_address,
        0,
      };
    }

    auto it = waiting_queue_.find( arp.sender_ip_address );
    if ( it != waiting_queue_.end() ) {
      while ( !it->second.empty() ) {
        EthernetFrame wframe = make_frame( ethernet_address_,
                                           arp.sender_ethernet_address,
                                           EthernetHeader::TYPE_IPv4,
                                           serialize( it->second.front() ) );
        transmit( wframe );
        it->second.pop();
      }
      waiting_queue_.erase( it );
    }
  } else if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram data;
    auto success = parse( data, frame.payload );
    if ( not success ) {
      return;
    }
    datagrams_received_.emplace( std::move( data ) );
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  for ( auto it = std::begin( ip2eth_ ); it != std::end( ip2eth_ ); ) {
    it->second.timer += ms_since_last_tick;
    if ( it->second.timer >= NetworkInterface::EXPIRE_TIME ) {
      it = ip2eth_.erase( it );
    } else {
      ++it;
    }
  }

  for ( auto it = std::begin( arp_request_cache_ ); it != std::end( arp_request_cache_ ); ) {
    it->second += ms_since_last_tick;
    if ( it->second >= NetworkInterface::ARP_REQUEST_EXPIRE_TIME ) {
      it = arp_request_cache_.erase( it );
    } else {
      ++it;
    }
  }
}

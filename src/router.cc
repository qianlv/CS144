#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

uint32_t Router::mask_ip( uint32_t ip, uint32_t prefix_length )
{
  return ( prefix_length > 0 ? ip & ( ( std::numeric_limits<uint32_t>::max() ) << ( NUM_PREFFIX - prefix_length ) )
                             : 0 );
}

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.()
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  routers_[prefix_length][Router::mask_ip( route_prefix, prefix_length )] = { next_hop, interface_num };
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( auto& inter : _interfaces ) {
    auto& datagrams = inter->datagrams_received();
    while ( !datagrams.empty() ) {
      InternetDatagram data { std::move( datagrams.front() ) };
      datagrams.pop();
      if ( data.header.ttl > 1 ) {
        data.header.ttl -= 1;
        data.header.compute_checksum();
        uint32_t ip = data.header.dst;
        auto entry = match( ip );
        if ( entry ) {
          interface( entry->interface_num )
            ->send_datagram( data, entry->next_hop.value_or( Address::from_ipv4_numeric( ip ) ) );
        }
      }
    }
  }
}

const Router::RouterEntry* Router::match( uint32_t ip ) const
{
  for ( uint32_t loop_index = routers_.size(); loop_index > 0; loop_index-- ) {
    uint32_t i = loop_index - 1;
    auto it = routers_[i].find( Router::mask_ip( ip, i ) );
    if ( it != routers_[i].end() ) {
      return &it->second;
    }
  }
  return nullptr;
}

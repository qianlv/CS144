#pragma once

#include <memory>
#include <optional>
#include <unordered_set>

#include "exception.hh"
#include "network_interface.hh"

// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    _interfaces.push_back( notnull( "add_interface", std::move( interface ) ) );
    return _interfaces.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return _interfaces.at( N ); }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces
  void route();

private:
  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> _interfaces {};

  static constexpr size_t NUM_PREFFIX = 32;
  struct RouterEntry
  {
    std::optional<Address> next_hop {};
    size_t interface_num {};
  };

  // The router's collection of forwarding rule,
  // prefix_length -> {route_prefix -> (next_hop, interface_num)}
  std::array<std::unordered_map<uint32_t, RouterEntry>, NUM_PREFFIX + 1> routers_ {};

  // Find the longest-prefix-match route
  const RouterEntry* match( uint32_t ip ) const;

  // Get prefix ip addres that mask ip with specificd prefix length
  // 192.168.1.1/16 -> 192.168.0.0 only keep the 16 bit prefix ip.
  static uint32_t mask_ip( uint32_t ip, uint32_t prefix_length );
};

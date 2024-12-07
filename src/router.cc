#include "router.hh"

#include <iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  if (prefix_length > 32) {
    cerr << "prefix length out of range " << prefix_length << endl;
    return;
  }
  uint32_t mask = prefix_length ? UINT32_MAX << (32 - prefix_length) : 0;
  route_table_.push_back({route_prefix & mask, mask, next_hop, interface_num});
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for (auto i : _interfaces) {
    auto &queue = i->datagrams_received();
    while (!queue.empty()) {
      auto &dgram = queue.front();
      bool match = false;
      uint32_t mask = 0;
      uint32_t next_hop;
      size_t forward_interface;
      uint32_t dst = dgram.header.dst;
      if (dgram.header.ttl <= 1) goto drop;
      --dgram.header.ttl;
      dgram.header.compute_checksum();
      for (auto &route : route_table_) {
        if (route.mask_ >= mask && route.prefix_ == (route.mask_ & dst)) {
          match = true;
          mask = route.mask_;
          next_hop = route.next_hop.has_value() ? route.next_hop->ipv4_numeric() : dst;
          forward_interface = route.interface_num_;
        }
      }
      if (match) {
        cerr << "routing from " << i->name() << " to " << interface(forward_interface)->name() <<
          " next_hop=" << Address::from_ipv4_numeric(next_hop).to_string() << " type=" << dgram.header.to_string() << " payload=";
        for (auto &payload : dgram.payload) {
          cerr << payload << " ";
        }
        cerr << endl;
        interface(forward_interface)->send_datagram(dgram, Address::from_ipv4_numeric(next_hop));
      }
drop:
      queue.pop();
    }
  }
}

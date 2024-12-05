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

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  EthernetFrame frame {
    {{}, ethernet_address_, EthernetHeader::TYPE_IPv4},
    serialize(dgram),
  };
  // if next_hop is known
  auto arp_cache_it = arp_cache_.find(next_hop.ipv4_numeric());
  if (arp_cache_it != arp_cache_.end()) {
    EthernetAddress dstAddr = arp_cache_it->second;
    frame.header.dst = dstAddr;
    transmit(frame);
  } else {
    auto [it, succ] = arp_request_time_.try_emplace(next_hop.ipv4_numeric(), time_ms_);
    if (succ || it->second + 5000 < time_ms_) {
      transmit_arp(next_hop.ipv4_numeric());
      it->second = time_ms_;
    }
    // append dgram to queue
    datagrams_[next_hop.ipv4_numeric()].push_back(std::move(frame));
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if (frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_)
    return;
  switch (frame.header.type) {
    case EthernetHeader::TYPE_IPv4:
    {
      InternetDatagram datagram;
      if (parse(datagram, frame.payload)) {
        datagrams_received_.push(datagram);
      } else {
        std::cerr << "fail parsing ethernet frame " << frame.header.to_string() << std::endl;
      }
      break;
    }
    case EthernetHeader::TYPE_ARP:
    {
      ARPMessage arp;
      if (parse(arp, frame.payload)) {
        // rembember
        arp_cache_[arp.sender_ip_address] = arp.sender_ethernet_address;
        arp_mapping_time_.emplace(time_ms_, arp.sender_ip_address);

        auto it = datagrams_.find(arp.sender_ip_address);
        if (it != datagrams_.end()) {
          for (auto& dgram : it->second) {
            dgram.header.dst = arp.sender_ethernet_address;
            transmit(dgram);
          }
          datagrams_.erase(it);
        }

        // hanle arp request
        if (arp.opcode == ARPMessage::OPCODE_REQUEST &&
            arp.target_ip_address == ip_address_.ipv4_numeric()) {
          transmit_arp(arp.sender_ip_address, arp.sender_ethernet_address);
        }
      } else {
        std::cerr << "fail parsing ethernet frame " << frame.header.to_string() << std::endl;
      }
      break;
    }
    default:
      std::cerr << "bad ethernet frame " << frame.header.to_string() << std::endl;
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  time_ms_ += ms_since_last_tick;
  while (!arp_mapping_time_.empty() && arp_mapping_time_.front().first + 30000 < time_ms_) {
    arp_cache_.erase(arp_mapping_time_.front().second);
    arp_mapping_time_.pop();
  }
}

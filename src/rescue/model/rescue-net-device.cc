/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 AGH Univeristy of Science and Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Author: Lukasz Prasnal <prasnal@kt.agh.edu.pl>
 *
 * basing on Simple CSMA/CA Protocol module by Junseok Kim <junseok@email.arizona.edu> <engr.arizona.edu/~junseok>
 */

#include "ns3/trace-source-accessor.h"
#include "ns3/traced-callback.h"
#include "ns3/llc-snap-header.h"
#include "ns3/pointer.h"
#include "ns3/node.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"

#include "rescue-net-device.h"
#include "rescue-phy.h"
#include "rescue-mac.h"
#include "rescue-channel.h"
#include "rescue-remote-station-manager.h"


NS_LOG_COMPONENT_DEFINE ("RescueNetDevice");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RescueNetDevice);

RescueNetDevice::RescueNetDevice ()
  : NetDevice (),
    m_mtu (64000),
    m_arp (true)
{
}

RescueNetDevice::~RescueNetDevice ()
{
}

void
RescueNetDevice::Clear ()
{
  m_node = 0;
  if (m_mac)
    {
      m_mac->Clear ();
      m_mac = 0;
    }
  if (m_phy)
    {
      m_phy->Clear ();
      m_phy = 0;
    }
  if (m_channel)
    {
      m_channel->Clear ();
      m_channel = 0;
    }
  if (m_stationManager)
    {
      m_stationManager->Dispose ();
      m_stationManager = 0;
    }
}

void
RescueNetDevice::DoDispose ()
{
  Clear ();
  NetDevice::DoDispose ();
}

void
RescueNetDevice::DoInitialize ()
{
  m_phy->Initialize ();
  m_mac->Initialize ();
  m_stationManager->Initialize ();
  NetDevice::DoInitialize ();
}

TypeId
RescueNetDevice::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::RescueNetDevice")
    .SetParent<NetDevice> ()
    .AddAttribute ("Channel", "The channel attached to this device",
                   PointerValue (),
                   MakePointerAccessor (&RescueNetDevice::DoGetChannel, &RescueNetDevice::SetChannel),
                   MakePointerChecker<RescueChannel> ())
    .AddAttribute ("Phy", "The PHY layer attached to this device.",
                   PointerValue (),
                   MakePointerAccessor (&RescueNetDevice::GetPhy, &RescueNetDevice::SetPhy),
                   MakePointerChecker<RescuePhy> ())
    /*.AddAttribute ("LowMac", "The lower MAC sublayer attached to this device.",
                   PointerValue (),
                   MakePointerAccessor (&RescueNetDevice::GetLowMac, &RescueNetDevice::SetLowMac),
                   MakePointerChecker<RescueMac> ())*/
    .AddAttribute ("Mac", "The MAC layer attached to this device.",
                   PointerValue (),
                   MakePointerAccessor (&RescueNetDevice::GetMac, &RescueNetDevice::SetMac),
                   MakePointerChecker<RescueMac> ())
    .AddAttribute ("RemoteStationManager", "The station manager attached to this device.",
                   PointerValue (),
                   MakePointerAccessor (&RescueNetDevice::SetRemoteStationManager,
                                        &RescueNetDevice::GetRemoteStationManager),
                   MakePointerChecker<RescueRemoteStationManager> ())
    .AddTraceSource ("Rx", "Received payload from the MAC layer.",
                     MakeTraceSourceAccessor (&RescueNetDevice::m_rxLogger))
    .AddTraceSource ("Tx", "Send payload to the MAC layer.",
                     MakeTraceSourceAccessor (&RescueNetDevice::m_txLogger))
  ;
  return tid;
}

void
RescueNetDevice::SetNode (Ptr<Node> node)
{
  m_node = node;
}

void
RescueNetDevice::SetMac (Ptr<RescueMac> mac)
{
  if (mac != 0)
    {
      m_mac = mac;
      NS_LOG_DEBUG ("Set MAC");

      if (m_phy != 0)
        {
          m_mac->AttachPhy (m_phy);
          m_mac->SetDevice (this);
          m_phy->SetMac (m_mac);
          NS_LOG_DEBUG ("Attached PHY to MAC");
        }
      /*if (m_lowMac != 0)
        {
          m_hiMac->SetlowMac (m_lowMac);
          m_lowMac->SetHiMac (m_hiMac);
          NS_LOG_DEBUG ("HI-MAC attached to lower MAC");
        }*/
      if (m_stationManager != 0)
        {
          m_mac->SetRemoteStationManager (m_stationManager);
          m_stationManager->SetupMac (m_mac);
          NS_LOG_DEBUG ("HI-MAC conected to Station Manager");
        }
      m_mac->SetForwardUpCb (MakeCallback (&RescueNetDevice::ForwardUp, this));
    }
}

/*void
RescueNetDevice::SetLowMac (Ptr<RescueMac> lowMac)
{
  if (mac != 0)
    {
      m_lowMac = lowMac;
      NS_LOG_DEBUG ("Set MAC");

      if (m_phy != 0)
        {
          m_phy->SetMac (m_lowMac);
          m_lowMac->AttachPhy (m_phy);
          m_lowMac->SetDevice (this);
          NS_LOG_DEBUG ("Attached MAC to PHY");
        }
      if (m_mac != 0)
        {
          m_mc->SetlowMac (m_lowMac);
          m_lowMac->SetMac (m_mac);
          NS_LOG_DEBUG ("HI-MAC attached to lower MAC");
        }
      if (m_stationManager != 0)
        {
          m_lowMac->SetRemoteStationManager (m_stationManager);
          NS_LOG_DEBUG ("MAC conected to Station Manager");
        }
      //m_lowMac->SetForwardUpCb (MakeCallback (&RescueNetDevice::ForwardUp, this));
    }
}*/

void
RescueNetDevice::SetPhy (Ptr<RescuePhy> phy)
{
  if (phy != 0)
    {
      m_phy = phy;
      m_phy->SetDevice (Ptr<RescueNetDevice> (this));
      NS_LOG_DEBUG ("Set PHY");
      /*if (m_lowMac != 0)
        {
          m_lowMac->AttachPhy (phy);
          m_lowMac->SetDevice (this);
          m_phy->SetMac (m_lowMac);
          NS_LOG_DEBUG ("Attached PHY to MAC");
        }*/
      if (m_mac != 0)
        {
          m_mac->AttachPhy (m_phy);
          m_mac->SetDevice (this);
          m_phy->SetMac (m_mac);
          //NS_LOG_DEBUG ("HI-MAC notified about PHY");
          NS_LOG_DEBUG ("Attached PHY to MAC");
        }
    }
}

void
RescueNetDevice::SetChannel (Ptr<RescueChannel> channel)
{
  if (channel != 0)
    {
      m_channel = channel;
      NS_LOG_DEBUG ("Set CHANNEL");
      if (m_phy != 0)
        {
          m_channel->AddDevice (this, m_phy);
          m_phy->SetChannel (channel);
        }
    }
}

void
RescueNetDevice::SetRemoteStationManager (Ptr<RescueRemoteStationManager> manager)
{
  if (manager != 0)
    {
      m_stationManager = manager;
      NS_LOG_DEBUG ("Set REMOTE STATION MANAGER");
      /*if (m_lowMac != 0)
        {
          m_lowMac->SetRemoteStationManager (m_stationManager);
          NS_LOG_DEBUG ("MAC conected to Station Manager");
        }*/
      if (m_mac != 0)
        {
          m_mac->SetRemoteStationManager (m_stationManager);
          m_stationManager->SetupMac (m_mac);
          NS_LOG_DEBUG ("HI-MAC conected to Station Manager");
        }
    }
}

void
RescueNetDevice::SetIfIndex (uint32_t index)
{
  m_ifIndex = index;
}

void
RescueNetDevice::SetAddress (Address address)
{
  m_mac->SetAddress (Mac48Address::ConvertFrom (address));
}

bool
RescueNetDevice::SetMtu (uint16_t mtu)
{
  m_mtu = mtu;
  return true;
}

bool
RescueNetDevice::NeedsArp () const
{
  return m_arp;
}

bool
RescueNetDevice::SupportsSendFrom (void) const
{
  return false;
}

Ptr<RescueChannel>
RescueNetDevice::DoGetChannel (void) const
{
  return m_channel;

}

Ptr<RescueMac>
RescueNetDevice::GetMac () const
{
  return m_mac;
}

/*Ptr<LowRescueMac>
RescueNetDevice::GetLowMac () const
{
  return m_lowMac;
}*/

Ptr<RescuePhy>
RescueNetDevice::GetPhy () const
{
  return m_phy;
}

uint32_t
RescueNetDevice::GetIfIndex () const
{
  return m_ifIndex;
}

Ptr<Channel>
RescueNetDevice::GetChannel () const
{
  return m_channel;
}

Ptr<RescueRemoteStationManager>
RescueNetDevice::GetRemoteStationManager (void) const
{
  return m_stationManager;
}

Address
RescueNetDevice::GetAddress () const
{
  return m_mac->GetAddress ();
}

uint16_t
RescueNetDevice::GetMtu () const
{
  return m_mtu;
}

bool
RescueNetDevice::IsLinkUp () const
{
  return  (m_linkup && (m_phy != 0));
}

bool
RescueNetDevice::IsBroadcast () const
{
  return true;
}

Address
RescueNetDevice::GetBroadcast () const
{
  return m_mac->GetBroadcast ();
}

Ptr<Node>
RescueNetDevice::GetNode () const
{
  return m_node;
}
bool
RescueNetDevice::IsMulticast () const
{
  return false;
}

Address
RescueNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_FATAL_ERROR ("RescueNetDevice does not support multicast");
  return m_mac->GetBroadcast ();
}

Address
RescueNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_FATAL_ERROR ("RescueNetDevice does not support multicast");
  return m_mac->GetBroadcast ();
}

bool
RescueNetDevice::IsBridge (void) const
{
  return false;
}

bool
RescueNetDevice::IsPointToPoint () const
{
  return false;
}

bool 
RescueNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION ("pkt" << packet << "dest" << dest);
  NS_ASSERT (Mac48Address::IsMatchingType (dest));
  Mac48Address destAddr = Mac48Address::ConvertFrom (dest);
  //Mac48Address srcAddr = Mac48Address::ConvertFrom (GetAddress ());

  LlcSnapHeader llc;
  llc.SetType (protocolNumber);
  packet->AddHeader (llc);
  
  m_mac->Enqueue (packet, destAddr);
  
  return true;
}

bool
RescueNetDevice::SendFrom (Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (src << dest);
  NS_ASSERT (Mac48Address::IsMatchingType (dest));
  NS_ASSERT (Mac48Address::IsMatchingType (src));
  Mac48Address destAddr = Mac48Address::ConvertFrom (dest);
  //Mac48Address srcAddr = Mac48Address::ConvertFrom (src);

  LlcSnapHeader llc;
  llc.SetType (protocolNumber);
  packet->AddHeader (llc);

  m_mac->Enqueue (packet, destAddr);

  return true;
}

void
RescueNetDevice::ForwardUp (Ptr<Packet> packet, Mac48Address src, Mac48Address dest)
{
  NS_LOG_FUNCTION ("pkt" << packet << "src" << src << "dest" << dest);
  NS_LOG_DEBUG ("Forwarding packet up to application");
  
  LlcSnapHeader llc;
  packet->RemoveHeader (llc);
  enum NetDevice::PacketType type;
  
  if (dest.IsBroadcast ())
    {
      type = NetDevice::PACKET_BROADCAST;
    }
  else if (dest.IsGroup ())
    {
      type = NetDevice::PACKET_MULTICAST;
    }
  else if (dest == m_mac->GetAddress ())
    {
      type = NetDevice::PACKET_HOST;
    }
  else 
    {
      type = NetDevice::PACKET_OTHERHOST;
    }

  if (type != NetDevice::PACKET_OTHERHOST)
    {
      m_rxLogger (packet, src);
      m_forwardUp (this, packet, llc.GetType (), src);
    }
  
}

void
RescueNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  m_forwardUp = cb;
}

void
RescueNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  m_linkChanges.ConnectWithoutContext (callback);
}

void
RescueNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  // Not implemented yet
  NS_ASSERT_MSG (0, "Not yet implemented");
}

} // namespace ns3


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

#ifndef RESCUE_NET_DEVICE_H
#define RESCUE_NET_DEVICE_H

#include "ns3/net-device.h"
#include "ns3/pointer.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class RescueRemoteStationManager;
class RescueChannel;
class RescuePhy;
class RescueMac;

/**
 * \defgroup rescue Rescue Models
 *
 * This section documents the API of the ns-3 Rescue module. For a generic functional description, please refer to the ns-3 manual.
 */

/**
 * \brief Hold together all Rescue-related objects.
 * \ingroup rescue
 *
 * This class holds together ns3::RescueChannel, ns3::RescuePhy,
 * ns3::RescueMac, and, ns3::RescueRemoteStationManager.
 */
class RescueNetDevice : public NetDevice
{
public:
  static TypeId GetTypeId (void);

  RescueNetDevice ();
  virtual ~RescueNetDevice ();

  /**
   * \param mac the MAC to use.
   */
  void SetMac (Ptr<RescueMac> mac);
  /**
   * \param phy the PHY to use.
   */
  void SetPhy (Ptr<RescuePhy> phy);
  /**
   * \param channel the channel to associate.
   */
  void SetChannel (Ptr<RescueChannel> channel);
  /**
   * \param manager the manager to use.
   */
  void SetRemoteStationManager (Ptr<RescueRemoteStationManager> manager);

  /**
   * \return pointer to used MAC
   */
  Ptr<RescueMac> GetMac (void) const;
  /**
   * \return pointer to used PHY
   */
  Ptr<RescuePhy> GetPhy (void) const;
  /**
   * \returns the remote station manager we are currently using.
   */
  Ptr<RescueRemoteStationManager> GetRemoteStationManager (void) const;

  /**
   * Clears configuration
   */
  void Clear (void);

  // Purely virtual functions from base class
  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;
  virtual Ptr<Channel> GetChannel (void) const;
  virtual Address GetAddress (void) const;
  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;
  virtual bool IsLinkUp (void) const;
  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;
  virtual bool IsMulticast (void) const;
  virtual Address GetMulticast (Ipv4Address multicastGroup) const;
  virtual Address GetMulticast (Ipv6Address addr) const;
  virtual bool IsBridge (void) const ;
  virtual bool IsPointToPoint (void) const;
  virtual bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);
  virtual bool NeedsArp (void) const;
  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom (void) const;
  virtual void AddLinkChangeCallback (Callback<void> callback);
  virtual void SetAddress (Address address);
  
private:
  /**
   * Receive a packet from the lower layer and pass the
   * packet up the stack.
   *
   * \param packet passed packet
   * \param src source address
   * \param dest destination address
   */
  virtual void ForwardUp (Ptr<Packet> packet, Mac48Address src, Mac48Address dest);
  /**
   * \returns the associated channel.
   */
  Ptr<RescueChannel> DoGetChannel (void) const;
  
  Ptr<Node> m_node;                                 //<! pointer to associated ns-3 node
  Ptr<RescueChannel> m_channel;                     //<! Pointer to RescueChannel
  Ptr<RescueMac> m_mac;                             //!< Pointer to RescueMac
  Ptr<RescuePhy> m_phy;                             //!< Pointer to RescuePhy
  Ptr<RescueRemoteStationManager> m_stationManager; //!< Pointer to WifiRemoteStationManager (rate control)

  uint32_t m_ifIndex;
  uint16_t m_mtu;
  bool m_linkup;
  TracedCallback<> m_linkChanges;
  ReceiveCallback m_forwardUp;

  TracedCallback<Ptr<const Packet>, Mac48Address> m_rxLogger;
  TracedCallback<Ptr<const Packet>, Mac48Address> m_txLogger;

  bool m_arp;

protected:
  virtual void DoDispose ();
};

} // namespace ns3

#endif // RESCUE_NET_DEVICE_H 

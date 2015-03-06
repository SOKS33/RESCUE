/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 University of Arizona
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
 * basing on Simple CSMA/CA Protocol module Junseok Kim <junseok@email.arizona.edu> <engr.arizona.edu/~junseok>
 */

#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/config.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/core-module.h"

#include "ns3/rescue-mac.h"
#include "ns3/rescue-phy.h"
#include "ns3/rescue-channel.h"
#include "ns3/rescue-remote-station-manager.h"
#include "rescue-helper.h"

#include <sstream>
#include <string>

NS_LOG_COMPONENT_DEFINE("RescueHelper");

namespace ns3
{
  
RescueMacHelper::~RescueMacHelper ()
{}

RescuePhyHelper::~RescuePhyHelper ()
{}

RescueHelper::RescueHelper ()
{
  m_mac.SetTypeId ("ns3::RescueMacCsma");
  m_phy.SetTypeId ("ns3::RescuePhy");
  //m_stationManager.SetTypeId ("ns3::RescueRemoteStationManager");
}

RescueHelper::~RescueHelper ()
{}

RescueHelper
RescueHelper::Default (void)
{
  RescueHelper helper;
  helper.SetRemoteStationManager ("ns3::ConstantRateRescueManager",
							      "DataMode", StringValue ("Ofdm36Mbps"),
							      "ControlMode", StringValue ("Ofdm6Mbps"),
								  "MaxSrc", UintegerValue (7));
  return helper;
}

void
RescueHelper::SetRemoteStationManager (std::string type,
                                       std::string n0, const AttributeValue &v0,
                                       std::string n1, const AttributeValue &v1,
                                       std::string n2, const AttributeValue &v2,
                                       std::string n3, const AttributeValue &v3,
                                       std::string n4, const AttributeValue &v4,
                                       std::string n5, const AttributeValue &v5,
                                       std::string n6, const AttributeValue &v6,
                                       std::string n7, const AttributeValue &v7)
{
  m_stationManager = ObjectFactory ();
  m_stationManager.SetTypeId (type);
  m_stationManager.Set (n0, v0);
  m_stationManager.Set (n1, v1);
  m_stationManager.Set (n2, v2);
  m_stationManager.Set (n3, v3);
  m_stationManager.Set (n4, v4);
  m_stationManager.Set (n5, v5);
  m_stationManager.Set (n6, v6);
  m_stationManager.Set (n7, v7);
}

NetDeviceContainer
RescueHelper::Install (NodeContainer c, Ptr<RescueChannel> channel, const RescuePhyHelper &phyHelper, const RescueMacHelper &macHelper) const
{
  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); i++)
    {
      Ptr<Node> node = *i;
      Ptr<RescueNetDevice> device = CreateObject<RescueNetDevice> ();
      Ptr<RescueRemoteStationManager> manager = m_stationManager.Create<RescueRemoteStationManager> ();

      Ptr<RescueMac> mac = macHelper.Create ();
      Ptr<RescuePhy> phy = phyHelper.Create ();
      mac->SetAddress (Mac48Address::Allocate ());
      device->SetMac (mac);
      device->SetPhy (phy);
      device->SetChannel (channel);
      device->SetRemoteStationManager (manager);

      node->AddDevice (device);
      devices.Add (device);

      NS_LOG_DEBUG ("node="<<node<<", mob="<<node->GetObject<MobilityModel> ());
  }
  return devices;
}

int64_t
RescueHelper::AssignStreams (NetDeviceContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<NetDevice> netDevice;
  for (NetDeviceContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      netDevice = (*i);
      Ptr<RescueNetDevice> rescue = DynamicCast<RescueNetDevice> (netDevice);
      if (rescue)
        {
          // Handle any random numbers in the PHY objects.
          currentStream += rescue->GetPhy ()->AssignStreams (currentStream);

          // Handle any random numbers in the station managers. /currently not needed/
          //Ptr<RescueRemoteStationManager> manager = rescue->GetRemoteStationManager ();

          // Handle any random numbers in the MAC objects. /currently not needed/
          currentStream += rescue->GetMac ()->AssignStreams (currentStream);
        }
    }
  return (currentStream - stream);
}

} //end namespace ns3

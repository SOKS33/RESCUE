/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 University of Arizona
 * Copyright (c) 2015 AGH University of Science and Technology
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

#ifndef RESCUE_HELPER_H_
#define RESCUE_HELPER_H_

#include <string>
#include "ns3/attribute.h"
#include "ns3/object-factory.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/rescue-net-device.h"

namespace ns3
{
class RescueMac;
class RescuePhy;
class RescueChannel;

class RescueMacHelper
{
public:
  virtual ~RescueMacHelper ();
  virtual Ptr<RescueMac> Create (void) const = 0;
};

/*class RescueLowMacHelper
{
public:
  virtual ~RescueLowMacHelper ();
  virtual Ptr<RescueMac> Create (void) const = 0;
};*/

class RescuePhyHelper
{
public:
  virtual ~RescuePhyHelper ();
  virtual Ptr<RescuePhy> Create (void) const = 0;
};

class RescueHelper
{
public:
  RescueHelper();
  virtual ~RescueHelper();
  /**
   * \returns a new WifiHelper in a default state
   *
   * The default state is defined as being an simple MAC rescue layer with an constant rate control algorithm
   * - 36 Mbps for DATA and 6 Mbps for CONTROL traffic - and both objects using their default attribute values. 
   */
  static RescueHelper Default (void);

  /**
   * \param type the type of ns3::RescueRemoteStationManager to create.
   * \param n0 the name of the attribute to set
   * \param v0 the value of the attribute to set
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   * \param n2 the name of the attribute to set
   * \param v2 the value of the attribute to set
   * \param n3 the name of the attribute to set
   * \param v3 the value of the attribute to set
   * \param n4 the name of the attribute to set
   * \param v4 the value of the attribute to set
   * \param n5 the name of the attribute to set
   * \param v5 the value of the attribute to set
   * \param n6 the name of the attribute to set
   * \param v6 the value of the attribute to set
   * \param n7 the name of the attribute to set
   * \param v7 the value of the attribute to set
   *
   * All the attributes specified in this method should exist
   * in the requested station manager.
   */
  void SetRemoteStationManager (std::string type,
                                std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                                std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                                std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                                std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                                std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
                                std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
                                std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
                                std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());

  /**
   * \param c the set of nodes on which a rescue device must be created
   * \param channel the channel for transmission of the rescue devices
   * \param phy the PHY helper to create PHY objects
   * \param mac the MAC helper to create MAC objects
   * \returns a device container which contains all the devices created by this method.
   */
  NetDeviceContainer Install (NodeContainer c, Ptr<RescueChannel> channel, const RescuePhyHelper &phyHelper, const RescueMacHelper &macHelper) const;

  /**
  * Assign a fixed random variable stream number to the random variables
  * used by the Phy and Mac aspects of the Rescue models.  Each device in
  * container c has fixed stream numbers assigned to its random variables.
  * The Rescue channel (e.g. propagation loss model) is excluded.
  * Return the number of streams (possibly zero) that
  * have been assigned. The Install() method should have previously been
  * called by the user.
  *
  * \param c NetDeviceContainer of the set of net devices for which the 
  *          RescueNetDevice should be modified to use fixed streams
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this helper
  */
  int64_t AssignStreams (NetDeviceContainer c, int64_t stream);

protected:
  ObjectFactory m_stationManager;  

private:
  ObjectFactory m_mac;
  ObjectFactory m_phy;
};


} //end namespace ns3

#endif /* RESCUE_HELPER_H_ */

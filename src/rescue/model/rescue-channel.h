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

#ifndef RESCUE_CHANNEL_H
#define RESCUE_CHANNEL_H

#include "ns3/channel.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "rescue-net-device.h"
#include "rescue-phy.h"
#include "rescue-mode.h"
#include <list>
#include <vector>

namespace ns3 {

class PropagationLossModel;
class PropagationDelayModel;

/**
 * \brief A Rescue channel
 * \ingroup rescue
 *
 * This simple channel implementation bases on Simple CSMA/CA Protocol module 
 * by Junseok Kim <junseok@email.arizona.edu> <engr.arizona.edu/~junseok>
 *
 * This class is expected to be used in tandem with the ns3::RescuePhy
 * class and contains a ns3::PropagationLossModel and a ns3::PropagationDelayModel.
 * By default, LogDistancePropagationLossModel and ConstantSpeedPropagationDelayModel, 
 * another models are possible and should be set before using the channel.
 */
class RescueChannel : public Channel
{

/**
 * A struct for transmission receive events
 *
 * \param packet transmitted packet
 * \param mode transmission mode
 * \param phy PHY receiving the packet
 * \param txDuration duration of transmission
 * \param txEnd end of transmission
 * \param rxPower received signal power
 */
typedef struct
{
  Ptr<Packet> packet;
  RescueMode mode;
  Ptr<RescuePhy> phy;
  Time txDuration;
  Time txEnd;
  double_t rxPower;
  Time procDelay;
} NoiseEntry;

public:
  RescueChannel ();
  virtual ~RescueChannel ();
  static TypeId GetTypeId ();
  
  // inherited from Channel.
  virtual uint32_t GetNDevices () const;
  virtual Ptr<NetDevice> GetDevice (uint32_t i) const;

  /**
   * Adds the given RescueNetDevice with RescuePhy set to the devices/phy list
   *
   * \param dev the RescueNetDevice to be added to the device list
   * \param phy the RescuePhy to associated with added RescueNetDevice
   */
  void AddDevice (Ptr<RescueNetDevice> dev, Ptr<RescuePhy> phy);

  /**
   * Clear the device list and erase all events.
   */
  void Clear ();
  
  /**
   * \param phy PHY of the device from which the packet is originating.
   * \param packet the packet to send
   * \param txPower the tx power in dBm associated to the packet
   * \param mode the mode (RescueMode) of this transmission
   * \param txDuration duration of transmission 
   * \return true if the packet was transmitted
   *
   * This method should not be invoked by normal users. It is
   * currently invoked only from RescuePhy::Send. 
   * Currently we do not support multichannel operation
   */
  bool SendPacket (Ptr<RescuePhy> phy, Ptr<Packet> packet, double txPower, RescueMode mode, Time txDuration);
  
  /**
   * Calculate noise and interference power in W.
   *
   * \param phy PHY of device for which the noise is calculated
   * \param signal the transmission for which the noise is calculated
   * \return noise and interference power
   */
  double GetNoiseW (Ptr<RescuePhy> phy, Ptr<Packet> signal);
  
  /**
   * Convert from dBm to Watts.
   *
   * \param dbm the power in dBm
   * \return the equivalent Watts for the given dBm
   */
  double DbmToW (double dbm);

  /**
   * Convert from Watts to dBm.
   *
   * \param w the power in Watts
   * \return the equivalent dBm for the given Watts
   */
  double WToDbm (double w);
  
private:
  /**
   * This method is scheduled by SendPacket to notify transmitting PHY
   * about transmission end
   *
   * \param phy transmitting PHY
   * \param packet transmitted packet
   */
  void SendPacketDone (Ptr<RescuePhy> phy, Ptr<Packet> packet);

  /**
   * This method is scheduled by SendPacket for each associated RescuePhy.
   * The method then calls the corresponding RescuePhy that the first
   * bit of the packet has arrived.
   *
   * \param i index of the corresponding RescuePhy in the PHY list
   * \param ne the noise entry related with transmitted packet
   */
  void ReceivePacket (uint32_t i, NoiseEntry ne);

  /**
   * This method is scheduled by ReceivePacket to notify receiving PHYs
   * about transmission end
   *
   * \param i index of the corresponding RescuePhy in the PHY list
   * \param ne the noise entry related with transmitted packet
   */
  void ReceivePacketDone (uint32_t i, NoiseEntry ne);

  /**
   * This method is scheduled by SendPacket to add 
   * given noise entry related
   *
   * \param i index of the corresponding RescuePhy in the PHY list
   * \param ne the noise entry to add
   */
  void AddNoiseEntry (uint32_t i, NoiseEntry ne);

  /**
   * This method is scheduled by ReceivePacketDone to delete 
   * given noise entry related
   *
   * \param ne the noise entry to delete
   */
  void DeleteNoiseEntry (NoiseEntry ne);

  Time m_addNoiseEntryEarlier; //!< Add tx-flow earlier a certain time  
  Time m_delNoiseEntryLater; //!< Delete tx-flow later a certain time
  double m_noiseFloor; //!< Noise Floor (dBm)
  
  Ptr<PropagationLossModel> m_loss; //!< Propagation loss model
  Ptr<PropagationDelayModel> m_delay; //!< Propagation delay model
  
  typedef std::vector<std::pair<Ptr<RescueNetDevice>, Ptr<RescuePhy> > > RescueDeviceList; 
  RescueDeviceList m_devList; //!< List of pairs RescueDevice+RescuePhy connected to this RescueChannel
  std::list<NoiseEntry> m_noiseEntry; //!< List of current noise entries
  
protected:
  
};

}

#endif // RESCUE_CHANNEL_H

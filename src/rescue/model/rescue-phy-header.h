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

#ifndef RESCUE_PHY_HEADER_H
#define RESCUE_PHY_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"

#define RESCUE_PHY_PKT_TYPE_DATA  0 //data frame
#define RESCUE_PHY_PKT_TYPE_ACK   1 //acknowledgment
#define RESCUE_PHY_PKT_TYPE_RR    2 //resource reservation
#define RESCUE_PHY_PKT_TYPE_B     3 //beacon


namespace ns3 {

/**
 * \ingroup rescue
 *
 * Implements the Rescue PHY header
 */
class RescuePhyHeader : public Header
{
public:
  RescuePhyHeader ();
  
  RescuePhyHeader (const Mac48Address srcAddr, const Mac48Address senderAddr, const Mac48Address dstAddr, uint8_t type);
  RescuePhyHeader (const Mac48Address srcAddr, const Mac48Address senderAddr, const Mac48Address dstAddr, 
                   uint8_t type, uint16_t seq, uint16_t il);
  RescuePhyHeader (const Mac48Address srcAddr, const Mac48Address dstAddr, uint8_t type);
  RescuePhyHeader (uint8_t type);
  virtual ~RescuePhyHeader ();
  
  static TypeId GetTypeId (void);

  /**
   * \param type the type field
   */
  void SetType (uint8_t type);
  /**
   * \param dataRate the data rate field
   */
  void SetDataRate (uint8_t dataRate);
  /**
   * \param length the length field
   */
  void SetLength (uint8_t length);
  /**
   * \param addr the source address field
   */
  void SetSource (Mac48Address addr);
  /**
   * \param addr the sender address field
   */
  void SetSender (Mac48Address addr);
  /**
   * \param addr the destination address field
   */
  void SetDestination (Mac48Address addr);
  /**
   * \param seq the sequence number field
   */
  void SetSequence (uint16_t seq);
  /**
   * \param il the interleaver field
   */
  void SetInterleaver (uint8_t il);
  /**
   * \param qos the QoS field
   */
  void SetQoS (uint8_t qos);
  /**
   * \param checksum the checksum field
   */
  void SetChecksum (uint32_t checksum);
  
  /**
   * \param subtype the subtype field
   */
  void SetSubtype (uint8_t subtype);
  /**
   * \param blockAck the block ACK flag/field
   */
  void SetBlockAck (uint8_t blockAck);
  /**
   * \param continousAck the continous ACK flag/field
   */
  void SetContinousAck (uint16_t continousAck);
  /**
   * \param 1st byte consisting of:
   *        - type field (2b) and data rate field (6b) (DATA frame)
   *        - type field (2b), subtype (4b) and block ACK field (2b) (ACK frame)
   */
  void SetFrameControl (uint8_t ctrl);

  /**
   * \return the type field value
   */
  uint8_t GetType () const;
  /**
   * \return the data rate field value
   */
  uint8_t GetDataRate () const;
  /**
   * \return the length field value
   */
  uint8_t GetLength () const;
  /**
   * \return the source address field value
   */
  Mac48Address GetSource () const;
  /**
   * \return the sender address field value
   */
  Mac48Address GetSender () const;
  /**
   * \return the destination address field value
   */
  Mac48Address GetDestination () const;
  /**
   * \return the sequence number field value
   */
  uint16_t GetSequence () const;
  /**
   * \return the interleaver field value
   */
  uint8_t GetInterleaver () const;
  /**
   * \return the QoS field value
   */
  uint8_t GetQoS () const;
  /**
   * \return the QoS field value
   */
  uint32_t GetChecksum () const;

  /**
   * \return the subtype field value
   */
  uint8_t GetSubtype () const;  
  /**
   * \return the block ACK flag/field value
   */
  uint8_t GetBlockAck () const;
  /**
   * \return continousAck the continous ACK flag/field value
   */
  uint16_t GetContinousAck () const;

  /**
   * \return 1st byte consisting of:
   *        - type field (2b) and data rate field (6b) (DATA frame)
   *        - type field (2b), subtype (4b) and block ACK field (2b) (ACK frame)
   */
  uint8_t GetFrameControl () const;

  uint32_t GetSize () const;

  // Inherrited methods
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId (void) const;
  
private:
  uint8_t m_type;           //<! type field
  uint8_t m_dataRate;       //<! data rate field
  uint16_t m_length;        //<! length field
  Mac48Address m_srcAddr;   //<! source address field
  Mac48Address m_senderAddr;//<! sender address field
  Mac48Address m_dstAddr;   //<! destination address field
  uint16_t m_sequence;      //<! sequence number field
  uint8_t m_interleaver;    //<! interleaver field
  uint8_t m_qos;            //<! QoS field
  uint32_t m_checksum;      //<! checksum field

  uint8_t m_subtype;        //<! subtype field
  uint8_t m_blockAck;       //<! block ACK field/flag
  uint16_t m_continousAck;  //<! continous ACK field/flag
};

}

#endif // RESCUE_PHY_HEADER_H

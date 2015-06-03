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
 * basing on ns-3 wifi module by:
 * Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ns3/address-utils.h"
#include "rescue-phy-header.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("RescuePhyHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RescuePhyHeader);

RescuePhyHeader::RescuePhyHeader ()
{
}
RescuePhyHeader::RescuePhyHeader (uint8_t type)
  : Header (),
    m_type (type)
{
}
RescuePhyHeader::RescuePhyHeader (const Mac48Address srcAddr, const Mac48Address dstAddr, uint8_t type)
  : Header (),
    m_type (type),
    m_srcAddr (srcAddr),
    m_dstAddr (dstAddr)
{
}
RescuePhyHeader::RescuePhyHeader (const Mac48Address srcAddr, const Mac48Address senderAddr, const Mac48Address dstAddr, uint8_t type)
  : Header (),
    m_type (type),
    m_srcAddr (srcAddr),
    m_senderAddr (senderAddr),
    m_dstAddr (dstAddr)
{
}
RescuePhyHeader::RescuePhyHeader (const Mac48Address srcAddr, const Mac48Address senderAddr, const Mac48Address dstAddr, 
                                  uint8_t type, uint16_t seq, uint16_t il)
  : Header (),
    m_type (type),
    m_srcAddr (srcAddr),
    m_senderAddr (senderAddr),
    m_dstAddr (dstAddr),
    m_sequence (seq),
    m_interleaver (il)
{
}
RescuePhyHeader::RescuePhyHeader (const Mac48Address srcAddr, const Mac48Address senderAddr, const Mac48Address dstAddr, 
                                  uint8_t type, uint16_t duration, uint16_t seq, uint16_t mbf, uint16_t il)
  : Header (),
    m_type (type),
    m_duration (duration),
    m_srcAddr (srcAddr),
    m_senderAddr (senderAddr),
    m_dstAddr (dstAddr),
    m_sequence (seq),
    m_mbf (mbf),
    m_interleaver (il)
{
}
RescuePhyHeader::RescuePhyHeader (const Mac48Address srcAddr, const Mac48Address senderAddr, const Mac48Address dstAddr, 
                                  uint8_t type, uint16_t duration, uint32_t beaconRep, uint16_t seq, uint16_t mbf, uint16_t il)
  : Header (),
    m_type (type),   
    m_duration (duration),
    m_beaconRep (beaconRep),
    m_srcAddr (srcAddr),
    m_senderAddr (senderAddr),
    m_dstAddr (dstAddr),
    m_sequence (seq),
    m_mbf (mbf),
    m_interleaver (il)
{
}
RescuePhyHeader::RescuePhyHeader (const Mac48Address srcAddr, const Mac48Address dstAddr, 
                                  uint8_t type, uint16_t blockAck, uint8_t contAck, uint16_t seq)
  : Header (),
    m_type (type),   
    m_duration (blockAck),
    m_beaconRep (contAck),
    m_srcAddr (srcAddr),
    m_dstAddr (dstAddr),
    m_sequence (seq)
{
}
RescuePhyHeader::RescuePhyHeader (const Mac48Address srcAddr,
                                  uint8_t type, uint32_t timestamp, uint32_t beaconInterval, uint32_t cfpPeriod, uint32_t tdmaTimeSlot)
  : Header (),
    m_type (type),   
    m_timestamp (timestamp),
    m_srcAddr (srcAddr),
    m_beaconInterval (beaconInterval),
    m_cfpPeriod (cfpPeriod),
    m_tdmaTimeSlot (tdmaTimeSlot)
{
}
RescuePhyHeader::RescuePhyHeader (const Mac48Address srcAddr, const Mac48Address dstAddr, 
                                  uint8_t type, uint16_t duration, uint16_t nextDuration, uint16_t seq, uint16_t mbf, uint16_t il)
  : Header (),
    m_type (type),
    m_duration (duration),
    m_nextDuration (nextDuration),
    m_srcAddr (srcAddr),
    m_dstAddr (dstAddr),
    m_sequence (seq),
    m_mbf (mbf),
    m_interleaver (il)
{
}

TypeId
RescuePhyHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RescuePhyHeader")
    .SetParent<Header> ()
    .AddConstructor<RescuePhyHeader> ()
  ;
  return tid;
}

TypeId
RescuePhyHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

RescuePhyHeader::~RescuePhyHeader ()
{}



// ------------------------ Set Functions -----------------------------
// ....................CTRL field - subfields/flags values.............
void
RescuePhyHeader::SetType (uint8_t type)
{
  m_type = type;
}
void
RescuePhyHeader::SetDataRate (uint8_t dataRate)
{
  m_dataRate = dataRate;
}
void
RescuePhyHeader::SetACK (void)
{
  m_nACK = 1;
}
void
RescuePhyHeader::SetNACK (void)
{
  m_nACK = 0;
}
void
RescuePhyHeader::SetSendWindow (uint8_t sendWindow)
{
  m_sendWindow = sendWindow;
}
void
RescuePhyHeader::SetBlockAckSupported (void)
{
  m_bAck = 1;
}
void
RescuePhyHeader::SetBlockAckDisabled (void)
{
  m_bAck = 0;
}
void
RescuePhyHeader::SetContinousAckSupported (void)
{
  m_continousAck = 1;
}
void
RescuePhyHeader::SetContinousAckDisabled (void)
{
  m_continousAck = 0;
}
void
RescuePhyHeader::SetQosSupported (void)
{
  m_qos = 1;
}
void
RescuePhyHeader::SetQosDisabled (void)
{
  m_qos = 0;
}
void
RescuePhyHeader::SetDistributedMacProtocol (void)
{
  m_macProtocol = 0;
}
void
RescuePhyHeader::SetCentralisedMacProtocol (void)
{
  m_macProtocol = 1;
}
void 
RescuePhyHeader::SetRetry (void)
{
  m_retry = 1;
}
void 
RescuePhyHeader::SetNoRetry (void)
{
  m_retry = 0;
}
void
RescuePhyHeader::SetNACKedFrames (uint8_t nackedFrames)
{
  m_NACKedFrames = nackedFrames;
}
void
RescuePhyHeader::SetNextDataRate (uint8_t nextDataRate)
{
  m_nextDataRate = nextDataRate;
}
void
RescuePhyHeader::SetChannel (uint8_t channel)
{
  m_channel = channel;
}
void
RescuePhyHeader::SetTxPower (uint8_t txPower)
{
  m_txPower = txPower;
}
void
RescuePhyHeader::SetSchedulesListSize (uint8_t schedListSize)
{
  m_schedListSize = schedListSize;
}



// ....................header fields values............................
void
RescuePhyHeader::SetFrameControl (uint16_t ctrl)
{
  m_type = ctrl & 0x07;
  switch (m_type)
    {
      case RESCUE_PHY_PKT_TYPE_DATA:
        m_dataRate = (ctrl >> 3) & 0x03;
        m_sendWindow = (ctrl >> 5) & 0x0f;
        m_bAck = (ctrl >> 9) & 0x01;
        m_contAck = (ctrl >> 10) & 0x01;
        m_qos = (ctrl >> 11) & 0x01;
        m_macProtocol = (ctrl >> 12) & 0x01;
        m_retry = (ctrl >> 13) & 0x01;
        break;
      case RESCUE_PHY_PKT_TYPE_E2E_ACK:
        m_nACK = (ctrl >> 3) & 0x01;
        m_sendWindow = (ctrl >> 4) & 0x0f;
        m_bAck = (ctrl >> 8) & 0x01;
        m_contAck = (ctrl >> 9) & 0x01;
        m_NACKedFrames = (ctrl >> 10) & 0x0f;
        break;
      case RESCUE_PHY_PKT_TYPE_PART_ACK:
       //currently not suported
       break;
      case RESCUE_PHY_PKT_TYPE_B:
        m_dataRate = (ctrl >> 3) & 0x03;
        m_qos = (ctrl >> 5) & 0x01;
        m_channel = (ctrl >> 6) & 0x03;
        m_txPower = (ctrl >> 8) & 0x07;
        m_schedListSize = (ctrl >> 11) & 0x1f;
        break;
      case RESCUE_PHY_PKT_TYPE_RR:
        m_dataRate = (ctrl >> 3) & 0x03;
        m_sendWindow = (ctrl >> 5) & 0x0f;
        m_bAck = (ctrl >> 9) & 0x01;
        m_contAck = (ctrl >> 10) & 0x01;
        m_qos = (ctrl >> 11) & 0x01;
        m_nextDataRate = (ctrl >> 12) & 0x03;
        break;
    }
}
void
RescuePhyHeader::SetDuration (uint16_t duration)
{
  m_duration = duration;
}
void
RescuePhyHeader::SetNextDuration (uint16_t nextDuration)
{
  m_nextDuration = nextDuration;
}
void
RescuePhyHeader::SetBeaconRep (uint32_t beaconRep)
{
  m_beaconRep = beaconRep;
}
void
RescuePhyHeader::SetBlockAck (uint16_t blockAck)
{
  m_blockAck = blockAck;
}
void
RescuePhyHeader::SetContinousAck (uint8_t continousAck)
{
  m_continousAck = continousAck;
}
void
RescuePhyHeader::SetTimestamp (uint32_t timestamp)
{
  m_timestamp = timestamp;
}
void
RescuePhyHeader::SetSource (Mac48Address addr)
{
  m_srcAddr = addr;
}
void
RescuePhyHeader::SetSender (Mac48Address addr)
{
  m_senderAddr = addr;
}
void
RescuePhyHeader::SetDestination (Mac48Address addr)
{
  m_dstAddr = addr;
}
void
RescuePhyHeader::SetSequence (uint16_t seq)
{
  m_sequence = seq;
}
void
RescuePhyHeader::SetMBF (uint16_t mbf)
{
  m_mbf = mbf;
}
void
RescuePhyHeader::SetInterleaver (uint8_t il)
{
  m_interleaver = il;
}
void
RescuePhyHeader::SetBeaconInterval (uint32_t beaconInterval)
{
  m_beaconInterval = beaconInterval;
}
void
RescuePhyHeader::SetCfpPeriod (uint32_t cfpPeriod)
{
  m_cfpPeriod = cfpPeriod;
}
void
RescuePhyHeader::SetTdmaTimeSlot (uint32_t tdmaTimeSlot)
{
  m_tdmaTimeSlot = tdmaTimeSlot;
}
void
RescuePhyHeader::SetScheduleEntry (uint8_t number, Mac48Address scheduleEntry)
{
  m_schedulesList [number] = scheduleEntry;
}
void
RescuePhyHeader::SetChecksum (uint32_t checksum)
{
  m_checksum = checksum;
}



// ------------------------ Get Functions -----------------------------
// ....................CTRL field - subfields/flags values.............
uint8_t
RescuePhyHeader::GetType (void) const
{
  return m_type;
}
bool
RescuePhyHeader::IsDataFrame (void) const
{
  return (m_type == RESCUE_PHY_PKT_TYPE_DATA);
}
bool
RescuePhyHeader::IsE2eAckFrame (void) const
{
  return (m_type == RESCUE_PHY_PKT_TYPE_E2E_ACK);
}
bool
RescuePhyHeader::IsPartialAckFrame (void) const
{
  return (m_type == RESCUE_PHY_PKT_TYPE_PART_ACK);
}
bool
RescuePhyHeader::IsBeaconFrame (void) const
{
  return (m_type == RESCUE_PHY_PKT_TYPE_B);
}
bool
RescuePhyHeader::IsResourceReservationFrame (void) const
{
  return (m_type == RESCUE_PHY_PKT_TYPE_RR);
}
uint8_t
RescuePhyHeader::GetDataRate (void) const
{
  return m_dataRate;
}
bool
RescuePhyHeader::IsACK (void) const
{
  return (m_nACK == 1);
}
uint8_t
RescuePhyHeader::GetSendWindow (void) const
{
  return m_sendWindow;
}
bool
RescuePhyHeader::IsBlockAckSupported (void) const
{
  return (m_bAck == 1);
}
bool
RescuePhyHeader::IsContinousAckSupported (void) const
{
  return (m_continousAck == 1);
}
bool
RescuePhyHeader::IsQosSupported (void) const
{
  return (m_qos == 1);
}
bool
RescuePhyHeader::IsCentralisedMacProtocol (void) const
{
  return (m_macProtocol == 1);
}
bool
RescuePhyHeader::IsRetry (void) const
{
  return (m_retry == 1);
}
uint8_t
RescuePhyHeader::GetNACKedFrames (void) const
{
  return m_NACKedFrames;
}
uint8_t
RescuePhyHeader::GetNextDataRate (void) const
{
  return (m_nextDataRate);
}
uint8_t
RescuePhyHeader::GetChannel (void) const
{
  return (m_channel);
}
uint8_t
RescuePhyHeader::GetTxPower (void) const
{
  return (m_txPower);
}
uint8_t
RescuePhyHeader::GetSchedulesListSize (void) const
{
  return (m_schedListSize);
}



// ....................header fields values............................
uint16_t
RescuePhyHeader::GetFrameControl (void) const
{
  uint16_t val = 0;
  val |= m_type & 0x7;
  switch (m_type)
    {
      case RESCUE_PHY_PKT_TYPE_DATA:
        val |= (m_dataRate << 3) & (0x03 << 3);
        val |= (m_sendWindow << 5) & (0x0f << 5);
        val |= (m_bAck << 9) & (0x01 << 9);
        val |= (m_contAck << 10) & (0x01 << 10);
        val |= (m_qos << 11) & (0x01 << 11);
        val |= (m_macProtocol << 12) & (0x01 << 12);
        val |= (m_retry << 13) & (0x01 << 13);
        break;
      case RESCUE_PHY_PKT_TYPE_E2E_ACK:
        val |= (m_nACK << 3) & (0x01 << 3);
        val |= (m_sendWindow << 4) & (0x0f << 4);
        val |= (m_bAck << 8) & (0x01 << 8);
        val |= (m_contAck << 9) & (0x01 << 9);
        val |= (m_NACKedFrames << 10) & (0x0f << 10);
        break;
      case RESCUE_PHY_PKT_TYPE_PART_ACK:
        //currently not supported
        break;
      case RESCUE_PHY_PKT_TYPE_B:
        val |= (m_dataRate << 3) & (0x03 << 3);
        val |= (m_qos << 5) & (0x01 << 5);
        val |= (m_channel << 6) & (0x03 << 6);
        val |= (m_txPower << 8) & (0x07 << 8);
        val |= (m_schedListSize << 11) & (0x1f << 11);
        break;
      case RESCUE_PHY_PKT_TYPE_RR:
        val |= (m_dataRate << 3) & (0x03 << 3);
        val |= (m_sendWindow << 5) & (0x0f << 5);
        val |= (m_bAck << 9) & (0x01 << 9);
        val |= (m_contAck << 10) & (0x01 << 10);
        val |= (m_qos << 11) & (0x01 << 11);
        val |= (m_nextDataRate << 12) & (0x03 << 12);
        break;
    }
  return val;
}
uint8_t
RescuePhyHeader::GetDuration (void) const
{
  return m_duration;
}
uint16_t
RescuePhyHeader::GetNextDuration (void) const
{
  return m_nextDuration;
}
uint32_t
RescuePhyHeader::GetBeaconRep (void) const
{
  return m_beaconRep;
}
uint8_t
RescuePhyHeader::GetBlockAck (void) const
{
  return m_blockAck;
}
uint16_t
RescuePhyHeader::GetContinousAck (void) const
{
  return m_continousAck;
}
uint32_t
RescuePhyHeader::GetTimestamp (void) const
{
  return m_timestamp;
}
Mac48Address
RescuePhyHeader::GetSource (void) const
{
  return m_srcAddr;
}
Mac48Address
RescuePhyHeader::GetSender (void) const
{
  return m_senderAddr;
}
Mac48Address
RescuePhyHeader::GetDestination (void) const
{
  return m_dstAddr;
}
uint16_t
RescuePhyHeader::GetSequence (void) const
{
  return m_sequence;
}
uint16_t
RescuePhyHeader::GetMBF (void) const
{
  return m_mbf;
}
uint8_t
RescuePhyHeader::GetInterleaver (void) const
{
  return m_interleaver;
}
uint32_t
RescuePhyHeader::GetBeaconInterval (void) const
{
  return m_beaconInterval;
}
uint32_t
RescuePhyHeader::GetCfpPeriod (void) const
{
  return m_cfpPeriod;
}
uint32_t
RescuePhyHeader::GetTdmaTimeSlot (void) const
{
  return m_tdmaTimeSlot;
}
Mac48Address
RescuePhyHeader::GetScheduleEntry (uint8_t number) const
{
  return m_schedulesList [number];
}
uint32_t
RescuePhyHeader::GetChecksum (void) const
{
  return m_checksum;
}

uint32_t 
RescuePhyHeader::GetSize (void) const
{
  uint32_t size = 0;
  switch (m_type)
    {
      case RESCUE_PHY_PKT_TYPE_DATA:
        size = 2 +
          //sizeof (m_type) +
          //sizeof (m_dataRate) +
          sizeof (m_duration) +
          sizeof (Mac48Address) * 3 +
          sizeof (m_sequence) +
          sizeof (m_mbf) +
          sizeof (m_interleaver) +
          //sizeof (m_qos) +
          sizeof (m_checksum);
        if (m_macProtocol == 1)
          size += sizeof (m_beaconRep);
        break;
      case RESCUE_PHY_PKT_TYPE_E2E_ACK:
        size = 2 +
          //sizeof (m_type) +
          //sizeof (m_subtype) +
          sizeof (m_blockAck) + 
          sizeof (m_continousAck) +
          sizeof (Mac48Address) * 2 +
          sizeof (m_sequence) +
          sizeof (m_checksum);
        break;
      case RESCUE_PHY_PKT_TYPE_PART_ACK:
        //currently not supported
        break;
    case RESCUE_PHY_PKT_TYPE_B:
        size = 2 +
          sizeof (m_timestamp) +
          sizeof (Mac48Address) +
          sizeof (m_beaconInterval) +
          sizeof (m_cfpPeriod) +
          sizeof (m_tdmaTimeSlot) +
          sizeof (Mac48Address) * m_schedListSize +
          sizeof (m_checksum);
      break;
    case RESCUE_PHY_PKT_TYPE_RR:
        size = 2 +
          //sizeof (m_type) +
          //sizeof (m_dataRate) +
          sizeof (m_duration) +
          sizeof (m_nextDuration) +
          sizeof (Mac48Address) * 2 +
          sizeof (m_sequence) +
          sizeof (m_mbf) +
          sizeof (m_interleaver) +
          //sizeof (m_qos) +
          sizeof (m_checksum);
        break;
      break;
    }
  return size;
}



// ------------------------ Inherrited methods -----------------------------
uint32_t
RescuePhyHeader::GetSerializedSize (void) const
{
  return GetSize ();
}

void
RescuePhyHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU16 (GetFrameControl ());
  //i.WriteHtolsbU16 (m_duration);
  switch (m_type)
    {
    case RESCUE_PHY_PKT_TYPE_DATA:
      i.WriteU16 (m_duration);
      if (m_macProtocol == 1)
        i.WriteU32 (m_beaconRep);
      WriteTo (i, m_srcAddr);
      WriteTo (i, m_senderAddr);
      WriteTo (i, m_dstAddr);
      i.WriteU16 (m_sequence);
      i.WriteU16 (m_mbf);
      i.WriteU8 (m_interleaver);
      //i.WriteU8 (m_qos);
      break;
    case RESCUE_PHY_PKT_TYPE_E2E_ACK:
      i.WriteU16 (m_blockAck);
      i.WriteU8 (m_continousAck);
      WriteTo (i, m_srcAddr);
      WriteTo (i, m_dstAddr);
      i.WriteU16 (m_sequence);
      break;
    case RESCUE_PHY_PKT_TYPE_PART_ACK:
      //currently not supported
      break;
    case RESCUE_PHY_PKT_TYPE_B:
      i.WriteU32 (m_timestamp);
      WriteTo (i, m_srcAddr);
      i.WriteU32 (m_beaconInterval);
      i.WriteU32 (m_cfpPeriod);
      i.WriteU32 (m_tdmaTimeSlot);
      for (int j=0; j<m_schedListSize; j++)
        WriteTo (i, m_schedulesList[j]);
      break;
    case RESCUE_PHY_PKT_TYPE_RR:
      i.WriteU16 (m_duration);
      i.WriteU16 (m_nextDuration);
      WriteTo (i, m_srcAddr);
      WriteTo (i, m_dstAddr);
      i.WriteU16 (m_sequence);
      i.WriteU16 (m_mbf);
      i.WriteU8 (m_interleaver);
      //i.WriteU8 (m_qos);
      break;
    }
  i.WriteU32 (m_checksum);
}

uint32_t
RescuePhyHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  
  uint16_t frame_control = i.ReadU16 ();
  SetFrameControl (frame_control);
  switch (m_type)
    {
    case RESCUE_PHY_PKT_TYPE_DATA:
      m_duration = i.ReadU16 ();
      if (m_macProtocol == 1)
        m_duration = i.ReadU32 ();
      ReadFrom (i, m_srcAddr);
      ReadFrom (i, m_senderAddr);
      ReadFrom (i, m_dstAddr);
      m_sequence = i.ReadU16 ();
      m_mbf = i.ReadU16 ();
      m_interleaver = i.ReadU8 ();
      //m_qos = i.ReadU8 ();
      break;
    case RESCUE_PHY_PKT_TYPE_E2E_ACK:
      m_blockAck = i.ReadU16 ();
      m_continousAck = i.ReadU8 ();
      ReadFrom (i, m_srcAddr);
      ReadFrom (i, m_dstAddr);
      m_sequence = i.ReadU16 ();
      break;
    case RESCUE_PHY_PKT_TYPE_PART_ACK:
      //currently not supported
      break;
    case RESCUE_PHY_PKT_TYPE_B:
      m_timestamp = i.ReadU32 ();
      ReadFrom (i, m_srcAddr);
      m_beaconInterval = i.ReadU32 ();
      m_cfpPeriod = i.ReadU32 ();
      m_tdmaTimeSlot = i.ReadU32 ();
      for (int j=0; j<m_schedListSize; j++)
        ReadFrom (i, m_schedulesList[j]);
      break;
    case RESCUE_PHY_PKT_TYPE_RR:
      m_duration = i.ReadU16 ();
      m_nextDuration = i.ReadU16 ();
      ReadFrom (i, m_srcAddr);
      ReadFrom (i, m_dstAddr);
      m_sequence = i.ReadU16 ();
      m_mbf = i.ReadU16 ();
      m_interleaver = i.ReadU8 ();
      //m_qos = i.ReadU8 ();
      break;
    }
  m_checksum = i.ReadU32 ();  

  return i.GetDistanceFrom (start);
}

void
RescuePhyHeader::Print (std::ostream &os) const
{
  os << "RESCUE src=" << m_srcAddr << " sender=" << m_senderAddr << " dest=" << m_dstAddr << " type=" << (uint32_t) m_type;
}


} // namespace ns3

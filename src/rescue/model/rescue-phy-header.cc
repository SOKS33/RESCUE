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

#include "ns3/address-utils.h"
#include "rescue-phy-header.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("RescuePhyHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RescuePhyHeader);

RescuePhyHeader::RescuePhyHeader ()
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

RescuePhyHeader::RescuePhyHeader (const Mac48Address srcAddr, const Mac48Address dstAddr, uint8_t type)
  : Header (),
    m_type (type),
    m_srcAddr (srcAddr),
    m_dstAddr (dstAddr)
{
}

RescuePhyHeader::RescuePhyHeader (uint8_t type)
  : Header (),
    m_type (type)
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
RescuePhyHeader::SetLength (uint8_t length)
{
  m_length = length;
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
RescuePhyHeader::SetInterleaver (uint8_t il)
{
  m_interleaver = il;
}
void
RescuePhyHeader::SetQoS (uint8_t qos)
{
  m_qos = qos;
}
void
RescuePhyHeader::SetChecksum (uint32_t checksum)
{
  m_checksum = checksum;
}

void
RescuePhyHeader::SetSubtype (uint8_t subtype)
{
  m_subtype = subtype;
}
void
RescuePhyHeader::SetBlockAck (uint8_t blockAck)
{
  m_blockAck = blockAck;
}
void
RescuePhyHeader::SetContinousAck (uint16_t continousAck)
{
  m_continousAck = continousAck;
}

void
RescuePhyHeader::SetFrameControl (uint8_t ctrl)
{
  m_type = ctrl & 0x03;
  switch (m_type)
    {
      case RESCUE_PHY_PKT_TYPE_DATA:
      case RESCUE_PHY_PKT_TYPE_RR:
        m_dataRate = (ctrl >> 2) & 0x3f;
        break;
      case RESCUE_PHY_PKT_TYPE_ACK:
        m_subtype = (ctrl >> 2) & 0x0f;
        m_blockAck = (ctrl >> 6) & 0x3;
        break;
      case RESCUE_PHY_PKT_TYPE_B:
        break;
    }
}



// ------------------------ Get Functions -----------------------------
uint8_t
RescuePhyHeader::GetType (void) const
{
  return m_type;
}
uint8_t
RescuePhyHeader::GetDataRate (void) const
{
  return m_dataRate;
}
uint8_t
RescuePhyHeader::GetLength (void) const
{
  return m_length;
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
uint8_t
RescuePhyHeader::GetInterleaver (void) const
{
  return m_interleaver;
}
uint8_t
RescuePhyHeader::GetQoS (void) const
{
  return m_qos;
}
uint32_t
RescuePhyHeader::GetChecksum (void) const
{
  return m_checksum;
}

uint8_t
RescuePhyHeader::GetSubtype (void) const
{
  return m_subtype;
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

uint8_t
RescuePhyHeader::GetFrameControl (void) const
{
  uint8_t val = 0;
  val |= m_type & 0x3;
  switch (m_type)
    {
      case RESCUE_PHY_PKT_TYPE_DATA:
      case RESCUE_PHY_PKT_TYPE_RR:
        val |= (m_dataRate << 2) & (0x3f << 2);
        break;
      case RESCUE_PHY_PKT_TYPE_ACK:
        val |= (m_subtype << 2) & (0xf << 2);
        val |= (m_blockAck << 6) & (0x3 << 6);
        break;
      case RESCUE_PHY_PKT_TYPE_B:
        break;
    }
  return val;
}

uint32_t 
RescuePhyHeader::GetSize (void) const
{
  uint32_t size = 0;
  switch (m_type)
    {
      case RESCUE_PHY_PKT_TYPE_DATA:
        size = 1 +
          //sizeof (m_type) +
          //sizeof (m_dataRate) +
          sizeof (m_length) +
          sizeof (Mac48Address) * 3 +
          sizeof (m_sequence) +
          sizeof (m_interleaver) +
          sizeof (m_qos) +
          sizeof (m_checksum);
        break;
      case RESCUE_PHY_PKT_TYPE_ACK:
        size = 1 +
          //sizeof (m_type) +
          //sizeof (m_subtype) +
          //sizeof (m_blockAck) + 
          sizeof (m_continousAck) +
          sizeof (Mac48Address) * 2 +
          sizeof (m_sequence) +
          sizeof (m_checksum);
        break;
    case RESCUE_PHY_PKT_TYPE_RR:
        size = 1 +
          //sizeof (m_type) +
          //sizeof (m_dataRate) +
          sizeof (m_length) +
          sizeof (Mac48Address) * 3 +
          sizeof (m_sequence) +
          sizeof (m_qos) +
          sizeof (m_checksum);
        break;
      break;
    case RESCUE_PHY_PKT_TYPE_B:
      //size = sizeof(m_type) + sizeof(m_duration) + sizeof(Mac48Address) * 2;
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
  i.WriteU8 (GetFrameControl ());
  //i.WriteHtolsbU16 (m_duration);
  switch (m_type)
    {
    case RESCUE_PHY_PKT_TYPE_DATA:
      i.WriteU16 (m_length);
      WriteTo (i, m_srcAddr);
      WriteTo (i, m_senderAddr);
      WriteTo (i, m_dstAddr);
      i.WriteU16 (m_sequence);
      i.WriteU8 (m_interleaver);
      i.WriteU8 (m_qos);
      break;
    case RESCUE_PHY_PKT_TYPE_ACK:
      i.WriteU16 (m_continousAck);
      WriteTo (i, m_srcAddr);
      WriteTo (i, m_dstAddr);
      i.WriteU16 (m_sequence);
      break;
    case RESCUE_PHY_PKT_TYPE_RR:
      i.WriteU16 (m_length);
      WriteTo (i, m_srcAddr);
      WriteTo (i, m_senderAddr);
      WriteTo (i, m_dstAddr);
      i.WriteU16 (m_sequence);
      i.WriteU8 (m_qos);
      break;
    case RESCUE_PHY_PKT_TYPE_B:
      break;
    }
  i.WriteU32 (m_checksum);
}

uint32_t
RescuePhyHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  
  uint8_t frame_control = i.ReadU8 ();
  SetFrameControl (frame_control);
  switch (m_type)
    {
    case RESCUE_PHY_PKT_TYPE_DATA:
      m_length = i.ReadU16 ();
      ReadFrom (i, m_srcAddr);
      ReadFrom (i, m_senderAddr);
      ReadFrom (i, m_dstAddr);
      m_sequence = i.ReadU16 ();
      m_interleaver = i.ReadU8 ();
      m_qos = i.ReadU8 ();
      break;
    case RESCUE_PHY_PKT_TYPE_ACK:
      m_continousAck = i.ReadU16 ();
      ReadFrom (i, m_srcAddr);
      ReadFrom (i, m_dstAddr);
      m_sequence = i.ReadU16 ();
      break;
    case RESCUE_PHY_PKT_TYPE_RR:
      m_length = i.ReadU16 ();
      ReadFrom (i, m_srcAddr);
      ReadFrom (i, m_senderAddr);
      ReadFrom (i, m_dstAddr);
      m_sequence = i.ReadU16 ();
      m_qos = i.ReadU8 ();
      break;
    case RESCUE_PHY_PKT_TYPE_B:
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

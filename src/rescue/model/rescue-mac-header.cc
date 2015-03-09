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
#include "rescue-mac-header.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("RescueMacHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RescueMacHeader);

RescueMacHeader::RescueMacHeader ()
{
}

RescueMacHeader::RescueMacHeader (const Mac48Address srcAddr, const Mac48Address dstAddr, uint8_t type)
  : Header (),
    m_srcAddr (srcAddr),
    m_dstAddr (dstAddr),
    m_type (type)
{
}

TypeId
RescueMacHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RescueMacHeader")
    .SetParent<Header> ()
    .AddConstructor<RescueMacHeader> ()
  ;
  return tid;
}

TypeId
RescueMacHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

RescueMacHeader::~RescueMacHeader ()
{}



// ------------------------ Set Functions -----------------------------
void
RescueMacHeader::SetSource (Mac48Address addr)
{
  m_srcAddr = addr;
}
void
RescueMacHeader::SetDestination (Mac48Address addr)
{
  m_dstAddr = addr;
}
void
RescueMacHeader::SetType (uint8_t type)
{
  m_type = type;
}
void
RescueMacHeader::SetSequence (uint16_t seq)
{
  m_sequence = seq;
}



// ------------------------ Get Functions -----------------------------
Mac48Address
RescueMacHeader::GetSource (void) const
{
  return m_srcAddr;
}
Mac48Address
RescueMacHeader::GetDestination (void) const
{
  return m_dstAddr;
}
uint8_t
RescueMacHeader::GetType (void) const
{
  return m_type;
}
uint32_t 
RescueMacHeader::GetSize (void) const
{
  uint32_t size = 0;
  switch (m_type)
    {
    case RESCUE_MAC_PKT_TYPE_DATA:
      size = sizeof(m_type) + sizeof(Mac48Address) * 2 + sizeof(m_sequence);
      break;
    case RESCUE_MAC_PKT_TYPE_ACK:
    case RESCUE_MAC_PKT_TYPE_RR:
    case RESCUE_MAC_PKT_TYPE_B:
      size = sizeof(m_type) + sizeof(Mac48Address) * 2;
      break;
    }
  return size;
}

uint16_t
RescueMacHeader::GetSequence (void) const
{
  return m_sequence;
}



// ------------------------ Inherrited methods -----------------------------
uint32_t
RescueMacHeader::GetSerializedSize (void) const
{
  return GetSize ();
}

void
RescueMacHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 (m_type);
  switch (m_type)
    {
    case RESCUE_MAC_PKT_TYPE_DATA:
      WriteTo (i, m_srcAddr);
      WriteTo (i, m_dstAddr);
      i.WriteU16 (m_sequence);
      break;
    case RESCUE_MAC_PKT_TYPE_ACK:
    case RESCUE_MAC_PKT_TYPE_RR:
    case RESCUE_MAC_PKT_TYPE_B:
      WriteTo (i, m_srcAddr);
      WriteTo (i, m_dstAddr);
      break;
    }
}

uint32_t
RescueMacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  
  m_type = i.ReadU8 ();
  switch (m_type)
    {
    case RESCUE_MAC_PKT_TYPE_DATA:
      ReadFrom (i, m_srcAddr);
      ReadFrom (i, m_dstAddr);
      m_sequence = i.ReadU16 ();
      break;
    case RESCUE_MAC_PKT_TYPE_ACK:
    case RESCUE_MAC_PKT_TYPE_RR:
    case RESCUE_MAC_PKT_TYPE_B:
      ReadFrom (i, m_srcAddr);
      ReadFrom (i, m_dstAddr);
      break;
    }
  
  return i.GetDistanceFrom (start);
}

void
RescueMacHeader::Print (std::ostream &os) const
{
  os << "RESCUE src=" << m_srcAddr << " dest=" << m_dstAddr << " type=" << (uint32_t) m_type;
}


} // namespace ns3

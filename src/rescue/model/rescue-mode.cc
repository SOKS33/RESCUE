/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 AGH Univeristy of Science and Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Lukasz Prasnal <prasnal@kt.agh.edu.pl>
 *
 * Adapted for RESCUE basing on NS-3 Wifi module by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "rescue-mode.h"
#include "ns3/simulator.h"
#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3 {

/**
 * Check if the two RescueModes are identical.
 *
 * \param a RescueMode
 * \param b RescueMode
 * \return true if the two RescueModes are identical,
 *         false otherwise
 */
bool operator == (const RescueMode &a, const RescueMode &b)
{
  return a.GetUid () == b.GetUid ();
}
/**
 * Serialize RescueMode to ostream (human-readable).
 *
 * \param os std::ostream
 * \param mode
 * \return std::ostream
 */
std::ostream & operator << (std::ostream & os, const RescueMode &mode)
{
  os << mode.GetUniqueName ();
  return os;
}
/**
 * Serialize RescueMode from istream (human-readable).
 *
 * \param is std::istream
 * \param mode
 * \return std::istream
 */
std::istream & operator >> (std::istream &is, RescueMode &mode)
{
  std::string str;
  is >> str;
  mode = RescueModeFactory::GetFactory ()->Search (str);
  return is;
}

uint32_t
RescueMode::GetBandwidth (void) const
{
  struct RescueModeFactory::RescueModeItem *item = RescueModeFactory::GetFactory ()->Get (m_uid);
  return item->bandwidth;
}
uint64_t
RescueMode::GetPhyRate (void) const
{
  struct RescueModeFactory::RescueModeItem *item = RescueModeFactory::GetFactory ()->Get (m_uid);
  return item->phyRate;
}
uint64_t
RescueMode::GetDataRate (void) const
{
  struct RescueModeFactory::RescueModeItem *item = RescueModeFactory::GetFactory ()->Get (m_uid);
  return item->dataRate;
}
enum RescueCodeRate
RescueMode::GetCodeRate (void) const
{
  struct RescueModeFactory::RescueModeItem *item = RescueModeFactory::GetFactory ()->Get (m_uid);
  return item->codingRate;
}
uint8_t
RescueMode::GetConstellationSize (void) const
{
  struct RescueModeFactory::RescueModeItem *item = RescueModeFactory::GetFactory ()->Get (m_uid);
  return item->constellationSize;
}
std::string
RescueMode::GetUniqueName (void) const
{
  // needed for ostream printing of the invalid mode
  struct RescueModeFactory::RescueModeItem *item = RescueModeFactory::GetFactory ()->Get (m_uid);
  return item->uniqueUid;
}
uint32_t
RescueMode::GetUid (void) const
{
  return m_uid;
}
enum RescueModulationClass
RescueMode::GetModulationClass () const
{
  struct RescueModeFactory::RescueModeItem *item = RescueModeFactory::GetFactory ()->Get (m_uid);
  return item->modClass;
}
double
RescueMode::GetSpectralEfficiency (void) const
{
  struct RescueModeFactory::RescueModeItem *item = RescueModeFactory::GetFactory ()->Get (m_uid);
  switch (item->codingRate)
    {
    case RESCUE_CODE_RATE_5_6:
      return log2 (item->constellationSize) * 5 / 6;
      break;
    case RESCUE_CODE_RATE_3_4:
      return log2 (item->constellationSize) * 3 / 4;
      break;
    case RESCUE_CODE_RATE_2_3:
      return log2 (item->constellationSize) * 2 / 3;
      break;
    case RESCUE_CODE_RATE_1_2:
      return log2 (item->constellationSize) * 1 / 2;
      break;
    case RESCUE_CODE_RATE_UNDEFINED:
    default:
      return 1;
      break;
    }
}
RescueMode::RescueMode ()
  : m_uid (0)
{
}
RescueMode::RescueMode (uint32_t uid)
  : m_uid (uid)
{
}
RescueMode::RescueMode (std::string name)
{
  *this = RescueModeFactory::GetFactory ()->Search (name);
}

ATTRIBUTE_HELPER_CPP (RescueMode);

RescueModeFactory::RescueModeFactory ()
{
}


RescueMode
RescueModeFactory::CreateRescueMode (std::string uniqueName,
                                     enum RescueModulationClass modClass,
                                     uint32_t bandwidth,
                                     uint32_t dataRate,
                                     enum RescueCodeRate codingRate,
                                     uint8_t constellationSize)
{
  RescueModeFactory *factory = GetFactory ();
  uint32_t uid = factory->AllocateUid (uniqueName);
  RescueModeItem *item = factory->Get (uid);
  item->uniqueUid = uniqueName;
  item->modClass = modClass;
  // The modulation class for this RescueMode must be valid.
  NS_ASSERT (modClass != RESCUE_MOD_CLASS_UNKNOWN);

  item->bandwidth = bandwidth;
  item->dataRate = dataRate;

  item->codingRate = codingRate;

  switch (codingRate)
    {
    case RESCUE_CODE_RATE_5_6:
      item->phyRate = dataRate * 6 / 5;
      break;
    case RESCUE_CODE_RATE_3_4:
      item->phyRate = dataRate * 4 / 3;
      break;
    case RESCUE_CODE_RATE_2_3:
      item->phyRate = dataRate * 3 / 2;
      break;
    case RESCUE_CODE_RATE_1_2:
      item->phyRate = dataRate * 2 / 1;
      break;
    case RESCUE_CODE_RATE_UNDEFINED:
    default:
      item->phyRate = dataRate;
      break;
    }

  // Check for compatibility between modulation class and coding
  // rate. If modulation class is DSSS then coding rate must be
  // undefined, and vice versa. I could have done this with an
  // assertion, but it seems better to always give the error (i.e.,
  // not only in non-optimised builds) and the cycles that extra test
  // here costs are only suffered at simulation setup.
  /*if ((codingRate == RESCUE_CODE_RATE_UNDEFINED) != (modClass == RESCUE_MOD_CLASS_DSSS))
    {
      NS_FATAL_ERROR ("Error in creation of RescueMode named " << uniqueName << std::endl
                                                               << "Code rate must be RESCUE_CODE_RATE_UNDEFINED iff Modulation Class is RESCUE_MOD_CLASS_DSSS");
    }*/

  item->constellationSize = constellationSize;

  return RescueMode (uid);
}

RescueMode
RescueModeFactory::Search (std::string name)
{
  RescueModeItemList::const_iterator i;
  uint32_t j = 0;
  for (i = m_itemList.begin (); i != m_itemList.end (); i++)
    {
      if (i->uniqueUid == name)
        {
          return RescueMode (j);
        }
      j++;
    }

  // If we get here then a matching RescueMode was not found above. This
  // is a fatal problem, but we try to be helpful by displaying the
  // list of RescueModes that are supported.
  NS_LOG_UNCOND ("Could not find match for RescueMode named \""
                 << name << "\". Valid options are:");
  for (i = m_itemList.begin (); i != m_itemList.end (); i++)
    {
      NS_LOG_UNCOND ("  " << i->uniqueUid);
    }
  // Empty fatal error to die. We've already unconditionally logged
  // the helpful information.
  NS_FATAL_ERROR ("");

  // This next line is unreachable because of the fatal error
  // immediately above, and that is fortunate, because we have no idea
  // what is in RescueMode (0), but we do know it is not what our caller
  // has requested by name. It's here only because it's the safest
  // thing that'll give valid code.
  return RescueMode (0);
}

uint32_t
RescueModeFactory::AllocateUid (std::string uniqueUid)
{
  uint32_t j = 0;
  for (RescueModeItemList::const_iterator i = m_itemList.begin ();
       i != m_itemList.end (); i++)
    {
      if (i->uniqueUid == uniqueUid)
        {
          return j;
        }
      j++;
    }
  uint32_t uid = m_itemList.size ();
  m_itemList.push_back (RescueModeItem ());
  return uid;
}

struct RescueModeFactory::RescueModeItem *
RescueModeFactory::Get (uint32_t uid)
{
  NS_ASSERT (uid < m_itemList.size ());
  return &m_itemList[uid];
}

RescueModeFactory *
RescueModeFactory::GetFactory (void)
{
  static bool isFirstTime = true;
  static RescueModeFactory factory;
  if (isFirstTime)
    {
      uint32_t uid = factory.AllocateUid ("Invalid-RescueMode");
      RescueModeItem *item = factory.Get (uid);
      item->uniqueUid = "Invalid-RescueMode";
      item->bandwidth = 0;
      item->dataRate = 0;
      item->phyRate = 0;
      item->modClass = RESCUE_MOD_CLASS_UNKNOWN;
      item->constellationSize = 0;
      item->codingRate = RESCUE_CODE_RATE_UNDEFINED;
      isFirstTime = false;
    }
  return &factory;
}

} // namespace ns3

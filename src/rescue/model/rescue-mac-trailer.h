/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA
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
 * Author (wifi module): Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *
 * Adapted for Rescue by: Lukasz Prasnal <prasnal@kt.agh.edu.pl>
 */
#ifndef RESCUE_MAC_TRAILER_H
#define RESCUE_MAC_TRAILER_H

#include "ns3/trailer.h"
#include <stdint.h>

namespace ns3 {



/**
 * The length in octects of the RESCUE MAC FCS field
 */
static const uint16_t RESCUE_MAC_FCS_LENGTH = 4;

/**
 * \ingroup rescue
 *
 * Implements the RESCUE MAC trailer
 */
class RescueMacTrailer : public Trailer
{
public:
  RescueMacTrailer ();
  ~RescueMacTrailer ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
};

} // namespace ns3

#endif /* RESCUE_MAC_TRAILER_H */

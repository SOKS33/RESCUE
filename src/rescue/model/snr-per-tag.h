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
 * basing on NS-3 Wifi module by: 
 * Mathieu Lacage <mathieu.lacage@sophia.inria.fr>, Mirko Banchi <mk.banchi@gmail.com>, Konstantinos Katsaros <dinos.katsaros@gmail.com>
 */

#ifndef SNR_PER_TAG_H
#define SNR_PER_TAG_H

#include "ns3/packet.h"

namespace ns3 {

class Tag;

/**
 * \ingroup rescue
 * \brief Tag keeping information about SNR and PER of a given frame
 */

class SnrPerTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  /**
   * Create a SnrPerTag with the default SNR = 0 and PER = 0 
   */
  SnrPerTag ();

  /**
   * Create a SnrPerTag with the given SNR, PER and BER value
   * \param snr the given SNR value
   * \param per the given PER value
   * \param per the given BER value
   */
  SnrPerTag (double snr, /*double per,*/ double ber);

  /**
   * Set the SNR to the given value.
   *
   * \param per the value of the SNR to set
   */
  void SetSNR (double snr);
  /**
   * Set the PER to the given value.
   *
   * \param per the value of the PER to set
   */
  //void SetPER (double per);
  /**
   * Set the BER to the given value.
   *
   * \param per the value of the BER to set
   */
  void SetBER (double ber);
  /**
   * Return the SNR value.
   *
   * \return the SNR value
   */
  double GetSNR (void) const;
  /**
   * Return the PER value.
   *
   * \return the PER value
   */
  //double GetPER (void) const;
  /**
   * Return the BER value.
   *
   * \return the BER value
   */
  double GetBER (void) const;

  // Inherrited methods
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

private:
  double m_snr;  //!< SNR value
  //double m_per;  //!< PER value
  double m_ber;  //!< BER value
};


}
#endif /* SNR_PER_TAG_H */

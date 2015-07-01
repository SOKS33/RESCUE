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

#include "snr-per-tag.h"
#include "ns3/tag.h"
#include "ns3/double.h"

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(SnrPerTag)
    ;

    TypeId
    SnrPerTag::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::SnrPerTag")
                .SetParent<Tag> ()
                .AddConstructor<SnrPerTag> ()
                .AddAttribute("SNR", "The SNR of the last packet received",
                DoubleValue(0.0),
                MakeDoubleAccessor(&SnrPerTag::GetSNR),
                MakeDoubleChecker<double> ())
                /*.AddAttribute ("PER", "The PER of the last packet received",
                               DoubleValue (0.0),
                               MakeDoubleAccessor (&SnrPerTag::GetPER),
                               MakeDoubleChecker<double> ())*/
                .AddAttribute("BER", "The BER of the last packet received",
                DoubleValue(0.0),
                MakeDoubleAccessor(&SnrPerTag::GetBER),
                MakeDoubleChecker<double> ())
                ;
        return tid;
    }

    TypeId
    SnrPerTag::GetInstanceTypeId(void) const {
        return GetTypeId();
    }

    SnrPerTag::SnrPerTag()
    : m_snr(0),
    //m_per (0),
    m_ber(0) {
    }

    SnrPerTag::SnrPerTag(double snr, /*double per,*/ double ber)
    : m_snr(snr),
    //m_per (per),
    m_ber(ber) {
    }

    uint32_t
    SnrPerTag::GetSerializedSize(void) const {
        return 2 * sizeof (double);
    }

    void
    SnrPerTag::Serialize(TagBuffer i) const {
        i.WriteDouble(m_snr);
        //i.WriteDouble (m_per);
        i.WriteDouble(m_ber);
    }

    void
    SnrPerTag::Deserialize(TagBuffer i) {
        m_snr = i.ReadDouble();
        //m_per = i.ReadDouble ();
        m_ber = i.ReadDouble();
    }

    void
    SnrPerTag::Print(std::ostream &os) const {
        os << "snr=" << m_snr << /*" per=" << m_per <<*/ " ber=" << m_ber;
    }

    void
    SnrPerTag::SetSNR(double snr) {
        m_snr = snr;
    }

    /*void
    SnrPerTag::SetPER (double per)
    {
      m_per = per;
    }*/

    void
    SnrPerTag::SetBER(double ber) {
        m_ber = ber;
    }

    double
    SnrPerTag::GetSNR(void) const {
        return m_snr;
    }

    /*double
    SnrPerTag::GetPER (void) const
    {
      return m_per;
    }*/

    double
    SnrPerTag::GetBER(void) const {
        return m_ber;
    }


}

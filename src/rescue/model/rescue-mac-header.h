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

#ifndef RESCUE_MAC_HEADER_H
#define RESCUE_MAC_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"

#define RESCUE_MAC_PKT_TYPE_DATA  0 //data frame
#define RESCUE_MAC_PKT_TYPE_ACK   1 //acknowledgment
#define RESCUE_MAC_PKT_TYPE_RR    2 //resource reservation
#define RESCUE_MAC_PKT_TYPE_B     3 //beacon

namespace ns3 {

    /**
     * \ingroup rescue
     *
     * Implements the Rescue MAC header ...do we need it?
     */
    class RescueMacHeader : public Header {
    public:
        RescueMacHeader(void);

        RescueMacHeader(const Mac48Address srcAddr, const Mac48Address dstAddr, uint8_t type);
        virtual ~RescueMacHeader(void);

        static TypeId GetTypeId(void);

        /**
         * \param type the type field
         */
        void SetType(uint8_t type);
        /**
         * Set the Retry bit in the Frame Control field.
         */
        void SetRetry(void);
        /**
         * Un-set the Retry bit in the Frame Control field.
         */
        void SetNoRetry(void);

        /**
         * \param addr the source address field
         */
        void SetFrameControl(uint8_t ctrl);

        void SetSource(Mac48Address addr);
        /**
         * \param addr the destination address field
         */
        void SetDestination(Mac48Address addr);
        /**
         * \param seq the sequence number field
         */
        void SetSequence(uint16_t seq);

        /**
         * \return type the type field value
         */
        uint8_t GetType(void) const;
        /**
         * Return if the Retry bit is set.
         *
         * \return true if the Retry bit is set, false otherwise
         */
        bool IsRetry(void) const;

        uint8_t GetFrameControl(void) const;

        /**
         * \return addr the source address field value
         */
        Mac48Address GetSource(void) const;
        /**
         * \return addr the destination address field value
         */
        Mac48Address GetDestination(void) const;
        /**
         * \return seq the sequence number field value
         */
        uint16_t GetSequence(void) const;
        /**
         * \return the size of this header
         */
        uint32_t GetSize(void) const;

        // Inherrited methods
        virtual uint32_t GetSerializedSize(void) const;
        virtual void Serialize(Buffer::Iterator start) const;
        virtual uint32_t Deserialize(Buffer::Iterator start);
        virtual void Print(std::ostream &os) const;
        virtual TypeId GetInstanceTypeId(void) const;

    private:
        uint8_t m_type; //<! type field
        uint8_t m_retry; //<! retry flag
        Mac48Address m_srcAddr; //<! source address field
        Mac48Address m_dstAddr; //<! destination address field
        uint16_t m_sequence; //<! sequence number field
    };

}

#endif // RESCUE_MAC_HEADER_H

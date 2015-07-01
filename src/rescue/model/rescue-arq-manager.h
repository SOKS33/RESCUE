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
 */

#ifndef RESCUE_ARQ_MANAGER_H
#define RESCUE_ARQ_MANAGER_H

#include <stdint.h>
#include <map>
#include "ns3/mac48-address.h"

#include "rescue-phy-header.h"
#include "rescue-mac-header.h"

namespace ns3 {

    /**
     * \ingroup rescue
     *
     * Handles advanced ARQ functions
     */
    class RescueArqManager : public Object {
    public:
        RescueArqManager();
        ~RescueArqManager();

        /**
         * Return the next sequence number for the given header.
         *
         * \param hdr rescue PHY header
         * \return the next sequence number
         */
        uint16_t GetNextSequenceNumber(const RescuePhyHeader *hdr);
        /**
         * Return the next sequence number for the given header.
         *
         * \param hdr rescue MAC header
         * \return the next sequence number
         */
        uint16_t GetNextSequenceNumber(const RescueMacHeader *hdr);
        /**
         * Return the next sequence number for the given destination.
         *
         * \param addr destination address
         * \return the next sequence number
         */
        uint16_t GetNextSeqNumberByAddress(Mac48Address addr) const;

    private:
        std::map <Mac48Address, uint16_t> m_sequences;
    };

} // namespace ns3

#endif /* RESCUE_ARQ_MANAGER_H */

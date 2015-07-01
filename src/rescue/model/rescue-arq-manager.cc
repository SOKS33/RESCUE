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
 */

#include "ns3/assert.h"

#include "rescue-arq-manager.h"

namespace ns3 {

    RescueArqManager::RescueArqManager() {
    }

    RescueArqManager::~RescueArqManager() {
    }

    uint16_t
    RescueArqManager::GetNextSequenceNumber(const RescuePhyHeader *hdr) {
        uint16_t retval;
        std::map<Mac48Address, uint16_t>::iterator it = m_sequences.find(hdr->GetDestination());
        if (it != m_sequences.end()) {
            retval = it->second;
            it->second++;
        } else {
            retval = 0;
            std::pair <Mac48Address, uint16_t> newSeq(hdr->GetDestination(), 0);
            std::pair < std::map<Mac48Address, uint16_t>::iterator, bool> newIns = m_sequences.insert(newSeq);
            NS_ASSERT(newIns.second == true);
            newIns.first->second++;
        }

        return retval;
    }

    uint16_t
    RescueArqManager::GetNextSequenceNumber(const RescueMacHeader *hdr) {
        uint16_t retval;
        std::map<Mac48Address, uint16_t>::iterator it = m_sequences.find(hdr->GetDestination());
        if (it != m_sequences.end()) {
            retval = it->second;
            it->second++;
        } else {
            retval = 0;
            std::pair <Mac48Address, uint16_t> newSeq(hdr->GetDestination(), 0);
            std::pair < std::map<Mac48Address, uint16_t>::iterator, bool> newIns = m_sequences.insert(newSeq);
            NS_ASSERT(newIns.second == true);
            newIns.first->second++;
        }

        return retval;
    }

    uint16_t
    RescueArqManager::GetNextSeqNumberByAddress(Mac48Address addr) const {
        uint16_t seq = 0;
        std::map <Mac48Address, uint16_t>::const_iterator it = m_sequences.find(addr);
        if (it != m_sequences.end()) {
            return it->second;
        }
        return seq;
    }

} // namespace ns3

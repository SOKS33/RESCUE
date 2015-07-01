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

#include "ns3/attribute.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/nstime.h"
#include "ns3/random-variable.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/node.h"

#include "sta-rescue-mac.h"
#include "rescue-mac-csma.h"
#include "rescue-phy.h"
#include "rescue-remote-station-manager.h"

NS_LOG_COMPONENT_DEFINE("StaRescueMac");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now() << "] [addr=" << m_address << "] [STA-MAC] "

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(StaRescueMac);

    StaRescueMac::StaRescueMac()
    : RescueMac() {
        NS_LOG_INFO("");
        SetTypeOfStation(STA);
    }

    StaRescueMac::~StaRescueMac() {
        Clear();
        m_remoteStationManager = 0;
        m_csmaMac->Clear();
        m_csmaMac = 0;
    }

    void
    StaRescueMac::DoInitialize() {
        NS_LOG_FUNCTION("");
        m_phy->NotifyCP();
        m_csmaMac->StartOperation();
    }

    void
    StaRescueMac::Clear() {
    }

    TypeId
    StaRescueMac::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::StaRescueMac")
                .SetParent<RescueMac> ()
                .AddConstructor<StaRescueMac> ()
                ;
        return tid;
    }



    // ------------------------ Set Functions -----------------------------



    // ------------------------ Get Functions -----------------------------



    // ----------------------- Queue Functions -----------------------------

    void
    StaRescueMac::Enqueue(Ptr<Packet> pkt, Mac48Address dst) {
        NS_LOG_INFO("enqueue packet to :" << dst <<
                ", size [B]:" << pkt->GetSize());

        RescueMacHeader hdr = RescueMacHeader(m_address, dst, RESCUE_MAC_PKT_TYPE_DATA);
        pkt->AddHeader(hdr);
        m_traceEnqueue(m_device->GetNode()->GetId(), m_device->GetIfIndex(), pkt, hdr);
        m_csmaMac->Enqueue(pkt, dst);
    }



    // ----------------------- Send Functions ------------------------------




    // ---------------------- Receive Functions ----------------------------

    void
    StaRescueMac::ReceivePacket(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
        m_forwardUpCb(pkt, phyHdr.GetSource(), phyHdr.GetDestination());
    }

    void
    StaRescueMac::ReceiveResourceReservation(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
    }

    void
    StaRescueMac::ReceiveBeacon(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
    }

} // namespace ns3

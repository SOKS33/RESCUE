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

#include "ap-rescue-mac.h"
#include "rescue-mac-csma.h"
#include "rescue-mac-tdma.h"
#include "rescue-phy.h"
#include "rescue-remote-station-manager.h"

NS_LOG_COMPONENT_DEFINE("ApRescueMac");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now() << "] [addr=" << m_address << "] [AP-MAC] "

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(ApRescueMac);

    ApRescueMac::ApRescueMac()
    : RescueMac() {
        NS_LOG_INFO("");
        SetTypeOfStation(AP);
        m_tdmaMac = CreateObject<RescueMacTdma> ();
        m_tdmaMac->SetHiMac(this);
    }

    ApRescueMac::~ApRescueMac() {
        Clear();
        m_remoteStationManager = 0;
        m_csmaMac->Clear();
        m_csmaMac = 0;
    }

    void
    ApRescueMac::DoInitialize() {
        NS_LOG_FUNCTION("");
        m_phy->NotifyCP();
        m_csmaMac->StartOperation();
    }

    void
    ApRescueMac::Clear() {
    }

    TypeId
    ApRescueMac::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::ApRescueMac")
                .SetParent<RescueMac> ()
                .AddConstructor<ApRescueMac> ()
                ;
        return tid;
    }



    // ------------------------ Set Functions -----------------------------



    // ------------------------ Get Functions -----------------------------



    // ----------------------- Queue Functions -----------------------------

    void
    ApRescueMac::Enqueue(Ptr<Packet> pkt, Mac48Address dst) {
        NS_LOG_INFO("enqueue packet to :" << dst <<
                ", size [B]:" << pkt->GetSize());

        RescueMacHeader hdr = RescueMacHeader(m_address, dst, RESCUE_MAC_PKT_TYPE_DATA);
        pkt->AddHeader(hdr);
        m_traceEnqueue(m_device->GetNode()->GetId(), m_device->GetIfIndex(), pkt, hdr);
        m_csmaMac->Enqueue(pkt, dst);
    }



    // ----------------------- Send Functions ------------------------------

    void
    ApRescueMac::EndCP() {
        m_phy->NotifyCFP(); //we need to notify PHY about CFP or CP !!!
        m_csmaMac->StopOperation();
        m_tdmaMac->StartOperation();

        //send beacon
        Ptr<Packet> pkt = Create<Packet> (0);
        RescueMacHeader hdr = RescueMacHeader(m_address, m_address, RESCUE_MAC_PKT_TYPE_B);
        pkt->AddHeader(hdr);
        m_traceEnqueue(m_device->GetNode()->GetId(), m_device->GetIfIndex(), pkt, hdr);
        RescuePhyHeader phyHdr = RescuePhyHeader(m_address, m_address, RESCUE_PHY_PKT_TYPE_B);
        m_tdmaMac->SendCtrlNow(pkt, phyHdr);
    }

    void
    ApRescueMac::GenerateRR() {
        Ptr<Packet> pkt = Create<Packet> (0);
        RescueMacHeader hdr = RescueMacHeader(m_address, m_address, RESCUE_MAC_PKT_TYPE_RR);
        pkt->AddHeader(hdr);
        m_traceEnqueue(m_device->GetNode()->GetId(), m_device->GetIfIndex(), pkt, hdr);
        RescuePhyHeader phyHdr = RescuePhyHeader(m_address, m_address, RESCUE_PHY_PKT_TYPE_RR);
        m_csmaMac->EnqueueCtrl(pkt, phyHdr);
    }

    void
    ApRescueMac::SendNow() {
        //methods to trigger transmissions
        //m_tdmaMac->SendData (); -> do not use
        //m_tdmaMac->SendRelayedData (); -> do not use
        m_tdmaMac->ChannelAccessGranted(); //ACK or relayed data or first queued data
        Time t = MicroSeconds(1000);
        //m_tdmaMac->ScheduleACKtx (t); -> do not use
    }


    // ---------------------- Receive Functions ----------------------------

    void
    ApRescueMac::ReceivePacket(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
        m_forwardUpCb(pkt, phyHdr.GetSource(), phyHdr.GetDestination());
    }

    void
    ApRescueMac::ReceiveResourceReservation(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
    }

    void
    ApRescueMac::ReceiveBeacon(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
    }

} // namespace ns3

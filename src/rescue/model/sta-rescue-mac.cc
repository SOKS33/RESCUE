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
#include "rescue-mac-tdma.h"
#include "rescue-phy.h"
#include "rescue-remote-station-manager.h"
#include "rescue-arq-manager.h"

NS_LOG_COMPONENT_DEFINE("StaRescueMac");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now () << "] [addr=" << m_address << "] [STA-MAC] "

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(StaRescueMac);

    StaRescueMac::StaRescueMac()
    : RescueMac() {
        NS_LOG_INFO("");
        SetTypeOfStation(STA);
        m_tdmaMac = CreateObject<RescueMacTdma> ();
        m_tdmaMac->SetHiMac(this);
    }

    StaRescueMac::~StaRescueMac() {
        Clear();
        m_remoteStationManager = 0;
        m_arqManager = 0;
        m_csmaMac->Clear();
        m_csmaMac = 0;
    }

    void
    StaRescueMac::DoInitialize() {
        NS_LOG_FUNCTION("");
        NS_LOG_INFO("AWAITING FOR BEACON");
        m_phy->NotifyCFP();
        m_csmaMac->StopOperation();
        m_tdmaMac->StartOperation();
    }

    void
    StaRescueMac::DoDispose() {
        NS_LOG_FUNCTION("");
        Clear();
        m_csmaMac->Clear();
        m_csmaMac = 0;
        m_tdmaMac->Clear();
        m_tdmaMac = 0;
        m_remoteStationManager = 0;
        m_arqManager = 0;
    }

    void
    StaRescueMac::Clear() {
        NS_LOG_FUNCTION("");
        if (m_beaconTimer.IsRunning())
            m_beaconTimer.Cancel();
        if (m_cfpTimer.IsRunning())
            m_cfpTimer.Cancel();
        for (std::map<uint8_t, Timer>::iterator it = m_slotAwaitingTimers.begin(); it != m_slotAwaitingTimers.end(); it++)
            if (it->second.IsRunning())
                it->second.Cancel();
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
        hdr.SetNoRetry();
        Ptr<Packet> p = pkt->Copy();
        pkt->AddHeader(hdr);

        //if (!m_cfpTimer.IsRunning ())
        //  GenerateResourceReservationFor (pkt);
        if (m_tdmaMac->Enqueue(pkt, dst))
            EnqueueOk(p, hdr);
    }

    void
    StaRescueMac::EnqueueRetry(Ptr<Packet> pkt, Mac48Address dst) {
        NS_LOG_FUNCTION("");
        //if (!m_cfpTimer.IsRunning ())
        //  GenerateResourceReservationFor (pkt);
        m_tdmaMac->EnqueueRetry(pkt, dst);
    }

    void
    StaRescueMac::EnqueueAck(Ptr<Packet> pkt, RescuePhyHeader ackHdr) {
        NS_LOG_FUNCTION("");
        //if (!m_cfpTimer.IsRunning ())
        //  GenerateResourceReservationFor (pkt);
        m_tdmaMac->EnqueueAck(pkt, ackHdr);
    }

    void
    StaRescueMac::NotifyEnqueuedPacket(Ptr<Packet> pkt, Mac48Address dst) {
        NS_LOG_FUNCTION("");
        GenerateResourceReservationFor(pkt);
    }

    // ----------------------- Send Functions ------------------------------

    void
    StaRescueMac::GenerateResourceReservationFor(Ptr<Packet> pktData) {
        NS_LOG_FUNCTION("");
        Ptr<Packet> pkt = Create<Packet> (0);
        RescueMacHeader hdr = RescueMacHeader(m_address, m_apAddress, RESCUE_MAC_PKT_TYPE_RR);
        pkt->AddHeader(hdr);
        //m_traceEnqueue (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt, hdr);
        RescuePhyHeader phyHdr = RescuePhyHeader(m_address, m_apAddress, RESCUE_PHY_PKT_TYPE_RR);

        RescueMacHeader dataHdr;
        pktData->PeekHeader(dataHdr);
        RescueMode nextMode = m_remoteStationManager->GetDataTxMode(dataHdr.GetDestination(), pktData, pktData->GetSize());
        phyHdr.SetNextDataRate(0);
        phyHdr.SetNextDuration(uint16_t((m_tdmaMac->GetDataDuration(pktData, nextMode)).GetMicroSeconds()));

        m_csmaMac->EnqueueCtrl(pkt, phyHdr);
    }

    void
    StaRescueMac::GenerateTdmaTimeSlotReservation() {
        NS_LOG_FUNCTION("");
        Ptr<Packet> pkt = Create<Packet> (0);
        RescueMacHeader hdr = RescueMacHeader(m_address, m_apAddress, RESCUE_MAC_PKT_TYPE_RR);
        pkt->AddHeader(hdr);
        //m_traceEnqueue (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt, hdr);
        RescuePhyHeader phyHdr = RescuePhyHeader(m_address, m_apAddress, RESCUE_PHY_PKT_TYPE_RR);

        //RescueMode nextMode = m_remoteStationManager->GetDataTxMode (hdr.GetDestination (), pktData, pktData->GetSize ());
        phyHdr.SetNextDataRate(0);

        NS_LOG_INFO("GENERATE RESOURCE RESERVATION FRAME");

        m_csmaMac->EnqueueCtrl(pkt, phyHdr);
    }

    void
    StaRescueMac::StartTDMAtimeSlot() {
        NS_LOG_FUNCTION("");
        m_currentSlotTimer.SetDelay(m_tdmaTimeSlot - NanoSeconds(1));
        m_currentSlotTimer.SetFunction(&StaRescueMac::TDMAtimeSlotEnd, this);
        m_currentSlotTimer.Schedule();

        NS_LOG_INFO("TDMA TIME SLOT STARTED");

        m_phy->NotifyCFP();
        m_tdmaMac->StartOperation();
        if (m_tdmaMac->GetNextTxSifsDuration() < m_tdmaTimeSlot)
            m_tdmaMac->ChannelAccessGranted();
    }

    void
    StaRescueMac::NotifySendPacketDone() {
        NS_LOG_FUNCTION("");
        if (m_tdmaMac->GetNextTxSifsDuration() < m_currentSlotTimer.GetDelayLeft())
            m_tdmaMac->ChannelAccessGranted();
    }

    // ---------------------- Receive Functions ----------------------------

    void
    StaRescueMac::ReceivePacket(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
        if (phyHdr.IsBeaconFrame())
            ReceiveBeacon(pkt, phyHdr);
        else if (phyHdr.IsResourceReservationFrame())
            ReceiveResourceReservation(pkt, phyHdr);
        else {
            Ptr<Packet> p = pkt->Copy();
            RxOk(p, phyHdr);
            m_forwardUpCb(pkt, phyHdr.GetSource(), phyHdr.GetDestination());
        }
    }

    void
    StaRescueMac::ReceiveResourceReservation(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
    }

    void
    StaRescueMac::ReceiveBeacon(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
        m_apAddress = phyHdr.GetSource();
        m_supportedDataRates = phyHdr.GetDataRate(); //supported data rates
        m_channel = phyHdr.GetChannel();
        m_txPower = phyHdr.GetTxPower();
        m_schedulesNumber = phyHdr.GetSchedulesListSize();
        //phyHdr.GetTimestamp ();
        m_beaconInterval = MicroSeconds(phyHdr.GetBeaconInterval());
        m_cfpDuration = MicroSeconds(phyHdr.GetCfpPeriod());
        m_tdmaTimeSlot = MicroSeconds(phyHdr.GetTdmaTimeSlot());

        NS_LOG_INFO("BEACON RECEIVED from: " << m_apAddress);
        NS_LOG_FUNCTION("beaconInterval: " << phyHdr.GetBeaconInterval() <<
                "cfpDuration: " << phyHdr.GetCfpPeriod() <<
                "tdmaTimeSlot: " << phyHdr.GetTdmaTimeSlot() <<
                "schedules: " << (int) m_schedulesNumber);

        //start timers
        if (m_beaconTimer.IsRunning()) {
            m_beaconTimer.Cancel();
            BeaconIntervalEnd();
        }
        m_beaconTimer.SetDelay(m_beaconInterval);
        m_beaconTimer.SetFunction(&StaRescueMac::BeaconIntervalEnd, this);
        m_beaconTimer.Schedule();

        if (m_cfpTimer.IsRunning())
            m_cfpTimer.Cancel();
        m_cfpTimer.SetDelay(m_cfpDuration);
        m_cfpTimer.SetFunction(&StaRescueMac::CfpEnd, this);
        m_cfpTimer.Schedule();

        for (uint8_t i = 0; i < m_schedulesNumber; i++)
            if (phyHdr.GetScheduleEntry(i) == m_address) {
                NS_LOG_INFO("TDMA TIME SLOT (no. " << (int) i << ") OBTAINED!");
                //TDMA time slot was assigned, start awaiting counter
                std::pair<uint8_t, Timer> newTimer(i, Timer(Timer::CANCEL_ON_DESTROY));
                std::pair < std::map<uint8_t, Timer>::iterator, bool> newIns = m_slotAwaitingTimers.insert(newTimer);

                if (newIns.first->second.IsRunning())
                    newIns.first->second.Cancel();

                newIns.first->second.SetDelay(m_tdmaMac->GetSifsTime() + i * m_tdmaTimeSlot);
                newIns.first->second.SetFunction(&StaRescueMac::StartTDMAtimeSlot, this);
                newIns.first->second.Schedule();
            }
    }



    // ---------------------- Other Functions ----------------------------

    void
    StaRescueMac::BeaconIntervalEnd() {
        NS_LOG_FUNCTION("");
        NS_LOG_INFO("BEACON INTERVAL ENDED");
        m_phy->NotifyCFP();
        m_csmaMac->StopOperation();
        m_tdmaMac->StartOperation();
    }

    void
    StaRescueMac::CfpEnd() {
        NS_LOG_FUNCTION("");
        NS_LOG_INFO("CFP ENDED");
        m_tdmaMac->ReportQueuedFrames();
        m_phy->NotifyCP();
        m_tdmaMac->StopOperation();
        m_csmaMac->StartOperation(m_beaconTimer.GetDelayLeft());

        //if (m_tdmaMac->GetNextTxSifsDuration () > 0)
        //  GenerateTdmaTimeSlotReservation ();
    }

    void
    StaRescueMac::TDMAtimeSlotEnd() {
        NS_LOG_FUNCTION("");
        NS_LOG_INFO("TDMA TIME SLOT ENDED");
        m_tdmaMac->StopOperation();
    }

    void
    StaRescueMac::NotifyTxAllowed(void) {
        //to be modified
        m_csmaMac->StartOperation();
    }

} // namespace ns3

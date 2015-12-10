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
#include "rescue-arq-manager.h"

NS_LOG_COMPONENT_DEFINE("ApRescueMac");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now () << "] [addr=" << m_address << "] [AP-MAC] "

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
    }

    void
    ApRescueMac::DoInitialize() {
        NS_LOG_FUNCTION("");
        m_phy->NotifyCFP();
        m_csmaMac->StopOperation();
        m_tdmaMac->StartOperation();
        GenerateBeacon();
    }

    void
    ApRescueMac::DoDispose() {
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
    ApRescueMac::Clear() {
        NS_LOG_FUNCTION("");
        if (m_beaconTimer.IsRunning())
            m_beaconTimer.Cancel();
        if (m_cfpTimer.IsRunning())
            m_cfpTimer.Cancel();
        for (std::map<uint8_t, Timer>::iterator it = m_slotAwaitingTimers.begin(); it != m_slotAwaitingTimers.end(); it++)
            if (it->second.IsRunning())
                it->second.Cancel();

        m_schedules.clear();
    }

    TypeId
    ApRescueMac::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::ApRescueMac")
                .SetParent<RescueMac> ()
                .AddConstructor<ApRescueMac> ()
                .AddAttribute("SupportedDataRate",
                "Supported data rates setting (0 - 6Mbps, 1 - 12 Mbps, 2 - 24 Mbps)",
                UintegerValue(0),
                MakeUintegerAccessor(&ApRescueMac::m_supportedDataRates),
                MakeUintegerChecker<uint8_t> ())
                .AddAttribute("Channel",
                "Channel number",
                UintegerValue(0),
                MakeUintegerAccessor(&ApRescueMac::m_channel),
                MakeUintegerChecker<uint8_t> ())
                .AddAttribute("TxPower",
                "TX power setting",
                UintegerValue(0),
                MakeUintegerAccessor(&ApRescueMac::m_txPower),
                MakeUintegerChecker<uint8_t> ())

                .AddAttribute("BeaconInterval",
                "Beacon interval duration",
                TimeValue(MilliSeconds(100)),
                MakeTimeAccessor(&ApRescueMac::m_beaconInterval),
                MakeTimeChecker())
                .AddAttribute("CFPduration",
                "Contention Free Period duration",
                TimeValue(MilliSeconds(70)),
                MakeTimeAccessor(&ApRescueMac::m_cfpDuration),
                MakeTimeChecker())
                .AddAttribute("TDMAtimeSlot",
                "TDMA time slot duration",
                TimeValue(MilliSeconds(10)),
                MakeTimeAccessor(&ApRescueMac::m_tdmaTimeSlot),
                MakeTimeChecker())
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
        hdr.SetNoRetry();
        Ptr<Packet> p = pkt->Copy();
        pkt->AddHeader(hdr);

        //if (!m_cfpTimer.IsRunning ())
        //  GenerateResourceReservationFor (pkt);
        if (m_tdmaMac->Enqueue(pkt, dst))
            EnqueueOk(p, hdr);
    }

    void
    ApRescueMac::EnqueueRetry(Ptr<Packet> pkt, Mac48Address dst) {
        NS_LOG_FUNCTION("");
        //if (!m_cfpTimer.IsRunning ())
        //  GenerateResourceReservationFor (pkt);
        m_tdmaMac->EnqueueRetry(pkt, dst);
    }

    void
    ApRescueMac::EnqueueAck(Ptr<Packet> pkt, RescuePhyHeader ackHdr) {
        NS_LOG_FUNCTION("");
        //if (!m_cfpTimer.IsRunning ())
        //  GenerateResourceReservationFor (pkt);
        m_tdmaMac->EnqueueAck(pkt, ackHdr);
    }

    void
    ApRescueMac::NotifyEnqueuedPacket(Ptr<Packet> pkt, Mac48Address dst) {
        NS_LOG_FUNCTION("");
        GenerateResourceReservationFor(pkt);
    }

    // ----------------------- Send Functions ------------------------------

    void
    ApRescueMac::GenerateBeacon() {
        NS_LOG_FUNCTION("");

        Ptr<Packet> pkt = Create<Packet> (0);
        RescueMacHeader hdr = RescueMacHeader(m_address, m_address, RESCUE_MAC_PKT_TYPE_B);
        pkt->AddHeader(hdr);
        //m_traceEnqueue (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt, hdr);
        RescuePhyHeader phyHdr = RescuePhyHeader(RESCUE_PHY_PKT_TYPE_B);

        phyHdr.SetDataRate(m_supportedDataRates); //supported data rate
        phyHdr.SetQosDisabled();
        phyHdr.SetChannel(m_channel);
        phyHdr.SetTxPower(m_txPower);

        m_schedulesNumber = uint8_t(std::min((int) m_schedules.size(), int(m_cfpDuration.GetMicroSeconds() / m_tdmaTimeSlot.GetMicroSeconds())));
        phyHdr.SetSchedulesListSize(m_schedulesNumber);

        phyHdr.SetTimestamp(ns3::Simulator::Now().GetMicroSeconds());
        phyHdr.SetSource(m_address);
        phyHdr.SetBeaconInterval(m_beaconInterval.GetMicroSeconds());
        //phyHdr.SetCfpPeriod (m_cfpDuration.GetMicroSeconds ());
        phyHdr.SetCfpPeriod(m_schedulesNumber * m_tdmaTimeSlot.GetMicroSeconds());
        phyHdr.SetTdmaTimeSlot(m_tdmaTimeSlot.GetMicroSeconds());

        std::list<Mac48Address>::iterator it = m_schedules.begin();
        if (it != m_schedules.end())
            for (uint8_t i = 0; i < m_schedulesNumber; i++) {
                phyHdr.SetScheduleEntry(i, *it);
                it = m_schedules.erase(it);

                std::map<Mac48Address, Time>::iterator it2 = m_reservedTime.find(*it);
                if (it2->second > m_tdmaTimeSlot)
                    it2->second = it2->second - m_tdmaTimeSlot;
                else
                    it2->second = Seconds(0);
            }

        NS_LOG_INFO("GENERATE BEACON");

        Time beaconTxDuration = m_tdmaMac->GetCtrlDuration(phyHdr, m_remoteStationManager->GetCtrlTxMode());
        m_tdmaMac->SendCtrlNow(pkt, phyHdr);

        NS_LOG_FUNCTION("beaconInterval: " << phyHdr.GetBeaconInterval() <<
                "cfpDuration: " << phyHdr.GetCfpPeriod() <<
                "tdmaTimeSlot: " << phyHdr.GetTdmaTimeSlot() <<
                "schedules: " << (int) m_schedulesNumber <<
                "beacon TX duration: " << beaconTxDuration);

        //start timers
        m_beaconTimer.SetDelay(beaconTxDuration + m_beaconInterval + m_tdmaMac->GetSifsTime());
        m_beaconTimer.SetFunction(&ApRescueMac::BeaconIntervalEnd, this);
        m_beaconTimer.Schedule();

        //m_cfpTimer.SetDelay (beaconTxDuration + m_cfpDuration);
        m_cfpTimer.SetDelay(beaconTxDuration + m_tdmaMac->GetSifsTime() + MicroSeconds(m_schedulesNumber * m_tdmaTimeSlot.GetMicroSeconds()));
        m_cfpTimer.SetFunction(&ApRescueMac::CfpEnd, this);
        m_cfpTimer.Schedule();

        for (uint8_t i = 0; i < m_schedulesNumber; i++)
            if (phyHdr.GetScheduleEntry(i) == m_address) {
                NS_LOG_INFO("TDMA TIME SLOT (no. " << (int) i << ") OBTAINED!");
                //TDMA time slot was assigned, start awaiting counter
                std::pair<uint8_t, Timer> newTimer(i, Timer(Timer::CANCEL_ON_DESTROY));
                std::pair < std::map<uint8_t, Timer>::iterator, bool> newIns = m_slotAwaitingTimers.insert(newTimer);

                if (newIns.first->second.IsRunning())
                    newIns.first->second.Cancel();

                newIns.first->second.SetDelay(beaconTxDuration + m_tdmaMac->GetSifsTime() + i * m_tdmaTimeSlot);
                newIns.first->second.SetFunction(&ApRescueMac::StartTDMAtimeSlot, this);
                newIns.first->second.Schedule();
            }
    }

    /*void
    ApRescueMac::GenerateAggregateResourceReservation ()
    {
      NS_LOG_FUNCTION ("");
      //this stations shouldn't send Resource Reservation frame, just put reservation to internal list
      Time totalTxDuration = m_tdmaMac->GetTotalTxSifsDuration ();

      uint32_t scheduledTdmaSlots = 0;
      for (it = m_schedules.begin (); it != m_schedules.end (); it++)
        if (it == m_address)
          scheduledTdmaSlots++;

      uint32_t newTdmaSlotsNeeded = 0;
      if (scheduledTdmaSlots * m_tdmaTimeSlot > totalTxDuration)
        newTdmaSlotsNeeded = uint32_t ((totalTxDuration - scheduledTdmaSlots * m_tdmaTimeSlot).GetMicroSeconds () / m_tdmaTimeSlot.GetMicroSeconds ());

      for (uint8_t i = 0; i < newTdmaSlotsNeeded; i++)
        m_schedules.push_back (m_address);
    }*/

    void
    ApRescueMac::GenerateResourceReservationFor(Ptr<Packet> pktData) {
        NS_LOG_FUNCTION("");
        //this stations shouldn't send Resource Reservation frame, just put reservation to internal list

        RescueMacHeader dataHdr;
        pktData->PeekHeader(dataHdr);
        RescueMode nextMode = m_remoteStationManager->GetDataTxMode(dataHdr.GetDestination(), pktData, pktData->GetSize());

        NS_LOG_DEBUG("data mode: " << nextMode << " tx time: " << m_tdmaMac->GetDataDuration(pktData, nextMode));

        bool newSlot = MakeResourceReservation(m_tdmaMac->GetDataDuration(pktData, nextMode), m_address);

        //reserve slot for ACKs
        RescueMode ackMode = m_remoteStationManager->GetAckTxMode(dataHdr.GetDestination());
        bool newAckSlot = MakeResourceReservation(m_tdmaMac->GetCtrlDuration(RESCUE_PHY_PKT_TYPE_E2E_ACK, ackMode), dataHdr.GetDestination());

        //if new data slot was reserved but acks dont need new slot, change order of slots to send ACK after DATA
        NS_LOG_DEBUG("newSlot: " << (newSlot ? "YES" : "NO") << ", newACKslot: " << (newAckSlot ? "YES" : "NO"));
        if (newSlot && !newAckSlot) {
            for (std::list<Mac48Address>::iterator it = m_schedules.end(); it != m_schedules.begin(); it--) {
                NS_LOG_DEBUG("current dst: " << *it);
                if (*it == dataHdr.GetDestination()) {
                    NS_LOG_INFO("MOVE TDMA SLOT for: " << dataHdr.GetDestination() << " to the end");
                    m_schedules.splice(m_schedules.end(), m_schedules, it);
                    break;
                }
            }
        }
    }

    void
    ApRescueMac::GenerateTdmaTimeSlotReservation() {
        NS_LOG_FUNCTION("");
        //this stations shouldn't send Resource Reservation frame, just put reservation to internal list
        m_schedules.push_back(m_address);
    }

    void
    ApRescueMac::SendNow() {
        NS_LOG_FUNCTION("");
        //methods to trigger transmissions
        m_tdmaMac->ChannelAccessGranted(); //ACK or relayed data or first queued data
    }

    void
    ApRescueMac::StartTDMAtimeSlot() {
        NS_LOG_FUNCTION("");
        m_currentSlotTimer.SetDelay(m_tdmaTimeSlot - NanoSeconds(1));
        m_currentSlotTimer.SetFunction(&ApRescueMac::TDMAtimeSlotEnd, this);
        m_currentSlotTimer.Schedule();

        NS_LOG_INFO("TDMA TIME SLOT STARTED");

        m_phy->NotifyCFP();
        m_tdmaMac->StartOperation();
        if (m_tdmaMac->GetNextTxSifsDuration() < m_tdmaTimeSlot)
            m_tdmaMac->ChannelAccessGranted();
    }

    void
    ApRescueMac::NotifySendPacketDone() {
        NS_LOG_FUNCTION("");
        if (m_tdmaMac->GetNextTxSifsDuration() < m_currentSlotTimer.GetDelayLeft())
            m_tdmaMac->ChannelAccessGranted();
    }

    // ---------------------- Receive Functions ----------------------------

    void
    ApRescueMac::ReceivePacket(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
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
    ApRescueMac::ReceiveResourceReservation(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
        NS_LOG_INFO("RECEIVE RESOURCE RESERVATION FRAME from: " << phyHdr.GetSource() << ", reserved time: " << phyHdr.GetNextDuration());

        bool newSlot = MakeResourceReservation(MicroSeconds(phyHdr.GetNextDuration()), phyHdr.GetSource());

        //reserve slot for ACKs
        RescueMode ackMode = m_remoteStationManager->GetAckTxMode(phyHdr.GetDestination());
        bool newAckSlot = MakeResourceReservation(m_tdmaMac->GetCtrlDuration(RESCUE_PHY_PKT_TYPE_E2E_ACK, ackMode), phyHdr.GetDestination());

        //if new data slot was reserved but acks dont need new slot, change order of slots to send ACK after DATA
        NS_LOG_DEBUG("newSlot: " << (newSlot ? "YES" : "NO") << ", newACKslot: " << (newAckSlot ? "YES" : "NO"));
        if (newSlot && !newAckSlot
                && (m_schedules.size() * m_tdmaTimeSlot <= m_cfpDuration)) {
            for (std::list<Mac48Address>::iterator it = m_schedules.end(); it != m_schedules.begin(); it--) {
                NS_LOG_DEBUG("current dst: " << *it);
                if (*it == phyHdr.GetDestination()) {
                    NS_LOG_INFO("MOVE TDMA SLOT for: " << phyHdr.GetDestination() << " to the end");
                    m_schedules.splice(m_schedules.end(), m_schedules, it);
                    break;
                }
            }
        }
    }

    bool
    ApRescueMac::MakeResourceReservation(Time duration, Mac48Address address) {
        NS_LOG_FUNCTION("");

        bool retval = false;
        uint32_t scheduledTdmaSlots = 0;
        //uint32_t newTdmaSlotsNeeded = 0;

        duration += m_tdmaMac->GetSifsTime();

        std::map<Mac48Address, Time>::iterator it = m_reservedTime.find(address);
        if (it != m_reservedTime.end()) {
            //it->second = it->second + duration;

            for (std::list<Mac48Address>::iterator it2 = m_schedules.begin(); it2 != m_schedules.end(); it2++)
                if (*it2 == address)
                    scheduledTdmaSlots++;
        } else {
            //new destination
            std::pair<Mac48Address, Time> newAdd(address, Seconds(0));
            std::pair < std::map<Mac48Address, Time>::iterator, bool> newIns = m_reservedTime.insert(newAdd);
            it = newIns.first;
        }

        NS_LOG_FUNCTION(scheduledTdmaSlots << " scheduled tdma slots for: " << address << ", duration of scheduled TX: " << duration << ", expected previous TX time: " << it->second);

        while (scheduledTdmaSlots * m_tdmaTimeSlot < duration + it->second) {
            NS_LOG_INFO("NEW TDMA SLOT FOR: " << address);
            m_schedules.push_back(address);
            scheduledTdmaSlots++;
            retval = true;
        }

        it->second = it->second + duration;

        return retval;
    }

    void
    ApRescueMac::ReceiveBeacon(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
        //don't know what to do
    }



    // ---------------------- Receive Functions ----------------------------

    void
    ApRescueMac::BeaconIntervalEnd() {
        NS_LOG_FUNCTION("");
        NS_LOG_INFO("BEACON INTERVAL ENDED");

        //GenerateAggregateResourceReservation ();

        m_phy->NotifyCFP();
        m_csmaMac->StopOperation();
        m_tdmaMac->StartOperation();
        GenerateBeacon();
    }

    void
    ApRescueMac::CfpEnd() {
        NS_LOG_FUNCTION("");
        NS_LOG_INFO("CFP ENDED");

        m_tdmaMac->ReportQueuedFrames();
        m_phy->NotifyCP();
        m_tdmaMac->StopOperation();
        m_csmaMac->StartOperation(m_beaconTimer.GetDelayLeft() - m_tdmaMac->GetSifsTime());
    }

    void
    ApRescueMac::TDMAtimeSlotEnd() {
        NS_LOG_FUNCTION("");
        NS_LOG_INFO("TDMA TIME SLOT ENDED");
        m_tdmaMac->StopOperation();
    }

    void
    ApRescueMac::NotifyTxAllowed(void) {
        //to be modified
        m_csmaMac->StartOperation();
    }

} // namespace ns3

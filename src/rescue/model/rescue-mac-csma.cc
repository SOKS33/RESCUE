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

#include "ns3/attribute.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/nstime.h"
#include "ns3/random-variable.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/node.h"

#include "ns3/arp-cache.h"
#include "ns3/object-vector.h"
#include "ns3/ipv4-interface.h"

#include "rescue-mac.h"
#include "rescue-phy.h"
#include "rescue-remote-station-manager.h"
#include "rescue-arq-manager.h"
#include "rescue-mac-header.h"
#include "rescue-mac-trailer.h"
#include "rescue-mac-csma.h"
#include "snr-per-tag.h"
#include "ns3/ipv4-static-routing.h"
#include "src/core/model/object.h"
#include "ns3/rescue-mac-header.h"

NS_LOG_COMPONENT_DEFINE("RescueMacCsma");

std::string COLOR_RED = "\x1b[31m";
std::string COLOR_GREEN = "\x1b[32m";
std::string COLOR_YELLOW = "\x1b[33m";
std::string COLOR_BLUE = "\x1b[34m";
std::string COLOR_DEFAULT = "\x1b[0m";

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now().GetSeconds() << "] [addr=" << ((m_hiMac != 0) ? compressMac(m_hiMac->GetAddress ()) : 0) << "] [MAC CSMA] "

namespace ns3 {


    NS_OBJECT_ENSURE_REGISTERED(RescueMacCsma);

    RescueMacCsma::RescueMacCsma()
    : m_phy(0),
    m_hiMac(0),
    m_arqManager(0),
    m_ccaTimeoutEvent(),
    m_backoffTimeoutEvent(),
    m_ackTimeoutEvent(),
    m_sendAckEvent(),
    m_sendDataEvent(),
    m_state(IDLE),
    m_pktTx(0),
    m_pktData(0) {
        //NS_LOG_FUNCTION (this);
        m_cw = m_cwMin;
        m_backoffRemain = Seconds(0);
        m_backoffStart = Seconds(0);
        m_sequence = 0;
        m_interleaver = 0;
        m_random = CreateObject<UniformRandomVariable> ();
    }

    RescueMacCsma::~RescueMacCsma() {
    }

    void
    RescueMacCsma::DoDispose() {
        Clear();
        m_remoteStationManager = 0;
        m_arqManager = 0;
    }

    void
    RescueMacCsma::Clear() {
        m_pktTx = 0;
        NS_LOG_DEBUG("RESET PACKET");
        m_pktData = 0;
        m_pktQueue.clear();
        m_seqList.clear();
    }

    RescueMacCsma::FrameInfo::FrameInfo(uint8_t il,
            bool ACKed,
            bool retried)
    : il(il),
    ACKed(ACKed),
    retried(retried) {
    }

    TypeId
    RescueMacCsma::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::RescueMacCsma")
                .SetParent<Object> ()
                .AddConstructor<RescueMacCsma> ()
                .AddAttribute("CwMin",
                "Minimum value of CW",
                UintegerValue(15),
                MakeUintegerAccessor(&RescueMacCsma::SetCwMin),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("CwMax",
                "Maximum value of CW",
                UintegerValue(1024),
                MakeUintegerAccessor(&RescueMacCsma::SetCwMax),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("SlotTime",
                "Time slot duration [us] for MAC backoff",
                TimeValue(MicroSeconds(15)),
                MakeTimeAccessor(&RescueMacCsma::m_slotTime),
                MakeTimeChecker())
                .AddAttribute("SifsTime",
                "Short Inter-frame Space [us]",
                TimeValue(MicroSeconds(30)),
                MakeTimeAccessor(&RescueMacCsma::m_sifs),
                MakeTimeChecker())
                .AddAttribute("LifsTime",
                "Long Inter-frame Space [us]",
                TimeValue(MicroSeconds(60)),
                MakeTimeAccessor(&RescueMacCsma::m_lifs),
                MakeTimeChecker())
                .AddAttribute("QueueLimits",
                "Maximum packets to queue at MAC",
                UintegerValue(20),
                MakeUintegerAccessor(&RescueMacCsma::m_queueLimit),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("BasicAckTimeout",
                "ACK awaiting time",
                TimeValue(MicroSeconds(3000)),
                MakeTimeAccessor(&RescueMacCsma::m_basicAckTimeout),
                MakeTimeChecker())

                /*.AddTraceSource ("AckTimeout",
                                 "Trace Hookup for ACK Timeout",
                                 MakeTraceSourceAccessor (&RescueMacCsma::m_traceAckTimeout))
                .AddTraceSource ("SendDataDone",
                                 "Trace Hookup for sending a DATA",
                                 MakeTraceSourceAccessor (&RescueMacCsma::m_traceSendDataDone))
                .AddTraceSource ("Enqueue",
                                 "Trace Hookup for enqueue a DATA",
                                 MakeTraceSourceAccessor (&RescueMacCsma::m_traceEnqueue))

                .AddTraceSource ("DataTx",
                                 "Trace Hookup for originated DATA TX",
                                 MakeTraceSourceAccessor (&RescueMacCsma::m_traceDataTx))
                .AddTraceSource ("DataRelay",
                                 "Trace Hookup for DATA relay TX",
                                 MakeTraceSourceAccessor (&RescueMacCsma::m_traceDataRelay))
                .AddTraceSource ("DataRx",
                                 "Trace Hookup for DATA RX by final node",
                                 MakeTraceSourceAccessor (&RescueMacCsma::m_traceDataRx))
                .AddTraceSource ("AckTx",
                                 "Trace Hookup for ACK TX",
                                 MakeTraceSourceAccessor (&RescueMacCsma::m_traceAckTx))
                .AddTraceSource ("ForwardAck",
                                 "Trace Hookup for ACK Forward",
                                 MakeTraceSourceAccessor (&RescueMacCsma::m_traceAckForward))*/

                ;
        return tid;
    }



    // ------------------------ Set Functions -----------------------------

    void
    RescueMacCsma::AttachPhy(Ptr<RescuePhy> phy) {
        //NS_LOG_FUNCTION (this << phy);
        m_phy = phy;
    }

    void
    RescueMacCsma::SetHiMac(Ptr<RescueMac> hiMac) {
        //NS_LOG_FUNCTION (this << hiMac);
        m_hiMac = hiMac;
    }

    void
    RescueMacCsma::SetDevice(Ptr<RescueNetDevice> dev) {
        //NS_LOG_FUNCTION (this << dev);
        m_device = dev;
        SetCw(m_cwMin);
    }

    void
    RescueMacCsma::SetRemoteStationManager(Ptr<RescueRemoteStationManager> manager) {
        //NS_LOG_FUNCTION (this << manager);
        m_remoteStationManager = manager;
        m_remoteStationManager->SetCwMin(m_cwMin);
        m_remoteStationManager->SetCwMax(m_cwMax);
    }

    void
    RescueMacCsma::SetCwMin(uint32_t cw) {
        m_cwMin = cw;
        if (m_remoteStationManager != 0)
            m_remoteStationManager->SetCwMin(m_cwMin);
    }

    void
    RescueMacCsma::SetCwMax(uint32_t cw) {
        m_cwMax = cw;
        if (m_remoteStationManager != 0)
            m_remoteStationManager->SetCwMax(m_cwMax);
    }

    void
    RescueMacCsma::SetCw(uint32_t cw) {
        m_cw = cw;
    }

    void
    RescueMacCsma::SetSlotTime(Time duration) {
        m_slotTime = duration;
    }

    void
    RescueMacCsma::SetSifsTime(Time duration) {
        m_sifs = duration;
    }

    void
    RescueMacCsma::SetLifsTime(Time duration) {
        m_lifs = duration;
    }

    void
    RescueMacCsma::SetQueueLimits(uint32_t length) {
        m_queueLimit = length;
    }

    void
    RescueMacCsma::SetBasicAckTimeout(Time duration) {
        m_basicAckTimeout = duration;
    }

    int64_t
    RescueMacCsma::AssignStreams(int64_t stream) {
        NS_LOG_FUNCTION(this << stream);
        m_random->SetStream(stream);
        return 1;
    }


    // ------------------------ Get Functions -----------------------------

    uint32_t
    RescueMacCsma::GetCw(void) const {
        return m_cw;
    }

    uint32_t
    RescueMacCsma::GetCwMin(void) const {
        return m_cwMin;
    }

    uint32_t
    RescueMacCsma::GetCwMax(void) const {
        return m_cwMax;
    }

    Time
    RescueMacCsma::GetSlotTime(void) const {
        return m_slotTime;
    }

    Time
    RescueMacCsma::GetSifsTime(void) const {
        return m_sifs;
    }

    Time
    RescueMacCsma::GetLifsTime(void) const {
        return m_lifs;
    }

    uint32_t
    RescueMacCsma::GetQueueLimits(void) const {
        return m_queueLimit;
    }

    Time
    RescueMacCsma::GetBasicAckTimeout(void) const {
        return m_basicAckTimeout;
    }

    Time
    RescueMacCsma::GetCtrlDuration(uint16_t type, RescueMode mode) {
        RescueMacHeader hdr = RescueMacHeader(m_hiMac->GetAddress(), m_hiMac->GetAddress(), type);
        return m_phy->CalTxDuration(hdr.GetSize(), 0, m_phy->GetPhyHeaderMode(mode), mode, type);
    }

    Time
    RescueMacCsma::GetDataDuration(Ptr<Packet> pkt, RescueMode mode) {
        return m_phy->CalTxDuration(0, pkt->GetSize(), m_phy->GetPhyHeaderMode(mode), mode, 0);
    }

    std::string
    RescueMacCsma::StateToString(State state) {
        switch (state) {
            case IDLE:
                return "IDLE";
            case BACKOFF:
                return "BACKOFF";
            case WAIT_TX:
                return "WAIT_TX";
            case TX:
                return "TX";
            case WAIT_RX:
                return "WAIT_RX";
            case RX:
                return "RX";
            case COLL:
                return "COLL";
            default:
                return "??";
        }
    }



    // ----------------------- Queue Functions -----------------------------

    bool
    RescueMacCsma::Enqueue(Ptr<Packet> pkt, Mac48Address dst) {
        NS_LOG_FUNCTION(this << pkt << dst);
        NS_LOG_INFO("dst:" << dst <<
                " size [B]:" << pkt->GetSize() <<
                " #Queue:" << m_pktQueue.size() <<
                " #relayQueue:" << m_pktRelayQueue.size() <<
                " #ctrlQueue:" << m_ctrlPktQueue.size() <<
                " #ackQueue:" << m_ackForwardQueue.size() <<
                " state:" << StateToString(m_state));
        if (m_pktQueue.size() >= m_queueLimit) {
            NS_LOG_DEBUG("PACKET QUEUE LIMIT REACHED - DROP FRAME!");
            return false;
        }

        //m_traceEnqueue (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt);
        m_pktQueue.push_back(pkt);

        if (m_state == IDLE) {
            CcaForLifs();
        }

        return true;
    }

    bool
    RescueMacCsma::EnqueueCtrl(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION(this << pkt << phyHdr);
        NS_LOG_INFO("dst:" << phyHdr.GetDestination() <<
                "size [B]:" << pkt->GetSize() <<
                "#Queue:" << m_pktQueue.size() <<
                "#relayQueue:" << m_pktRelayQueue.size() <<
                "#ctrlQueue:" << m_ctrlPktQueue.size() <<
                "#ackQueue:" << m_ackForwardQueue.size() <<
                "state:" << StateToString(m_state));
        if (m_ctrlPktQueue.size() >= m_queueLimit) {
            NS_LOG_DEBUG("CTRL QUEUE LIMIT REACHED - DROP CTRL FRAME!");
            return false;
        }

        //m_traceEnqueue (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt);
        std::pair<Ptr<Packet>, RescuePhyHeader> ctrlPkt(pkt, phyHdr);
        m_ctrlPktQueue.push_back(ctrlPkt);

        if (m_state == IDLE) {
            CcaForLifs();
        }

        return false;
    }

    void
    RescueMacCsma::Dequeue() {
        NS_LOG_FUNCTION(this);
        NS_LOG_INFO("#Queue:" << m_pktQueue.size() <<
                "#relayQueue:" << m_pktRelayQueue.size() <<
                "#ctrlQueue:" << m_ctrlPktQueue.size() <<
                "#ackQueue:" << m_ackForwardQueue.size());
        m_pktQueue.remove(m_pktData);
    }

    /*void
    RescueMacCsma::DequeueCtrl ()
    {
      NS_LOG_FUNCTION ("#queue:" << m_pktQueue.size () <<
                       "#relay queue:" << m_pktRelayQueue.size () <<
                       "#ctrl queue:" << m_ctrlPktQueue.size () <<
                       "#ack queue:" << m_ackForwardQueue.size ());
      m_ctrlPktQueue.remove (m_pktRelay);
    }*/



    // ------------------ Channel Access Functions -------------------------

    //MODIF
    //    std::map<Mac48Address, bool> RescueMacCsma::ControlChannel;

#define CC_ENABLED 1

    bool RescueMacCsma::GetControlChannel() {
        return m_controlChannel;
    }

    bool RescueMacCsma::GetDataChannel() {
        return (m_state == RX || m_state == TX);
    }

    //    void RescueMacCsma::resetChannel() {
    //        NS_LOG_FUNCTION(this);
    //        m_controlChannel = false;
    //        m_state = IDLE;
    //    }

    bool RescueMacCsma::CheckCCForTransmission() {
        if (!CC_ENABLED)
            return true;

        if (!m_controlChannel)
            return true;

        if (m_neighbors.size() >= 2) {
            //            NS_LOG_UNCOND(COLOR_RED << "LOOOOOOOOOOOOOOL " << m_device->GetNode()->GetId() << COLOR_DEFAULT);
            return true;
        }
        return false;
        //        uint32_t count = 0;
        //        for (std::map < Mac48Address, std::pair<bool, bool>>::const_iterator iter = m_neighbors.begin();
        //                iter != m_neighbors.end(); iter++)
        //            if (!iter->second.second)
        //                count++; //Count neighbors with free CC

    }

    void RescueMacCsma::updateNeighborhood() {
        if (!CC_ENABLED)
            return;

        NS_LOG_FUNCTION(this);
        for (uint32_t i = 0; i < m_phy->GetChannel()->GetNDevices(); i++) {
            Ptr<RescueNetDevice> nd = DynamicCast<RescueNetDevice> (m_phy->GetChannel()->GetDevice(i));
            if (!nd)
                NS_FATAL_ERROR("Node doesn't have any RescueNetDevice");
            Ptr<RescueMac> mac = nd->GetMac();
            if (!mac)
                NS_FATAL_ERROR("Node doesn't have MAC CSMA");

            std::map < Mac48Address, std::pair<bool, bool>>::iterator it = m_neighbors.find(mac->GetAddress());
            if (it != m_neighbors.end()) {
                it->second.first = mac->GetCsmaMac()->GetDataChannel();
                it->second.second = mac->GetCsmaMac()->GetControlChannel();
                NS_LOG_DEBUG("Node " << it->first << " -> DC=" << it->second.first << " CC=" << it->second.second);
            }

        }
    }

    bool RescueMacCsma::updateLocalControlChannel() {
        //        NS_LOG_FUNCTION(this);
        bool old = m_controlChannel;

        //My CC is busy if i'm in TX or RX mode
        if (m_state == TX || m_state == RX)
            m_controlChannel = true;
        else {
            //Otherwise, it depends on my neighbors DC state
            uint32_t count = 0;
            for (std::map < Mac48Address, std::pair<bool, bool>>::const_iterator iter = m_neighbors.begin();
                    iter != m_neighbors.end(); iter++)
                if (iter->second.first)
                    count++; //Count neighbors with DC state

            m_controlChannel = (count > 0 ? true : false);
        }
        NS_LOG_DEBUG("Local state DC=" << GetDataChannel() << " CC=" << GetControlChannel() <<
                COLOR_RED << (old != m_controlChannel ? " CHANGED" : "") << COLOR_DEFAULT);

        return (old != m_controlChannel);
    }

    void RescueMacCsma::triggerNeighborhoodUpdate() {
        NS_LOG_FUNCTION(this);
        for (std::map<Mac48Address, Ptr < RescueMacCsma>>::iterator it = m_neighborsPtr.begin();
                it != m_neighborsPtr.end(); it++) {
            NS_LOG_DEBUG(COLOR_YELLOW << "Triggering updateControlChannel for neighbor " << it->first << COLOR_DEFAULT);
            Simulator::ScheduleNow(&RescueMacCsma::TriggeredUpdateControlChannel, it->second);
        }
    }

    void RescueMacCsma::TriggeredUpdateControlChannel() {
        NS_LOG_DEBUG(COLOR_BLUE << "Control Channel update TRIGGERED " << COLOR_DEFAULT);
        updateControlChannel();
    }

    void RescueMacCsma::updateControlChannel() {
        //        NS_LOG_FUNCTION(this);
        if (!CC_ENABLED)
            return;

        updateNeighborhood();
        if (updateLocalControlChannel())
            triggerNeighborhoodUpdate();
    }

    void RescueMacCsma::printTopology() {

        std::ostringstream os;

        os << std::endl << "Topology : " << std::endl;
        for (uint32_t i = 0; i < m_phy->GetChannel()->GetNDevices(); i++) {
            Ptr<RescueNetDevice> nd = DynamicCast<RescueNetDevice> (m_phy->GetChannel()->GetDevice(i));
            if (!nd)
                NS_FATAL_ERROR("Node doesn't have any RescueNetDevice");
            Ptr<RescueMac> mac = nd->GetMac();
            if (!mac)
                NS_FATAL_ERROR("Node doesn't have MAC CSMA");
            os << mac->GetAddress() << " DC=" << mac->GetCsmaMac()->GetDataChannel() <<
                    " CC=" << mac->GetCsmaMac()->GetControlChannel() << std::endl;

        }
        NS_LOG_DEBUG(os.str());
    }

    void RescueMacCsma::printCC() {
        NS_LOG_FUNCTION(this);
        std::ostringstream os;
        os << "Neighbors from node " << this->m_device->GetNode()->GetId() + 1 <<
                " with MAC address " << this->m_hiMac->GetAddress() << std::endl;
        os << "\t myself -> \t\t\t" << " DC=" << (!m_phy->IsIdle() ? "BUSY" : "FREE") <<
                " CC=" << (m_controlChannel ? "BUSY" : "FREE") << std::endl;
        for (std::map < Mac48Address, std::pair<bool, bool>>::const_iterator it = m_neighbors.begin();
                it != m_neighbors.end(); it++)
            os << "\t Neighbor " << it->first << " ->\t" <<
                " DC=" << (it->second.first ? "BUSY" : "FREE") <<
            " CC=" << (it->second.second ? "BUSY" : "FREE") << std::endl;
        NS_LOG_DEBUG(os.str());
    }

    void RescueMacCsma::AddNeighbor(Mac48Address addr) {
        m_neighbors.insert(std::make_pair(addr, std::make_pair(false, false)));

        Ptr<RescueMacCsma> ptr;
        for (uint32_t i = 0; i < m_phy->GetChannel()->GetNDevices(); i++) {
            Ptr<RescueNetDevice> nd = DynamicCast<RescueNetDevice> (m_phy->GetChannel()->GetDevice(i));
            if (!nd)
                NS_FATAL_ERROR("Node doesn't have any RescueNetDevice");
            Ptr<RescueMac> mac = nd->GetMac();
            if (!mac)
                NS_FATAL_ERROR("Node doesn't have MAC CSMA");
            std::map < Mac48Address, std::pair<bool, bool>>::iterator it = m_neighbors.find(mac->GetAddress());
            if (it != m_neighbors.end())
                ptr = mac->GetCsmaMac();
        }
        if (!ptr)
            NS_FATAL_ERROR("Neighbor RescueMacCsma doesn't exist...");

        m_neighborsPtr.insert(std::make_pair(addr, ptr));
        NS_LOG_DEBUG(COLOR_RED << "New neighbor with address " << addr << COLOR_DEFAULT);
    }

    void
    RescueMacCsma::ReceiveBeacon(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION(this);
        if (!CC_ENABLED)
            return;

        AddNeighbor(phyHdr.GetSource());
    }

    void RescueMacCsma::SendBeacon() {
        NS_LOG_FUNCTION(this);

        RescueMacHeader hdr = RescueMacHeader(this->m_hiMac->GetAddress(), this->m_hiMac->GetAddress(), RESCUE_MAC_PKT_TYPE_B);
        hdr.SetNoRetry();
        Ptr<Packet> p = Create<Packet>(0);
        p->AddHeader(hdr);
        RescueMacTrailer fcs;
        p->AddTrailer(fcs);
        RescueMode mode = m_remoteStationManager->GetDataTxMode(hdr.GetDestination(), p, p->GetSize());
        if (SendPacket(p, mode, m_interleaver)) {
            NS_LOG_DEBUG("Beacon sent");
        }
    }

    bool
    RescueMacCsma::StartOperation() {
        NS_LOG_FUNCTION(this);

        if (CC_ENABLED) {
            //MODIF
            m_controlChannel = false;
            NS_LOG_DEBUG("Scheduling beacon in " << Seconds(1 + this->m_device->GetNode()->GetId()).GetSeconds() << " seconds");
            Simulator::Schedule(Seconds(1 + this->m_device->GetNode()->GetId() * 0.1), &RescueMacCsma::SendBeacon, this);
            //        Simulator::Schedule(Seconds(8.8), &RescueMacCsma::resetChannel, this);
            Simulator::Schedule(Seconds(8.9), &RescueMacCsma::updateControlChannel, this);
            Simulator::Schedule(Seconds(9), &RescueMacCsma::printCC, this);
        }

        m_enabled = true;
        m_opEnd = Seconds(0);
        CcaForLifs();
        return true;
    }
    // -------------------------------------------------------------------------

    bool
    RescueMacCsma::StartOperation(Time duration) {
        NS_LOG_FUNCTION(duration);
        m_enabled = true;
        m_opEnd = Simulator::Now() + duration;
        CcaForLifs();
        return true;
    }

    bool
    RescueMacCsma::StopOperation() {
        NS_LOG_FUNCTION(this);
        m_enabled = false;
        m_nopBegin = Seconds(0);
        ChannelBecomesBusy();
        return true;
    }

    bool
    RescueMacCsma::StopOperation(Time duration) {
        NS_LOG_FUNCTION(duration);
        m_enabled = false;
        m_nopBegin = Simulator::Now() + duration;
        ChannelBecomesBusy();
        return true;
    }

    void
    RescueMacCsma::CcaForLifs() {
        if (!m_enabled) {
            return;
        }
        //        NS_LOG_FUNCTION(this);

        //        NS_LOG_INFO("#Queue:" << m_pktQueue.size() <<
        //                " #relayQueue:" << m_pktRelayQueue.size() <<
        //                " #ctrlQueue:" << m_ctrlPktQueue.size() <<
        //                " #ackQueue:" << m_ackForwardQueue.size() <<
        //                " state:" << StateToString(m_state) <<
        //                " phy idle:" << (m_phy->IsIdle() ? "TRUE" : "FALSE"));

        if (((m_pktQueue.size() == 0)
                && (m_pktRelayQueue.size() == 0)
                && (m_ctrlPktQueue.size() == 0)
                && (m_ackForwardQueue.size() == 0))
                || m_ccaTimeoutEvent.IsRunning()) {
            return;
        }
        if (m_state != IDLE || !m_phy->IsIdle()) {
            m_ccaTimeoutEvent = Simulator::Schedule(GetLifsTime(), &RescueMacCsma::CcaForLifs, this);
            return;
        }
        m_ccaTimeoutEvent = Simulator::Schedule(GetLifsTime(), &RescueMacCsma::BackoffStart, this);
    }

    void
    RescueMacCsma::BackoffStart() {
        //        NS_LOG_FUNCTION("BACKOFF remain:" << m_backoffRemain <<
        //                " state:" << StateToString(m_state) <<
        //                " phy idle:" << (m_phy->IsIdle() ? "TRUE" : "FALSE"));
        if (m_state != IDLE || !m_phy->IsIdle()) {
            CcaForLifs();
            return;
        }
        if (m_backoffRemain == Seconds(0)) {
            //CW setting
            if (m_ackForwardQueue.size() != 0) {
                std::pair<Ptr<Packet>, RescuePhyHeader> pktRelay = m_ackForwardQueue.front();
                SetCw(m_remoteStationManager->GetAckCw(pktRelay.second.GetDestination()));
            } else if (m_ctrlPktQueue.size() != 0) {
                std::pair<Ptr<Packet>, RescuePhyHeader> pktRelay = m_ctrlPktQueue.front();
                SetCw(m_remoteStationManager->GetCtrlCw(pktRelay.second.GetDestination()));
            } else if (m_pktRelayQueue.size() != 0) {
                std::pair<Ptr<Packet>, RescuePhyHeader> pktRelay = m_pktRelayQueue.front();
                SetCw(m_remoteStationManager->GetDataCw(pktRelay.second.GetDestination(), pktRelay.first, pktRelay.first->GetSize()));
            } else if ((m_pktQueue.size() != 0) && !m_ackTimeoutEvent.IsRunning()) {
                Ptr<Packet> pktData = m_pktQueue.front();
                RescueMacHeader hdr;
                pktData->PeekHeader(hdr);
                RescueMacTrailer fcs;
                SetCw(m_remoteStationManager->GetDataCw(hdr.GetDestination(), pktData, pktData->GetSize() + fcs.GetSerializedSize()));
            }

            uint32_t slots = m_random->GetInteger(0, m_cw - 1);
            m_backoffRemain = Seconds((double) (slots) * GetSlotTime().GetSeconds());

            //            NS_LOG_DEBUG("BACKOFF(0," << m_cw - 1 << ")=" << slots <<
            //                    ", backoffRemain " << m_backoffRemain.GetSeconds() <<
            //                    ", will finish " << (m_backoffRemain + Simulator::Now()).GetSeconds());
        }
        m_backoffStart = Simulator::Now();
        m_backoffTimeoutEvent = Simulator::Schedule(m_backoffRemain, &RescueMacCsma::ChannelAccessGranted, this);
    }

    void
    RescueMacCsma::ChannelBecomesBusy() {
        NS_LOG_FUNCTION(this);
        if (m_backoffTimeoutEvent.IsRunning()) {
            m_backoffTimeoutEvent.Cancel();
            Time elapse;
            if (Simulator::Now() > m_backoffStart) {
                elapse = Simulator::Now() - m_backoffStart;
            }
            if (elapse < m_backoffRemain) {

                m_backoffRemain = m_backoffRemain - elapse;
                m_backoffRemain = RoundOffTime(m_backoffRemain);
            }
            NS_LOG_DEBUG("Freeze backoff! Remain " << m_backoffRemain);
        }
        CcaForLifs();
    }

    void
    RescueMacCsma::ChannelAccessGranted() {
        NS_LOG_FUNCTION(this);

        for (RelayQueueI it = m_pktRelayQueue.begin(); it != m_pktRelayQueue.end();) {
            if (m_resendAck) break;
            if (CheckRelayedFrame(*it)) {
                it = m_pktRelayQueue.erase(it);
                //NS_LOG_DEBUG("erase!");
            } else {
                it++;
            }
        }

        if (m_ackForwardQueue.size() != 0) {
            m_backoffStart = Seconds(0);
            m_backoffRemain = Seconds(0);

            //MODIF
            m_state = WAIT_TX;
            //            updateControlChannel();

            m_pktRelay = m_ackForwardQueue.front();
            m_ackForwardQueue.pop_front();
            if (!m_resendAck) m_ackCache.push_front(m_pktRelay); //record ACK in cache

            if (m_pktRelay.first == 0)
                NS_ASSERT("Null packet for ack forward tx");

            NS_LOG_INFO("FORWARD ACK!");
            //m_traceAckForward (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), m_pktRelay.first, m_pktRelay.second);

            SendRelayedData();
            m_resendAck = false;
        } else if (m_ctrlPktQueue.size() != 0) {
            m_backoffStart = Seconds(0);
            m_backoffRemain = Seconds(0);

            //MODIF
            m_state = WAIT_TX;
            //            updateControlChannel();

            m_pktRelay = m_ctrlPktQueue.front();
            m_ctrlPktQueue.pop_front();

            if (m_pktRelay.first == 0)
                NS_ASSERT("Null control packet for tx");

            NS_LOG_INFO("TRANSMIT CONTROL FRAME!");
            //m_traceCtrlTx (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), m_pktRelay.first, m_pktRelay.second);

            SendRelayedData();
        } else if (m_pktRelayQueue.size() != 0) {
            m_backoffStart = Seconds(0);
            m_backoffRemain = Seconds(0);

            //MODIF
            m_state = WAIT_TX;
            //            updateControlChannel();

            m_pktRelay = m_pktRelayQueue.front();
            m_pktRelayQueue.pop_front();

            if (m_pktRelay.first == 0)
                NS_ASSERT("Null packet for relay tx");

            NS_LOG_INFO("RELAY DATA FRAME!");
            //m_traceDataRelay (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), m_pktRelay.first, m_pktRelay.second);

            SendRelayedData();
        } else if ((m_pktQueue.size() != 0) && !m_ackTimeoutEvent.IsRunning()) {
            NS_LOG_DEBUG("SEND QUEUED PACKET!");

            m_backoffStart = Seconds(0);
            m_backoffRemain = Seconds(0);

            //MODIF
            m_state = WAIT_TX;
            //            updateControlChannel();

            m_pktData = m_pktQueue.front();
            m_pktQueue.pop_front();
            NS_LOG_INFO("dequeue packet from TX queue, size: " << m_pktData->GetSize());

            if (m_pktData == 0)
                NS_ASSERT("Queue has null packet");

            //m_traceDataTx (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), m_pktData);
            SendData();
        } else {
            if (m_ackTimeoutEvent.IsRunning()) {
                NS_LOG_DEBUG("Awaiting for ACK");
            } else {

                NS_LOG_DEBUG("No queued frames for TX");
            }
        }
    }



    // ----------------------- Send Functions ------------------------------

    void
    RescueMacCsma::SendData() {
        NS_LOG_FUNCTION(this);

        RescueMacHeader hdr;
        m_pktData->PeekHeader(hdr);

        m_interleaver++; //increment interleaver counter

        NS_LOG_DEBUG((hdr.IsRetry() ? "RETRY" : "NOT RETRY"));

        if (!hdr.IsRetry()) {
            m_pktData->RemoveHeader(hdr);
            if (m_arqManager != 0) {
                hdr.SetSequence(m_arqManager->GetNextSequenceNumber(&hdr));
            } else
                hdr.SetSequence(m_sequence);

            m_pktData->AddHeader(hdr);
            RescueMacTrailer fcs;
            m_pktData->AddTrailer(fcs);

            //store info about sended packet
            SendSeqList::iterator it = m_createdFrames.find(hdr.GetDestination());
            if (it != m_createdFrames.end()) {
                //found destination
                SeqList::iterator it2 = dstSeqList(it).seqList.find(hdr.GetSequence());
                if (it2 != dstSeqList(it).seqList.end()) {
                    //found TXed frame with given SEQ - seq. numbers reuse - overwrite
                    seqAck(it2).ACKed = false;
                } else {
                    //new SEQ number for given destination
                    dstSeqList(it).seqList.insert(std::pair <uint16_t, bool> (hdr.GetSequence(), false));
                }
            } else {
                //new destination
                SeqList newSeqList;
                newSeqList.insert(std::pair <uint16_t, bool> (hdr.GetSequence(), false));
                m_createdFrames.insert(std::pair <Mac48Address, SeqList> (hdr.GetDestination(), newSeqList));
            }
        }

        m_pktData->PeekHeader(hdr);
        uint32_t pktSize = m_pktData->GetSize();

        NS_LOG_INFO("dst:" << hdr.GetDestination() <<
                " seq:" << hdr.GetSequence() <<
                " IL:" << (int) m_interleaver <<
                " size:" << pktSize <<
                " retry:" << (hdr.IsRetry() ? "YES" : "NO") <<
                " #Queue:" << m_pktQueue.size() <<
                " #relayQueue:" << m_pktRelayQueue.size() <<
                " #ctrlQueue:" << m_ctrlPktQueue.size() <<
                " #ackQueue:" << m_ackForwardQueue.size());

        //remember to prevent loops
        RecvSendSeqList::iterator it = m_sentFrames.find(std::pair<Mac48Address, Mac48Address> (hdr.GetSource(), hdr.GetDestination()));
        if (it != m_sentFrames.end()) {
            //found source and destination
            SeqIlList::iterator it2 = srcDstSeqList(it).seqList.find(hdr.GetSequence());
            if (it2 != srcDstSeqList(it).seqList.end()) {
                //found TXed frame with given SEQ - seq. numbers reuse - overwrite
                seqIlAck(it2).il = m_interleaver;
                seqIlAck(it2).ACKed = false;
                seqIlAck(it2).retried = hdr.IsRetry();
            } else {
                //new SEQ number for given destination
                srcDstSeqList(it).seqList.insert(std::pair <uint16_t, struct FrameInfo> (hdr.GetSequence(), FrameInfo(m_interleaver, false, hdr.IsRetry())));
            }
        } else {
            //new source and destination
            SeqIlList newSeqList;
            newSeqList.insert(std::pair <uint16_t, struct FrameInfo> (hdr.GetSequence(), FrameInfo(m_interleaver, false, hdr.IsRetry())));
            m_sentFrames.insert(std::pair<std::pair<Mac48Address, Mac48Address>, SeqIlList> (std::pair<Mac48Address, Mac48Address> (hdr.GetSource(), hdr.GetDestination()), newSeqList));
        }

        RescueMode mode = m_remoteStationManager->GetDataTxMode(hdr.GetDestination(), m_pktData, pktSize);

        if (hdr.GetDestination() != m_hiMac->GetBroadcast()) // Unicast
        {
            NS_LOG_DEBUG("pktData total Size: " << m_pktData->GetSize());
            if (SendPacket(m_pktData, mode, m_interleaver)) {
                //Time ackTimeout = GetDataDuration (m_pktData, mode) + GetSifsTime () + GetCtrlDuration (RESCUE_MAC_PKT_TYPE_ACK, mode) + GetSlotTime ();
                Ptr<Packet> p = m_pktData->Copy();
                m_ackTimeoutEvent = Simulator::Schedule(m_basicAckTimeout, &RescueMacCsma::AckTimeout, this, p);
                NS_LOG_DEBUG("ACK TIMEOUT TIMER STARTED");
            } else {
                StartOver(m_pktData);
            }
        } else // Broadcast
        {
            /*if (SendPacket (m_pktData, mode, 0))
              {
              }
            else
              {
                StartOver (m_pktData);
              }*/
        }
    }

    void
    RescueMacCsma::SendRelayedData() {
        NS_LOG_FUNCTION("src:" << m_pktRelay.second.GetSource() <<
                " dst:" << m_pktRelay.second.GetDestination() <<
                " seq:" << m_pktRelay.second.GetSequence());

        RescueMode mode;
        switch (m_pktRelay.second.GetType()) {
            case RESCUE_PHY_PKT_TYPE_DATA:
                mode = m_remoteStationManager->GetDataTxMode(m_pktRelay.second.GetDestination(), m_pktRelay.first, m_pktRelay.first->GetSize());
                break;
            case RESCUE_PHY_PKT_TYPE_E2E_ACK:
            case RESCUE_PHY_PKT_TYPE_PART_ACK:
                mode = m_remoteStationManager->GetAckTxMode(m_pktRelay.second.GetDestination());
                break;
            case RESCUE_PHY_PKT_TYPE_B:
            case RESCUE_PHY_PKT_TYPE_RR:
                mode = m_remoteStationManager->GetCtrlTxMode(m_pktRelay.second.GetDestination());
                break;
        }

        if (m_pktRelay.second.GetDestination() != m_hiMac->GetBroadcast()) // Unicast
        {
            m_pktRelay.second.SetSender(m_hiMac->GetAddress());
            if (SendPacket(m_pktRelay, mode)) {
            } else {
                StartOver(m_pktRelay);
            }
            m_pktRelay.first = 0;
            m_pktRelay.second = 0;
        } else // Broadcast
        {
            if (SendPacket(m_pktRelay, mode)) {
            } else {

                StartOver(m_pktRelay);
            }
            m_pktRelay.first = 0;
            m_pktRelay.second = 0;
        }
    }

    void
    RescueMacCsma::SendAck(Mac48Address dest, uint16_t seq, RescueMode dataTxMode, SnrPerTag tag) {
        NS_LOG_FUNCTION(this << dest << seq << dataTxMode);
        NS_LOG_INFO("SEND ACK");

        Ptr<Packet> pkt = Create<Packet> (0);
        RescuePhyHeader ackHdr = RescuePhyHeader(m_hiMac->GetAddress(), dest, RESCUE_PHY_PKT_TYPE_E2E_ACK);
        ackHdr.SetSequence(seq);
        pkt->AddPacketTag(tag);

        RescueMode mode = m_remoteStationManager->GetAckTxMode(dest, dataTxMode);
        std::pair<Ptr<Packet>, RescuePhyHeader> ackPkt(pkt, ackHdr);

        //m_traceAckTx (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt, ackHdr);

        SendPacket(ackPkt, mode);
    }

    bool
    RescueMacCsma::SendPacket(Ptr<Packet> pkt, RescueMode mode, uint16_t il) {
        NS_LOG_FUNCTION("state:" << StateToString(m_state) << "packet size: " << pkt->GetSize());

        if ((m_state == IDLE || m_state == WAIT_TX)
                && ((m_opEnd == Seconds(0))
                || (Simulator::Now() + GetDataDuration(pkt, mode) + GetSifsTime() < m_opEnd))) {
            if (m_phy->SendPacket(pkt, mode, il)) {

                //MODIF
                m_state = TX;
                if (CC_ENABLED)
                    updateLocalControlChannel();

                m_pktTx = pkt;
                return true;
            } else {

                //MODIF
                m_state = IDLE;
                if (CC_ENABLED)
                    updateLocalControlChannel();
            }
        }
        return false;
    }

    bool
    RescueMacCsma::SendPacket(std::pair<Ptr<Packet>, RescuePhyHeader> relayedPkt, RescueMode mode) {
        NS_LOG_FUNCTION("state:" << StateToString(m_state));

        if ((m_state == IDLE || m_state == WAIT_TX)
                && CheckCCForTransmission()
                && ((m_opEnd == Seconds(0))
                || (Simulator::Now() + GetDataDuration(relayedPkt.first, mode) + GetSifsTime() < m_opEnd))) {
            if (m_phy->SendPacket(relayedPkt, mode)) {
                //MODIF
                m_state = TX;
                if (CC_ENABLED)
                    updateLocalControlChannel();

                m_pktTx = relayedPkt.first;
                return true;
            } else {

                m_state = IDLE;
                if (CC_ENABLED)
                    updateLocalControlChannel();
            }
        }
        return false;
    }

    void
    RescueMacCsma::SendPacketDone(Ptr<Packet> pkt) {
        if (CC_ENABLED)
            printTopology();
        NS_LOG_FUNCTION("state:" << StateToString(m_state));

        if (m_state != TX /*|| m_pktTx != pkt*/) {
            NS_LOG_DEBUG("Something is wrong!");
            return;
        }
        //MODIF
        m_state = IDLE;
        updateControlChannel();

        RescueMacHeader hdr;
        pkt->PeekHeader(hdr);
        switch (hdr.GetType()) {
            case RESCUE_MAC_PKT_TYPE_DATA:
                if (hdr.GetDestination() == m_hiMac->GetBroadcast()) {
                    //don't know what to do
                    SendDataDone();
                    CcaForLifs();
                    return;
                }
                break;
            case RESCUE_MAC_PKT_TYPE_ACK:
            case RESCUE_MAC_PKT_TYPE_RR:
            case RESCUE_MAC_PKT_TYPE_B:
                CcaForLifs();
                break;
            default:
                CcaForLifs();

                break;
        }
    }

    void
    RescueMacCsma::SendDataDone() {

        NS_LOG_FUNCTION(this);
        m_sequence++;
        NS_LOG_DEBUG("RESET PACKET");
        m_pktData = 0;
        m_backoffStart = Seconds(0);
        m_backoffRemain = Seconds(0);
        // According to IEEE 802.11-2007 std (p261)., CW should be reset to minimum value
        // when retransmission reaches limit or when DATA is transmitted successfully
        SetCw(m_cwMin);
        CcaForLifs();
    }

    void
    RescueMacCsma::StartOver(Ptr<Packet> pkt) {

        NS_LOG_FUNCTION(this);
        NS_LOG_INFO("return packet to the front of TX queue");
        m_pktData = 0;
        m_pktQueue.push_front(pkt);
        m_backoffStart = Seconds(0);
        m_backoffRemain = Seconds(0);
        CcaForLifs();
    }

    void
    RescueMacCsma::StartOver(std::pair<Ptr<Packet>, RescuePhyHeader> pktHdr) {
        NS_LOG_FUNCTION(this);
        NS_LOG_INFO("return packet to the front of TX queue");
        switch (pktHdr.second.GetType()) {
            case RESCUE_PHY_PKT_TYPE_DATA:
                m_pktRelayQueue.push_front(pktHdr);
                break;
            case RESCUE_PHY_PKT_TYPE_E2E_ACK:
            case RESCUE_PHY_PKT_TYPE_PART_ACK:
                m_ackForwardQueue.push_front(pktHdr);
                break;
            case RESCUE_PHY_PKT_TYPE_RR:
                m_ctrlPktQueue.push_front(pktHdr);

                break;
            default:
                break;
        }

        m_backoffStart = Seconds(0);
        m_backoffRemain = Seconds(0);
        CcaForLifs();
    }


    // ---------------------- Receive Functions ----------------------------

    void
    RescueMacCsma::ReceiveData(Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode, bool correctData) {
        NS_LOG_FUNCTION(this);
        if (correctData) {
            ReceiveCorrectData(pkt, phyHdr, mode);
        } else //uncorrect data
        {
            //check - have I already received the correct/complete frame?
            RecvSeqList::iterator it = m_receivedFrames.find(phyHdr.GetSource());
            if (it != m_receivedFrames.end()) {
                if (srcSeq(it).seq >= phyHdr.GetSequence()) {
                    NS_LOG_INFO("(unnecessary DATA frame copy) DROP!");
                } else {

                    NS_LOG_INFO("THE FRAME COPY WAS STORED!");
                    //m_phy->StoreFrameCopy (pkt, phyHdr);
                }
            }

        }
    }

    void
    RescueMacCsma::ReceiveCorrectData(Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode) {
        NS_LOG_FUNCTION(this << pkt << phyHdr << mode);

        RescueMacHeader hdr;
        pkt->RemoveHeader(hdr);
        RescueMacTrailer fcs;
        pkt->RemoveTrailer(fcs);
        NS_LOG_FUNCTION(this << "src:" << phyHdr.GetSource() <<
                "sequence: " << phyHdr.GetSequence());

        //MODIF
        m_state = WAIT_TX;
        updateControlChannel();

        NS_LOG_DEBUG(COLOR_GREEN << "RECEIVED CORRECT DATA KIKOOOO" << COLOR_DEFAULT);

        // forward upper layer
        if (IsNewSequence(phyHdr.GetSource(), phyHdr.GetSequence())) {
            NS_LOG_INFO("DATA RX OK!");
            SnrPerTag tag;
            pkt->PeekPacketTag(tag); //to send back SnrPerTag - for possible use in the future (multi rate control etc.)
            m_sendAckEvent = Simulator::Schedule(GetSifsTime(), &RescueMacCsma::SendAck, this, phyHdr.GetSource(), phyHdr.GetSequence(), mode, tag);
            //m_forwardUpCb (pkt, phyHdr.GetSource (), phyHdr.GetDestination ());
            m_hiMac->ReceivePacket(pkt, phyHdr);
        } else {

            NS_LOG_INFO("(duplicate) DROP!");
        }
    }

    void
    RescueMacCsma::ReceiveRelayData(Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode) {
        NS_LOG_FUNCTION(this << " src:" << phyHdr.GetSource() <<
                " dst:" << phyHdr.GetDestination() <<
                " seq:" << phyHdr.GetSequence() <<
                " mode:" << mode);

        //check - have I recently transmitted this frame? have I transmitted an ACK for it?
        bool tx = true;
        RecvSendSeqList::iterator it = m_sentFrames.find(std::pair<Mac48Address, Mac48Address> (phyHdr.GetSource(), phyHdr.GetDestination()));
        if (it != m_sentFrames.end()) {
            //found source and destination
            SeqIlList::iterator it2 = srcDstSeqList(it).seqList.find(phyHdr.GetSequence());
            if (it2 != srcDstSeqList(it).seqList.end()) {
                //found TXed frame with given SEQ - seq. numbers reuse - overwrite
                if ((seqIlAck(it2).ACKed) //ACKed?
                        && (phyHdr.IsRetry())) {
                    //frame was already ACKed and unnecessary retransmission is detected
                    ResendAckFor(std::pair<Ptr<Packet>, RescuePhyHeader> (pkt, phyHdr));
                    tx = false;
                } else if (seqIlAck(it2).il == phyHdr.GetInterleaver()) {
                    //frame was not already ACKed but we transmitted copy of given TX try, just break
                    tx = false;
                }//otherwise - it is new TX try, forward it!
                else {
                    //another retry detected, update interleaver
                    seqIlAck(it2).il = phyHdr.GetInterleaver();
                    seqIlAck(it2).retried = true;
                    if (!phyHdr.IsRetry())
                        NS_ASSERT("new interleaver but no retry!");
                }
            } else {
                //new SEQ number for given destination
                srcDstSeqList(it).seqList.insert(std::pair <uint16_t, struct FrameInfo> (phyHdr.GetSequence(), FrameInfo(phyHdr.GetInterleaver(), false, phyHdr.IsRetry())));
            }
        } else {
            //new source and destination
            SeqIlList newSeqList;
            newSeqList.insert(std::pair <uint16_t, struct FrameInfo> (phyHdr.GetSequence(), FrameInfo(phyHdr.GetInterleaver(), false, phyHdr.IsRetry())));
            m_sentFrames.insert(std::pair<std::pair<Mac48Address, Mac48Address>, SeqIlList> (std::pair<Mac48Address, Mac48Address> (phyHdr.GetSource(), phyHdr.GetDestination()), newSeqList));
        }

        //ask routing - should I forward it?
        /*if (tx)
            {
        //====== PLACE FOR ROUTING INTERACTION ======

            }*/

        if (tx) {
            NS_LOG_INFO("FRAME TO RELAY!");

            //phyHdr.SetSender (m_hiMac->GetAddress ());
            std::pair<Ptr<Packet>, RescuePhyHeader> pktRelay(pkt, phyHdr);
            m_pktRelayQueue.push_back(pktRelay);
        } else {
            NS_LOG_INFO("(DATA already TX-ed) DROP!");
        }

        //MODIF
        m_state = IDLE;
        updateControlChannel();

        CcaForLifs();

        return;
    }

    void
    RescueMacCsma::ReceiveBroadcastData(Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode, bool correctData) {

        NS_LOG_FUNCTION("");

        //don't know what to do
    }

    void
    RescueMacCsma::ReceiveAck(Ptr<Packet> ackPkt, RescuePhyHeader phyHdr, double ackSnr, RescueMode ackMode) {
        NS_LOG_FUNCTION("src:" << phyHdr.GetSource() <<
                "dst:" << phyHdr.GetDestination() <<
                "seq:" << phyHdr.GetSequence());

        //MODIF
        m_state = IDLE;
        updateControlChannel();

        if (phyHdr.GetDestination() == m_hiMac->GetAddress()) {
            //check - have I recently transmitted frame which is acknowledged?
            SendSeqList::iterator it = m_createdFrames.find(phyHdr.GetSource());
            if (it != m_createdFrames.end()) {
                //found destination
                SeqList::iterator it2 = dstSeqList(it).seqList.find(phyHdr.GetSequence());
                if (it2 != dstSeqList(it).seqList.end()) {
                    //found TXed frame with given SEQ - notify frame ACK
                    if (!seqAck(it2).ACKed) {
                        NS_LOG_INFO("GOT ACK - DATA TX OK!");
                        seqAck(it2).ACKed = true;
                        m_ackTimeoutEvent.Cancel();
                        SnrPerTag tag;
                        ackPkt->PeekPacketTag(tag);
                        m_remoteStationManager->ReportDataOk(phyHdr.GetSource(),
                                ackSnr, ackMode,
                                tag.GetSNR(), /*tag.GetPER ()*/ tag.GetBER());
                        //m_traceSendDataDone (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), phyHdr.GetSource (), true);
                        SendDataDone();
                    } else
                        NS_LOG_INFO("DUPLICATED ACK");
                } else {
                    //new SEQ number for given destination???
                    NS_LOG_INFO("ACK ERROR (frame with given seq. no. was not originated here)");
                }
            } else {
                //ACK for other
                NS_LOG_INFO("ACK ERROR (acknowledged frame was not originated here)");
            }
        } else {
            //check - have I recently transmitted frame which is acknowledgded here?
            RecvSendSeqList::iterator it = m_sentFrames.find(std::pair<Mac48Address, Mac48Address> (phyHdr.GetDestination(), phyHdr.GetSource()));
            if (it != m_sentFrames.end()) {
                //found source and destination
                SeqIlList::iterator it2 = srcDstSeqList(it).seqList.find(phyHdr.GetSequence());
                if (it2 != srcDstSeqList(it).seqList.end()) {
                    //found TXed frame with given SEQ
                    if (!seqIlAck(it2).ACKed) //ACKed?
                    {
                        seqIlAck(it2).ACKed = true;

                        //erase queued copies of acked frame
                        bool found = false;
                        for (RelayQueueI it = m_pktRelayQueue.begin(); it != m_pktRelayQueue.end();) {
                            if ((it->second.GetSource() == phyHdr.GetDestination())
                                    && (it->second.GetDestination() == phyHdr.GetSource())
                                    && (it->second.GetSequence() == phyHdr.GetSequence())) {
                                it = m_pktRelayQueue.erase(it);
                                NS_LOG_INFO("Erase unnecessary frame copy!");
                                found = true;
                            } else
                                it++;
                        }

                        NS_LOG_FUNCTION("#queue:" << m_pktQueue.size() <<
                                "#relay queue:" << m_pktRelayQueue.size() <<
                                "#ctrl queue:" << m_ctrlPktQueue.size() <<
                                "#ack queue:" << m_ackForwardQueue.size());

                        /* Forward ACK only if:
                         * - retry frame was detected - links may be corrupted so multipath ACK forwarding may be more reliable
                         * or:
                         * - there was no enqueued copies - probably this station is on the best TX path so it should forward this ACK
                         */
                        if (seqIlAck(it2).retried || !found) {
                            NS_LOG_INFO("ACK to forward!");

                            std::pair<Ptr<Packet>, RescuePhyHeader> forwardAck(ackPkt, phyHdr);
                            m_ackForwardQueue.push_back(forwardAck);
                        }
                    } else {
                        NS_LOG_INFO("(ACK already received) DROP!");
                    }
                } else {
                    //new SEQ number for given destination
                    NS_LOG_INFO("(ACK for frame which was not relayed here) DROP!");
                }
            } else {
                //new source and destination
                NS_LOG_INFO("(ACK for frame which was not relayed here) DROP!");
            }

            CcaForLifs();

            return;
        }
    }

    void
    RescueMacCsma::ReceiveResourceReservation(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {

        NS_LOG_FUNCTION("");
    }

    void
    RescueMacCsma::ReceivePacket(Ptr<RescuePhy> phy, Ptr<Packet> pkt) {
        NS_LOG_FUNCTION(this);
        ChannelBecomesBusy();
        switch (m_state) {
            case WAIT_TX:
            case RX:
            case WAIT_RX:
            case BACKOFF:
            case IDLE:
                //MODIF
                m_state = RX;
                m_controlChannel = false; //re-trigger neighbor update upon reception
                updateControlChannel();

                break;
            case TX:
            case COLL:
                break;
        }
    }

    void
    RescueMacCsma::ReceivePacketDone(Ptr<RescuePhy> phy, Ptr<Packet> pkt, RescuePhyHeader phyHdr,
            double snr, RescueMode mode,
            bool correctPhyHdr, bool correctData, bool wasReconstructed) {
        NS_LOG_FUNCTION(this);

        //MODIF
        m_state = IDLE;
        updateControlChannel();

        if (!correctPhyHdr) {
            NS_LOG_INFO("PHY HDR DAMAGED - DROP!");
            CcaForLifs();
            return;
        } else {
            if (correctData) {
                m_remoteStationManager->ReportRxOk(phyHdr.GetSender(), phyHdr.GetSource(), snr, mode, wasReconstructed);
            } else {
                m_remoteStationManager->ReportRxFail(phyHdr.GetSender(), phyHdr.GetSource(), snr, mode, wasReconstructed);
            }

            if (phyHdr.GetSource() == m_hiMac->GetAddress()) {
                NS_LOG_INFO("FRAME RETURNED TO SOURCE - DROP!");
                CcaForLifs();
                return;
            } else {
                switch (phyHdr.GetType()) {
                    case RESCUE_PHY_PKT_TYPE_DATA:
                        if (phyHdr.GetDestination() == m_hiMac->GetBroadcast()) {
                            NS_LOG_INFO("PHY HDR OK!" <<
                                    ", DATA: " << (correctData ? "OK!" : "DAMAGED!") <<
                                    ", BROADCASTED FRAME!");
                            ReceiveBroadcastData(pkt, phyHdr, mode, correctData);
                        } else if (phyHdr.GetDestination() != m_hiMac->GetAddress()) {
                            NS_LOG_INFO("PHY HDR OK!" <<
                                    ", DATA: " << (correctData ? "PER OK!, FRAME TO RELAY ?!" : "UNUSABLE - DROP!"));
                            if (correctData) {
                                ReceiveRelayData(pkt, phyHdr, mode);
                            } else {
                                CcaForLifs();
                                return;
                            }
                        } else {
                            NS_LOG_INFO("PHY HDR OK!" <<
                                    ", DATA: " << (correctData ? "OK!" : "DAMAGED!") <<
                                    ", RECONSTRUCTION STATUS: " << ((wasReconstructed) ? (correctData ? "SUCCESS" : "MORE COPIES NEEDED") : "NOT NEEDED"));
                            ReceiveData(pkt, phyHdr, mode, correctData);
                        }
                        break;
                    case RESCUE_PHY_PKT_TYPE_E2E_ACK:
                        ReceiveAck(pkt, phyHdr, snr, mode);
                        break;
                    case RESCUE_PHY_PKT_TYPE_PART_ACK:
                        //currently not supported
                        break;
                    case RESCUE_PHY_PKT_TYPE_B:
                        ReceiveBeacon(pkt, phyHdr);
                        break;
                    case RESCUE_PHY_PKT_TYPE_RR:
                        ReceiveResourceReservation(pkt, phyHdr);
                        break;
                    default:
                        CcaForLifs();

                        break;
                }
            }
        }
    }



    // -------------------------- Timeout ----------------------------------

    void
    RescueMacCsma::AckTimeout(Ptr<Packet> pkt) {
        RescueMacHeader hdr;
        pkt->RemoveHeader(hdr);

        NS_LOG_FUNCTION("try: " << m_remoteStationManager->GetRetryCount(hdr.GetDestination()));
        NS_LOG_INFO("!!! ACK TIMEOUT !!!");
        //m_state = IDLE;
        m_remoteStationManager->ReportDataFailed(hdr.GetDestination());
        //m_traceAckTimeout (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt, hdr);

        if (m_remoteStationManager->NeedDataRetransmission(hdr.GetDestination(), pkt)) {
            NS_LOG_INFO("RETRANSMISSION");
            hdr.SetRetry();
            pkt->AddHeader(hdr);
            StartOver(pkt);
            //SendData ();
        } else {
            // Retransmission is over the limit. Drop it!

            NS_LOG_INFO("DATA TX FAIL!");
            m_remoteStationManager->ReportFinalDataFailed(hdr.GetDestination());
            //m_traceSendDataDone (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), hdr.GetDestination (), false);
            SendDataDone();
        }
    }



    // --------------------------- ETC -------------------------------------

    bool
    RescueMacCsma::CheckRelayedFrame(std::pair<Ptr<Packet>, RescuePhyHeader> relayedPkt) {
        NS_LOG_FUNCTION(this);
        bool rx = false;

        //check - have I recently received ACK for this DATA fame copy?
        RecvSendSeqList::iterator it = m_sentFrames.find(std::pair<Mac48Address, Mac48Address> (relayedPkt.second.GetSource(), relayedPkt.second.GetDestination()));
        if (it != m_sentFrames.end()) {
            //found source and destination
            SeqIlList::iterator it2 = srcDstSeqList(it).seqList.find(relayedPkt.second.GetSequence());
            if (it2 != srcDstSeqList(it).seqList.end()) {
                //found TXed frame with given SEQ
                if (seqIlAck(it2).ACKed) //ACKed?
                {
                    NS_LOG_INFO("RELAYED FRAME (src: " << srcDstSeqList(it).src <<
                            ", dst: " << srcDstSeqList(it).dst <<
                            ", seq: " << seqIlAck(it2).seq <<
                            ") was already ACKed, DROP!");
                    rx = true;

                    if (seqIlAck(it2).il != relayedPkt.second.GetInterleaver()) //new unnecessary retry detected
                        ResendAckFor(relayedPkt);
                }
            }
        }

        return rx;
    }

    void
    RescueMacCsma::ResendAckFor(std::pair<Ptr<Packet>, RescuePhyHeader> relayedPkt) {
        NS_LOG_INFO("UNNECESSARY RETRANSMISSION DETECTED!");
        NS_LOG_FUNCTION(relayedPkt.first << relayedPkt.second);

        for (RelayQueueI it = m_ackCache.begin(); it != m_ackCache.end(); it++) {
            if ((it->second.GetSource() == relayedPkt.second.GetDestination())
                    && (it->second.GetDestination() == relayedPkt.second.GetSource())
                    && (it->second.GetSequence() == relayedPkt.second.GetSequence())) {

                NS_LOG_INFO("ACK found in cache, RESEND!");
                m_ackForwardQueue.push_front(*it);
                m_resendAck = true;
            }
        }
    }

    bool
    RescueMacCsma::IsNewSequence(Mac48Address addr, uint16_t seq) {
        NS_LOG_FUNCTION(addr << seq);

        RecvSeqList::iterator it = m_receivedFrames.find(addr);
        if (it != m_receivedFrames.end()) {
            //found source
            if ((seq < 100) && (srcSeq(it).seq >= (65435))) //65535 - 100
            {
                //reuse seq. numbers
                NS_LOG_INFO("resuse seq. numbers");
                srcSeq(it).seq = seq;
                return true;
            } else if (seq > srcSeq(it).seq) {
                //new seq. number
                NS_LOG_INFO("new seq. number");
                srcSeq(it).seq = seq;
                return true;
            } else {
                //old seq. number (duplicate or reorder)
                NS_LOG_INFO("old seq. number (duplicate or reorder)");
                return false;
            }
        } else {
            //new source

            NS_LOG_INFO("new source");
            m_receivedFrames.insert(std::pair<Mac48Address, uint16_t> (addr, seq));
        }
        return true;
    }

    void
    RescueMacCsma::DoubleCw() {
        if (m_cw * 2 > m_cwMax) {
            m_cw = m_cwMax;
        } else {

            m_cw = m_cw * 2;
        }
    }

    // Nodes can start backoff procedure at different time because of propagation
    // delay and processing jitter (it's very small but matter in simulation),

    Time
    RescueMacCsma::RoundOffTime(Time time) {
        int64_t realTime = time.GetMicroSeconds();
        int64_t slotTime = GetSlotTime().GetMicroSeconds();
        if (realTime % slotTime >= slotTime / 2) {
            return Seconds(GetSlotTime().GetSeconds() * (double) (realTime / slotTime + 1));
        } else {
            return Seconds(GetSlotTime().GetSeconds() * (double) (realTime / slotTime));
        }
    }

} // namespace ns3

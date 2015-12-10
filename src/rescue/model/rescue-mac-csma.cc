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

#include "rescue-mac.h"
#include "rescue-phy.h"
#include "rescue-remote-station-manager.h"
#include "rescue-arq-manager.h"
#include "rescue-mac-header.h"
#include "rescue-mac-trailer.h"
#include "rescue-mac-csma.h"
#include "snr-per-tag.h"

#include "rescue-utils.h"

NS_LOG_COMPONENT_DEFINE("RescueMacCsma");

std::string COLOR_RED = "\x1b[31m";
std::string COLOR_GREEN = "\x1b[32m";
std::string COLOR_YELLOW = "\x1b[33m";
std::string COLOR_BLUE = "\x1b[34m";
std::string COLOR_DEFAULT = "\x1b[0m";

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now().GetMicroSeconds() << "] [addr=" << ((m_hiMac != 0) ? compressMac(m_hiMac->GetAddress ()) : 0) << "] [MAC CSMA] "

#define LOG_STR "[time=" << ns3::Simulator::Now().GetMicroSeconds() << "] [addr=" << ((m_hiMac != 0) ? compressMac(m_hiMac->GetAddress ()) : 0) << "] [MAC CSMA] "
namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(RescueMacCsma);

    RescueMacCsma::RescueMacCsma()
    : m_phy(0),
    m_hiMac(0),
    m_ccaTimeoutEvent(),
    m_backoffTimeoutEvent(),
    m_sendAckEvent(),
    m_state(IDLE),
    m_pktTx(0),
    m_pktData(0) {
        NS_LOG_FUNCTION("");
        m_cw = m_cwMin;
        m_backoffRemain = Seconds(0);
        m_backoffStart = Seconds(0);
        m_interleaver = 0;
        m_random = CreateObject<UniformRandomVariable> ();
    }

    RescueMacCsma::~RescueMacCsma() {
        NS_LOG_FUNCTION("");
    }

    void
    RescueMacCsma::DoDispose() {
        NS_LOG_FUNCTION("");
        Clear();
        m_remoteStationManager = 0;
        m_arqManager = 0;
    }

    void
    RescueMacCsma::Clear() {
        NS_LOG_FUNCTION("");
        if (m_ccaTimeoutEvent.IsRunning())
            m_ccaTimeoutEvent.Cancel();
        if (m_backoffTimeoutEvent.IsRunning())
            m_backoffTimeoutEvent.Cancel();
        if (m_sendAckEvent.IsRunning())
            m_sendAckEvent.Cancel();

        m_pktTx = 0;
        NS_LOG_DEBUG("RESET PACKET");
        m_pktData = 0;
        m_pktQueue.clear();
        m_pktRetryQueue.clear();
        m_pktRelayQueue.clear();
        m_ctrlPktQueue.clear();
        m_ackQueue.clear();
        m_ackCache.clear();
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
    RescueMacCsma::SetArqManager(Ptr<RescueArqManager> arqManager) {
        //NS_LOG_FUNCTION (this << arqManager);
        m_arqManager = arqManager;
        //m_arqManager->SetBasicAckTimeout (m_basicAckTimeout);
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
    RescueMacCsma::GetCtrlDuration(uint16_t type, RescueMode mode) {
        RescueMacHeader hdr = RescueMacHeader(m_hiMac->GetAddress(), m_hiMac->GetAddress(), type);
        return m_phy->CalTxDuration(hdr.GetSize(), 0, m_phy->GetPhyHeaderMode(mode), mode, type, false);
    }

    Time
    RescueMacCsma::GetDataDuration(Ptr<Packet> pkt, RescueMode mode) {
        return m_phy->CalTxDuration(0, pkt->GetSize(), m_phy->GetPhyHeaderMode(mode), mode, RESCUE_PHY_PKT_TYPE_DATA, false);
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
    int dropped = 0;

    bool
    RescueMacCsma::Enqueue(Ptr<Packet> pkt, Mac48Address dst) {
        NS_LOG_FUNCTION("dst:" << dst <<
                "size [B]:" << pkt->GetSize() <<
                "#data queue:" << m_pktQueue.size() <<
                "#retry queue:" << m_pktRetryQueue.size() <<
                "#relay queue:" << m_pktRelayQueue.size() <<
                "#ctrl queue:" << m_ctrlPktQueue.size() <<
                "#ack queue:" << m_ackQueue.size() <<
                "state:" << StateToString(m_state));
        if (m_pktQueue.size() >= m_queueLimit) {
            dropped++;
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
    RescueMacCsma::EnqueueRetry(Ptr<Packet> pkt, Mac48Address dst) {
        NS_LOG_FUNCTION("dst:" << dst <<
                "size [B]:" << pkt->GetSize() <<
                "#data queue:" << m_pktQueue.size() <<
                "#retry queue:" << m_pktRetryQueue.size() <<
                "#relay queue:" << m_pktRelayQueue.size() <<
                "#ctrl queue:" << m_ctrlPktQueue.size() <<
                "#ack queue:" << m_ackQueue.size() <<
                "state:" << StateToString(m_state));
        if (m_pktRetryQueue.size() >= m_queueLimit) {
            NS_LOG_DEBUG("PACKET RETRY QUEUE LIMIT REACHED - DROP FRAME!");
            return false;
        }

        //m_traceEnqueue (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt);
        m_pktRetryQueue.push_back(pkt);

        if (m_state == IDLE) {
            CcaForLifs();
        }

        return true;
    }

    bool
    RescueMacCsma::EnqueueRelay(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("dst:" << phyHdr.GetDestination() <<
                "size [B]:" << pkt->GetSize() <<
                "#data queue:" << m_pktQueue.size() <<
                "#retry queue:" << m_pktRetryQueue.size() <<
                "#relay queue:" << m_pktRelayQueue.size() <<
                "#ctrl queue:" << m_ctrlPktQueue.size() <<
                "#ack queue:" << m_ackQueue.size() <<
                "state:" << StateToString(m_state));
        if (m_pktRelayQueue.size() >= m_queueLimit) {
            NS_LOG_DEBUG("PACKET RELAY QUEUE LIMIT REACHED - DROP FRAME!");
            return false;
        }

        //m_traceEnqueue (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt);
        std::pair<Ptr<Packet>, RescuePhyHeader> relayPkt(pkt, phyHdr);
        NS_LOG_INFO("ENQUEUE RELAY DATA FRAME! from src: " << relayPkt.second.GetSource() << " to dst: " << relayPkt.second.GetDestination() << ", seq: " << relayPkt.second.GetSequence());
        m_pktRelayQueue.push_back(relayPkt);

        if (m_state == IDLE) {
            //MODIF3
            if (CC_ENABLED || DISTRIBUTED_ACK_ENABLED)
                CcaForSifs();
            else
                CcaForLifs();
        }

        return true;
    }

    bool
    RescueMacCsma::EnqueueCtrl(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("dst:" << phyHdr.GetDestination() <<
                "size [B]:" << pkt->GetSize() <<
                "#data queue:" << m_pktQueue.size() <<
                "#retry queue:" << m_pktRetryQueue.size() <<
                "#relay queue:" << m_pktRelayQueue.size() <<
                "#ctrl queue:" << m_ctrlPktQueue.size() <<
                "#ack queue:" << m_ackQueue.size() <<
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

        return true;
    }

    bool
    RescueMacCsma::EnqueueAck(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("dst:" << phyHdr.GetDestination() <<
                "size [B]:" << pkt->GetSize() <<
                "#data queue:" << m_pktQueue.size() <<
                "#retry queue:" << m_pktRetryQueue.size() <<
                "#relay queue:" << m_pktRelayQueue.size() <<
                "#ctrl queue:" << m_ctrlPktQueue.size() <<
                "#ack queue:" << m_ackQueue.size() <<
                "state:" << StateToString(m_state));
        if (m_ackQueue.size() >= m_queueLimit) {
            NS_LOG_DEBUG("ACK QUEUE LIMIT REACHED - DROP CTRL FRAME!");
            return false;
        }

        //m_traceEnqueue (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt);
        std::pair<Ptr<Packet>, RescuePhyHeader> ackPkt(pkt, phyHdr);
        m_ackQueue.push_back(ackPkt);

        if (m_state == IDLE) {
            //MODIF3
            if (CC_ENABLED || DISTRIBUTED_ACK_ENABLED)
                CcaForSifs();
            else
                CcaForLifs();
        }

        return true;
    }



    // ------------------ Channel Access Functions -------------------------

    void RescueMacCsma::SetTxDuration(Time txDuration) {
        this->m_txDuration = txDuration;
    }

    Time RescueMacCsma::GetTxDuration() {
        return m_txDuration;
    }

    void RescueMacCsma::SetChannelDelay(Time delay) {
        this->m_channelDelay = delay;
    }

    Time RescueMacCsma::GetChannelDelay() {
        return m_channelDelay;
    }

    bool RescueMacCsma::GetControlChannel() {
        return m_controlChannel;
    }

    bool RescueMacCsma::GetDataChannel() {
        return (m_state == RX || m_state == TX);
    }

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
                if (CC_LOG_ENABLED)
                    NS_LOG_DEBUG("Node " << it->first << " -> DC=" << it->second.first << " CC=" << it->second.second);
            }

        }
    }

    bool RescueMacCsma::updateLocalControlChannel() {
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
        if (CC_LOG_ENABLED)
            NS_LOG_DEBUG("Local state DC=" << GetDataChannel() << " CC=" << GetControlChannel() <<
                COLOR_RED << (old != m_controlChannel ? " CHANGED" : "") << COLOR_DEFAULT);

        return (old != m_controlChannel);
    }

    void RescueMacCsma::triggerNeighborhoodUpdate() {
        for (std::map<Mac48Address, Ptr < RescueMacCsma>>::iterator it = m_neighborsPtr.begin();
                it != m_neighborsPtr.end(); it++) {
            if (CC_LOG_ENABLED)
                NS_LOG_DEBUG(COLOR_YELLOW << "Triggering updateControlChannel for neighbor " << it->first << COLOR_DEFAULT);
            Simulator::ScheduleNow(&RescueMacCsma::TriggeredUpdateControlChannel, it->second);
        }
    }

    void RescueMacCsma::TriggeredUpdateControlChannel() {
        if (CC_LOG_ENABLED)
            NS_LOG_DEBUG(COLOR_BLUE << "Control Channel update TRIGGERED " << COLOR_DEFAULT);
        updateControlChannel();
    }

    void RescueMacCsma::updateControlChannel() {
        if (!CC_ENABLED)
            return;

        updateNeighborhood();
        if (updateLocalControlChannel())
            triggerNeighborhoodUpdate();
    }

    void RescueMacCsma::printTopology() {
        if (!CC_ENABLED)
            return;

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
        if (CC_LOG_ENABLED)
            NS_LOG_DEBUG(os.str());
    }

    void RescueMacCsma::printCC() {
        if (!CC_ENABLED)
            return;

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
        //        if (CC_LOG_ENABLED)
        NS_LOG_DEBUG(os.str());
    }

    void RescueMacCsma::AddNeighbor(Mac48Address addr) {
        if (!CC_ENABLED)
            return;

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
        if (CC_LOG_ENABLED)
            NS_LOG_DEBUG(COLOR_RED << "New neighbor with address " << addr << COLOR_DEFAULT);
    }

    void
    RescueMacCsma::ReceiveBeacon(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        if (CC_ENABLED)
            AddNeighbor(phyHdr.GetSource());
        else
            m_hiMac->ReceiveBeacon(pkt, phyHdr);
    }

    void RescueMacCsma::SendBeacon() {
        if (!CC_ENABLED)
            NS_FATAL_ERROR("Should not send beacon when CC is disabled");

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
        NS_LOG_FUNCTION("");

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
        m_state = IDLE;
        m_opEnd = Seconds(0);
        CcaForLifs();
        return true;
    }
    // -------------------------------------------------------------------------

    bool
    RescueMacCsma::StartOperation(Time duration) {
        NS_LOG_FUNCTION("");
        m_enabled = true;
        m_state = IDLE;
        m_opEnd = Simulator::Now() + duration;
        CcaForLifs();
        return true;
    }

    bool
    RescueMacCsma::StopOperation() {
        NS_LOG_FUNCTION("");
        m_enabled = false;
        m_nopBegin = Seconds(0);
        ChannelBecomesBusy();
        for (RelayQueueI it = m_ctrlPktQueue.begin(); it != m_ctrlPktQueue.end(); it++) {
            it = m_ctrlPktQueue.erase(it);
        }

        return true;
    }

    bool
    RescueMacCsma::StopOperation(Time duration) {
        NS_LOG_FUNCTION("");
        m_enabled = false;
        m_nopBegin = Simulator::Now() + duration;
        ChannelBecomesBusy();
        return true;
    }

    void
    RescueMacCsma::CcaForSifs() {
        if (!CC_ENABLED)
            NS_FATAL_ERROR("Should not wait for SIFS when CC is disabled");

        NS_LOG_FUNCTION(this);
        if (!m_enabled) {
            return;
        }

        NS_LOG_FUNCTION("#data queue:" << m_pktQueue.size() <<
                "#retry queue:" << m_pktRetryQueue.size() <<
                "#relay queue:" << m_pktRelayQueue.size() <<
                "#ctrl queue:" << m_ctrlPktQueue.size() <<
                "#ack queue:" << m_ackQueue.size() <<
                "state:" << StateToString(m_state) <<
                "phy idle:" << (m_phy->IsIdle() ? "TRUE" : "FALSE") <<
                "CCA running:" << (m_ccaTimeoutEvent.IsRunning() ? "YES" : "NO"));


        if (((m_pktQueue.size() == 0)
                && (m_pktRetryQueue.size() == 0)
                && (m_pktRelayQueue.size() == 0)
                && (m_ctrlPktQueue.size() == 0)
                && (m_ackQueue.size() == 0))
                || m_ccaTimeoutEvent.IsRunning()) {
            return;
        }
        if (m_state != IDLE || !m_phy->IsIdle()) {
            NS_LOG_FUNCTION("not idle, schedule another CcaForSifs ()");
            m_ccaTimeoutEvent = Simulator::Schedule(GetSifsTime(), &RescueMacCsma::CcaForSifs, this);
            return;
        }
        NS_LOG_FUNCTION("idle, schedule backoff start");
        m_ccaTimeoutEvent = Simulator::Schedule(GetSifsTime(), &RescueMacCsma::BackoffStartSifs, this);
    }

    void
    RescueMacCsma::BackoffStartSifs() {
        if (!CC_ENABLED)
            NS_FATAL_ERROR("Should not BACKOFF SIFS when CC is disabled");

        NS_LOG_FUNCTION("B-OFF SIFS remain:" << m_backoffRemain <<
                "state:" << StateToString(m_state) <<
                "phy idle:" << (m_phy->IsIdle() ? "TRUE" : "FALSE"));

        if (m_state != IDLE || !m_phy->IsIdle()) {
            CcaForSifs();
            return;
        }

        if (m_backoffRemain == Seconds(0)) {

            //Relay ACK with SIFS only when DISTRIBUTED_ACK_ENABLED set
            if (DISTRIBUTED_ACK_ENABLED && m_ackQueue.size() != 0) {
                std::pair<Ptr<Packet>, RescuePhyHeader> pktRelay = m_ackQueue.front();
                SetCw(m_remoteStationManager->GetAckCw(pktRelay.second.GetDestination()));
            } else
                if (m_pktRelayQueue.size() != 0) {
                std::pair<Ptr<Packet>, RescuePhyHeader> pktRelay = m_pktRelayQueue.front();
                SetCw(m_remoteStationManager->GetDataCw(pktRelay.second.GetDestination(), pktRelay.first, pktRelay.first->GetSize()));
            }


            m_backoffRemain = Seconds(0);
        }
        m_backoffStart = Simulator::Now();
        m_backoffTimeoutEvent = Simulator::ScheduleNow(&RescueMacCsma::ChannelAccessGrantedRelay, this);
    }

    void
    RescueMacCsma::ChannelAccessGrantedRelay() {
        if (!CC_ENABLED)
            NS_LOG_UNCOND("Should not execute ChannelAccessGrantedRelay when CC is disabled ");
        NS_LOG_FUNCTION(this);

        //Try to relay ACK when DISTRIBUTED_ACK_ENABLED only

        if (DISTRIBUTED_ACK_ENABLED && m_ackQueue.size() != 0) {

            NS_LOG_UNCOND("CHANNEL ACCESS GRANTED RELAY ACK");

            m_backoffStart = Seconds(0);
            m_backoffRemain = Seconds(0);
            m_state = WAIT_TX;

            m_pktRelay = m_ackQueue.front();
            m_ackQueue.pop_front();

            if (m_pktRelay.first == 0)
                NS_ASSERT("Null packet for ack forward tx");

            NS_LOG_INFO("FORWARD ACK to: " << m_pktRelay.second.GetDestination() << ", seq: " << m_pktRelay.second.GetSequence() << "!");

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
            SendRelayedData();
        } else
            NS_LOG_INFO("WEIRD : No packet to relay ....");

    }

    void
    RescueMacCsma::CheckRelaying(RescueMacHeader hdr, Ptr<Packet> pkt) {
        if (!CC_ENABLED)
            NS_FATAL_ERROR("Should not check relaying when CC is disabled");

        NS_LOG_FUNCTION(this << hdr << pkt);
        updateNeighborhood();

        RescuePhyHeader phyHdr;
        pkt->PeekHeader(phyHdr);
        NS_LOG_FUNCTION(phyHdr);

        bool relayingSuccessful = false;
        bool nodeIsLastHop = false;
        bool nodeIsSource = false;

        for (std::map < Mac48Address, std::pair<bool, bool>>::const_iterator it = m_neighbors.begin();
                it != m_neighbors.end(); it++) {
            if (it->second.first)
                relayingSuccessful = true;
            if (it->first == hdr.GetDestination())
                nodeIsLastHop = true;
        }
        if (m_hiMac->GetAddress() == hdr.GetSource())
            nodeIsSource = true;

        if (relayingSuccessful) {

            if (DISTRIBUTED_ACK_ENABLED) {
                //DistributedMAC with E2E ACK
                //Don't know what to
            } else { //DistributedMAC
                if (nodeIsSource) {
                    NS_LOG_DEBUG(LOG_STR << COLOR_YELLOW << "I'm the source ! Relaying OK : Stop ACK timeout scheduling !" << COLOR_DEFAULT);
                    m_arqManager->RelayingStopAck(hdr.GetDestination(), hdr.GetSequence());
                } else
                    NS_LOG_DEBUG(LOG_STR << COLOR_YELLOW << "Some neighbor just transmitted (hopefully)" << COLOR_DEFAULT);
            }
            SendDataDone();
        } else if (nodeIsLastHop) {
            NS_LOG_DEBUG(LOG_STR << COLOR_YELLOW << "I'm LAST HOP shouldn't check anything" << COLOR_DEFAULT);
            //            m_arqManager->RelayingStopAck(hdr.GetDestination(), hdr.GetSequence());
            SendDataDone();
        } else {
            NS_LOG_DEBUG(LOG_STR << COLOR_RED << "Transmission failed ? TRY ACKTIMEOUT" << COLOR_DEFAULT);
            m_arqManager->AckTimeout(pkt);
        }

    }

    //--------------------------------------------------------------------------

    void
    RescueMacCsma::CcaForLifs() {
        NS_LOG_FUNCTION("");
        if (!m_enabled) {
            return;
        }

        NS_LOG_FUNCTION("#data queue:" << m_pktQueue.size() <<
                "#retry queue:" << m_pktRetryQueue.size() <<
                "#relay queue:" << m_pktRelayQueue.size() <<
                "#ctrl queue:" << m_ctrlPktQueue.size() <<
                "#ack queue:" << m_ackQueue.size() <<
                "state:" << StateToString(m_state) <<
                "phy idle:" << (m_phy->IsIdle() ? "TRUE" : "FALSE") <<
                "CCA running:" << (m_ccaTimeoutEvent.IsRunning() ? "YES" : "NO"));
        Time now = Simulator::Now();

        if (((m_pktQueue.size() == 0)
                && (m_pktRetryQueue.size() == 0)
                && (m_pktRelayQueue.size() == 0)
                && (m_ctrlPktQueue.size() == 0)
                && (m_ackQueue.size() == 0))
                || m_ccaTimeoutEvent.IsRunning()) {
            return;
        }

        if (m_state != IDLE || !m_phy->IsIdle()) {
            //MODIF 4
            //        if ((m_state != IDLE && !CheckCCForTransmission()) || !m_phy->IsIdle()) {
            NS_LOG_FUNCTION("not idle, schedule another CcaForLifs ()");
            m_ccaTimeoutEvent = Simulator::Schedule(GetLifsTime(), &RescueMacCsma::CcaForLifs, this);
            return;
        }
        NS_LOG_FUNCTION("idle, schedule backoff start");
        m_ccaTimeoutEvent = Simulator::Schedule(GetLifsTime(), &RescueMacCsma::BackoffStart, this);
    }

    void
    RescueMacCsma::BackoffStart() {
        NS_LOG_FUNCTION("B-OFF remain:" << m_backoffRemain <<
                "state:" << StateToString(m_state) <<
                "phy idle:" << (m_phy->IsIdle() ? "TRUE" : "FALSE"));
        if (m_state != IDLE || !m_phy->IsIdle()) {
            CcaForLifs();
            return;
        }
        if (m_backoffRemain == Seconds(0)) {
            //CW setting
            if (m_ackQueue.size() != 0) {
                std::pair<Ptr<Packet>, RescuePhyHeader> pktRelay = m_ackQueue.front();
                SetCw(m_remoteStationManager->GetAckCw(pktRelay.second.GetDestination()));
            } else if (m_ctrlPktQueue.size() != 0) {
                std::pair<Ptr<Packet>, RescuePhyHeader> pktRelay = m_ctrlPktQueue.front();
                SetCw(m_remoteStationManager->GetCtrlCw(pktRelay.second.GetDestination()));
            } else if (m_pktRelayQueue.size() != 0) {
                std::pair<Ptr<Packet>, RescuePhyHeader> pktRelay = m_pktRelayQueue.front();
                SetCw(m_remoteStationManager->GetDataCw(pktRelay.second.GetDestination(), pktRelay.first, pktRelay.first->GetSize()));
            } else if (m_pktRetryQueue.size() != 0) {
                Ptr<Packet> pktData = m_pktRetryQueue.front();
                RescueMacHeader hdr;
                pktData->PeekHeader(hdr);
                RescueMacTrailer fcs;
                SetCw(m_remoteStationManager->GetDataCw(hdr.GetDestination(), pktData, pktData->GetSize() + fcs.GetSerializedSize()));
            } else if (m_pktQueue.size() != 0) {
                Ptr<Packet> pktData = m_pktQueue.front();
                RescueMacHeader hdr;
                pktData->PeekHeader(hdr);
                RescueMacTrailer fcs;
                SetCw(m_remoteStationManager->GetDataCw(hdr.GetDestination(), pktData, pktData->GetSize() + fcs.GetSerializedSize()));
            }

            uint32_t slots = m_random->GetInteger(0, m_cw - 1);
            m_backoffRemain = Seconds((double) (slots) * GetSlotTime().GetSeconds());
            NS_LOG_DEBUG("Select a random number (0, " << m_cw - 1 << "): " << slots <<
                    ", backoffRemain " << m_backoffRemain <<
                    ", will finish " << m_backoffRemain + Simulator::Now());
        }
        m_backoffStart = Simulator::Now();
        m_backoffTimeoutEvent = Simulator::Schedule(m_backoffRemain, &RescueMacCsma::ChannelAccessGranted, this);
    }

    void
    RescueMacCsma::ChannelBecomesBusy() {
        NS_LOG_FUNCTION("");
        if (m_ccaTimeoutEvent.IsRunning())
            m_ccaTimeoutEvent.Cancel();
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
        NS_LOG_FUNCTION("");

        //erase queued copies of acked frame
        for (RelayQueueI it2 = m_pktRelayQueue.begin(); it2 != m_pktRelayQueue.end();) {
            if (m_arqManager->IsFwdACKed(&(it2->second))) {
                NS_LOG_INFO("Erase unnecessary frame copy! from: " << it2->second.GetSource() << " to: " << it2->second.GetDestination() << ", seq: " << it2->second.GetSequence());
                it2 = m_pktRelayQueue.erase(it2);
            } else
                it2++;
        }

        /*for (RelayQueueI it = m_pktRelayQueue.begin (); it != m_pktRelayQueue.end ();)
          {
            //if (m_resendAck) break;
            if (CheckRelayedFrame (*it))
              {
                it = m_pktRelayQueue.erase (it);
                //NS_LOG_DEBUG("erase!");
              }
            else
              {
                        it++;
              }
          }*/

        if (m_ackQueue.size() != 0) {

            //MODIF 3
            if (!ACK_ENABLED)
                NS_FATAL_ERROR("Should not have any ACK in queue when ACK disabled");

            m_backoffStart = Seconds(0);
            m_backoffRemain = Seconds(0);
            m_state = WAIT_TX;

            m_pktRelay = m_ackQueue.front();
            m_ackQueue.pop_front();
            //if (!m_resendAck && m_pktRelay.second.IsACK ())
            /*if (m_pktRelay.second.IsACK ())
              m_ackCache.push_front (m_pktRelay); //record ACK in cache*/

            if (m_pktRelay.first == 0)
                NS_ASSERT("Null packet for ack forward tx");

            NS_LOG_INFO("FORWARD ACK to: " << m_pktRelay.second.GetDestination() << ", seq: " << m_pktRelay.second.GetSequence() << "!");
            //m_traceAckForward (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), m_pktRelay.first, m_pktRelay.second);

            SendRelayedData();
            //m_resendAck = false;
        } else if (m_ctrlPktQueue.size() != 0) {
            m_backoffStart = Seconds(0);
            m_backoffRemain = Seconds(0);
            m_state = WAIT_TX;

            m_pktRelay = m_ctrlPktQueue.front();
            m_ctrlPktQueue.pop_front();

            if (m_pktRelay.first == 0)
                NS_ASSERT("Null control packet for tx");

            NS_LOG_INFO("TRANSMIT CONTROL FRAME!");
            //m_traceCtrlTx (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), m_pktRelay.first, m_pktRelay.second);

            SendRelayedData();
        } else if (m_pktRelayQueue.size() != 0) {
            //MODIF 2
            //    if (ACK_ENABLED) {
            m_backoffStart = Seconds(0);
            m_backoffRemain = Seconds(0);
            m_state = WAIT_TX;

            m_pktRelay = m_pktRelayQueue.front();
            m_pktRelayQueue.pop_front();

            if (m_pktRelay.first == 0)
                NS_ASSERT("Null packet for relay tx");

            NS_LOG_INFO("RELAY DATA FRAME! from src: " << m_pktRelay.second.GetSource() << " to dst: " << m_pktRelay.second.GetDestination() << ", seq: " << m_pktRelay.second.GetSequence());
            //m_traceDataRelay (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), m_pktRelay.first, m_pktRelay.second);

            SendRelayedData();

            //MODIF 2
            //  } else
            //      NS_LOG_UNCOND(COLOR_RED << "ACK DISABLED, RELAY SHOULD NOT WAIT FOR SIFS" << COLOR_DEFAULT);

        } else if (m_pktRetryQueue.size() != 0) {
            NS_LOG_DEBUG("SEND RETRY QUEUED PACKET!");

            m_backoffStart = Seconds(0);
            m_backoffRemain = Seconds(0);
            m_state = WAIT_TX;

            m_pktData = m_pktRetryQueue.front();
            m_pktRetryQueue.pop_front();
            NS_LOG_INFO("dequeue packet from retry TX queue, size: " << m_pktData->GetSize());

            if (m_pktData == 0)
                NS_ASSERT("Queue has null packet");

            NS_LOG_DEBUG("SEND RETRANSMITTED DATA PACKET!");
            //m_traceDataTx (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), m_pktData);
            SendData();
        } else if (m_pktQueue.size() != 0) {
            Ptr<Packet> pkt = m_pktQueue.front();
            RescueMacHeader hdr;
            pkt->PeekHeader(hdr);

            if (m_arqManager->IsTxAllowed(hdr.GetDestination())) {
                m_backoffStart = Seconds(0);
                m_backoffRemain = Seconds(0);
                m_state = WAIT_TX;

                m_pktData = pkt;
                m_pktQueue.pop_front();
                NS_LOG_INFO("dequeue packet from TX queue, size: " << m_pktData->GetSize());

                if (m_pktData == 0)
                    NS_ASSERT("Queue has null packet");

                NS_LOG_DEBUG("SEND QUEUED DATA PACKET!");
                //m_traceDataTx (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), m_pktData);
                SendData();
            } else
                NS_LOG_DEBUG("Awaiting for ACK");
        } else
            NS_LOG_DEBUG("No queued frames for TX");

    }



    // ----------------------- Send Functions ------------------------------

    void
    RescueMacCsma::SendData() {
        NS_LOG_FUNCTION("");

        RescueMacHeader hdr;
        m_pktData->PeekHeader(hdr);

        m_interleaver++; //increment interleaver counter

        NS_LOG_DEBUG((hdr.IsRetry() ? "RETRY" : "NOT RETRY"));

        if (!hdr.IsRetry() && (hdr.GetSequence() == 0)) {
            m_pktData->RemoveHeader(hdr);
            hdr.SetSequence(m_arqManager->GetNextSequenceNumber(&hdr));

            m_pktData->AddHeader(hdr);
            RescueMacTrailer fcs;
            m_pktData->AddTrailer(fcs);

            //store info about sended packet
            m_arqManager->ReportNewDataFrame(hdr.GetDestination(), hdr.GetSequence());
        }

        m_pktData->PeekHeader(hdr);
        uint32_t pktSize = m_pktData->GetSize();
        uint16_t seq = hdr.GetSequence();

        NS_LOG_FUNCTION("dst:" << hdr.GetDestination() <<
                "seq:" << seq <<
                "IL:" << (int) m_interleaver <<
                "size:" << pktSize <<
                "retry:" << (hdr.IsRetry() ? "YES" : "NO") <<
                "#data queue:" << m_pktQueue.size() <<
                "#retry queue:" << m_pktRetryQueue.size() <<
                "#relay queue:" << m_pktRelayQueue.size() <<
                "#ctrl queue:" << m_ctrlPktQueue.size() <<
                "#ack queue:" << m_ackQueue.size());


        RescuePhyHeader phyHdr = RescuePhyHeader(hdr.GetSource(),
                hdr.GetSource(),
                hdr.GetDestination(),
                hdr.GetType(),
                hdr.GetSequence(),
                m_interleaver);

        hdr.IsRetry() ? phyHdr.SetRetry() : phyHdr.SetNoRetry();
        phyHdr.SetDistributedMacProtocol();

        m_arqManager->ConfigurePhyHeader(&phyHdr);

        RescueMode mode = m_remoteStationManager->GetDataTxMode(hdr.GetDestination(), m_pktData, pktSize);

        if (hdr.GetDestination() != m_hiMac->GetBroadcast()) // Unicast
        {
            NS_LOG_DEBUG("pktData total Size: " << m_pktData->GetSize());
            NS_LOG_DEBUG("SEND DATA PACKET! to dst: " << phyHdr.GetDestination() << ", seq: " << phyHdr.GetSequence());
            if (SendPacket(std::pair<Ptr<Packet>, RescuePhyHeader> (m_pktData->Copy(), phyHdr), mode)) {
                m_arqManager->ReportDataTx(m_pktData->Copy());

            } else {
                StartOver(m_pktData);
            }
        } else // Broadcast
        {

        }
    }

    void
    RescueMacCsma::SendRelayedData() {
        NS_LOG_FUNCTION("src:" << m_pktRelay.second.GetSource() <<
                "dst:" << m_pktRelay.second.GetDestination() <<
                "seq:" << m_pktRelay.second.GetSequence());

        m_pktTx = m_pktRelay.first;

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
                if (m_pktRelay.second.IsDataFrame())
                    m_arqManager->ReportRelayDataTx(&(m_pktRelay.second));
            } else {
                StartOver(m_pktRelay);
            }
        } else // Broadcast
        {
            if (SendPacket(m_pktRelay, mode)) {
            } else {
                StartOver(m_pktRelay);
            }
        }
    }

    void
    RescueMacCsma::SendAck(RescuePhyHeader ackHdr, RescueMode dataTxMode, SnrPerTag tag) {
        NS_LOG_INFO("SEND ACK from: " << ackHdr.GetSource() << " to: " << ackHdr.GetDestination() << ", seq: " << ackHdr.GetSequence());
        NS_LOG_FUNCTION("to:" << ackHdr.GetDestination());

        //MODIF 2
        if (!ACK_ENABLED)
            NS_FATAL_ERROR(COLOR_RED << "SHOULD NOT SEND ACK; ACK DISABLED" << COLOR_DEFAULT);

        Ptr<Packet> pkt = Create<Packet> (0);
        pkt->AddPacketTag(tag);

        RescueMode mode = m_remoteStationManager->GetAckTxMode(ackHdr.GetDestination(), dataTxMode);
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
                m_state = TX;

                //MODIF
                if (CC_ENABLED)
                    updateLocalControlChannel();

                m_pktTx = pkt;
                return true;
            } else {
                m_state = IDLE;

                //MODIF
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
                m_state = TX;

                //MODIF
                if (CC_ENABLED)
                    updateLocalControlChannel();

                m_pktTx = relayedPkt.first;
                return true;
            } else {
                m_state = IDLE;

                //MODIF
                if (CC_ENABLED)
                    updateLocalControlChannel();
            }
        }
        return false;
    }

    void
    RescueMacCsma::SendPacketDone(Ptr<Packet> pkt) {
        //MODIF
        if (CC_ENABLED)
            printTopology();

        NS_LOG_FUNCTION("state:" << StateToString(m_state));
        RescuePhyHeader phyHdr;
        pkt->RemoveHeader(phyHdr);

        if (m_state != TX || m_pktTx != pkt) {
            NS_LOG_DEBUG("Something is wrong!");
            //if (phyHdr.GetSource () == m_hiMac->GetAddress ())
            //  StartOver (pkt);
            //else
            StartOver(std::pair<Ptr<Packet>, RescuePhyHeader> (pkt, phyHdr));
            return;
        }
        m_state = IDLE;

        //MODIF
        if (CC_ENABLED)
            updateControlChannel();
        RescueMacHeader hdr;

        switch (phyHdr.GetType()) {
            case RESCUE_MAC_PKT_TYPE_DATA:
                pkt->PeekHeader(hdr);
                //MODIF 2
                //Check relaying + bad hook
                if (CC_ENABLED && !ACK_ENABLED)
                    if (Simulator::Now() > Seconds(10)) {
                        NS_LOG_DEBUG(LOG_STR << COLOR_BLUE << "SendPacketDone : SCHEDULING CHECK AT " <<
                                (Simulator::Now() + GetSifsTime() + GetChannelDelay() + NanoSeconds(1)).GetMicroSeconds() <<
                                "(" << GetSifsTime().GetMicroSeconds() <<
                                // ", " << GetTxDuration().GetMicroSeconds() <<
                                ", " << GetChannelDelay().GetMicroSeconds() <<
                                ")" << COLOR_DEFAULT);
                        Simulator::Schedule(GetSifsTime() + GetChannelDelay() + MicroSeconds(1), &RescueMacCsma::CheckRelaying, this, hdr, pkt);
                    }

                //MODIF 3
                if (!CC_ENABLED && !ACK_ENABLED) // SimpleMAC without ACK
                {
                    if (m_hiMac->GetAddress() == hdr.GetSource()) {
                        NS_LOG_DEBUG("SimpleMAC without ACK. Stop ACK wait");
                        m_arqManager->RelayingStopAck(hdr.GetDestination(), hdr.GetSequence());
                    }
                }

                SendDataDone();
                CcaForLifs();
                return;
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

        NS_LOG_FUNCTION("");
        //m_sequence++;
        NS_LOG_DEBUG("RESET PACKET");
        m_pktData = 0;
        m_pktRelay = std::pair<Ptr<Packet>, RescuePhyHeader> (0, 0);
        m_backoffStart = Seconds(0);
        m_backoffRemain = Seconds(0);
        // According to IEEE 802.11-2007 std (p261)., CW should be reset to minimum value
        // when retransmission reaches limit or when DATA is transmitted successfully
        SetCw(m_cwMin);
        CcaForLifs();
    }

    void
    RescueMacCsma::StartOver(Ptr<Packet> pkt) {
        NS_LOG_FUNCTION("");
        NS_LOG_INFO("return packet to the front of TX queue");
        m_pktData = 0;
        RescueMacHeader hdr;
        pkt->PeekHeader(hdr);
        if (hdr.IsRetry())
            m_pktRetryQueue.push_front(pkt);
        else
            m_pktQueue.push_front(pkt);
        m_backoffStart = Seconds(0);
        m_backoffRemain = Seconds(0);

        //MODIF DEBUG 876876
        m_state = IDLE;

        CcaForLifs();
    }

    void
    RescueMacCsma::StartOver(std::pair<Ptr<Packet>, RescuePhyHeader> pktHdr) {
        NS_LOG_FUNCTION("");
        NS_LOG_INFO("_return packet to the front of TX queue");
        switch (pktHdr.second.GetType()) {
            case RESCUE_PHY_PKT_TYPE_DATA:
                m_pktRelayQueue.push_front(pktHdr);
                break;
            case RESCUE_PHY_PKT_TYPE_E2E_ACK:
            case RESCUE_PHY_PKT_TYPE_PART_ACK:
                m_ackQueue.push_front(pktHdr);
                break;
            case RESCUE_PHY_PKT_TYPE_RR:
                m_ctrlPktQueue.push_front(pktHdr);

                break;
            default:
                break;
        }

        m_pktRelay = std::pair<Ptr<Packet>, RescuePhyHeader> (0, 0);
        m_backoffStart = Seconds(0);
        m_backoffRemain = Seconds(0);
        //MODIF 5
        m_state = IDLE;
        CcaForLifs();
    }


    // ---------------------- Receive Functions ----------------------------

    void
    RescueMacCsma::ReceiveData(Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode, bool correctData) {
        NS_LOG_FUNCTION("");
        if (correctData) {
            ReceiveCorrectData(pkt, phyHdr, mode);
        } else //uncorrect data
        {
            //check - have I already received the correct/complete frame?
            if (m_arqManager->CheckRxSequence(phyHdr.GetSource(), phyHdr.GetSequence(), phyHdr.IsContinousAckEnabled())) {
                NS_LOG_INFO("THE FRAME COPY WAS STORED!");
                m_arqManager->ReportDamagedFrame(&phyHdr);
            } else
                NS_LOG_INFO("(unnecessary DATA frame copy) DROP!");
        }
    }

    void
    RescueMacCsma::ReceiveCorrectData(Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode) {
        NS_LOG_FUNCTION("");

        RescueMacHeader hdr;
        pkt->RemoveHeader(hdr);
        RescueMacTrailer fcs;
        pkt->RemoveTrailer(fcs);
        NS_LOG_FUNCTION("src:" << phyHdr.GetSource() <<
                "sequence: " << phyHdr.GetSequence());

        m_state = WAIT_TX;
        //MODIF
        if (CC_ENABLED)
            updateControlChannel();
        NS_LOG_DEBUG(COLOR_GREEN << "RECEIVED CORRECT DATA ==> " << phyHdr.GetSequence() << COLOR_DEFAULT);

        // forward upper layer

        RescuePhyHeader ackHdr = RescuePhyHeader(RESCUE_PHY_PKT_TYPE_E2E_ACK);
        if (m_arqManager->CheckRxFrame(&phyHdr, &ackHdr)) {
            NS_LOG_INFO("DATA RX OK!");
            //m_forwardUpCb (pkt, phyHdr.GetSource (), phyHdr.GetDestination ());
            m_hiMac->ReceivePacket(pkt, phyHdr);
        } else {
            NS_LOG_DEBUG(COLOR_RED << "DUPLICATE?" << COLOR_DEFAULT);
            NS_LOG_INFO("(duplicate) DROP!");
        }

        if (ackHdr.GetDestination() != Mac48Address("00:00:00:00:00:00")) {
            SnrPerTag tag;
            pkt->PeekPacketTag(tag); //to send back SnrPerTag - for possible use in the future (multi rate control etc.)
            //MODIF 2

            if (ACK_ENABLED)
                m_sendAckEvent = Simulator::Schedule(GetSifsTime(), &RescueMacCsma::SendAck, this, ackHdr, mode, tag);
        }
    }

    void
    RescueMacCsma::ReceiveRelayData(Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode) {
        SnrPerTag tag;
        pkt->PeekPacketTag(tag); //to send back SnrPerTag - for possible use in the future (multi rate control etc.)

        NS_LOG_FUNCTION("src:" << phyHdr.GetSource() <<
                "dst:" << phyHdr.GetDestination() <<
                "seq:" << phyHdr.GetSequence() <<
                "mode:" << mode <<
                "ber:" << tag.GetBER());

        std::pair<RelayBehavior, RescuePhyHeader> txAck = m_arqManager->ReportRelayFrameRx(&phyHdr, tag.GetBER());


        switch (txAck.first) {
            case FORWARD:
                NS_LOG_INFO("PHY HDR OK! FRAME TO RELAY! src: " << phyHdr.GetSource() << ", dst: " << phyHdr.GetDestination() << ", from: " << phyHdr.GetSender() << ", seq: " << phyHdr.GetSequence());
                EnqueueRelay(pkt, phyHdr);
                break;
            case REPLACE_COPY:
                NS_LOG_INFO("FRAME TO RELAY! (instead of the worse copy) src: " << phyHdr.GetSource() << " dst: " << phyHdr.GetDestination() << ", seq: " << phyHdr.GetSequence());
                for (RelayQueueI it = m_pktRelayQueue.begin(); it != m_pktRelayQueue.end(); it++)
                    if ((it->second.GetSource() == phyHdr.GetSource())
                            && (it->second.GetDestination() == phyHdr.GetDestination())
                            && (it->second.GetSequence() == phyHdr.GetSequence())
                            && (it->second.GetInterleaver() == phyHdr.GetInterleaver())) {
                        //found enqueued copy, switch with better one
                        NS_LOG_INFO("Erase unnecessary frame copy! from: " << it->second.GetSource() << " to: " << it->second.GetDestination() << ", seq: " << it->second.GetSequence());
                        it = m_pktRelayQueue.erase(it);
                        it = m_pktRelayQueue.insert(it, std::pair<Ptr<Packet>, RescuePhyHeader> (pkt, phyHdr));
                        break;
                    }
                break;
            case RESEND_ACK:
                NS_LOG_INFO("UNNECESSARY RETRANSMISSION DETECTED! DROP and resend ACK! src: " << txAck.second.GetSource() << " dst: " << txAck.second.GetDestination() << ", seq: " << txAck.second.GetSequence());
                m_sendAckEvent = Simulator::Schedule(GetSifsTime(), &RescueMacCsma::SendAck, this, txAck.second, mode, tag);
                //check ACK queue, if this ACK frame is queued - erase it!
                for (RelayQueueI it = m_ackQueue.begin(); it != m_ackQueue.end(); it++)
                    if (m_arqManager->ACKcomp(&(it->second), &(txAck.second))) {
                        //found enqueued ACK, erase it
                        it = m_ackQueue.erase(it);
                        NS_LOG_INFO("Erase enqueued ACK and TX it!");
                        break;
                    }
                break;
            case DROP:
                NS_LOG_INFO("UNNECESSARY COPY! DROP! src: " << phyHdr.GetSource() << " dst: " << phyHdr.GetDestination() << ", seq: " << phyHdr.GetSequence());
                break;
            default:
                break;
        }

        //MODIF
        //m_state = IDLE;
        if (CC_ENABLED)
            updateControlChannel();

        return;
    }

    void
    RescueMacCsma::ReceiveBroadcastData(Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode, bool correctData) {

        NS_LOG_FUNCTION("");

        //don't know what to do
    }

    void
    RescueMacCsma::ReceiveAck(Ptr<Packet> ackPkt, RescuePhyHeader ackHdr, double ackSnr, RescueMode ackMode) {
        NS_LOG_FUNCTION("src:" << ackHdr.GetSource() <<
                "dst:" << ackHdr.GetDestination() <<
                "seq:" << ackHdr.GetSequence());

        m_state = IDLE;
        //MODIF
        if (CC_ENABLED)
            updateControlChannel();

        //check - have I recently received this ACK?
        for (AckCacheI it = m_ackCache.begin(); it != m_ackCache.end(); it++) {
            if (it->expires < ns3::Simulator::Now()) {
                NS_LOG_INFO("Expired ACK in cache - erase!");
                it = m_ackCache.erase(it);
            } else if (m_arqManager->ACKcomp(&(it->ack.second), &(ackHdr))) {
                //found cached ACK, dont process it
                NS_LOG_INFO("Duplicated ACK!");
                CcaForLifs();
                return;
            }
        }

        //store frame in ACK cache
        StoredAck newIns;
        newIns.ack = std::pair<Ptr<Packet>, RescuePhyHeader> (ackPkt, ackHdr);
        newIns.expires = ns3::Simulator::Now() + m_arqManager->GetTimeoutFor(&ackHdr);
        m_ackCache.push_front(newIns);

        if (ackHdr.GetDestination() == m_hiMac->GetAddress()) {
            //check - have I recently transmitted frame which is acknowledged?
            int retryCounter = m_arqManager->ReceiveAck(&ackHdr);

            if (retryCounter >= 0) {
                NS_LOG_INFO("GOT ACK - DATA TX OK! from: " << ackHdr.GetSource() << ", seq: " << ackHdr.GetSequence());
                SnrPerTag tag;
                ackPkt->PeekPacketTag(tag);
                m_remoteStationManager->ReportDataOk(ackHdr.GetSource(),
                        ackSnr, ackMode,
                        tag.GetSNR(), tag.GetBER(),
                        (uint16_t) retryCounter);

                //check for unnecessary retransmissions in retry queue
                if (m_pktRetryQueue.size() > 0) {
                    for (QueueI it = m_pktRetryQueue.begin(); it != m_pktRetryQueue.end(); it++) {
                        RescueMacHeader hdr;
                        (*it)->PeekHeader(hdr);
                        if (m_arqManager->IsRetryACKed(&hdr)) {
                            it = m_pktRetryQueue.erase(it);
                            NS_LOG_INFO("Erase unnecessary frame retry!");
                            //it++;
                        }
                        //else
                        //  it++;
                    }
                }

                SendDataDone();
            }
        } else {
            NS_LOG_INFO("ACK to forward? from: " << ackHdr.GetSource() << " to: " << ackHdr.GetDestination() << ", seq: " << ackHdr.GetSequence());
            if (m_arqManager->ReportAckToFwd(&ackHdr)) {
                //erase queued copies of acked frame
                for (RelayQueueI it2 = m_pktRelayQueue.begin(); it2 != m_pktRelayQueue.end();) {
                    if (m_arqManager->IsFwdACKed(&(it2->second))) {
                        NS_LOG_INFO("Erase unnecessary frame copy! from: " << it2->second.GetSource() << " to: " << it2->second.GetDestination() << ", seq: " << it2->second.GetSequence());
                        it2 = m_pktRelayQueue.erase(it2);
                    } else
                        it2++;
                }

                NS_LOG_FUNCTION("#data queue:" << m_pktQueue.size() <<
                        "#retry queue:" << m_pktRetryQueue.size() <<
                        "#relay queue:" << m_pktRelayQueue.size() <<
                        "#ctrl queue:" << m_ctrlPktQueue.size() <<
                        "#ack queue:" << m_ackQueue.size());

                NS_LOG_INFO("ACK to forward!");

                EnqueueAck(ackPkt, ackHdr);
            }
        }

        CcaForLifs();

        return;
    }

    void
    RescueMacCsma::ReceiveResourceReservation(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {

        NS_LOG_FUNCTION("");
        m_hiMac->ReceiveResourceReservation(pkt, phyHdr);
    }

    void
    RescueMacCsma::ReceivePacket(Ptr<RescuePhy> phy, Ptr<Packet> pkt) {
        NS_LOG_FUNCTION("");
        ChannelBecomesBusy();
        switch (m_state) {
            case WAIT_TX:
            case RX:
            case WAIT_RX:
            case BACKOFF:
            case IDLE:
                m_state = RX;
                //MODIF
                if (CC_ENABLED) {
                    //re-trigger neighbor update upon reception
                    m_controlChannel = false;
                    updateControlChannel();
                }

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
        NS_LOG_FUNCTION("");
        m_state = IDLE;
        //MODIF
        if (CC_ENABLED)
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
                            if (correctData)
                                ReceiveRelayData(pkt, phyHdr, mode);
                        } else {
                            NS_LOG_INFO("PHY HDR OK!" <<
                                    ", DATA: " << (correctData ? "OK!" : "DAMAGED!") <<
                                    ", RECONSTRUCTION STATUS: " << ((wasReconstructed) ? (correctData ? "SUCCESS" : "MORE COPIES NEEDED") : "NOT NEEDED") <<
                                    ", src: " << phyHdr.GetSource() << ", dst: " << phyHdr.GetDestination() << ", seq: " << phyHdr.GetSequence());
                            ReceiveData(pkt, phyHdr, mode, correctData);
                        }
                        if (!m_sendAckEvent.IsRunning()) {
                            m_state = IDLE;
                            CcaForLifs();
                        }
                        break;
                    case RESCUE_PHY_PKT_TYPE_E2E_ACK:

                        //MODIF 2
                        if (!ACK_ENABLED)
                            NS_FATAL_ERROR("RECEIVED AN ACK. Something went wrong !!");

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
                return;
            }
        }
    }

    // --------------------------- ETC -------------------------------------

    void
    RescueMacCsma::DoubleCw() {
        NS_LOG_FUNCTION("");
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
        NS_LOG_FUNCTION("");
        int64_t realTime = time.GetMicroSeconds();
        int64_t slotTime = GetSlotTime().GetMicroSeconds();
        if (realTime % slotTime >= slotTime / 2) {
            return Seconds(GetSlotTime().GetSeconds() * (double) (realTime / slotTime + 1));
        } else {
            return Seconds(GetSlotTime().GetSeconds() * (double) (realTime / slotTime));
        }
    }

} // namespace ns3

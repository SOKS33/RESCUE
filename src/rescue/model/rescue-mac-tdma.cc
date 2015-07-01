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
#include "rescue-mac-tdma.h"
#include "snr-per-tag.h"

NS_LOG_COMPONENT_DEFINE("RescueMacTdma");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now() << "] [addr=" << ((m_hiMac != 0) ? m_hiMac->GetAddress () : 0) << "] [MAC TDMA] ";

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(RescueMacTdma);

    RescueMacTdma::RescueMacTdma()
    : m_phy(0),
    m_hiMac(0),
    m_arqManager(0),
    m_ackTimeoutEvent(),
    //m_sendAckEvent (),
    //m_sendDataEvent (),
    m_state(IDLE),
    m_pktTx(0),
    m_pktData(0) {
        //NS_LOG_FUNCTION (this);
        m_sequence = 0;
        m_interleaver = 0;
        m_random = CreateObject<UniformRandomVariable> ();
    }

    RescueMacTdma::~RescueMacTdma() {
    }

    void
    RescueMacTdma::DoDispose() {
        Clear();
        m_remoteStationManager = 0;
        m_arqManager = 0;
    }

    void
    RescueMacTdma::Clear() {
        m_pktTx = 0;
        NS_LOG_DEBUG("RESET PACKET");
        m_pktData = 0;
        m_pktQueue.clear();
        m_seqList.clear();
    }

    TypeId
    RescueMacTdma::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::RescueMacTdma")
                .SetParent<Object> ()
                .AddConstructor<RescueMacTdma> ()
                .AddAttribute("SifsTime",
                "Short Inter-frame Space [us]",
                TimeValue(MicroSeconds(30)),
                MakeTimeAccessor(&RescueMacTdma::m_sifs),
                MakeTimeChecker())
                .AddAttribute("LifsTime",
                "Long Inter-frame Space [us]",
                TimeValue(MicroSeconds(60)),
                MakeTimeAccessor(&RescueMacTdma::m_lifs),
                MakeTimeChecker())
                .AddAttribute("QueueLimits",
                "Maximum packets to queue at MAC",
                UintegerValue(20),
                MakeUintegerAccessor(&RescueMacTdma::m_queueLimit),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("AckTimeout",
                "ACK awaiting time",
                TimeValue(MicroSeconds(3000)),
                MakeTimeAccessor(&RescueMacTdma::m_basicAckTimeout),
                MakeTimeChecker())

                .AddTraceSource("AckTimeout",
                "Trace Hookup for ACK Timeout",
                MakeTraceSourceAccessor(&RescueMacTdma::m_traceAckTimeout))
                .AddTraceSource("SendDataDone",
                "Trace Hookup for sending a DATA",
                MakeTraceSourceAccessor(&RescueMacTdma::m_traceSendDataDone))
                .AddTraceSource("Enqueue",
                "Trace Hookup for enqueue a DATA",
                MakeTraceSourceAccessor(&RescueMacTdma::m_traceEnqueue))

                .AddTraceSource("DataTx",
                "Trace Hookup for originated DATA TX",
                MakeTraceSourceAccessor(&RescueMacTdma::m_traceDataTx))
                .AddTraceSource("DataRelay",
                "Trace Hookup for DATA relay TX",
                MakeTraceSourceAccessor(&RescueMacTdma::m_traceDataRelay))
                .AddTraceSource("DataRx",
                "Trace Hookup for DATA RX by final node",
                MakeTraceSourceAccessor(&RescueMacTdma::m_traceDataRx))
                .AddTraceSource("AckTx",
                "Trace Hookup for ACK TX",
                MakeTraceSourceAccessor(&RescueMacTdma::m_traceAckTx))
                .AddTraceSource("ForwardAck",
                "Trace Hookup for ACK Forward",
                MakeTraceSourceAccessor(&RescueMacTdma::m_traceAckForward))

                ;
        return tid;
    }



    // ------------------------ Set Functions -----------------------------

    void
    RescueMacTdma::AttachPhy(Ptr<RescuePhy> phy) {
        //NS_LOG_FUNCTION (this << phy);
        m_phy = phy;
    }

    void
    RescueMacTdma::SetHiMac(Ptr<RescueMac> hiMac) {
        //NS_LOG_FUNCTION (this << hiMac);
        m_hiMac = hiMac;
    }

    void
    RescueMacTdma::SetDevice(Ptr<RescueNetDevice> dev) {
        //NS_LOG_FUNCTION (this << dev);
        m_device = dev;
    }

    void
    RescueMacTdma::SetRemoteStationManager(Ptr<RescueRemoteStationManager> manager) {
        //NS_LOG_FUNCTION (this << manager);
        m_remoteStationManager = manager;
    }

    void
    RescueMacTdma::SetSifsTime(Time duration) {
        m_sifs = duration;
    }

    void
    RescueMacTdma::SetLifsTime(Time duration) {
        m_lifs = duration;
    }

    void
    RescueMacTdma::SetQueueLimits(uint32_t length) {
        m_queueLimit = length;
    }

    void
    RescueMacTdma::SetBasicAckTimeout(Time duration) {
        m_basicAckTimeout = duration;
    }

    int64_t
    RescueMacTdma::AssignStreams(int64_t stream) {
        NS_LOG_FUNCTION(this << stream);
        m_random->SetStream(stream);
        return 1;
    }


    // ------------------------ Get Functions -----------------------------

    Time
    RescueMacTdma::GetSifsTime(void) const {
        return m_sifs;
    }

    Time
    RescueMacTdma::GetLifsTime(void) const {
        return m_lifs;
    }

    uint32_t
    RescueMacTdma::GetQueueLimits(void) const {
        return m_queueLimit;
    }

    Time
    RescueMacTdma::GetBasicAckTimeout(void) const {
        return m_basicAckTimeout;
    }

    Time
    RescueMacTdma::GetCtrlDuration(RescuePhyHeader hdr, RescueMode mode) {
        return m_phy->CalTxDuration(hdr.GetSize(), 0, m_phy->GetPhyHeaderMode(mode), mode);
    }

    Time
    RescueMacTdma::GetCtrlDuration(uint16_t type, RescueMode mode) {
        return m_phy->CalTxDuration(0, 0, m_phy->GetPhyHeaderMode(mode), mode, type);
    }

    Time
    RescueMacTdma::GetDataDuration(Ptr<Packet> pkt, RescueMode mode) {
        return m_phy->CalTxDuration(0, pkt->GetSize(), m_phy->GetPhyHeaderMode(mode), mode, 0);
    }

    Time
    RescueMacTdma::GetDataDuration(Ptr<Packet> pkt, RescuePhyHeader hdr, RescueMode mode) {
        return m_phy->CalTxDuration(hdr.GetSize(), pkt->GetSize(), m_phy->GetPhyHeaderMode(mode), mode);
    }

    Time
    RescueMacTdma::GetNextTxSifsDuration(void) {
        NS_LOG_FUNCTION("");

        if (m_ackQueue.size() != 0) {
            std::pair<Ptr<Packet>, RescuePhyHeader> pktHdr = m_ackQueue.front();
            RescueMode mode = m_remoteStationManager->GetAckTxMode(pktHdr.second.GetDestination());
            Time duration = GetCtrlDuration(pktHdr.second, mode) + GetSifsTime();
            return duration;
        } else if (m_ctrlPktQueue.size() != 0) {
            std::pair<Ptr<Packet>, RescuePhyHeader> pktHdr = m_ctrlPktQueue.front();
            RescueMode mode = m_remoteStationManager->GetDataTxMode(pktHdr.second.GetDestination(),
                    pktHdr.first,
                    pktHdr.first->GetSize());
            Time duration = GetDataDuration(pktHdr.first, mode) + GetSifsTime();
            return duration;
        } else if (m_pktRelayQueue.size() != 0) {
            std::pair<Ptr<Packet>, RescuePhyHeader> pktHdr = m_pktRelayQueue.front();
            RescueMode mode = m_remoteStationManager->GetDataTxMode(pktHdr.second.GetDestination(),
                    pktHdr.first,
                    pktHdr.first->GetSize());
            Time duration = GetDataDuration(pktHdr.first, mode) + GetSifsTime();
            return duration;
        } else if (m_pktQueue.size() != 0) {
            Ptr<Packet> pkt = m_pktQueue.front();
            RescueMacHeader hdr;
            pkt->PeekHeader(hdr);
            RescueMacTrailer fcs;

            RescueMode mode = m_remoteStationManager->GetDataTxMode(hdr.GetDestination(),
                    pkt,
                    pkt->GetSize() + fcs.GetSerializedSize());
            Time duration = GetDataDuration(pkt, mode) + GetSifsTime();
            return duration;
        } else {
            return Seconds(0);
        }
    }

    std::string
    RescueMacTdma::StateToString(State state) {
        switch (state) {
            case IDLE:
                return "IDLE";
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
    RescueMacTdma::Enqueue(Ptr<Packet> pkt, Mac48Address dst) {
        NS_LOG_FUNCTION("dst:" << dst <<
                "size [B]:" << pkt->GetSize() <<
                "#queue:" << m_pktQueue.size() <<
                "#relay queue:" << m_pktRelayQueue.size() <<
                "#ctrl queue:" << m_ctrlPktQueue.size() <<
                "#ack queue:" << m_ackQueue.size() <<
                "state:" << StateToString(m_state));
        if (m_pktQueue.size() >= m_queueLimit) {
            NS_LOG_DEBUG("PACKET QUEUE LIMIT REACHED - DROP FRAME!");
            return false;
        }

        RescueMacHeader hdr = RescueMacHeader(m_address, dst, RESCUE_MAC_PKT_TYPE_DATA);
        pkt->AddHeader(hdr);
        m_traceEnqueue(m_device->GetNode()->GetId(), m_device->GetIfIndex(), pkt, hdr);
        m_pktQueue.push_back(pkt);

        return false;
    }

    bool
    RescueMacTdma::EnqueueCtrl(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("dst:" << phyHdr.GetDestination() <<
                "size [B]:" << pkt->GetSize() <<
                "#queue:" << m_pktQueue.size() <<
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

        return false;
    }

    void
    RescueMacTdma::Dequeue() {
        NS_LOG_FUNCTION("#queue:" << m_pktQueue.size() <<
                "#relay queue:" << m_pktRelayQueue.size() <<
                "#ctrl queue:" << m_ctrlPktQueue.size() <<
                "#ack queue:" << m_ackQueue.size());
        m_pktQueue.remove(m_pktData);
    }

    /*void
    RescueMacCsma::DequeueCtrl ()
    {
      NS_LOG_FUNCTION ("#queue:" << m_pktQueue.size () <<
                       "#relay queue:" << m_pktRelayQueue.size () <<
                       "#ctrl queue:" << m_ctrlPktQueue.size () <<
                       "#ack queue:" << m_ackQueue.size ());
      m_ctrlPktQueue.remove (m_pktRelay);
    }*/


    // ------------------ Channel Access Functions -------------------------

    bool
    RescueMacTdma::StartOperation() {
        NS_LOG_FUNCTION("");
        m_enabled = true;
        m_opEnd = Seconds(0);
        return true;
    }

    bool
    RescueMacTdma::StartOperation(Time duration) {
        NS_LOG_FUNCTION("");
        m_enabled = true;
        m_opEnd = Simulator::Now() + duration;
        return true;
    }

    bool
    RescueMacTdma::StopOperation() {
        NS_LOG_FUNCTION("");
        m_enabled = false;
        m_nopBegin = Seconds(0);
        return true;
    }

    bool
    RescueMacTdma::StopOperation(Time duration) {
        NS_LOG_FUNCTION("");
        m_enabled = false;
        m_nopBegin = Simulator::Now() + duration;
        return true;
    }

    void
    RescueMacTdma::ChannelAccessGranted() {
        NS_LOG_FUNCTION("");

        if (m_ackQueue.size() != 0) {
            m_state = WAIT_TX;

            m_pktRelay = m_ackQueue.front();
            m_ackQueue.pop_front();

            if (m_pktRelay.first == 0)
                NS_ASSERT("Null packet for ack tx");

            NS_LOG_INFO("ACK TX!");
            m_traceAckTx(m_device->GetNode()->GetId(), m_device->GetIfIndex(), m_pktRelay.first, m_pktRelay.second);

            SendRelayedData();
        } else if (m_ctrlPktQueue.size() != 0) {
            m_state = WAIT_TX;

            m_pktRelay = m_ctrlPktQueue.front();
            m_ctrlPktQueue.pop_front();

            if (m_pktRelay.first == 0)
                NS_ASSERT("Null control packet for tx");

            NS_LOG_INFO("TRANSMIT CONTROL FRAME!");
            m_traceCtrlTx(m_device->GetNode()->GetId(), m_device->GetIfIndex(), m_pktRelay.first, m_pktRelay.second);

            SendRelayedData();
        } else if (m_pktRelayQueue.size() != 0) {
            m_state = WAIT_TX;

            m_pktRelay = m_pktRelayQueue.front();
            m_pktRelayQueue.pop_front();

            if (m_pktRelay.first == 0)
                NS_ASSERT("Null packet for relay tx");

            NS_LOG_INFO("RELAY DATA FRAME!");
            m_traceDataRelay(m_device->GetNode()->GetId(), m_device->GetIfIndex(), m_pktRelay.first, m_pktRelay.second);

            SendRelayedData();
        } else if (m_pktQueue.size() != 0) {
            NS_LOG_DEBUG("SEND QUEUED PACKET!");

            m_state = WAIT_TX;

            m_pktData = m_pktQueue.front();
            m_pktQueue.pop_front();
            NS_LOG_INFO("dequeue packet from TX queue, size: " << m_pktData->GetSize());

            if (m_pktData == 0)
                NS_ASSERT("Queue has null packet");

            m_traceDataTx(m_device->GetNode()->GetId(), m_device->GetIfIndex(), m_pktData);
            SendData();
        } else {
            NS_LOG_DEBUG("No queued frames for TX");
        }
    }


    // ----------------------- Send Functions ------------------------------

    void
    RescueMacTdma::SendData() {
        NS_LOG_FUNCTION("");

        RescueMacHeader hdr;
        m_pktData->PeekHeader(hdr);

        m_interleaver++; //increment interleaver counter

        if (!hdr.IsRetry()) {
            if (m_arqManager != 0) {
                hdr.SetSequence(m_arqManager->GetNextSequenceNumber(&hdr));
            } else
                hdr.SetSequence(m_sequence);

            m_pktData->AddHeader(hdr);
            RescueMacTrailer fcs;
            m_pktData->AddTrailer(fcs);
        }

        uint32_t pktSize = m_pktData->GetSize();

        NS_LOG_FUNCTION("dst:" << hdr.GetDestination() <<
                "seq:" << hdr.GetSequence() <<
                "IL:" << m_interleaver <<
                "size:" << pktSize <<
                "#queue:" << m_pktQueue.size() <<
                "#relay queue:" << m_pktRelayQueue.size() <<
                "#ctrl queue:" << m_ctrlPktQueue.size() <<
                "#ack queue:" << m_ackQueue.size());

        RescueMode mode = m_remoteStationManager->GetDataTxMode(hdr.GetDestination(), m_pktData, pktSize);

        if (hdr.GetDestination() != m_hiMac->GetBroadcast()) // Unicast
        {
            NS_LOG_DEBUG("pktData total Size: " << m_pktData->GetSize());
            if (SendPacket(m_pktData, mode, m_interleaver)) {
                //Time ackTimeout = GetDataDuration (m_pktData, mode) + GetSifsTime () + GetCtrlDuration (RESCUE_MAC_PKT_TYPE_ACK, mode) + GetSlotTime ();
                Ptr<Packet> p = m_pktData->Copy();
                m_ackTimeoutEvent = Simulator::Schedule(m_basicAckTimeout, &RescueMacTdma::AckTimeout, this, p, hdr);
                NS_LOG_DEBUG("ACK TIMEOUT TIMER START");
            } else {
                StartOver();
            }
        } else // Broadcast
        {
            /*if (SendPacket (m_pktData, mode, 0))
              {
              }
            else
              {
                StartOver ();
              }*/
        }
    }

    bool
    RescueMacTdma::SendCtrlNow(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("src:" << phyHdr.GetSource() <<
                "dst:" << phyHdr.GetDestination() <<
                "seq:" << phyHdr.GetSequence());

        RescueMode mode;
        switch (phyHdr.GetType()) {
            case RESCUE_PHY_PKT_TYPE_DATA:
                mode = m_remoteStationManager->GetDataTxMode(phyHdr.GetDestination(), pkt, pkt->GetSize());
                break;
            case RESCUE_PHY_PKT_TYPE_E2E_ACK:
            case RESCUE_PHY_PKT_TYPE_PART_ACK:
                mode = m_remoteStationManager->GetAckTxMode(phyHdr.GetDestination());
                break;
            case RESCUE_PHY_PKT_TYPE_B:
            case RESCUE_PHY_PKT_TYPE_RR:
                mode = m_remoteStationManager->GetCtrlTxMode(phyHdr.GetDestination());
                break;
        }

        phyHdr.SetSender(m_address);

        std::pair<Ptr<Packet>, RescuePhyHeader> ctrlPkt(pkt, phyHdr);

        return SendPacket(ctrlPkt, mode);

    }

    void
    RescueMacTdma::SendRelayedData() {
        NS_LOG_FUNCTION("src:" << m_pktRelay.second.GetSource() <<
                "dst:" << m_pktRelay.second.GetDestination() <<
                "seq:" << m_pktRelay.second.GetSequence());

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
            m_pktRelay.second.SetSender(m_address);
            if (SendPacket(m_pktRelay, mode)) {
            } else {
                //StartOver ();
            }
            m_pktRelay.first = 0;
            m_pktRelay.second = 0;
        } else // Broadcast
        {
            if (SendPacket(m_pktRelay, mode)) {
            } else {
                //StartOver ();
            }
            m_pktRelay.first = 0;
            m_pktRelay.second = 0;
        }
    }

    /*void
    RescueMacTdma::ScheduleACKtx (Time ackTxTime)
    {
      m_ackTxTime = ackTxTime;
    }*/

    void
    RescueMacTdma::GenerateAck(Mac48Address dest, uint16_t seq, RescueMode dataTxMode, SnrPerTag tag) {
        NS_LOG_INFO("GENERATE ACK");
        NS_LOG_FUNCTION("to:" << dest);

        Ptr<Packet> pkt = Create<Packet> (0);
        RescuePhyHeader ackHdr = RescuePhyHeader(m_address, dest, RESCUE_MAC_PKT_TYPE_ACK);
        ackHdr.SetSequence(seq);
        pkt->AddPacketTag(tag);

        //RescueMode mode = m_remoteStationManager->GetAckTxMode (m_pktRelay.second.GetDestination (), dataTxMode);
        std::pair<Ptr<Packet>, RescuePhyHeader> ackPkt(pkt, ackHdr);

        m_ackQueue.push_back(ackPkt);

        //m_traceAckTx (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt, ackHdr);

        //SendPacket (ackPkt, mode);
    }

    bool
    RescueMacTdma::SendPacket(Ptr<Packet> pkt, RescueMode mode, uint16_t il) {
        NS_LOG_FUNCTION("state:" << StateToString(m_state) << "packet size: " << pkt->GetSize());

        if (m_state == IDLE || m_state == WAIT_TX) {
            if (m_phy->SendPacket(pkt, mode, il)) {
                m_state = TX;
                m_pktTx = pkt;
                return true;
            } else {
                m_state = IDLE;
            }
        }
        return false;
    }

    bool
    RescueMacTdma::SendPacket(std::pair<Ptr<Packet>, RescuePhyHeader> relayedPkt, RescueMode mode) {
        NS_LOG_FUNCTION("state:" << StateToString(m_state));

        if (m_state == IDLE || m_state == WAIT_TX) {
            if (m_phy->SendPacket(relayedPkt, mode)) {
                m_state = TX;
                m_pktTx = relayedPkt.first;
                return true;
            } else {
                m_state = IDLE;
            }
        }
        return false;
    }

    void
    RescueMacTdma::SendPacketDone(Ptr<Packet> pkt) {
        NS_LOG_FUNCTION("state:" << StateToString(m_state));

        if (m_state != TX /*|| m_pktTx != pkt*/) {
            NS_LOG_DEBUG("Something is wrong!");
            return;
        }
        m_state = IDLE;
        RescueMacHeader hdr;
        pkt->PeekHeader(hdr);
        switch (hdr.GetType()) {
            case RESCUE_MAC_PKT_TYPE_DATA:
                if (hdr.GetDestination() == m_hiMac->GetBroadcast()) {
                    //don't know what to do
                    SendDataDone();
                    return;
                }
                break;
            case RESCUE_MAC_PKT_TYPE_ACK:
            case RESCUE_MAC_PKT_TYPE_RR:
            case RESCUE_MAC_PKT_TYPE_B:
                break;
            default:
                break;
        }
    }

    void
    RescueMacTdma::SendDataDone() {
        NS_LOG_FUNCTION("");
        m_sequence++;
        NS_LOG_DEBUG("RESET PACKET");
        m_pktData = 0;
    }

    void
    RescueMacTdma::StartOver() {
        NS_LOG_FUNCTION("");
        NS_LOG_INFO("return packet to the front of TX queue");
        m_pktQueue.push_front(m_pktData);
    }


    // ---------------------- Receive Functions ----------------------------

    void
    RescueMacTdma::ReceiveData(Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode, bool correctData) {
        NS_LOG_FUNCTION("");
        if (correctData) {
            ReceiveCorrectData(pkt, phyHdr, mode);
        } else //uncorrect data
        {
            NS_LOG_INFO("THE FRAME COPY WAS STORED!");
        }
    }

    void
    RescueMacTdma::ReceiveCorrectData(Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode) {
        NS_LOG_FUNCTION("");

        RescueMacHeader hdr;
        pkt->RemoveHeader(hdr);
        RescueMacTrailer fcs;
        pkt->RemoveTrailer(fcs);
        NS_LOG_FUNCTION("src:" << phyHdr.GetSource() <<
                "sequence: " << phyHdr.GetSequence());

        m_state = WAIT_TX;
        // forward upper layer
        if (IsNewSequence(phyHdr.GetSource(), phyHdr.GetSequence())) {
            NS_LOG_INFO("DATA RX OK!");
            SnrPerTag tag;
            pkt->PeekPacketTag(tag); //to send back SnrPerTag - for possible use in the future (multi rate control etc.)
            /*m_sendAckEvent = Simulator::Schedule ( (m_ackTxTime - Simulator::Now()),
                                                   &RescueMacTdma::SendAck, this,
                                                   phyHdr.GetSource (), phyHdr.GetSequence (),
                                                   mode, tag );*/
            GenerateAck(phyHdr.GetSource(), phyHdr.GetSequence(), mode, tag);
            //m_forwardUpCb (pkt, phyHdr.GetSource (), phyHdr.GetDestination ());
            m_hiMac->ReceivePacket(pkt, phyHdr);
        } else {
            NS_LOG_INFO("(duplicate) DROP!");
        }
    }

    void
    RescueMacTdma::ReceiveRelayData(Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode) {
        NS_LOG_FUNCTION("src:" << phyHdr.GetSource() <<
                "dst:" << phyHdr.GetDestination() <<
                "seq:" << phyHdr.GetSequence() <<
                "mode:" << mode);

        NS_LOG_INFO("FRAME TO RELAY!");

        phyHdr.SetSender(m_address);
        std::pair<Ptr<Packet>, RescuePhyHeader> pktRelay(pkt, phyHdr);
        m_pktRelayQueue.push_back(pktRelay);
        //m_pktRelay = std::pair<Ptr<Packet>, RescuePhyHeader> (pkt, phyHdr);

        m_state = IDLE;
        return;
    }

    void
    RescueMacTdma::ReceiveBroadcastData(Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode, bool correctData) {
        NS_LOG_FUNCTION("");

        //don't know what to do
    }

    void
    RescueMacTdma::ReceiveAck(Ptr<Packet> ackPkt, RescuePhyHeader phyHdr, double ackSnr, RescueMode ackMode) {
        NS_LOG_FUNCTION("src:" << phyHdr.GetSource() <<
                "dst:" << phyHdr.GetDestination() <<
                "seq:" << phyHdr.GetSequence());

        m_state = IDLE;

        RescueMacHeader hdr;
        m_pktData->PeekHeader(hdr);

        //check - have I recently transmitted frame which is acknowledged?
        if ((phyHdr.GetDestination() == m_hiMac->GetAddress())
                && (hdr.GetDestination() == phyHdr.GetSource())
                && (hdr.GetSequence() == phyHdr.GetSequence())) {
            NS_LOG_INFO("GOT ACK!");
            m_ackTimeoutEvent.Cancel();
            SnrPerTag tag;
            ackPkt->PeekPacketTag(tag);
            m_remoteStationManager->ReportDataOk(phyHdr.GetSource(),
                    ackSnr, ackMode,
                    tag.GetSNR(), /*tag.GetPER ()*/tag.GetBER());
            NS_LOG_INFO("DATA TX OK!");
            m_traceSendDataDone(m_device->GetNode()->GetId(), m_device->GetIfIndex(), phyHdr.GetSource(), true);
            SendDataDone();
        } else {
            NS_LOG_INFO("ACK ERROR (acknowledged frame was not originated here)");
        }
        return;
    }

    void
    RescueMacTdma::ReceiveResourceReservation(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
    }

    void
    RescueMacTdma::ReceiveBeacon(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
    }

    void
    RescueMacTdma::ReceivePacket(Ptr<RescuePhy> phy, Ptr<Packet> pkt) {
        NS_LOG_FUNCTION("");
        switch (m_state) {
            case WAIT_TX:
            case RX:
            case WAIT_RX:
            case IDLE:
                m_state = RX;
                break;
            case TX:
            case COLL:
                break;
        }
    }

    void
    RescueMacTdma::ReceivePacketDone(Ptr<RescuePhy> phy, Ptr<Packet> pkt, RescuePhyHeader phyHdr,
            double snr, RescueMode mode,
            bool correctPhyHdr, bool correctData, bool wasReconstructed) {
        NS_LOG_FUNCTION("");
        m_state = IDLE;

        if (!correctPhyHdr) {
            NS_LOG_INFO("PHY HDR DAMAGED - DROP!");
            return;
        } else if (phyHdr.GetSource() == m_address) {
            NS_LOG_INFO("FRAME RETURNED TO SOURCE - DROP!");
            return;
        } else {
            switch (phyHdr.GetType()) {
                case RESCUE_PHY_PKT_TYPE_DATA:
                    if (phyHdr.GetDestination() == m_hiMac->GetBroadcast()) {
                        NS_LOG_INFO("PHY HDR OK!" <<
                                ", DATA: " << (correctData ? "OK!" : "DAMAGED!") <<
                                ", BROADCASTED FRAME!");
                        ReceiveBroadcastData(pkt, phyHdr, mode, correctData);
                    } else if (phyHdr.GetDestination() != m_address) {
                        NS_LOG_INFO("PHY HDR OK!" <<
                                ", DATA: " << (correctData ? "BER OK!" : "UNUSABLE - DROP!") <<
                                ", FRAME TO RELAY ?!" << ((m_hiMac->ShouldBeForwarded(pkt, phyHdr)) ? "YES!" : "NO - DROP!"));
                        if (correctData
                                && m_hiMac->ShouldBeForwarded(pkt, phyHdr)) {
                            ReceiveRelayData(pkt, phyHdr, mode);
                        } else {
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
                    //no partial ACK in TDMA mode
                    break;
                case RESCUE_PHY_PKT_TYPE_B:
                    ReceiveBeacon(pkt, phyHdr);
                    break;
                case RESCUE_PHY_PKT_TYPE_RR:
                    ReceiveResourceReservation(pkt, phyHdr);
                    break;
                default:
                    break;
            }
        }
    }



    // -------------------------- Timeout ----------------------------------

    void
    RescueMacTdma::AckTimeout(Ptr<Packet> pkt, RescueMacHeader hdr) {
        NS_LOG_FUNCTION("try: " << m_remoteStationManager->GetRetryCount(hdr.GetDestination()));
        NS_LOG_INFO("!!! ACK TIMEOUT !!!");
        //m_state = IDLE;
        m_remoteStationManager->ReportDataFailed(hdr.GetDestination());
        m_traceAckTimeout(m_device->GetNode()->GetId(), m_device->GetIfIndex(), pkt, hdr);

        if (m_remoteStationManager->NeedDataRetransmission(hdr.GetDestination(), pkt)) {
            NS_LOG_INFO("RETRANSMISSION");
            StartOver();
            //SendData ();
        } else {
            // Retransmission is over the limit. Drop it!
            NS_LOG_INFO("DATA TX FAIL!");
            m_remoteStationManager->ReportFinalDataFailed(hdr.GetDestination());
            m_traceSendDataDone(m_device->GetNode()->GetId(), m_device->GetIfIndex(), hdr.GetDestination(), false);
            SendDataDone();
        }
    }



    // --------------------------- ETC -------------------------------------

    bool
    RescueMacTdma::IsNewSequence(Mac48Address addr, uint16_t seq) {
        NS_LOG_FUNCTION("");
        std::list<std::pair<Mac48Address, uint16_t> >::iterator it = m_seqList.begin();
        for (; it != m_seqList.end(); ++it) {
            if (it->first == addr) {
                if (it->second == 65536 && seq < it->second) {
                    it->second = seq;
                    return true;
                } else if (seq > it->second) {
                    it->second = seq;
                    return true;
                } else {
                    return false;
                }
            }
        }
        std::pair<Mac48Address, uint16_t> newEntry(addr, seq);
        m_seqList.push_back(newEntry);
        return true;
    }

} // namespace ns3

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
#include "ns3/assert.h"
#include "ns3/log.h"

#include "rescue-arq-manager.h"
#include "rescue-mac.h"
#include "rescue-mac-header.h"

NS_LOG_COMPONENT_DEFINE("RescueArqManager");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_mac != 0) { std::clog << "[time=" << ns3::Simulator::Now() << "] [addr=" << m_mac->GetAddress () << "] [ARQ] "; }

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(RescueArqManager);

    TypeId
    RescueArqManager::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::RescueArqManager")
                .SetParent<Object> ()
                .AddConstructor<RescueArqManager> ()
                .AddAttribute("MaxRetryCount", "The maximum number of retransmission attempts for a DATA packet. This value"
                " will not have any effect on some rate control algorithms.",
                UintegerValue(7),
                MakeUintegerAccessor(&RescueArqManager::m_maxRetryCount),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("BasicAckTimeout",
                "ACK awaiting time",
                TimeValue(MicroSeconds(3000)),
                MakeTimeAccessor(&RescueArqManager::m_basicAckTimeout),
                MakeTimeChecker())

                .AddTraceSource("MacTxDataFailed",
                "The transmission of a data packet by the MAC layer has failed",
                MakeTraceSourceAccessor(&RescueArqManager::m_macTxDataFailed))
                .AddTraceSource("MacTxFinalDataFailed",
                "The transmission of a data packet has exceeded the maximum number of attempts",
                MakeTraceSourceAccessor(&RescueArqManager::m_macTxFinalDataFailed))
                ;
        return tid;
    }

    RescueArqManager::RescueArqManager()
    : m_txAllowed(true),
    m_ackTimeoutEvent() {
        NS_LOG_FUNCTION(m_basicAckTimeout);
    }

    RescueArqManager::~RescueArqManager() {
        NS_LOG_FUNCTION("");
    }

    void
    RescueArqManager::DoDispose() {
        NS_LOG_FUNCTION("");
        if (m_ackTimeoutEvent.IsRunning())
            m_ackTimeoutEvent.Cancel();
    }

    void
    RescueArqManager::SetupMac(Ptr<RescueMac> mac) {
        m_mac = mac;
    }

    void
    RescueArqManager::SetBasicAckTimeout(Time duration) {
        NS_LOG_FUNCTION(duration);
        m_basicAckTimeout = duration;
    }

    Time
    RescueArqManager::GetBasicAckTimeout(void) const {
        return m_basicAckTimeout;
    }

    uint16_t
    RescueArqManager::GetMaxRetryCount(void) const {
        return m_maxRetryCount;
    }

    void
    RescueArqManager::SetMaxRetryCount(uint16_t maxRetryCount) {
        m_maxRetryCount = maxRetryCount;
    }

    bool
    RescueArqManager::IsTxAllowed() {
        return m_txAllowed;
    }

    void
    RescueArqManager::EnableDataTx() {
        m_txAllowed = true;
        m_mac->NotifyTxAllowed();
    }

    void
    RescueArqManager::DisableDataTx() {
        m_txAllowed = false;
    }

    uint16_t
    RescueArqManager::GetNextSequenceNumber(const RescuePhyHeader *hdr) {
        NS_LOG_FUNCTION("");
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
        NS_LOG_FUNCTION("");
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
        NS_LOG_FUNCTION("");
        uint16_t seq = 0;
        std::map <Mac48Address, uint16_t>::const_iterator it = m_sequences.find(addr);
        if (it != m_sequences.end()) {
            return it->second;
        }
        return seq;
    }

    void
    RescueArqManager::ReportNewDataFrame(Mac48Address dst, uint16_t seq) {
        NS_LOG_FUNCTION("");
        //store info about sended packet
        SendSeqList::iterator it = m_createdFrames.find(dst);
        if (it != m_createdFrames.end()) {
            //found destination
            SeqList::iterator it2 = dstSeqList(it).seqList.find(seq);
            if (it2 != dstSeqList(it).seqList.end()) {
                //found TXed frame with given SEQ - seq. numbers reuse - overwrite
                seqAck(it2).ACKed = false;
                seqAck(it2).retryCount = 0;
            } else {
                //new SEQ number for given destination
                std::pair <bool, uint16_t > ACKedRetryCount(false, 0);
                dstSeqList(it).seqList.insert(std::pair <uint16_t, std::pair <bool, uint16_t> > (seq, ACKedRetryCount));
            }
        } else {
            //new destination
            SeqList newSeqList;
            std::pair <bool, uint16_t > ACKedRetryCount(false, 0);
            newSeqList.insert(std::pair <uint16_t, std::pair <bool, uint16_t> > (seq, ACKedRetryCount));
            m_createdFrames.insert(std::pair <Mac48Address, SeqList> (dst, newSeqList));
        }
    }

    void
    RescueArqManager::ReportDataTx(Ptr<Packet> pkt) {
        NS_LOG_FUNCTION("");
        DisableDataTx();
        NS_LOG_DEBUG("ACK TIMEOUT TIMER START");
        m_ackTimeoutEvent = Simulator::Schedule(m_basicAckTimeout, &RescueArqManager::AckTimeout, this, pkt);
    }

    void
    RescueArqManager::AckTimeout(Ptr<Packet> pkt) {
        NS_LOG_FUNCTION("");
        RescueMacHeader hdr;
        pkt->RemoveHeader(hdr);

        NS_LOG_INFO("!!! ACK TIMEOUT !!!");
        //m_state = IDLE;
        //m_remoteStationManager->ReportDataFailed (hdr.GetDestination ());
        //m_traceAckTimeout (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt, hdr);

        if (NeedDataRetransmission(hdr.GetDestination(), hdr.GetSequence())) {
            NS_LOG_INFO("RETRANSMISSION");
            hdr.SetRetry();
            pkt->AddHeader(hdr);
            m_mac->EnqueueRetry(pkt, hdr.GetDestination());
            //SendData ();
        } else {
            // Retransmission is over the limit. Drop it!
            NS_LOG_INFO("DATA TX FAIL!");
            EnableDataTx();
            //m_remoteStationManager->ReportFinalDataFailed (hdr.GetDestination ());
            //m_traceSendDataDone (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), hdr.GetDestination (), false);
            //SendDataDone ();
        }
    }

    bool
    RescueArqManager::NeedDataRetransmission(Mac48Address dst, uint16_t seq) {
        NS_LOG_FUNCTION("");
        NS_ASSERT(!dst.IsGroup());

        //search for checked frame
        SendSeqList::iterator it = m_createdFrames.find(dst);
        if (it != m_createdFrames.end()) {
            //found destination
            SeqList::iterator it2 = dstSeqList(it).seqList.find(seq);
            if (it2 != dstSeqList(it).seqList.end()) {
                //found TXed frame with given SEQ - check retry counter
                NS_LOG_FUNCTION("try: " << seqAck(it2).retryCount);
                return (seqAck(it2).retryCount < GetMaxRetryCount());
            } else {
                //new SEQ number for given destination???
                NS_ASSERT("frame with given seq. no. was not originated here");
            }
        } else {
            //ACK for other
            NS_ASSERT("ACK ERROR (acknowledged frame was not originated here)");
        }
        return false;
    }

    int
    RescueArqManager::ReceiveAck(RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
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
                    EnableDataTx();
                    m_ackTimeoutEvent.Cancel();
                    //m_traceSendDataDone (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), phyHdr.GetSource (), true);
                    return (int) seqAck(it2).retryCount;
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
        return -1;
    }

    uint16_t
    RescueArqManager::CheckRxSequenceFor(Mac48Address src) {
        NS_LOG_FUNCTION("");
        RecvSeqList::iterator it = m_receivedFrames.find(src);
        if (it != m_receivedFrames.end())
            return srcSeq(it).seq;
        else
            return 0;
    }

    bool
    RescueArqManager::CheckRxSequence(Mac48Address src, uint16_t seq) {
        NS_LOG_FUNCTION("");

        RecvSeqList::iterator it = m_receivedFrames.find(src);
        if (it != m_receivedFrames.end()) {
            //found source
            if ((seq < 100) && (srcSeq(it).seq >= (65435))) //65535 - 100
            {
                //reuse seq. numbers
                NS_LOG_INFO("NEW - reuse seq. numbers");
                return true;
            } else if (seq > srcSeq(it).seq) {
                //new seq. number
                NS_LOG_INFO("NEW - new seq. number");
                return true;
            } else {
                //old seq. number (duplicate or reorder)
                NS_LOG_INFO("OLD - old seq. number (duplicate or reorder)");
                return false;
            }
        } else {
            //new source
            NS_LOG_INFO("NEW - new source");
            return true;
        }
        return true;
    }

    bool
    RescueArqManager::CheckRxFrame(Mac48Address src, uint16_t seq) {
        NS_LOG_FUNCTION("");

        bool isNewSequence = false;
        RecvSeqList::iterator it = m_receivedFrames.find(src);
        if (it != m_receivedFrames.end()) {
            //found source
            if ((seq < 100) && (srcSeq(it).seq >= (65435))) //65535 - 100
            {
                //reuse seq. numbers
                NS_LOG_INFO("reuse seq. numbers");
                srcSeq(it).seq = seq;
                isNewSequence = true;
            } else if (seq > srcSeq(it).seq) {
                //new seq. number
                NS_LOG_INFO("new seq. number");
                srcSeq(it).seq = seq;
                isNewSequence = true;
            } else {
                //old seq. number (duplicate or reorder)
                NS_LOG_INFO("old seq. number (duplicate or reorder)");
                isNewSequence = false;
            }
        } else {
            //new source
            NS_LOG_INFO("new source");
            m_receivedFrames.insert(std::pair<Mac48Address, uint16_t> (src, seq));
            isNewSequence = true;
        }

        return isNewSequence;
    }

    bool
    RescueArqManager::CheckRxFrame(Mac48Address src, uint16_t seq, RescuePhyHeader *ackHdr) {
        NS_LOG_FUNCTION("");

        bool isNewSequence = CheckRxFrame(src, seq);

        bool txACK = isNewSequence;

        if (txACK) {
            ackHdr->SetSequence(seq);
        } else {
            ackHdr->SetDestination("00:00:00:00:00:00");
        }

        return isNewSequence;
    }

} // namespace ns3

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

#include "rescue-utils.h"

#include <algorithm>

NS_LOG_COMPONENT_DEFINE("RescueArqManager");

#undef NS_LOG_APPEND_CONTEXT
//#define NS_LOG_APPEND_CONTEXT if (m_mac != 0) { std::clog << "[time=" << ns3::Simulator::Now () << "] [addr=" << m_mac->GetAddress () << "] [ARQ] "; }
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now().GetMicroSeconds() << "] [addr=" << ((m_mac != 0) ? compressMac(m_mac->GetAddress ()) : 0) << "] [ARQ] "

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
                .AddAttribute("SendWindow", "The maximum number of frame transmitted since the last acknowledged frame",
                UintegerValue(4),
                MakeUintegerAccessor(&RescueArqManager::m_sendWindow),
                MakeUintegerChecker<uint8_t> ())
                .AddAttribute("SendNack", "If true, send NACKs after NACK Timeout period",
                BooleanValue(false),
                MakeBooleanAccessor(&RescueArqManager::m_sendNACK),
                MakeBooleanChecker())
                .AddAttribute("NackTimeout",
                "NACK timeout",
                TimeValue(MicroSeconds(3000)),
                MakeTimeAccessor(&RescueArqManager::m_nackTimeout),
                MakeTimeChecker())
                .AddAttribute("BasicAckTimeout",
                "ACK awaiting time",
                TimeValue(MicroSeconds(3000)),
                MakeTimeAccessor(&RescueArqManager::m_basicAckTimeout),
                MakeTimeChecker())
                .AddAttribute("LongAckTimeout",
                "ACK awaiting time",
                TimeValue(MicroSeconds(15000)),
                MakeTimeAccessor(&RescueArqManager::m_longAckTimeout),
                MakeTimeChecker())
                .AddAttribute("UseBlockACK", "If true, use block ACK feature",
                BooleanValue(false),
                MakeBooleanAccessor(&RescueArqManager::m_useBlockACK),
                MakeBooleanChecker())
                .AddAttribute("BlockAckTimeout",
                "Bock ACK timeout",
                TimeValue(MicroSeconds(15000)),
                MakeTimeAccessor(&RescueArqManager::m_blockAckTimeout),
                MakeTimeChecker())
                .AddAttribute("UseBlockNACK", "If true, BlockACK can be started with unsuccessfully received frame",
                BooleanValue(false),
                MakeBooleanAccessor(&RescueArqManager::m_useBlockNACK),
                MakeBooleanChecker())
                .AddAttribute("UseContinousACK", "If true, use continous ACK feature",
                BooleanValue(false),
                MakeBooleanAccessor(&RescueArqManager::m_useContinousACK),
                MakeBooleanChecker())

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
    : m_txAllowed(true) {
        NS_LOG_FUNCTION("");
    }

    RescueArqManager::~RescueArqManager() {
        NS_LOG_FUNCTION("");
    }

    void
    RescueArqManager::DoDispose() {
        NS_LOG_FUNCTION("");

        for (TimersList::iterator it = m_ackTimeoutTimers.begin(); it != m_ackTimeoutTimers.end(); ++it) {
            if (it->second.IsRunning())
                it->second.Cancel();
        }
        for (TimersList::iterator it = m_nackTimeoutTimers.begin(); it != m_nackTimeoutTimers.end(); ++it) {
            if (it->second.IsRunning())
                it->second.Cancel();
        }
        for (TimersList::iterator it = m_blockAckTimeoutTimers.begin(); it != m_blockAckTimeoutTimers.end(); ++it) {
            if (it->second.IsRunning())
                it->second.Cancel();
        }
    }



    // ------------------------ Set Functions -----------------------------

    void
    RescueArqManager::SetupMac(Ptr<RescueMac> mac) {
        m_mac = mac;
    }

    void
    RescueArqManager::SetBasicAckTimeout(Time duration) {
        NS_LOG_FUNCTION(duration);
        m_basicAckTimeout = duration;
    }

    void
    RescueArqManager::SetMaxRetryCount(uint16_t maxRetryCount) {
        m_maxRetryCount = maxRetryCount;
    }



    // ------------------------ Get Functions -----------------------------

    Time
    RescueArqManager::GetBasicAckTimeout(void) const {
        return m_basicAckTimeout;
    }

    uint16_t
    RescueArqManager::GetMaxRetryCount(void) const {
        return m_maxRetryCount;
    }



    // ---------------------- Sender Functions ----------------------------

    void
    RescueArqManager::EnableDataTx() {
        m_txAllowed = true;
        m_mac->NotifyTxAllowed();
    }

    void
    RescueArqManager::DisableDataTx() {
        m_txAllowed = false;
    }

    bool
    RescueArqManager::IsTxAllowed() {
        return m_txAllowed;
    }

    bool
    RescueArqManager::IsTxAllowed(Mac48Address dst) {
        SendSeqList::iterator it = m_createdFrames.find(dst);
        NS_LOG_DEBUG("unACKedFrames: " << (int) it->second.unACKedFrames << "sendWindow: " << (int) m_sendWindow);
        if (it != m_createdFrames.end())
            return ( (it->second.unACKedFrames < m_sendWindow)
                && (!m_useContinousACK || (it->second.contACKed + MAX_WINDOW_SIZE > it->second.nextSequence)));
        else
            return true;
    }

    uint16_t
    RescueArqManager::GetNextSequenceNumber(const RescuePhyHeader *hdr) {
        NS_LOG_FUNCTION("");
        uint16_t retval;
        Mac48Address dst = hdr->GetDestination();

        SendSeqList::iterator it = m_createdFrames.find(dst);
        if (it != m_createdFrames.end()) {
            retval = it->second.nextSequence;
            it->second.nextSequence++;
        } else {
            retval = 0;

            //new destination
            SendList newSendList;
            for (uint16_t i = 0; i < ARRAY_END; i++) {
                newSendList.ACKed[i] = false;
                newSendList.retryCount[i] = 0;
            }
            newSendList.ACKed[ARRAY_END] = false;
            newSendList.retryCount[ARRAY_END] = 0;
            newSendList.nextSequence = 0;
            newSendList.unACKedFrames = 0;
            newSendList.rxedUnACKedFrames = 0;
            newSendList.contACKed = 0;

            std::pair < SendSeqList::iterator, bool> newIns = m_createdFrames.insert(std::pair <Mac48Address, SendList> (dst, newSendList));
            NS_ASSERT(newIns.second == true);

            newIns.first->second.nextSequence++;
        }
        NS_LOG_INFO("SET SEQ: " << retval);
        return retval;
    }

    uint16_t
    RescueArqManager::GetNextSequenceNumber(const RescueMacHeader *hdr) {
        NS_LOG_FUNCTION("");
        uint16_t retval;
        Mac48Address dst = hdr->GetDestination();

        SendSeqList::iterator it = m_createdFrames.find(dst);
        if (it != m_createdFrames.end()) {
            retval = it->second.nextSequence;
            it->second.nextSequence++;
        } else {
            retval = 0;

            //new destination
            SendList newSendList;
            for (uint16_t i = 0; i < ARRAY_END; i++) {
                newSendList.ACKed[i] = false;
                newSendList.retryCount[i] = 0;
            }
            newSendList.ACKed[ARRAY_END] = false;
            newSendList.retryCount[ARRAY_END] = 0;
            newSendList.nextSequence = 0;
            newSendList.unACKedFrames = 0;
            newSendList.rxedUnACKedFrames = 0;
            newSendList.contACKed = 0;

            std::pair < SendSeqList::iterator, bool> newIns = m_createdFrames.insert(std::pair <Mac48Address, SendList> (dst, newSendList));
            NS_ASSERT(newIns.second == true);

            newIns.first->second.nextSequence++;
        }
        NS_LOG_INFO("SET SEQ: " << retval);
        return retval;
    }

    uint16_t
    RescueArqManager::GetNextSeqNumberByAddress(Mac48Address dst) const {
        NS_LOG_FUNCTION("");
        SendSeqList::const_iterator it = m_createdFrames.find(dst);
        if (it != m_createdFrames.end()) {
            return it->second.nextSequence;
        }
        return 0;
    }

    void
    RescueArqManager::ReportNewDataFrame(Mac48Address dst, uint16_t seq) {
        NS_LOG_FUNCTION("");
        //store info about sended packet
        SendSeqList::iterator it = m_createdFrames.find(dst);
        if (it != m_createdFrames.end()) {
            //found destination
            it->second.ACKed[seq] = false;
            it->second.retryCount[seq] = 0;
        } else
            NS_ASSERT("SendSeqList record should be already created!");
    }

    void
    RescueArqManager::ConfigurePhyHeader(RescuePhyHeader *phyHdr) {
        NS_LOG_FUNCTION("");
        phyHdr->SetSendWindow(m_sendWindow - 1);

        if (m_useBlockACK) {
            NS_LOG_FUNCTION("BLOCK ACK enabled");
            phyHdr->SetBlockAckEnabled();
        } else {
            NS_LOG_FUNCTION("BLOCK ACK disabled");
            phyHdr->SetBlockAckDisabled();
        }

        if (m_useContinousACK) {
            NS_LOG_FUNCTION("CONTINOUS ACK enabled");
            phyHdr->SetContinousAckEnabled();
        } else {
            NS_LOG_FUNCTION("CONTINOUS ACK disabled");
            phyHdr->SetContinousAckDisabled();
        }
    }

    void
    RescueArqManager::ReportDataTx(Ptr<Packet> pkt) {
        NS_LOG_FUNCTION("");
        //DisableDataTx ();

        RescueMacHeader hdr;
        pkt->PeekHeader(hdr);
        Mac48Address dst = hdr.GetDestination();

        if (!hdr.IsRetry()) {
            //increase unACKEd frames counter
            SendSeqList::iterator it = m_createdFrames.find(dst);
            if (it != m_createdFrames.end()) {
                it->second.unACKedFrames++;
                NS_LOG_INFO("total number of transmissions to " << dst << " is: " << (int) it->second.unACKedFrames);
            } else
                NS_ASSERT("SendSeqList record should be already created!");
        }

        NS_LOG_DEBUG("ACK TIMEOUT TIMER START for dst: " << hdr.GetDestination() << ", seq: " << hdr.GetSequence());

        std::pair <Mac48Address, uint16_t> frameId(hdr.GetDestination(), hdr.GetSequence());
        std::pair < TimersList::iterator, bool> newIns = m_ackTimeoutTimers.insert(std::pair <std::pair <Mac48Address, uint16_t>, Timer> (frameId, Timer(Timer::CANCEL_ON_DESTROY)));

        if (newIns.first->second.IsRunning())
            newIns.first->second.Cancel();

        newIns.first->second.SetDelay(m_longAckTimeout);
        newIns.first->second.SetFunction(&RescueArqManager::AckTimeout, this);
        newIns.first->second.SetArguments(pkt);
        newIns.first->second.Schedule();
    }

    void
    RescueArqManager::AckTimeout(Ptr<Packet> pkt) {

        NS_LOG_FUNCTION("");
        RescueMacHeader hdr;
        pkt->RemoveHeader(hdr);

        NS_LOG_INFO("!!! ACK TIMEOUT !!! for dst: " << hdr.GetDestination() << ", seq: " << hdr.GetSequence());

        if (NeedDataRetransmission(hdr.GetDestination(), hdr.GetSequence())) {
            NS_LOG_INFO("RETRANSMISSION");
            hdr.SetRetry();
            pkt->AddHeader(hdr);
            m_mac->EnqueueRetry(pkt, hdr.GetDestination());
        } else {
            // Retransmission is over the limit. Drop it!
            NS_LOG_INFO("DATA TX FAIL!");
            //EnableDataTx ();

            //erase ACK timer
            std::pair <Mac48Address, uint16_t> frameId(hdr.GetDestination(), hdr.GetSequence());
            TimersList::iterator it2 = m_ackTimeoutTimers.find(std::pair <Mac48Address, uint16_t> (frameId));
            if (it2 != m_ackTimeoutTimers.end())
                m_ackTimeoutTimers.erase(it2);

            //decrease unACKEd frames counter
            /*SendSeqList::iterator it = m_createdFrames.find (hdr.GetDestination ());
            if (it != m_createdFrames.end ())
              {
                it->second.unACKedFrames--;
                NS_LOG_INFO ("total number of transmissions to " << hdr.GetDestination () << " is: " << (int)it->second.unACKedFrames);
              }*/
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
            //return (!it->second.ACKed[seq] && ((it->second.retryCount[seq] < GetMaxRetryCount ()) || m_useContinousACK));
            return (!it->second.ACKed[seq]);
        } else {
            //ACK for other
            NS_ASSERT("ACK ERROR (acknowledged frame was not originated here)");
        }
        return false;
    }

    //MODIF 3

    void RescueArqManager::RelayingStopAck(Mac48Address dst, uint16_t seq) {


        TimersList::iterator it = m_ackTimeoutTimers.find(std::pair <Mac48Address, uint16_t> (std::make_pair(dst, seq)));
        if (it != m_ackTimeoutTimers.end()) {
            m_ackTimeoutTimers.erase(it);
            NS_LOG_INFO("\t\t\t\t RETRANSMISSION HAS BEEN CANCELED !!!!!!!!!!!!!!!!!!!!!! :) ");
        } else
            NS_LOG_INFO("\t\t\t\t COULD NOT FIND ACK TIMEOUT EVENT");

        SendSeqList::iterator it2 = m_createdFrames.find(dst);
        if (it2 != m_createdFrames.end()) {
            if (!it2->second.ACKed[seq]) {
                NS_LOG_INFO("\t\t\t\t frame " << seq << " from " << dst << " should be ACKED NOW ");
                AcknowledgeFrame(it2, seq);
            } else
                NS_LOG_INFO("\t\t\t\t frame " << seq << " from " << dst << " has already been ACKED");
        }
    }

    int
    RescueArqManager::ReceiveAck(const RescuePhyHeader *ackHdr) {
        NS_LOG_FUNCTION("");
        //check - have I recently transmitted frame which is acknowledged?
        Mac48Address src = ackHdr->GetSource();
        uint16_t seq = ackHdr->GetSequence();

        int retval = -1;
        int8_t tempUnACKedFrames = 0;

        SendSeqList::iterator it = m_createdFrames.find(src);
        if (it != m_createdFrames.end()) {
            //found destination

            //check Continous ACK
            if (m_useContinousACK && ackHdr->IsContinousAckEnabled()) {
                //if ( ((seq - ackHdr->GetContinousAck ()) >= it->second.contACKed)
                //     || ( ((seq - ackHdr->GetContinousAck ()) < MAX_WINDOW_SIZE) && (it->second.contACKed > ARRAY_END - MAX_WINDOW_SIZE) ) )
                if (!SeqComp(it->second.contACKed, seq - ackHdr->GetContinousAck())) {
                    for (uint16_t i = it->second.contACKed; SeqComp(seq - ackHdr->GetContinousAck() + 1, i); i++)
                        if (!it->second.ACKed[i]) {
                            NS_LOG_INFO("CONTINOUS ACK for src: " << src << ", seq: " << i << " RECEIVED");
                            AcknowledgeFrame(it, i);
                            retval = 0;
                        }
                    it->second.contACKed = seq - ackHdr->GetContinousAck();
                }
            } else if (m_useContinousACK && !ackHdr->IsContinousAckEnabled())
                //first frame not received? - try to unacknowledge it!
                if (!it->second.ACKed[0])
                    UnacknowledgeFrame(it, 0);

            //check Block ACK
            if (m_useBlockACK && ackHdr->IsBlockAckEnabled()) {
                uint16_t start_seq = seq;
                if (m_useContinousACK && ackHdr->IsContinousAckEnabled())
                    start_seq = SeqComp(seq - ackHdr->GetContinousAck() + 17, seq) ? seq : seq - ackHdr->GetContinousAck() + 17;

                uint16_t start_i = 15;
                /*if ( ((start_seq - 16) <= it->second.contACKed)
                     && (it->second.contACKed < HALF_OF_ARRAY) )
                  start_i = start_seq - it->second.contACKed;*/
                if (((start_seq - 16) < 0)
                        && (it->second.contACKed < HALF_OF_ARRAY))
                    start_i = start_seq - 1;

                if (start_i < 0) start_i = 0;
                if (start_i > HALF_OF_ARRAY) start_i = 0;

                for (int i = start_i; i >= 0; i--) {
                    uint16_t seq_temp = start_seq - i - 1;
                    NS_LOG_INFO("i: " << i << " BLOCK ACK for src: " << src << ", seq: " << seq_temp << " " << (ackHdr->IsBlockAckFrameReceived(i) ? "RECEIVED" : "MISSED"));
                    if (!it->second.ACKed[seq_temp]) {
                        if (ackHdr->IsBlockAckFrameReceived(i)) {
                            AcknowledgeFrame(it, seq_temp);
                            retval = 0;
                        } else
                            UnacknowledgeFrame(it, seq_temp);
                    }
                    if (!ackHdr->IsBlockAckFrameReceived(i))
                        tempUnACKedFrames++;
                }
            }

            //process basic ACK/NACK
            if (!ackHdr->IsACK())
                tempUnACKedFrames++;
            if (!it->second.ACKed[seq]) {
                if (ackHdr->IsACK()) {
                    NS_LOG_INFO("BASIC ACK for src: " << src << ", seq: " << seq);
                    AcknowledgeFrame(it, seq);
                    retval = (int) it->second.retryCount[seq];
                } else {
                    NS_LOG_INFO("BASIC NACK for src: " << src << ", seq: " << seq);
                    UnacknowledgeFrame(it, seq);
                }
            } else if (GetNextSeqNumberByAddress(src) > seq)
                NS_LOG_INFO("DUPLICATED ACK (or corrupted)");
            else
                //new SEQ number for given destination???
                NS_LOG_INFO("ACK ERROR (frame with given seq. no. was not originated here)");

            NS_LOG_INFO("old unACKedFrames: " << (int) it->second.unACKedFrames << ", this ACK NACKedFrames: " << (int) ackHdr->GetNACKedFrames());
            //it->second.unACKedFrames = ackHdr->GetNACKedFrames () + (it->second.nextSequence - 1 - seq);

            uint8_t NewUnACKedFrames = 0;
            if (seq + 1 < it->second.nextSequence)
                for (uint16_t i = seq + 1; SeqComp(it->second.nextSequence, i); i++) {
                    NS_LOG_DEBUG("i: " << i << "it->second.ACKed[i]: " << ((it->second.ACKed[i]) ? "YES" : "NO"));
                    if (!it->second.ACKed[i])
                        NewUnACKedFrames++;
                }
            if ((it->second.unACKedFrames - NewUnACKedFrames) > ackHdr->GetNACKedFrames())
                it->second.unACKedFrames = ackHdr->GetNACKedFrames() + NewUnACKedFrames;
            NS_LOG_INFO("new unACKedFrames: " << (int) it->second.unACKedFrames);

            if ((m_useBlockACK && ackHdr->IsBlockAckEnabled())
                    && (m_useContinousACK && ackHdr->IsContinousAckEnabled())) {
                NS_LOG_DEBUG("tempUnACKedFrames: " << (int) tempUnACKedFrames);
                if (ackHdr->GetNACKedFrames() > tempUnACKedFrames) {
                    uint16_t last_block_seq = SeqComp(seq - ackHdr->GetContinousAck() + 17, seq) ? seq : seq - ackHdr->GetContinousAck() + 17;
                    NS_LOG_DEBUG("last_block_seq: " << last_block_seq);
                    //some frames were received but not ACKed by this ACK frame
                    it->second.rxedUnACKedFrames = (seq - last_block_seq) - (ackHdr->GetNACKedFrames() - tempUnACKedFrames);
                } else
                    it->second.rxedUnACKedFrames = 0;
                NS_LOG_INFO("rxedUnACKedFrames: " << (int) it->second.rxedUnACKedFrames);
            }
        } else
            //ACK for other station!
            NS_LOG_INFO("ACK ERROR (acknowledged frame was not originated here)");
        return retval;
    }

    void
    RescueArqManager::AcknowledgeFrame(SendSeqList::iterator it, uint16_t seq) {
        NS_LOG_FUNCTION("");
        Mac48Address src = (*it).first;
        NS_LOG_INFO("GOT ACK - DATA TX OK! for dst: " << src << ", seq: " << seq);
        (*it).second.ACKed[seq] = true;
        //m_traceSendDataDone (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), phyHdr.GetSource (), true);
        //EnableDataTx ();
        //m_ackTimeoutEvent.Cancel ();

        //cancel ACK timer
        std::pair <Mac48Address, uint16_t> frameId(src, seq);
        TimersList::iterator it2 = m_ackTimeoutTimers.find(std::pair <Mac48Address, uint16_t> (frameId));
        if (it2 != m_ackTimeoutTimers.end()) {
            NS_LOG_INFO("CANCEL ACK TIMEOUT COUNTER for dst: " << src << ", seq: " << seq);
            it2->second.Cancel();
            m_ackTimeoutTimers.erase(it2);
        }

        /*//cancel NACK timer
        TimersList::iterator it3 = m_nackTimeoutTimers.find (std::pair <Mac48Address, uint16_t> (frameId));
        if (it3 != m_nackTimeoutTimers.end () )
          {
            NS_LOG_INFO ("CANCEL NACK TIMEOUT COUNTER for dst: " << src << ", seq: " << seq);
            it3->second.Cancel ();
            m_nackTimeoutTimers.erase (it3);
          }*/

        //decrease unACKEd frames counter
        if (it->second.rxedUnACKedFrames > 0) {
            NS_LOG_DEBUG("decrease rxedUnACKedFrames counter as reception of this frame was already notified by sendWindow field");
            (*it).second.rxedUnACKedFrames--;
        } else if ((*it).second.unACKedFrames > 0) {
            NS_LOG_DEBUG("decrease unACKedFrames counter as reception of this frame was not notified yet");
            (*it).second.unACKedFrames--;
        }
        NS_LOG_INFO("total number of transmissions (unACKedFrames) to " << src << " is: " << (int) (*it).second.unACKedFrames);
    }

    void
    RescueArqManager::UnacknowledgeFrame(SendSeqList::iterator it, uint16_t seq) {
        NS_LOG_FUNCTION("");

        Mac48Address src = (*it).first;
        NS_LOG_INFO("GOT NACK - FORCE ACK TIMEOUT! for src: " << src << ", seq: " << seq);

        std::pair <Mac48Address, uint16_t> frameId(src, seq);
        TimersList::iterator it2 = m_ackTimeoutTimers.find(std::pair <Mac48Address, uint16_t> (frameId));
        if (it2 != m_ackTimeoutTimers.end()) {
            if (it2->second.IsRunning())
                it2->second.Cancel();
            it2->second.SetDelay(Seconds(0));
            it2->second.Schedule();

            NS_LOG_INFO("CANCEL ACK TIMEOUT COUNTER for dst: " << src << ", seq: " << seq);
        } else
            NS_LOG_INFO("NACK ERROR - timeout event is not running here!");
    }

    bool
    RescueArqManager::IsRetryACKed(const RescueMacHeader *hdr) {
        Mac48Address dst = hdr->GetDestination();
        uint16_t seq = hdr->GetSequence();

        SendSeqList::iterator it = m_createdFrames.find(dst);
        if (it != m_createdFrames.end())
            return it->second.ACKed[seq];
        else
            return false;
    }


    // ---------------------- Receiver Functions ----------------------------

    bool
    RescueArqManager::CheckRxSequence(Mac48Address src, uint16_t seq, bool useContinousACK) {
        NS_LOG_FUNCTION("");
        RecvSeqList::iterator it = m_receivedFrames.find(src);
        if (it != m_receivedFrames.end()) {
            //found source

            /* We cannot reuse all sequences at once because in advanced ARQ operation some frames with high seq. number
             * may be transmitted at the same time with low seq. numbers. (E.g. SEQ=0 is the next seq for SEQ=65535)
             * Thus we cane reuse main part of sequences at once, but the ending part shoud be reused "step by step".
             */

            if (!it->second.seq1stHalfCleaned
                    && (seq > (ARRAY_END - MAX_WINDOW_SIZE))) {
                //first half of sequence numbers should be reused
                for (uint16_t i = 0; i <= HALF_OF_ARRAY; i++) {
                    //std::cout << "seq: " << seq << ", i: " << i << std::endl;
                    it->second.copyRXed[i] = false;
                    it->second.RXed[i] = false;
                    it->second.RXtime[i] = Seconds(0);
                }
                it->second.seq1stHalfCleaned = true;
                it->second.seq2ndHalfCleaned = false;
            } else if (!it->second.seq2ndHalfCleaned
                    && (seq > (HALF_OF_ARRAY - MAX_WINDOW_SIZE))
                    && (seq <= HALF_OF_ARRAY)) {
                //second half of sequence numbers should be reused
                for (uint16_t i = HALF_OF_ARRAY + 1; i > HALF_OF_ARRAY; i++) {
                    //std::cout << "seq: " << seq << ", i: " << i << std::endl;
                    it->second.copyRXed[i] = false;
                    it->second.RXed[i] = false;
                    it->second.RXtime[i] = Seconds(0);
                }
                it->second.seq1stHalfCleaned = false;
                it->second.seq2ndHalfCleaned = true;
            }

            //it->second.copyRXed[seq] = true;
            //if ( (seq > it->second.maxSeqCopyRXed)
            //     || ((seq < MAX_WINDOW_SIZE) && (it->second.maxSeqCopyRXed > ARRAY_END - MAX_WINDOW_SIZE)) )
            if (SeqComp(seq, it->second.maxSeqCopyRXed))
                it->second.maxSeqCopyRXed = seq;
            NS_LOG_INFO("SEQ: " << (it->second.RXed[seq] ? "OLD" : "NEW"));
            return !it->second.RXed[seq];
        } else {
            //new source
            NS_LOG_INFO("new source");
            RecvList newIns;
            for (uint16_t i = 0; i < ARRAY_END; i++) {
                newIns.copyRXed[i] = false;
                newIns.RXed[i] = false;
                newIns.RXtime[i] = Seconds(0);
            }
            newIns.copyRXed[ARRAY_END] = false;
            newIns.RXed[ARRAY_END] = false;
            newIns.copyRXed[0] = true;
            newIns.maxSeqCopyRXed = 0;
            newIns.expectedSeq = 0;
            newIns.receivedFirstFrame = false;
            newIns.newestHeader = RescuePhyHeader(RESCUE_PHY_PKT_TYPE_E2E_ACK);
            newIns.seq1stHalfCleaned = true;
            newIns.seq2ndHalfCleaned = false;

            m_receivedFrames.insert(std::pair<Mac48Address, RecvList> (src, newIns));
            return true;
        }
        return true;
    }

    bool
    RescueArqManager::CheckRxFrame(const RescuePhyHeader *phyHdr, RescuePhyHeader *ackHdr) {
        NS_LOG_FUNCTION("");

        Mac48Address src = phyHdr->GetSource();
        uint16_t seq = phyHdr->GetSequence();

        bool isNewSequence = CheckRxSequence(src, seq, phyHdr->IsContinousAckEnabled());
        RecvSeqList::iterator it = m_receivedFrames.find(src);

        if (isNewSequence) {
            //mark received frame
            if (it != m_receivedFrames.end()) {
                it->second.RXed[seq] = true;
                it->second.RXtime[seq] = Simulator::Now();

                if (SeqComp(phyHdr->GetSequence(), it->second.newestHeader.GetSequence())) {
                    NS_LOG_DEBUG("store newest header for src: " << src << ", seq: " << seq);
                    RescuePhyHeader phyHdr_ = *phyHdr;
                    it->second.newestHeader = phyHdr_;
                } else if ((it->second.newestHeader.GetSequence() == 0) && !it->second.receivedFirstFrame) {
                    NS_LOG_DEBUG("store newest header for src: " << src << ", seq: " << seq);
                    RescuePhyHeader phyHdr_ = *phyHdr;
                    it->second.newestHeader = phyHdr_;
                }
                if ((seq == 0) && !it->second.receivedFirstFrame)
                    it->second.receivedFirstFrame = true;
            } else
                NS_ASSERT("receivedFrames record not created!");
        }

        bool txACK = isNewSequence;

        if (m_sendNACK) {
            std::pair <Mac48Address, uint16_t> frameId(src, seq);
            TimersList::iterator it3 = m_nackTimeoutTimers.find(frameId);
            if (it3 != m_nackTimeoutTimers.end()) {
                if (it3->second.IsRunning())
                    it3->second.Cancel();
                NS_LOG_INFO("CANCEL NACK TIMER for dst: " << src << ", seq: " << seq);
                m_nackTimeoutTimers.erase(it3);
            }
        }

        if (m_useBlockACK && phyHdr->IsBlockAckEnabled())
            //if (m_useContinousACK && phyHdr->IsContinousAckEnabled ())
        {
            std::pair <Mac48Address, uint16_t> frameId(phyHdr->GetSource(), 0);
            std::pair < TimersList::iterator, bool> newIns = m_blockAckTimeoutTimers.insert(std::pair <std::pair <Mac48Address, uint16_t>, Timer> (frameId, Timer(Timer::CANCEL_ON_DESTROY)));

            if (!newIns.first->second.IsRunning()) {
                NS_LOG_DEBUG("BLOCK ACK TIMEOUT TIMER START for src: " << phyHdr->GetSource());

                newIns.first->second.SetDelay(m_blockAckTimeout);
                newIns.first->second.SetFunction(&RescueArqManager::BlockAckTimeout, this);
                newIns.first->second.SetArguments(src);
                newIns.first->second.Schedule();
            } else
                NS_LOG_DEBUG("BLOCK ACK TIMEOUT TIMER for src: " << phyHdr->GetSource() << " ALREADY RUNNING");
            txACK = false;
        }

        if (!txACK
                && phyHdr->IsRetry()
                && ((Simulator::Now() - m_longAckTimeout) > it->second.RXtime[seq])) {
            NS_LOG_INFO("UNNECESSARY RETRANSMISSION DETECTED! resend ACK!");
            txACK = true;
        }

        if (txACK)
            ConfigureAckHeader(phyHdr, ackHdr);
        else
            ackHdr->SetDestination("00:00:00:00:00:00");

        return isNewSequence;
    }

    void
    RescueArqManager::ReportDamagedFrame(const RescuePhyHeader *phyHdr) {
        NS_LOG_FUNCTION("");
        //store info about sended packet

        Mac48Address src = phyHdr->GetSource();
        uint16_t seq = phyHdr->GetSequence();

        RecvSeqList::iterator it = m_receivedFrames.find(src);
        if (it != m_receivedFrames.end()) {
            it->second.copyRXed[seq] = true;
            if (it->second.maxSeqCopyRXed < seq)
                seq = it->second.maxSeqCopyRXed;
        }

        if (m_sendNACK) {
            NS_LOG_DEBUG("NACK TIMEOUT TIMER START for src: " << src << ", seq: " << seq);

            std::pair <Mac48Address, uint16_t> frameId(src, seq);
            std::pair < TimersList::iterator, bool> newIns = m_nackTimeoutTimers.insert(std::pair <std::pair <Mac48Address, uint16_t>, Timer> (frameId, Timer(Timer::CANCEL_ON_DESTROY)));

            if (newIns.first->second.IsRunning())
                newIns.first->second.Cancel();

            RescuePhyHeader phyHdr_ = *phyHdr;

            newIns.first->second.SetDelay(m_nackTimeout);
            newIns.first->second.SetFunction(&RescueArqManager::NackTimeout, this);
            newIns.first->second.SetArguments(phyHdr_);
            newIns.first->second.Schedule();
        }
        if (m_useBlockNACK && m_useBlockACK && phyHdr->IsBlockAckEnabled())
            //if (m_useContinousACK && phyHdr->IsContinousAckEnabled ())
        {
            //store header of damaged frame for Block ACK

            if (it != m_receivedFrames.end()) {
                if (seq > it->second.newestHeader.GetSequence()) {
                    NS_LOG_DEBUG("store newest header for src: " << src << ", seq: " << seq);
                    RescuePhyHeader phyHdr_ = *phyHdr;
                    it->second.newestHeader = phyHdr_;
                } else if ((it->second.newestHeader.GetSequence() == 0) && !it->second.receivedFirstFrame) {
                    NS_LOG_DEBUG("store newest header for src: " << src << ", seq: " << seq);
                    RescuePhyHeader phyHdr_ = *phyHdr;
                    it->second.newestHeader = phyHdr_;
                }
            }

            std::pair <Mac48Address, uint16_t> frameId(src, 0);
            std::pair < TimersList::iterator, bool> newIns = m_blockAckTimeoutTimers.insert(std::pair <std::pair <Mac48Address, uint16_t>, Timer> (frameId, Timer(Timer::CANCEL_ON_DESTROY)));

            if (!newIns.first->second.IsRunning()) {
                NS_LOG_DEBUG("BLOCK ACK TIMEOUT TIMER START for src: " << src);

                newIns.first->second.SetDelay(m_blockAckTimeout);
                newIns.first->second.SetFunction(&RescueArqManager::BlockAckTimeout, this);
                newIns.first->second.SetArguments(src);
                newIns.first->second.Schedule();
            } else
                NS_LOG_DEBUG("BLOCK ACK TIMEOUT TIMER for src: " << src << " ALREADY RUNNING");
        } else
            NS_ASSERT("receivedFrames record not created!");
    }

    void
    RescueArqManager::NackTimeout(RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");

        Mac48Address src = phyHdr.GetSource();
        uint16_t seq = phyHdr.GetSequence();

        NS_LOG_INFO("!!! NACK TIMEOUT !!! for src: " << src << ", seq: " << seq);

        Ptr<Packet> pkt = Create<Packet> (0);
        RescuePhyHeader ackHdr = RescuePhyHeader(m_mac->GetAddress(), src, RESCUE_PHY_PKT_TYPE_E2E_ACK);

        ConfigureAckHeader(&phyHdr, &ackHdr);

        m_mac->EnqueueAck(pkt, ackHdr);

        //reschedule
        NS_LOG_DEBUG("NACK TIMEOUT TIMER START for src: " << src << ", seq: " << seq);

        std::pair <Mac48Address, uint16_t> frameId(src, seq);
        std::pair < TimersList::iterator, bool> newIns = m_nackTimeoutTimers.insert(std::pair <std::pair <Mac48Address, uint16_t>, Timer> (frameId, Timer(Timer::CANCEL_ON_DESTROY)));

        if (newIns.first->second.IsRunning())
            newIns.first->second.Cancel();

        newIns.first->second.SetDelay(m_longAckTimeout);
        newIns.first->second.SetFunction(&RescueArqManager::NackTimeout, this);
        newIns.first->second.SetArguments(phyHdr);
        newIns.first->second.Schedule();
    }

    void
    RescueArqManager::BlockAckTimeout(Mac48Address src) {
        NS_LOG_FUNCTION("");

        RecvSeqList::iterator it = m_receivedFrames.find(src);

        Ptr<Packet> pkt = Create<Packet> (0);
        RescuePhyHeader ackHdr = RescuePhyHeader(m_mac->GetAddress(), src, RESCUE_PHY_PKT_TYPE_E2E_ACK);

        NS_LOG_INFO("BLOCK ACK from: " << ackHdr.GetSource() << ", to: " << ackHdr.GetDestination() << ", seq: " << it->second.newestHeader.GetSequence());

        ConfigureAckHeader(&(it->second.newestHeader), &ackHdr);

        m_mac->EnqueueAck(pkt, ackHdr);

        //reschedule
        std::pair <Mac48Address, uint16_t> frameId(src, 0);
        std::pair < TimersList::iterator, bool> newIns = m_blockAckTimeoutTimers.insert(std::pair <std::pair <Mac48Address, uint16_t>, Timer> (frameId, Timer(Timer::CANCEL_ON_DESTROY)));

        if (!newIns.first->second.IsRunning()) {
            NS_LOG_DEBUG("BLOCK ACK TIMEOUT TIMER START for src: " << src);

            newIns.first->second.SetDelay(m_blockAckTimeout);
            newIns.first->second.SetFunction(&RescueArqManager::BlockAckTimeout, this);
            newIns.first->second.SetArguments(src);
            newIns.first->second.Schedule();
        } else
            NS_LOG_DEBUG("BLOCK ACK TIMEOUT TIMER for src: " << src << " ALREADY RUNNING");
    }

    void
    RescueArqManager::ConfigureAckHeader(const RescuePhyHeader *phyHdr, RescuePhyHeader *ackHdr) {
        NS_LOG_FUNCTION("");

        Mac48Address src = phyHdr->GetSource();
        uint16_t seq = phyHdr->GetSequence();
        RecvSeqList::iterator it = m_receivedFrames.find(src);

        ackHdr->SetSource(phyHdr->GetDestination());
        ackHdr->SetDestination(src);
        ackHdr->SetSequence(seq);
        if (it->second.RXed[seq]) {
            ackHdr->SetACK();
            //if (seq > it->second.maxSeqACKed)
            if (SeqComp(seq, it->second.maxSeqACKed))
                it->second.maxSeqACKed = seq;
        } else
            ackHdr->SetNACK();
        NS_LOG_INFO((it->second.RXed[seq] ? "ACK" : "NACK") << " for: " << ackHdr->GetDestination() << ", from: " << ackHdr->GetSource() << ", seq: " << ackHdr->GetSequence());

        //actualize expected SEQ number for given source station
        while (it->second.RXed[it->second.expectedSeq])
            it->second.expectedSeq++; //increment for all already ACKed frames

        if (m_useContinousACK && phyHdr->IsContinousAckEnabled()) {
            //check if it is not the first frame
            if (it->second.receivedFirstFrame) {
                NS_LOG_INFO("SET CONTINOUS ACK");
                ackHdr->SetContinousAckEnabled();
                ackHdr->SetContinousAck(seq - it->second.expectedSeq + 1);
                NS_LOG_INFO("EXPECTED SEQ: " << it->second.expectedSeq << ", CONTINOUS ACK set to: " << (int) ackHdr->GetContinousAck());

                //if ((it->second.expectedSeq - 1) > it->second.maxSeqACKed)
                if (SeqComp(it->second.expectedSeq - 1, it->second.maxSeqACKed))
                    it->second.maxSeqACKed = it->second.expectedSeq - 1;
                NS_LOG_INFO("MAX SEQ. ACKED: " << it->second.maxSeqACKed);
            } else {
                NS_LOG_INFO("DISABLE CONTINOUS ACK (no first frame received)");
                ackHdr->SetContinousAckDisabled();
            }
        } else {
            NS_LOG_INFO("DISABLE CONTINOUS ACK");
            ackHdr->SetContinousAckDisabled();
        }

        NS_LOG_FUNCTION("m_useBlockACK" << (m_useBlockACK ? "TRUE" : "FALSE") << "phyHdr->IsBlockAckEnabled (): " << (phyHdr->IsBlockAckEnabled() ? "TRUE" : "FALSE"));
        if (m_useBlockACK && phyHdr->IsBlockAckEnabled()) {
            NS_LOG_INFO("SET BLOCK ACK");
            ackHdr->SetBlockAckEnabled();
            uint16_t start_seq = seq;
            if (m_useContinousACK && ackHdr->IsContinousAckEnabled())
                start_seq = SeqComp(it->second.expectedSeq + 16, seq) ? seq : it->second.expectedSeq + 16;
            //start_seq = std::min (seq, uint16_t(it->second.expectedSeq + 16));
            for (int i = 0; i < 16; i++) {
                uint16_t seq_temp = start_seq - 1 - i;

                ackHdr->SetBlockAckFlagFor(it->second.RXed[seq_temp], i);
                NS_LOG_INFO("SET BLOCK ACK for src: " << src << ", seq: " << (seq_temp) << " " << (ackHdr->IsBlockAckFrameReceived(i) ? "RECEIVED" : "MISSED"));

                if (ackHdr->IsBlockAckFrameReceived(i) && (seq_temp > it->second.maxSeqACKed))
                    it->second.maxSeqACKed = seq_temp;
            }
        } else {
            NS_LOG_INFO("DISABLE BLOCK ACK");
            ackHdr->SetBlockAckDisabled();
        }

        uint8_t NACKedFrames = 0;
        //for (uint16_t i = it->second.expectedSeq; i != it->second.maxSeqCopyRXed + 1; i++)
        NS_LOG_DEBUG("it->second.expectedSeq: " << it->second.expectedSeq << ", seq + 1: " << seq + 1);
        for (uint16_t i = it->second.expectedSeq; SeqComp(seq + 1, i); i++) {
            NS_LOG_DEBUG("i: " << i << "it->second.RXed[i]: " << ((it->second.RXed[i]) ? "YES" : "NO"));
            if (!it->second.RXed[i])
                NACKedFrames++;
        }

        ackHdr->SetNACKedFrames(NACKedFrames);
        NS_LOG_INFO("SetNACKedFrames: " << (int) ackHdr->GetNACKedFrames());

        return;
    }



    // ---------------------- Relay Functions ----------------------------

    std::pair<RelayBehavior, RescuePhyHeader>
    RescueArqManager::ReportRelayFrameRx(const RescuePhyHeader *phyHdr, double ber) {
        NS_LOG_FUNCTION("");
        uint16_t seq = phyHdr->GetSequence();
        Mac48Address src = phyHdr->GetSource();
        Mac48Address dst = phyHdr->GetDestination();

        //check - have I recently transmitted this frame? have I transmitted an ACK for it?
        FwdSeqList::iterator it = m_forwardedFrames.find(std::pair<Mac48Address, Mac48Address> (src, dst));
        if (it != m_forwardedFrames.end()) {
            //found source and destination

            //check if sequence should be reused

            /* We cannot reuse all sequences at once because in advanced ARQ operation some frames with high seq. number
             * may be transmitted at the same time with low seq. numbers. (E.g. SEQ=0 is the next seq for SEQ=65535)
             * Thus we reuse first half of sequences at once, the second half will be used later.
             */

            if (!it->second.seq1stHalfCleaned
                    && (seq > (ARRAY_END - MAX_WINDOW_SIZE))) {
                //first half of sequence numbers should be reused
                for (uint16_t i = 0; i <= HALF_OF_ARRAY; i++) {
                    //std::cout << "seq: " << seq << ", i: " << i << std::endl;
                    it->second.RXed[i] = false;
                    it->second.TXed[i] = false;
                    it->second.IL[i] = 0;
                    it->second.ber[i] = 0.0;
                    it->second.ACKed[i] = false;
                    it->second.retried[i] = false;
                }
                it->second.seq1stHalfCleaned = true;
                it->second.seq2ndHalfCleaned = false;
            } else if (!it->second.seq2ndHalfCleaned
                    && (seq > (HALF_OF_ARRAY - MAX_WINDOW_SIZE))
                    && (seq <= HALF_OF_ARRAY)) {
                //second half of sequence numbers should be reused
                for (uint16_t i = HALF_OF_ARRAY + 1; i > HALF_OF_ARRAY; i++) {
                    //std::cout << "seq: " << seq << ", i: " << i << std::endl;
                    it->second.RXed[i] = false;
                    it->second.TXed[i] = false;
                    it->second.IL[i] = 0;
                    it->second.ber[i] = 0.0;
                    it->second.ACKed[i] = false;
                    it->second.retried[i] = false;
                }
                it->second.seq1stHalfCleaned = false;
                it->second.seq2ndHalfCleaned = true;
            }


            if (it->second.ACKed[seq] //ACKed?
                    && (!phyHdr->IsRetry())) {
                //frame was already ACKed and this copy is not needed - drop it!
                return std::pair<RelayBehavior, RescuePhyHeader> (DROP, RescuePhyHeader(RESCUE_PHY_PKT_TYPE_E2E_ACK));
            } else if (it->second.ACKed[seq] //ACKed?
                    && (phyHdr->IsRetry())) {
                //frame was already ACKed and unnecessary retransmission is detected
                RescuePhyHeader ackHdr;

                uint16_t i_start = seq;
                uint16_t i_stop = seq;

                if (phyHdr->IsBlockAckEnabled() && !phyHdr->IsContinousAckEnabled()) //Block ACK, no Cont ACK
                    i_start = SeqComp(it->second.maxSeqACKed, seq + 16) ? seq + 16 : it->second.maxSeqACKed;
                    //i_start = std::min (uint16_t(seq + 16), it->second.maxSeqACKed);
                else if (!phyHdr->IsBlockAckEnabled() && phyHdr->IsContinousAckEnabled()) //no Block ACK, Cont ACK
                {
                    //i_start = ((seq > it->second.contACKed) ? it->second.maxSeqACKed : seq);
                    i_start = SeqComp(seq, it->second.contACKed) ? it->second.maxSeqACKed : seq;
                    i_stop = it->second.contACKed;
                } else if (phyHdr->IsBlockAckEnabled() && phyHdr->IsContinousAckEnabled()) //Block ACK, Cont ACK
                {
                    //i_start = ((seq > it->second.contACKed) ? it->second.maxSeqACKed : std::min (uint16_t(seq + 16), it->second.maxSeqACKed));
                    i_start = SeqComp(seq, it->second.contACKed) ? it->second.maxSeqACKed : (SeqComp(seq + 16, it->second.maxSeqACKed) ? it->second.maxSeqACKed : seq + 16);
                    i_stop = std::max(seq, it->second.contACKed);
                }

                for (uint16_t i = i_start; i >= i_stop; i--)
                    if (it->second.ACKed[i] && IsSeqACKed(seq, &(it->second.ackHdr[i]))) {
                        ackHdr = it->second.ackHdr[i];
                        break;
                    }
                return std::pair<RelayBehavior, RescuePhyHeader> (RESEND_ACK, ackHdr);
            } else if (it->second.TXed[seq] && (it->second.IL[seq] == phyHdr->GetInterleaver())) {
                //frame was not already ACKed but the copy of given frame was transmitted, just break
                return std::pair<RelayBehavior, RescuePhyHeader> (DROP, RescuePhyHeader(RESCUE_PHY_PKT_TYPE_E2E_ACK));
                /*if (it->second.ber[seq] > ber)
                  {
                    it->second.ber[seq] = ber;
                    return std::pair<RelayBehavior, RescuePhyHeader> (FORWARD, RescuePhyHeader (RESCUE_PHY_PKT_TYPE_E2E_ACK));
                  }
                else
                  return std::pair<RelayBehavior, RescuePhyHeader> (DROP, RescuePhyHeader (RESCUE_PHY_PKT_TYPE_E2E_ACK));*/
            } else if (it->second.RXed[seq] && !it->second.TXed[seq]) {
                //frame copy was received but still awaits for transmission
                if (it->second.ber[seq] > ber) {
                    it->second.ber[seq] = ber;
                    return std::pair<RelayBehavior, RescuePhyHeader> (REPLACE_COPY, RescuePhyHeader(RESCUE_PHY_PKT_TYPE_E2E_ACK));
                } else
                    return std::pair<RelayBehavior, RescuePhyHeader> (DROP, RescuePhyHeader(RESCUE_PHY_PKT_TYPE_E2E_ACK));
            }//otherwise - it is new TX try, forward it!
            else {
                //new or another retry detected, update interleaver
                it->second.RXed[seq] = true;
                it->second.TXed[seq] = false;
                it->second.IL[seq] = phyHdr->GetInterleaver();
                it->second.ber[seq] = ber;
                it->second.ACKed[seq] = false;
                it->second.retried[seq] = phyHdr->IsRetry();

                return std::pair<RelayBehavior, RescuePhyHeader> (FORWARD, RescuePhyHeader(RESCUE_PHY_PKT_TYPE_E2E_ACK));
            }
        } else {
            //new source and destination
            NS_LOG_INFO("new source and destination");
            FwdList newIns;
            newIns.RXed[seq] = true;
            newIns.TXed[seq] = false;
            newIns.IL[seq] = phyHdr->GetInterleaver();
            newIns.ber[seq] = ber;
            newIns.ACKed[seq] = false;
            //newIns.ackHdr[seq] = RescuePhyHeader (RESCUE_PHY_PKT_TYPE_E2E_ACK);
            newIns.retried[seq] = phyHdr->IsRetry();
            //newIns.lastNACKrx[seq] = Seconds (0);

            newIns.ackHdr = std::vector<RescuePhyHeader> (ARRAY_END + 1, RescuePhyHeader(RESCUE_PHY_PKT_TYPE_E2E_ACK));

            for (uint16_t i = seq + 1; i != seq; i++) {
                newIns.RXed[i] = false;
                newIns.TXed[i] = false;
                newIns.IL[i] = 0;
                newIns.ber[i] = 0;
                newIns.ACKed[i] = false;
                //newIns.ackHdr[i] = RescuePhyHeader (RESCUE_PHY_PKT_TYPE_E2E_ACK);
                newIns.retried[i] = false;
                //newIns.lastNACKrx[i] = Seconds (0);
            }

            if (seq < HALF_OF_ARRAY) {
                newIns.seq1stHalfCleaned = true;
                newIns.seq2ndHalfCleaned = false;
            } else {
                newIns.seq1stHalfCleaned = false;
                newIns.seq2ndHalfCleaned = true;
            }

            std::pair<Mac48Address, Mac48Address> srcDst(src, dst);
            m_forwardedFrames.insert(std::pair<std::pair<Mac48Address, Mac48Address>, FwdList> (srcDst, newIns));
            return std::pair<RelayBehavior, RescuePhyHeader> (FORWARD, RescuePhyHeader(RESCUE_PHY_PKT_TYPE_E2E_ACK));
        }
    }

    bool
    RescueArqManager::ReportAckToFwd(const RescuePhyHeader *ackHdr) {
        NS_LOG_FUNCTION("");
        bool fwd = false;
        uint16_t seq = ackHdr->GetSequence();
        Mac48Address dst = ackHdr->GetDestination();
        Mac48Address src = ackHdr->GetSource();

        //check - have I recently transmitted frame which is acknowledgded here?
        FwdSeqList::iterator it = m_forwardedFrames.find(std::pair<Mac48Address, Mac48Address> (dst, src));
        if (it != m_forwardedFrames.end()) {
            //found source and destination - process ACK frame and check if it should be forwarded

            //check Continous ACK
            if (m_useContinousACK && ackHdr->IsContinousAckEnabled()) {
                //if ( ((seq - ackHdr->GetContinousAck ()) > it->second.contACKed)
                //     || (((seq - ackHdr->GetContinousAck ()) < MAX_WINDOW_SIZE) && (it->second.contACKed > ARRAY_END - MAX_WINDOW_SIZE)) )
                if (SeqComp(seq - ackHdr->GetContinousAck(), it->second.contACKed)) {
                    NS_LOG_INFO("RELAY: CONTINOUS ACK for dst: " << dst << ", from src: " << src << ", contACK: " << (int) ackHdr->GetContinousAck() << ", new continously acknowledged seq: " << uint16_t(seq - ackHdr->GetContinousAck()) << ", previously acknowledged seq: " << (int) it->second.contACKed);
                    for (uint16_t i = it->second.contACKed; SeqComp(seq - ackHdr->GetContinousAck() + 1, i); i++) {
                        RescuePhyHeader ackHdr_ = *ackHdr;
                        it->second.ackHdr[i] = ackHdr_;
                        if (!it->second.ACKed[i]) {
                            NS_LOG_INFO("RELAY: CONTINOUS ACK for dst: " << dst << ", from src: " << src << ", seq: " << i << " RECEIVED");
                            fwd = true;
                            it->second.ACKed[i] = true;
                            //RescuePhyHeader ackHdr_ = *ackHdr;
                            //it->second.ackHdr[i] = ackHdr_;
                        }
                    }

                    it->second.contACKed = seq - ackHdr->GetContinousAck();
                    if (SeqComp(it->second.contACKed, it->second.maxSeqACKed))
                        it->second.maxSeqACKed = it->second.contACKed;
                }
            }

            //check Block ACK
            if (m_useBlockACK && ackHdr->IsBlockAckEnabled()) {
                uint16_t start_seq = seq;
                NS_LOG_DEBUG("useContinousACK: " << (m_useContinousACK ? "TRUE" : "FALSE") << "ackHdr->IsContinousAckEnabled: " << (ackHdr->IsContinousAckEnabled() ? "TRUE" : "FALSE"));
                if (m_useContinousACK && ackHdr->IsContinousAckEnabled())
                    start_seq = SeqComp(seq - ackHdr->GetContinousAck() + 17, seq) ? seq : seq - ackHdr->GetContinousAck() + 17;
                //start_seq = std::min (seq, uint16_t(it->second.contACKed + 17));

                uint16_t start_i = 15;
                /*if ( ((start_seq - 16) <= it->second.contACKed)
                       && (it->second.contACKed < HALF_OF_ARRAY) )
                  start_i = start_seq - it->second.contACKed;*/
                if (((start_seq - 16) < 0)
                        && (it->second.contACKed < HALF_OF_ARRAY))
                    start_i = start_seq - 1;

                if (start_i < 0) start_i = 0;
                if (start_i > HALF_OF_ARRAY)
                    start_i = 0;

                for (int i = start_i; i >= 0; i--) {
                    uint16_t seq_temp = start_seq - i - 1;
                    NS_LOG_INFO("RELAY: BLOCK ACK for dst: " << dst << ", from src: " << src << ", seq: " << seq_temp << " " << (ackHdr->IsBlockAckFrameReceived(i) ? "RECEIVED" : "MISSED"));
                    if (!it->second.ACKed[seq_temp]
                            && ackHdr->IsBlockAckFrameReceived(i)) {
                        //RescuePhyHeader ackHdr_ = *ackHdr;
                        //it->second.ackHdr[seq_temp] = ackHdr_;
                        fwd = true;
                        it->second.ACKed[seq_temp] = true;
                        if (seq_temp > it->second.maxSeqACKed)
                            it->second.maxSeqACKed = seq_temp;
                    } else if (!it->second.ACKed[seq_temp]
                            && !ackHdr->IsBlockAckFrameReceived(i))
                        fwd = true; //forward NACK if the given frame copy was not transmitted recently
                    if (ackHdr->IsBlockAckFrameReceived(i)) {
                        RescuePhyHeader ackHdr_ = *ackHdr;
                        it->second.ackHdr[seq_temp] = ackHdr_;
                    }
                }
            }

            //process basic ACK/NACK
            if (!it->second.ACKed[seq]) {
                if (ackHdr->IsACK()) {
                    NS_LOG_INFO("RELAY: BASIC ACK for dst: " << dst << ", from src: " << src << ", seq: " << ackHdr->GetSequence());
                    fwd = true;
                    it->second.ACKed[seq] = true;
                    //if (seq > it->second.maxSeqACKed)
                    if (SeqComp(seq, it->second.maxSeqACKed))
                        it->second.maxSeqACKed = seq;
                    RescuePhyHeader ackHdr_ = *ackHdr;
                    it->second.ackHdr[seq] = ackHdr_;
                }//else if ( (!ackHdr->IsBlockAckEnabled () && (ns3::Simulator::Now () - it->second.lastNACKrx[seq] > m_nackTimeout)) //NACK
                    //          || (ackHdr->IsBlockAckEnabled () && (ns3::Simulator::Now () - it->second.lastNACKrx[seq] > m_blockAckTimeout)) ) //BLOCK NACK
                else {
                    fwd = true;
                    //it->second.lastNACKrx[seq] = ns3::Simulator::Now ();
                }
                //else
                //  NS_LOG_INFO ("DUPLICATED NACK - DROP! (for dst: " << dst << ", from src: " << src << ", seq: " << ackHdr->GetSequence () << ")");
            } else
                NS_LOG_INFO("(ACK already received) DROP!");

            //check if it should be forwarded - dont forward ACKs for frames which were not transmitted here
            //fwd = fwd && it->second.RXed[seq];

            if (!it->second.RXed[seq])
                //new SEQ number for given destination???
                NS_LOG_INFO("ACK for frame seq which was not relayed here - forward to cancel unnecessary copies!");
        } else
            //new source and destination
            NS_LOG_INFO("(ACK for tx stream which was not relayed here) DROP!");

        if (!ackHdr->IsACK() && !ackHdr->IsBlockAckEnabled() && !ackHdr->IsContinousAckEnabled())
            fwd = fwd && (it->second.RXed[seq] == it->second.TXed[seq]); //special rule for pure NACK frames
        else
            /* Forward ACKs only if:
             * - retry frame was detected - links may be corrupted so multipath ACK forwarding may be more reliable
             * or:
             * - there was no enqueued copies
             *       - if the copy was already transmitted - probably this station is on the best TX path so it should forward this ACK
             *       - if the copy was never received - probably it will be received i nthe future - forward ACK to cancel this transmission
             */
            fwd = fwd
                && (it->second.retried[seq] //retry frame was detected
                //|| (it->second.RXed[seq] && it->second.TXed[seq]) );
                || (it->second.RXed[seq] == it->second.TXed[seq]));
        //fwd = fwd && (it->second.RXed[seq] == it->second.TXed[seq]); //special rule for pure NACK frames

        NS_LOG_INFO("fwd: " << (fwd ? "YES" : "NO") <<
                ", it->second.retried[seq]: " << (it->second.retried[seq] ? "YES" : "NO") <<
                ", it->second.RXed[seq]: " << (it->second.RXed[seq] ? "YES" : "NO") <<
                ", it->second.TXed[seq]: " << (it->second.TXed[seq] ? "YES" : "NO"));

        return fwd;
    }

    bool
    RescueArqManager::IsFwdACKed(const RescuePhyHeader *phyHdr) {
        NS_LOG_FUNCTION("");
        Mac48Address src = phyHdr->GetSource();
        Mac48Address dst = phyHdr->GetDestination();
        uint16_t seq = phyHdr->GetSequence();

        FwdSeqList::iterator it = m_forwardedFrames.find(std::pair<Mac48Address, Mac48Address> (src, dst));
        if (it != m_forwardedFrames.end())
            return it->second.ACKed[seq];
        else
            return false;
    }

    void
    RescueArqManager::ReportRelayDataTx(const RescuePhyHeader *phyHdr) {
        NS_LOG_FUNCTION("");
        FwdSeqList::iterator it = m_forwardedFrames.find(std::pair<Mac48Address, Mac48Address> (phyHdr->GetSource(), phyHdr->GetDestination()));
        if (it != m_forwardedFrames.end())
            it->second.TXed[phyHdr->GetSequence()] = true;
        else
            NS_ASSERT("TX of unnotified frame!");
    }

    bool
    RescueArqManager::IsSeqACKed(uint16_t seq, const RescuePhyHeader *ackHdr) {
        if ((seq > ackHdr->GetSequence()) && (seq > MAX_WINDOW_SIZE)) //for SEQ greater than ACK seq number
            return false;
        else if ((seq + MAX_WINDOW_SIZE < ackHdr->GetSequence()) && (seq < ARRAY_END - MAX_WINDOW_SIZE)) //for SEQ lower than ACK seq number more than MAX_WINDOW_SIZE
            return false;

        bool retval = ((seq == ackHdr->GetSequence()) && ackHdr->IsACK());

        if (!retval && ackHdr->IsContinousAckEnabled())
            retval = !SeqComp(seq, ackHdr->GetSequence() - ackHdr->GetContinousAck());
        //retval = (seq <= (ackHdr->GetSequence() - ackHdr->GetContinousAck ()));

        if (!retval && ackHdr->IsBlockAckEnabled() && !ackHdr->IsContinousAckEnabled())
            //if (ackHdr->GetSequence () - seq < 17)
            if (SeqComp(seq + 17, ackHdr->GetSequence()))
                retval = (ackHdr->IsBlockAckFrameReceived(ackHdr->GetSequence() - seq - 1));

        if (!retval && ackHdr->IsBlockAckEnabled() && ackHdr->IsContinousAckEnabled()) {
            //if (ackHdr->GetSequence () - seq < 17)
            if (SeqComp(seq + 17, ackHdr->GetSequence()))
                retval = (ackHdr->IsBlockAckFrameReceived(ackHdr->GetSequence() - seq - 1));
                //else if (seq - (ackHdr->GetSequence() - ackHdr->GetContinousAck ()) < 17)
            else if (SeqComp(ackHdr->GetSequence() - ackHdr->GetContinousAck(), seq - 17))
                retval = (ackHdr->IsBlockAckFrameReceived(ackHdr->GetSequence() - ackHdr->GetContinousAck() + 16 - seq - 1));
        }

        return retval;
    }

    bool
    RescueArqManager::ACKcomp(const RescuePhyHeader *ackHdr1, const RescuePhyHeader *ackHdr2) {
        //NS_LOG_FUNCTION ("");

        bool retval = (ackHdr1->GetSource() == ackHdr2->GetSource())
                && (ackHdr1->GetDestination() == ackHdr2->GetDestination())
                && (ackHdr1->GetSequence() == ackHdr2->GetSequence())
                && (ackHdr1->IsACK() == ackHdr2->IsACK());

        if (ackHdr1->IsBlockAckEnabled() && ackHdr2->IsBlockAckEnabled())
            retval = retval && (ackHdr1->GetBlockAck() == ackHdr2->GetBlockAck());
        else
            retval = retval && (ackHdr1->IsBlockAckEnabled() == ackHdr2->IsBlockAckEnabled());

        if (ackHdr1->IsContinousAckEnabled() && ackHdr2->IsContinousAckEnabled())
            retval = retval && (ackHdr1->GetContinousAck() == ackHdr2->GetContinousAck());
        else
            retval = retval && (ackHdr1->IsContinousAckEnabled() == ackHdr2->IsContinousAckEnabled());

        return retval;
    }

    bool
    RescueArqManager::SeqComp(uint16_t seq1, uint16_t seq2) {
        NS_LOG_FUNCTION("seq1: " << seq1 << ", seq2: " << seq2 << " return: " << ((((seq1 > seq2) != ((seq1 > ARRAY_END - MAX_WINDOW_SIZE) && (seq2 < MAX_WINDOW_SIZE))) || ((seq2 > ARRAY_END - MAX_WINDOW_SIZE) && (seq1 < MAX_WINDOW_SIZE))) ? "TRUE" : "FALSE"));
        return ( ((seq1 > seq2) != ((seq1 > ARRAY_END - MAX_WINDOW_SIZE) && (seq2 < MAX_WINDOW_SIZE)))
                || ((seq2 > ARRAY_END - MAX_WINDOW_SIZE) && (seq1 < MAX_WINDOW_SIZE)));
    }

    Time
    RescueArqManager::GetTimeoutFor(const RescuePhyHeader *ackHdr) {
        if (ackHdr->IsBlockAckEnabled()) {
            NS_LOG_FUNCTION("block ACK");
            return m_blockAckTimeout;
        } else {
            NS_LOG_FUNCTION("normal ACK");
            return m_longAckTimeout;
        }
    }

} // namespace ns3

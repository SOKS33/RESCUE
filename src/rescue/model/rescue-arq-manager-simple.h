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

#ifndef RESCUE_ARQ_MANAGER_H
#define RESCUE_ARQ_MANAGER_H

#include "ns3/mac48-address.h"
#include "ns3/traced-callback.h"
#include "ns3/packet.h"
#include "ns3/event-id.h"

#include "rescue-phy-header.h"
#include "rescue-mac-header.h"

#include <stdint.h>
#include <map>

namespace ns3 {

    class RescueMac;

    /**
     * \ingroup rescue
     *
     * Handles advanced ARQ functions
     */
    class RescueArqManager : public Object {
    public:
        static TypeId GetTypeId(void);

        RescueArqManager();
        ~RescueArqManager();

        /**
         * \param phy the MAC of this device
         */
        void SetupMac(Ptr<RescueMac> mac);
        /**
         * \param duration the Basic ACK Timeout duration
         */
        void SetBasicAckTimeout(Time duration);

        /**
         * \return duration the Basic ACK Timeout duration
         */
        Time GetBasicAckTimeout(void) const;
        /**
         * \return the maximum retry count
         */
        uint16_t GetMaxRetryCount(void) const;
        /**
         * \param maxRetryCount the maximum retry count
         */
        void SetMaxRetryCount(uint16_t maxRetryCount);
        /**
         * \return true if ARQ allows for next data transmission
         */
        bool IsTxAllowed();

        /**
         * sets next data transmission enabled
         */
        void EnableDataTx();
        /**
         * sets next data transmission disabled
         */
        void DisableDataTx();

        /**
         * Return the next sequence number for the given header.
         *
         * \param hdr rescue PHY header
         * \return the next sequence number
         */
        uint16_t GetNextSequenceNumber(const RescuePhyHeader *hdr);
        /**
         * Return the next sequence number for the given header.
         *
         * \param hdr rescue MAC header
         * \return the next sequence number
         */
        uint16_t GetNextSequenceNumber(const RescueMacHeader *hdr);
        /**
         * Return the next sequence number for the given destination.
         *
         * \param addr destination address
         * \return the next sequence number
         */
        uint16_t GetNextSeqNumberByAddress(Mac48Address addr) const;

        /**
         * Used to store info about sended packet
         *
         * \param dst destination address
         * \param seq sequence number of the frame
         */
        void ReportNewDataFrame(Mac48Address dst, uint16_t seq);

        /**
         * Used to inform about data frame tx (new or retry)
         *
         * \param pkt transmitted packet
         */
        void ReportDataTx(Ptr<Packet> pkt);

        /**
         * Used to process ACK frame
         *
         * \param phyHdr header of the ACK frame (in fact whole frame)
         * \return the retry counter value or -1 in case of unsuccessful ACK reception
         *         (duplicated ACK etc.)
         */
        int ReceiveAck(RescuePhyHeader phyHdr);

        /**
         * invoked when ACK Timoeut counter expires
         *
         * \param pkt packet for which the timeout has expired (including header)
         */
        void AckTimeout(Ptr<Packet> pkt);

        /**
         * \param address remote address
         * \param packet the packet to send
         * \return true if we want to resend a packet
         *          after a failed transmission attempt, false otherwise.
         */
        bool NeedDataRetransmission(Mac48Address dst, uint16_t seq);

        /**
         * used to check recent sequence number from given source
         *
         * \param src address of frame originator
         * \return sequence number of recently received frame
         */
        uint16_t CheckRxSequenceFor(Mac48Address src);

        /**
         * used to check sequence number of incoming frame to prevent
         * unnecessary frame copies reception
         *
         * \param addr address of frame originator
         * \param seq sequence number of a given frame
         * \return false if frame is duplicated
         */
        bool CheckRxSequence(Mac48Address src, uint16_t seq);

        /**
         * used to notify and check sequence number of incoming frame
         * to prevent duplicated frame reception
         *
         * \param addr address of frame originator
         * \param seq sequence number of a given frame
         * \return false if frame is duplicated
         */
        bool CheckRxFrame(Mac48Address src, uint16_t seq);

        /**
         * used to check sequence number of incoming frame to prevent
         * duplicated frame reception
         *
         * \param addr address of frame originator
         * \param seq sequence number of a given frame
         * \param ackHdr ack header to configure in case of ACK transmission after this packet reception
         * \return false if frame is duplicated
         */
        bool CheckRxFrame(Mac48Address src, uint16_t seq, RescuePhyHeader *ackHdr);

    protected:
        void DoDispose(void);

    private:
        std::map <Mac48Address, uint16_t> m_sequences;

        bool m_txAllowed; //!< Allowance for next data frame transmission
        Time m_basicAckTimeout; //!< The maximal duration of ACK awaiting
        uint16_t m_maxRetryCount; //!< Maximum Retry Count

        Ptr<RescueMac> m_mac; //!< Pointer to associated RescueMac

        EventId m_ackTimeoutEvent; //!< End-to-end ACK timeout event

        /*
         * List to keep information about transmitted frames
         */
        typedef std::map<uint16_t, std::pair<bool, uint16_t> > SeqList;

        struct seqAck {

            seqAck(SeqList::iterator& it)
            : seq(it->first),
            ACKed(it->second.first),
            retryCount(it->second.second) {
            }
            const uint16_t& seq;
            bool& ACKed;
            uint16_t& retryCount;
        };
        typedef std::map<Mac48Address, SeqList> SendSeqList;

        struct dstSeqList {

            dstSeqList(SendSeqList::iterator& it)
            : dst(it->first),
            seqList(it->second) {
            }
            const Mac48Address& dst;
            SeqList& seqList;
        };
        SendSeqList m_createdFrames; //!< List of originated frames

        /*
         * List to keep information about transmitted frames in order to eliminate duplicates
         */
        typedef std::map<Mac48Address, uint16_t> RecvSeqList;

        struct srcSeq {

            srcSeq(RecvSeqList::iterator& it)
            : src(it->first), seq(it->second) {
            }
            const Mac48Address& src;
            uint16_t& seq;
        };
        RecvSeqList m_receivedFrames; //!< List of received frames seq. numbers - to eliminate duplicates

        /**
         * The trace source fired when the transmission of a single data packet has failed
         */
        TracedCallback<Mac48Address> m_macTxDataFailed;
        /**
         * The trace source fired when the transmission of a data packet has
         * exceeded the maximum number of attempts
         */
        TracedCallback<Mac48Address> m_macTxFinalDataFailed;
    };

} // namespace ns3

#endif /* RESCUE_ARQ_MANAGER_H */

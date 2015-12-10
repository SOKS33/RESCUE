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

#define MAX_WINDOW_SIZE 255
#define ARRAY_END 65535
#define HALF_OF_ARRAY 32767

#include "ns3/mac48-address.h"
#include "ns3/traced-callback.h"
#include "ns3/packet.h"
#include "ns3/event-id.h"
#include "ns3/timer.h"

#include "rescue-phy-header.h"
#include "rescue-mac-header.h"
#include "rescue-mac.h"

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

        //MODIF 3
        void RelayingStopAck(Mac48Address, uint16_t);


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
         * \return true if ARQ allows for next data transmission to given destination
         */
        bool IsTxAllowed(Mac48Address dst);

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
        uint16_t GetNextSeqNumberByAddress(Mac48Address dst) const;

        /**
         * Used to store info about sended packet
         *
         * \param dst destination address
         * \param seq sequence number of the frame
         */
        void ReportNewDataFrame(Mac48Address dst, uint16_t seq);

        void ConfigurePhyHeader(RescuePhyHeader *phyHdr);

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
        int ReceiveAck(const RescuePhyHeader *ackHdr);

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
         * used to check sequence number of incoming frame to prevent
         * unnecessary frame copies reception
         *
         * \param addr address of frame originator
         * \param seq sequence number of a given frame
         * \param useContinousACK true if Continous ACK should be used
         * \return false if frame is duplicated
         */
        bool CheckRxSequence(Mac48Address src, uint16_t seq, bool useContinousACK);

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
         * \param phyHdr PHY header of received DATA frame
         * \param ackHdr ACK header to configure in case of ACK transmission after this packet reception
         * \return false if frame is duplicated
         */
        bool CheckRxFrame(const RescuePhyHeader *phyHdr, RescuePhyHeader *ackHdr);

        void ReportDamagedFrame(const RescuePhyHeader *phyHdr);

        void NackTimeout(RescuePhyHeader phyHdr);

        void BlockAckTimeout(Mac48Address src);

        void ConfigureAckHeader(const RescuePhyHeader *phyHdr, RescuePhyHeader *ackHdr);

        std::pair<RelayBehavior, RescuePhyHeader> ReportRelayFrameRx(const RescuePhyHeader *phyHdr, double ber);

        bool ReportAckToFwd(const RescuePhyHeader *ackHdr);

        bool IsRetryACKed(const RescueMacHeader *hdr);

        bool IsFwdACKed(const RescuePhyHeader *phyHdr);

        void ReportRelayDataTx(const RescuePhyHeader *phyHdr);

        bool IsSeqACKed(uint16_t seq, const RescuePhyHeader *ackHdr);

        bool ACKcomp(const RescuePhyHeader *ackHdr1, const RescuePhyHeader *ackHdr2);

        bool SeqComp(uint16_t seq1, uint16_t seq2);

        Time GetTimeoutFor(const RescuePhyHeader *ackHdr);

    protected:
        void DoDispose(void);

    private:
        bool m_txAllowed; //!< Allowance for next data frame transmission
        Time m_basicAckTimeout; //!< The maximal duration of ACK awaiting
        Time m_longAckTimeout; //!< The maximal duration of ACK awaiting
        uint16_t m_maxRetryCount; //!< Maximum Retry Count
        uint8_t m_sendWindow; //!< Send window size
        bool m_sendNACK; //!< If true, send NACKs
        Time m_nackTimeout; //!< After this period NACK will be send
        bool m_useContinousACK; //!< Enable/disable use of continous ACK
        bool m_useBlockACK; //!< Enable/disable use of continous ACK
        Time m_blockAckTimeout; //!< After this period Block ACK will be send
        bool m_useBlockNACK; //!< If true, BlockACK can be started with unsuccessfully received frame

        Ptr<RescueMac> m_mac; //!< Pointer to associated RescueMac

        typedef std::map<std::pair <Mac48Address, uint16_t>, Timer> TimersList;
        TimersList m_ackTimeoutTimers; //!< End-to-end ACK timeout events
        TimersList m_nackTimeoutTimers; //!< End-to-end NACK timeout events
        TimersList m_blockAckTimeoutTimers; //!< End-to-end BLOCK ACK timeout events

        /*
         * List to keep information about transmitted and ACKed frames
         */
        struct SendList {
            bool ACKed [ARRAY_END + 1]; //!< Indicates that the frame was correctly acknowledged
            uint8_t retryCount [ARRAY_END + 1]; //!< Retry counter for given frame
            uint16_t nextSequence; //!< Stores the next sequence number to be send
            uint8_t unACKedFrames; //!< Unacknowledged frames counter
            uint8_t rxedUnACKedFrames; //!< Number of frames that are received by remote station but were not acknowledged (because of blockACK limitation)
            uint16_t contACKed; //!< Indicates last SEQ number acknowledged by Continous ACK
        };
        typedef std::map<Mac48Address, SendList> SendSeqList;

        SendSeqList m_createdFrames; //!< List of originated frames

        /**
         * Used to acknowledge given sended frame
         *
         * \param addr address of frame originator
         * \param seq sequence number of a given frame
         */
        void AcknowledgeFrame(SendSeqList::iterator it, uint16_t seq);

        void UnacknowledgeFrame(SendSeqList::iterator it, uint16_t seq);

        /*
         * List to keep information about received frames in order to prepare ACKs
         */
        struct RecvList {
            bool copyRXed [ARRAY_END + 1]; //!< Indicates that almost one copy of the given frame was received
            bool RXed [ARRAY_END + 1]; //!< Indicates that the frame was correctly received
            Time RXtime [ARRAY_END + 1]; //!< Stores the time of frame correct reception
            uint16_t expectedSeq; //!< First expected SEQ number, for Continous ACK preparation
            bool receivedFirstFrame; //!< Indicates if the first frame was received from given source, important for continous ACK
            uint16_t maxSeqACKed; //!< Indicates last SEQ number for which ACK was sent
            uint16_t maxSeqCopyRXed; //!< Indicates last SEQ number for which almost one frame copy was received
            //uint8_t NACKedFrames;          //!< Unsuccessfully received frames counter
            RescuePhyHeader newestHeader; //!< Stores the header of recently received frame

            /* We cannot reuse all sequences at once because in advanced ARQ operation some frames with high seq. number
             * may be transmitted at the same time with low seq. numbers. (E.g. SEQ=0 is the next seq for SEQ=65535)
             * Thus we reuse first half of sequences at once, the second half will be used later.
             */

            bool seq1stHalfCleaned; //!< Used to indicate which part of RecvSendSeqList should be prepared for reuse
            bool seq2ndHalfCleaned; //!< Used to indicate which part of RecvSendSeqList should be prepared for reuse

        };
        typedef std::map<Mac48Address, RecvList> RecvSeqList;
        RecvSeqList m_receivedFrames; //!< List of originated frames

        /*
         * List to keep information about forwarded frames and ACKs - to prevent loops and to reduce traffic intensity
         * while flooding mechanism is in use
         */
        struct FwdList {
            bool RXed [ARRAY_END + 1]; //!< Indicates that the copy of the given frame is enqueued and awiats for transmission
            bool TXed [ARRAY_END + 1]; //!< Indicates that the frame copy was transmitted
            uint8_t IL [ARRAY_END + 1]; //!< Stores the InterLeaver number of last transmitted copy
            double ber [ARRAY_END + 1]; //!< Stores the BER of last received/transmitted copy
            bool ACKed [ARRAY_END + 1]; //!< Indicates that the frame copy was acknowledged
            bool retried [ARRAY_END + 1]; //!< Indicates that the frame copy was retransmitted
            uint16_t contACKed; //!< Indicates last SEQ number acknowledged by Continous ACK
            uint16_t maxSeqACKed; //!< Indicates last ACKed SEQ number
            std::vector<RescuePhyHeader> ackHdr; //!< Stores ACK header for ACK retransmission
            //RescuePhyHeader ackHdr[ARRAY_END + 1];   //!< Stores ACK header for ACK retransmission
            //Time lastNACKrx [ARRAY_END + 1];          //!< Stores the time of last NACK reception

            /* We cannot reuse all sequences at once because in advanced ARQ operation some frames with high seq. number
             * may be transmitted at the same time with low seq. numbers. (E.g. SEQ=0 is the next seq for SEQ=65535)
             * Thus we reuse first half of sequences at once, the second half will be used later.
             */

            bool seq1stHalfCleaned; //!< Used to indicate which part of RecvSendSeqList should be prepared for reuse
            bool seq2ndHalfCleaned; //!< Used to indicate which part of RecvSendSeqList should be prepared for reuse

        };

        typedef std::map<std::pair<Mac48Address, Mac48Address>, FwdList> FwdSeqList;

        FwdSeqList m_forwardedFrames; //!< List of recently transmitted / relayed frames - to prevent loops



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

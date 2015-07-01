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
 * basing on ns-3 wifi module by:
 * Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

#ifndef RESCUE_PHY_HEADER_H
#define RESCUE_PHY_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"

#define RESCUE_PHY_PKT_TYPE_DATA        0 //data frame
#define RESCUE_PHY_PKT_TYPE_E2E_ACK     1 //end-to-end acknowledgment
#define RESCUE_PHY_PKT_TYPE_PART_ACK    2 //partial acknowledgment
#define RESCUE_PHY_PKT_TYPE_B           3 //beacon
#define RESCUE_PHY_PKT_TYPE_RR          4 //resource reservation


namespace ns3 {

    /**
     * \ingroup rescue
     *
     * Implements the Rescue PHY header
     */
    class RescuePhyHeader : public Header {
    public:
        RescuePhyHeader(void);
        RescuePhyHeader(uint8_t type);
        RescuePhyHeader(const Mac48Address srcAddr, const Mac48Address dstAddr, uint8_t type);
        RescuePhyHeader(const Mac48Address srcAddr, const Mac48Address senderAddr, const Mac48Address dstAddr, uint8_t type);
        RescuePhyHeader(const Mac48Address srcAddr, const Mac48Address senderAddr, const Mac48Address dstAddr,
                uint8_t type, uint16_t seq, uint16_t il);
        RescuePhyHeader(const Mac48Address srcAddr, const Mac48Address senderAddr, const Mac48Address dstAddr,
                uint8_t type, uint16_t duration, uint16_t seq, uint16_t mbf, uint16_t il);
        RescuePhyHeader(const Mac48Address srcAddr, const Mac48Address senderAddr, const Mac48Address dstAddr,
                uint8_t type, uint16_t duration, uint32_t beaconRep, uint16_t seq, uint16_t mbf, uint16_t il);
        RescuePhyHeader(const Mac48Address srcAddr, const Mac48Address dstAddr,
                uint8_t type, uint16_t blockAck, uint8_t contAck, uint16_t seq);
        RescuePhyHeader(const Mac48Address srcAddr,
                uint8_t type, uint32_t timestamp, uint32_t beaconInterval, uint32_t cfpPeriod, uint32_t tdmaTimeSlot);
        RescuePhyHeader(const Mac48Address srcAddr, const Mac48Address dstAddr,
                uint8_t type, uint16_t duration, uint16_t nextDuration, uint16_t seq, uint16_t mbf, uint16_t il);
        virtual ~RescuePhyHeader(void);

        static TypeId GetTypeId(void);



        // ....................CTRL field - subfields/flags values.............
        /**
         * \param type the type field
         */
        void SetType(uint8_t type);
        /**
         * \param dataRate the data rate field
         */
        void SetDataRate(uint8_t dataRate);
        /**
         * Sets positive ACK
         */
        void SetACK(void);
        /**
         * Sets negative ACK
         */
        void SetNACK(void);
        /**
         * \param sendWindow the duration field
         */
        void SetSendWindow(uint8_t sendWindow);
        /**
         * Sets Block ACK supported/requested
         */
        void SetBlockAckSupported(void);
        /**
         * Sets Block ACK disabled
         */
        void SetBlockAckDisabled(void);
        /**
         * Sets Continous ACK supported/requested
         */
        void SetContinousAckSupported(void);
        /**
         * Sets Continous ACK disabled
         */
        void SetContinousAckDisabled(void);
        /**
         * Sets QoS supported/requested
         */
        void SetQosSupported(void);
        /**
         * Sets QoS disabled
         */
        void SetQosDisabled(void);
        /**
         * Sets Distributed MAC protocl use
         */
        void SetDistributedMacProtocol(void);
        /**
         * Sets Centralised MAC protocl use
         */
        void SetCentralisedMacProtocol(void);
        /**
         * Set the Retry bit in the Frame Control field.
         */
        void SetRetry(void);
        /**
         * Un-set the Retry bit in the Frame Control field.
         */
        void SetNoRetry(void);
        /**
         * \param number of missed (unacknowledged) frames
         */
        void SetNACKedFrames(uint8_t nackedFrames);
        /**
         * \param nextDataRate the next data rate field
         */
        void SetNextDataRate(uint8_t nextDataRate);
        /**
         * \param channel the channel information field
         */
        void SetChannel(uint8_t channel);
        /**
         * \param txPower the TX Power information field
         */
        void SetTxPower(uint8_t txPower);
        /**
         * \param schedListSize the schedules list size field
         */
        void SetSchedulesListSize(uint8_t schedListSize);



        // ....................header fields values............................
        /**
         * \param 2 bytes consisting of following fields:
         * DATA frame:
         * - type (3b)
         * - data rate (2b)
         * - send window (4b)
         * - block ACK (1b)
         * - continous ACK (1b)
         * - QoS (1b)
         * - MAC protocol (1b)
         * - reserved (3b)
         * ACK frame:
         * - type (3b)
         * - NACK/ACK (1b)
         * - send window (4b)
         * - block ACK (1b)
         * - continous ACK (1b)
         * - NACKed frames (4b)
         * - reserved (2b)
         * RESOURCE RESERVATION frame:
         * - type (3b)
         * - data rate (2b)
         * - send window (4b)
         * - block ACK (1b)
         * - continous ACK (1b)
         * - QoS (1b)
         * - next data rate (2b)
         * - reserved (2b)
         * BEACON frame:
         * - type (3b)
         * - supported data rates (2b)
         * - QoS capability (1b)
         * - channel (2b)
         * - TX power (3b)
         * - schedules list size (5b)
         */
        void SetFrameControl(uint16_t ctrl);
        /**
         * \param duration the duration field
         */
        void SetDuration(uint16_t duration);
        /**
         * \param duration the duration field
         */
        void SetNextDuration(uint16_t nextDuration);
        /**
         * \param duration the beacon rep. field
         */
        void SetBeaconRep(uint32_t beaconRep);
        /**
         * \param blockAck the block ACK flag/field
         */
        void SetBlockAck(uint16_t blockAck);
        /**
         * \param continousAck the continous ACK flag/field
         */
        void SetContinousAck(uint8_t continousAck);
        /**
         * \param timestamp the timestamp field
         */
        void SetTimestamp(uint32_t timestamp);
        /**
         * \param addr the source address field
         */
        void SetSource(Mac48Address addr);
        /**
         * \param addr the sender address field
         */
        void SetSender(Mac48Address addr);
        /**
         * \param addr the destination address field
         */
        void SetDestination(Mac48Address addr);
        /**
         * \param seq the sequence number field
         */
        void SetSequence(uint16_t seq);
        /**
         * \param mbf the MBF field
         */
        void SetMBF(uint16_t mbf);
        /**
         * \param il the interleaver field
         */
        void SetInterleaver(uint8_t il);
        /**
         * \param beaconInterval the beacon interval field
         */
        void SetBeaconInterval(uint32_t beaconInterval);
        /**
         * \param cfpPeriod the CFP period
         */
        void SetCfpPeriod(uint32_t cfpPeriod);
        /**
         * \param tdmaDuration the TDMA time slot field
         */
        void SetTdmaTimeSlot(uint32_t tdmaTimeSlot);
        /**
         * \param number number of entry in the schedules list
         * \param scheduleEntry entry to add to the schedules TX list
         */
        void SetScheduleEntry(uint8_t number, Mac48Address scheduleEntry);
        /**
         * \param checksum the checksum field
         */
        void SetChecksum(uint32_t checksum);



        // ....................CTRL field - subfields/flags values.............
        /**
         * \return the type field value
         */
        uint8_t GetType(void) const;
        /**
         * \return true for DATA frame
         */
        bool IsDataFrame(void) const;
        /**
         * \return true for END-TO-END ACK frame
         */
        bool IsE2eAckFrame(void) const;
        /**
         * \return true for PARTIAL ACK frame
         */
        bool IsPartialAckFrame(void) const;
        /**
         * \return true for BEACON frame
         */
        bool IsBeaconFrame(void) const;
        /**
         * \return true for RESOURCE RESERVATION frame
         */
        bool IsResourceReservationFrame(void) const;
        /**
         * \return the data rate field value
         */
        uint8_t GetDataRate(void) const;
        /**
         * \return true if ACK flag is set (positive ACK)
         */
        bool IsACK(void) const;
        /**
         * \return the send window field value
         */
        uint8_t GetSendWindow(void) const;
        /**
         * \return true if block ACK flag is set
         */
        bool IsBlockAckSupported(void) const;
        /**
         * \return true if continous ACK flag is set
         */
        bool IsContinousAckSupported(void) const;
        /**
         * \return true if QoS flag is set
         */
        bool IsQosSupported(void) const;
        /**
         * \return true if MAC protocol flag is set
         */
        bool IsCentralisedMacProtocol(void) const;
        /**
         * \return true if the retry flag is set (frame is retransmitted)
         */
        bool IsRetry(void) const;
        /**
         * \return unACKed (missed) frames field value
         */
        uint8_t GetNACKedFrames(void) const;
        /**
         * \return the next data rate field value
         */
        uint8_t GetNextDataRate(void) const;
        /**
         * \return the channel field value
         */
        uint8_t GetChannel(void) const;
        /**
         * \return the TX power field value
         */
        uint8_t GetTxPower(void) const;
        /**
         * \return the schedules list size field value
         */
        uint8_t GetSchedulesListSize(void) const;



        // ....................header fields values............................
        /**
         * \return 2 bytes consisting of following fields:
         * DATA frame:
         * - type (3b)
         * - data rate (2b)
         * - send window (4b)
         * - block ACK (1b)
         * - continous ACK (1b)
         * - QoS (1b)
         * - MAC protocol (1b)
         * - reserved (3b)
         * ACK frame:
         * - type (3b)
         * - NACK/ACK (1b)
         * - send window (4b)
         * - block ACK (1b)
         * - continous ACK (1b)
         * - NACKed frames (4b)
         * - reserved (2b)
         * RESOURCE RESERVATION frame:
         * - type (3b)
         * - data rate (2b)
         * - send window (4b)
         * - block ACK (1b)
         * - continous ACK (1b)
         * - QoS (1b)
         * - next data rate (2b)
         * - reserved (2b)
         * BEACON frame:
         * - type (3b)
         * - supported data rates (2b)
         * - QoS capability (1b)
         * - channel (2b)
         * - TX power (3b)
         * - schedules list size (5b)
         */
        uint16_t GetFrameControl(void) const;
        /**
         * \return the duration field value
         */
        uint8_t GetDuration(void) const;
        /**
         * \return next duration ACK field value
         */
        uint16_t GetNextDuration(void) const;
        /**
         * \return beacon rep.field value
         */
        uint32_t GetBeaconRep(void) const;
        /**
         * \return the block ACK field value
         */
        uint8_t GetBlockAck(void) const;
        /**
         * \return the continous ACK field value
         */
        uint16_t GetContinousAck(void) const;
        /**
         * \return timestamp field value
         */
        uint32_t GetTimestamp(void) const;
        /**
         * \return the source address field value
         */
        Mac48Address GetSource(void) const;
        /**
         * \return the sender address field value
         */
        Mac48Address GetSender(void) const;
        /**
         * \return the destination address field value
         */
        Mac48Address GetDestination(void) const;
        /**
         * \return the sequence number field value
         */
        uint16_t GetSequence(void) const;
        /**
         * \return the MBF field value
         */
        uint16_t GetMBF(void) const;
        /**
         * \return the interleaver field value
         */
        uint8_t GetInterleaver(void) const;
        /**
         * \return beacon interval field value
         */
        uint32_t GetBeaconInterval(void) const;
        /**
         * \return contention free period field value
         */
        uint32_t GetCfpPeriod(void) const;
        /**
         * \return TDMA time slot field value
         */
        uint32_t GetTdmaTimeSlot(void) const;
        /**
         * \param number the number of demanded entry
         *
         * \return schedule entry value
         */
        Mac48Address GetScheduleEntry(uint8_t number) const;
        /**
         * \return the QoS field value
         */
        uint32_t GetChecksum(void) const;

        /**
         * \return the size of this header
         */
        uint32_t GetSize(void) const;

        // Inherrited methods
        virtual uint32_t GetSerializedSize(void) const;
        virtual void Serialize(Buffer::Iterator start) const;
        virtual uint32_t Deserialize(Buffer::Iterator start);
        virtual void Print(std::ostream &os) const;
        virtual TypeId GetInstanceTypeId(void) const;

    private:
        // ....................CTRL field - subfields/flags values.............
        uint8_t m_type; //<! type field
        uint8_t m_dataRate; //<! data rate field (supported data rate in Beacon frame)
        uint8_t m_nACK; //<! NACK/ACK field (0-NACK, 1-ACK)
        uint8_t m_sendWindow; //<! send window field
        uint8_t m_bAck; //<! block ACK field (1-requested/supported, 0-not allowed)
        uint8_t m_contAck; //<! continous ACK field (1-requested/supported, 0-not allowed)
        uint8_t m_qos; //<! QoS (1-supported, 0-not supported)
        uint8_t m_macProtocol; //<! MAC Protocol (0-distributed, 1-centralized)
        uint8_t m_retry; //<! retry flag (1-frame is retransmitted, 0-new frame)
        uint8_t m_NACKedFrames; //<! NACKed Frames field (how many frames are missed)
        uint8_t m_nextDataRate; //<! next data rate field
        int8_t m_channel; //<! channel number field
        int8_t m_txPower; //<! TX power field
        int8_t m_schedListSize; //<! schedules list size field (the number of entries in the schedules list)

        // ....................header fields values............................
        uint16_t m_duration; //<! duration field
        uint16_t m_nextDuration; //<! next duration field
        uint32_t m_beaconRep; //<! beacon repetition field
        uint16_t m_blockAck; //<! block ACK field/flag
        uint8_t m_continousAck; //<! continous ACK field/flag
        uint32_t m_timestamp; //<! timestamp field
        Mac48Address m_srcAddr; //<! source address field
        Mac48Address m_senderAddr; //<! sender address field
        Mac48Address m_dstAddr; //<! destination address field
        uint16_t m_sequence; //<! sequence number field
        uint16_t m_mbf; //<! interleaver field
        uint8_t m_interleaver; //<! interleaver field
        uint32_t m_beaconInterval; //<! beacon interval field
        uint32_t m_cfpPeriod; //<! CFP period field
        uint32_t m_tdmaTimeSlot; //<! TDMA time slot field
        Mac48Address m_schedulesList [32]; //<! schedules list field (?)
        uint32_t m_checksum; //<! checksum field
    };

}

#endif // RESCUE_PHY_HEADER_H

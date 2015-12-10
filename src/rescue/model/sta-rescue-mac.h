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

#ifndef STA_RESCUE_MAC_H
#define STA_RESCUE_MAC_H

#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/event-id.h"
#include "ns3/traced-value.h"
#include "ns3/random-variable-stream.h"
#include "ns3/mac48-address.h"
#include "ns3/timer.h"

#include "rescue-mac.h"
#include "rescue-mac-header.h"

#include <list>

namespace ns3 {

    class RescuePhy;
    class RescueChannel;
    class LowRescueMac;
    class RescueMacCsma;
    class RescueRemoteStationManager;
    class RescueArqManager;
    class RescueNetDevice;

    /**
     * \brief base class for High-Level functionalities of
     * Decentralised Rescue MAC
     * \ingroup rescue
     */
    class StaRescueMac : public RescueMac {
    public:
        StaRescueMac();
        virtual ~StaRescueMac();
        static TypeId GetTypeId(void);

        /**
         * Clear the transmission queues and pending packets, reset counters.
         */
        virtual void Clear(void);

        /**
         * \param pkt the packet to send.
         * \param dest the address to which the packet should be sent.
         *
         * The packet should be enqueued in a tx queue, and should be
         * dequeued as soon as the CSMA determines that
         * access is granted to this MAC.
         */
        virtual void Enqueue(Ptr<Packet> pkt, Mac48Address dest);

        /**
         * \param pkt the packet to send.
         * \param dest the address to which the packet should be sent.
         *
         * Should be used to enqueue retransmitted packets by ARQ manager
         */
        virtual void EnqueueRetry(Ptr<Packet> pkt, Mac48Address dest);

        //virtual void NotifyEnqueueRelay (Ptr<Packet> pkt, Mac48Address dest);

        virtual void NotifyEnqueuedPacket(Ptr<Packet> pkt, Mac48Address dest);

        /**
         * \param pkt the packet to send.
         * \param phyHdr the header to send.
         *
         * Should be used to enqueue ACK/NACK packets by ARQ manager
         */
        virtual void EnqueueAck(Ptr<Packet> pkt, RescuePhyHeader ackHdr);

        /**
         * invoked by ReceivePacketDone to process DATA frames
         *
         * \param pkt the received DATA frame
         * \param phyHdr PHY header associated with the received DATA frame
         */
        virtual void ReceivePacket(Ptr<Packet> pkt, RescuePhyHeader phyHdr);
        /**
         * invoked by ReceivePacketDone to process RESOURCE RESERVATION frame
         *
         * \param pkt the received RESOURCE RESERVATION frame
         * \param phyHdr PHY header associated with the received RESOURCE RESERVATION frame
         */
        virtual void ReceiveResourceReservation(Ptr<Packet> pkt, RescuePhyHeader phyHdr);
        /**
         * invoked by ReceivePacketDone to process BEACON frame
         *
         * \param pkt the received BEACON frame
         * \param phyHdr PHY header associated with the received BEACON frame
         */
        virtual void ReceiveBeacon(Ptr<Packet> pkt, RescuePhyHeader phyHdr);

        /**
         * used by ARQ manager to notify about allowance for TX
         */
        virtual void NotifyTxAllowed(void);

        virtual void NotifySendPacketDone(void);

    private:
        void GenerateResourceReservationFor(Ptr<Packet> pktData);
        void GenerateTdmaTimeSlotReservation();

        void StartTDMAtimeSlot(void);

        void BeaconIntervalEnd(void);
        void CfpEnd(void);
        void TDMAtimeSlotEnd(void);

        Mac48Address m_apAddress; //<! associated AP address
        uint8_t m_supportedDataRates; //<! supported data rates setting
        uint8_t m_channel; //<! channel number
        uint8_t m_txPower; //<! TX power setting
        Time m_beaconInterval; //<! beacon interval duration
        Time m_cfpDuration; //<! Contention Free Period duration
        Time m_tdmaTimeSlot; //<! TDMA time slot duration

        uint8_t m_schedulesNumber; //<! Number of schedules in current CFP

        Timer m_beaconTimer;
        Timer m_cfpTimer;
        std::map<uint8_t, Timer> m_slotAwaitingTimers;
        Timer m_currentSlotTimer;



    protected:
        virtual void DoInitialize();
        virtual void DoDispose();
    };

}

#endif // STA_RESCUE_MAC_H

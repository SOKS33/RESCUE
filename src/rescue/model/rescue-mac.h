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

#ifndef RESCUE_MAC_H
#define RESCUE_MAC_H

#include "ns3/address.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"

#include "rescue-mac-header.h"
#include "rescue-phy-header.h"
#include "rescue-mode.h"

namespace ns3 {

    uint8_t compressMac(ns3::Mac48Address addr);


    class RescuePhy;
    class RescueChannel;
    class LowRescueMac;
    class RescueMacCsma;
    class RescueMacTdma;
    class RescueRemoteStationManager;
    class RescueArqManager;
    class RescueNetDevice;

    enum StationType {
        STA,
        AP,
        ADHOC_STA
    };

    enum RelayBehavior {
        FORWARD,
        REPLACE_COPY,
        RESEND_ACK,
        DROP
    };

    /**
     * \brief base class for all MAC-high-level rescue objects.
     * \ingroup rescue
     */
    class RescueMac : public Object {
    public:
        RescueMac();
        virtual ~RescueMac();
        static TypeId GetTypeId(void);

        /**
         * Clear the transmission queues and pending packets, reset counters.
         */
        virtual void Clear(void);

        /**
         * \param phy the physical layer to attach to this MAC.
         */
        void AttachPhy(Ptr<RescuePhy> phy);
        /**
         * \param dev the device this MAC is attached to.
         */
        void SetDevice(Ptr<RescueNetDevice> dev);
        /**
         * \param manager RescueRemoteStationManager associated with this MAC
         */
        void SetRemoteStationManager(Ptr<RescueRemoteStationManager> manager);
        /**
         * \param arqManager RescueArqManager associated with this MAC
         */
        void SetArqManager(Ptr<RescueArqManager> arqManager);
        /**
         * Assign a fixed random variable stream number to the random variables
         * used by this model.  Return the number of streams (possibly zero) that
         * have been assigned.
         *
         * \param stream first stream index to use
         * \return the number of stream indices assigned by this model
         */
        int64_t AssignStreams(int64_t stream);
        /**
         * \param upCallback the callback to invoke when a packet must be forwarded up the stack.
         */
        void SetForwardUpCb(Callback<void, Ptr<Packet>, Mac48Address, Mac48Address> cb);

        /**
         * \param address the current address of this MAC layer.
         */
        void SetAddress(Mac48Address addr);
        /**
         * This method is invoked by a subclass to specify what type of
         * station it is implementing.
         *
         * \param type the type of station.
         */
        void SetTypeOfStation(StationType type);
        /**
         * \param cw the minimum contetion window
         */
        void SetCwMin(uint32_t cw);
        /**
         * \param cw the maximal contetion window
         */
        void SetCwMax(uint32_t cw);
        /**
         * \param duration the slot duration
         */
        void SetSlot(Time duration);
        /**
         * \param duration the SIFS duration
         */
        void SetSifs(Time duration);
        /**
         * \param duration the LIFS duration
         */
        void SetLifs(Time duration);
        /**
         * \param length the length of queues used by this MAC
         */
        void SetQueueLimits(uint32_t length);
        /**
         * \param duration the Basic ACK Timeout duration
         */
        //void SetBasicAckTimeout (Time duration);

        /**
         * \return the current address of this MAC layer.
         */
        Mac48Address GetAddress(void) const;
        /**
         * \return broadcast address
         */
        Mac48Address GetBroadcast(void) const;
        /**
         * \return pointer to CsmaMac object
         */
        Ptr<RescueMacCsma> GetCsmaMac(void) const;
        /**
         * \return pointer to CsmaMac object
         */
        Ptr<RescueMacTdma> GetTdmaMac(void) const;

        /**
         * \return minimal contention window
         */
        uint32_t GetCwMin(void) const;
        /**
         * \return maximal contention window
         */
        uint32_t GetCwMax(void) const;
        /**
         * \return the current slot duration.
         */
        Time GetSlot(void) const;
        /**
         * \return the current SIFS duration.
         */
        Time GetSifs(void) const;
        /**
         * \return the current LIFS duration.
         */
        Time GetLifs(void) const;
        /**
         * \return length the length of queues used by this MAC
         */
        uint32_t GetQueueLimits(void) const;
        /**
         * \return duration the Basic ACK Timeout duration
         */
        //Time GetBasicAckTimeout (void) const;

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
         * invoked to notify about successful DATA frame enqueing
         *
         * \param pkt the received DATA frame
         * \param phyHdr MAC header associated with the received DATA frame
         */
        void EnqueueOk(Ptr<const Packet> pkt, const RescueMacHeader &hdr);

        /**
         * invoked by ReceivePacketDone to process DATA frames
         *
         * \param pkt the received DATA frame
         * \param phyHdr PHY header associated with the received DATA frame
         */
        virtual void ReceivePacket(Ptr<Packet> pkt, RescuePhyHeader phyHdr);

        /**
         * invoked to notify about new DATA frame received
         *
         * \param pkt the received DATA frame
         * \param phyHdr PHY header associated with the received DATA frame
         */
        void RxOk(Ptr<const Packet> pkt, const RescuePhyHeader &hdr);

        /**
         * invoked by ReceivePacketDone to process RESOURCE RESERVATION frame
         * currently NOT SUPPORTED
         *
         * \param pkt the received RESOURCE RESERVATION frame
         * \param phyHdr PHY header associated with the received RESOURCE RESERVATION frame
         */
        virtual void ReceiveResourceReservation(Ptr<Packet> pkt, RescuePhyHeader phyHdr);
        /**
         * invoked by ReceivePacketDone to process BEACON frame
         * currently NOT SUPPORTED
         *
         * \param pkt the received BEACON frame
         * \param phyHdr PHY header associated with the received BEACON frame
         */
        virtual void ReceiveBeacon(Ptr<Packet> pkt, RescuePhyHeader phyHdr);

        /**
         * \param pkt the received packet
         * \param phyHdr PHY header associated with the received packet
         *
         * \return true if the received frame is scheduled for forwarding
         */
        virtual bool ShouldBeForwarded(Ptr<Packet> pkt, RescuePhyHeader phyHdr);

        /**
         * used by ARQ manager to notify about allowance for TX
         */
        virtual void NotifyTxAllowed(void);

        virtual void NotifySendPacketDone(void);

    protected:
        virtual void DoInitialize();
        virtual void DoDispose();

        Callback <void, Ptr<Packet>, Mac48Address, Mac48Address> m_forwardUpCb; //!< The callback to invoke when a packet must be forwarded up the stack

        Ptr<RescuePhy> m_phy; //!< Pointer to RescuePhy (actually send/receives frames)
        Ptr<RescueMacCsma> m_csmaMac; //!< Pointer to RescueMacCsma
        Ptr<RescueMacTdma> m_tdmaMac; //!< Pointer to RescueMacTdma
        Ptr<RescueNetDevice> m_device; //!< Pointer to RescueNetDevice
        Ptr<RescueRemoteStationManager> m_remoteStationManager; //!< Pointer to RescueRemoteStationManager (rate control)
        Ptr<RescueArqManager> m_arqManager; //!< Pointer to RescueArqManager (rate control)
        Ptr<UniformRandomVariable> m_random; //!< Provides uniform random variables.

        Mac48Address m_address; //!< Address of this MAC
        StationType m_type; //!< Type of this station

        // for trace and performance evaluation
        TracedCallback<uint32_t, uint32_t, Ptr<const Packet>, const RescueMacHeader &> m_traceEnqueue; //<! Trace Hookup for enqueue a DATA
        TracedCallback<uint32_t, uint32_t, Ptr<const Packet>, const RescuePhyHeader &> m_traceDataRxOk; //<! Trace Hookup for DATA RX (by final station)

    };

}

#endif // RESCUE_MAC_H

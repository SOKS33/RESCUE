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

#ifndef LOW_RESCUE_MAC_H
#define LOW_RESCUE_MAC_H

#include "ns3/address.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "rescue-net-device.h"
#include "rescue-phy.h"
#include "rescue-channel.h"
#include "rescue-phy-header.h"
#include "rescue-mode.h"

namespace ns3 {

    class RescuePhy;
    class RescueChannel;
    class RescueMac;
    class RescueNetDevice;

    /**
     * \brief base class for all MAC-level rescue objects.
     * \ingroup rescue
     *
     * Currently this class encapsulates only RescueMacCsma and RescueMacTdma
     *
     */
    class LowRescueMac : public Object {
    public:

        //MODIF 2
        virtual void SetTxDuration(Time txDuration) = 0;
        virtual void SetChannelDelay(Time delay) = 0;

        /**
         * \param phy the physical layer to attach to this MAC.
         */
        virtual void AttachPhy(Ptr<RescuePhy> phy) = 0;
        /**
         * \param dev the device this MAC is attached to.
         */
        virtual void SetDevice(Ptr<RescueNetDevice> dev) = 0;
        /**
         * \param mac the high MAC sublayer to attach to this (lower) MAC.
         */
        virtual void SetHiMac(Ptr<RescueMac> mac) = 0;
        /**
         * \param manager RescueRemoteStationManager associated with this MAC
         */
        virtual void SetRemoteStationManager(Ptr<RescueRemoteStationManager> manager) = 0;
        /**
         * Assign a fixed random variable stream number to the random variables
         * used by this model.  Return the number of streams (possibly zero) that
         * have been assigned.
         *
         * \param stream first stream index to use
         * \return the number of stream indices assigned by this model
         */
        virtual int64_t AssignStreams(int64_t stream) = 0;

        /**
         * \param pkt the packet to send.
         * \param dest the address to which the packet should be sent.
         *
         * The packet should be enqueued in a tx queue, and should be
         * dequeued as soon as the CSMA determines that
         * access is granted to this MAC.
         */
        virtual bool Enqueue(Ptr<Packet> pkt, Mac48Address dest) = 0;

        /**
         * Method used to start operation of this lower MAC entity
         * (eg. start CSMA-CA MAC at the beginning of Contention Period)
         *
         * /return MAC was started succesfully.
         */
        virtual bool StartOperation() = 0;
        /**
         * Method used to start operation of this lower MAC entity
         * (eg. start CSMA-CA MAC at the beginning of Contention Period)
         *
         * \param duration the duration of operation period
         *
         * /return MAC was started succesfully.
         */
        virtual bool StartOperation(Time duration) = 0;
        /**
         * Method used to stop operation of this lower MAC entity
         * (eg. stop CSMA-CA MAC at the and of Contention Period)
         *
         * /return MAC was stopped succesfully.
         */
        virtual bool StopOperation() = 0;
        /**
         * Method used to stop operation of this lower MAC entity
         * (eg. stop CSMA-CA MAC at the and of Contention Period)
         *
         * \param duration the duration of inactivity period
         *
         * /return MAC was stopped succesfully.
         */
        virtual bool StopOperation(Time duration) = 0;

        /**
         * This method is called after end of packet transmission
         *
         * \param pkt transmitted packet
         */
        virtual void SendPacketDone(Ptr<Packet> packet) = 0;
        /**
         * Method used by PHY to notify MAC about begin of frame reception
         *
         * \param phy receiving phy
         * \param pkt the received packet
         */
        virtual void ReceivePacket(Ptr<RescuePhy> phy, Ptr<Packet> packet) = 0;
        /**
         * Method used by PHY to notify MAC about end of frame reception
         *
         * \param phy receiving phy
         * \param pkt the received packet
         * \param phyHdr PHY header associated with the received packet
         * \param snr SNR of this transmission
         * \param mode the mode (RescueMode) of this transmission
         * \param correctPhyHdr true if the PHY header was correctly received
         * \param correctData true if the payload was correctly received
         * \param wasReconstructed true if the frame was reconstructed basing on almost two received frame copies
         */
        virtual void ReceivePacketDone(Ptr<RescuePhy> phy, Ptr<Packet> pkt, RescuePhyHeader phyHdr,
                double snr, RescueMode mode,
                bool correctPhyHdr, bool correctData, bool wasReconstructed) = 0;
        /**
         * \param upCallback the callback to invoke when a packet must be forwarded up the stack.
         */
        //virtual void SetForwardUpCb (Callback<void, Ptr<Packet>, Mac48Address, Mac48Address> cb) = 0;

        /**
         * Clear the transmission queues and pending packets, reset counters.
         */
        virtual void Clear(void) = 0;

    private:

    };

}

#endif // LOW_RESCUE_MAC_H

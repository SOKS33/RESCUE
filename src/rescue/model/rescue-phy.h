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
 * and ns-3 wifi module by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef RESCUE_PHY_H
#define RESCUE_PHY_H

#include "ns3/simulator.h"
#include "ns3/event-id.h"
#include "ns3/random-variable-stream.h"

#include "rescue-mac.h"
#include "low-rescue-mac.h"
//#include "rescue-mac-csma.h"
//#include "rescue-mac-tdma.h"
#include "rescue-phy.h"
#include "rescue-phy-header.h"
#include "rescue-mode.h"
//#include "rescue-error-rate-model.h"


namespace ns3 {

    class LowRescueMac;
    class RescueMacCsma;
    //class RescueMacTdma;

    /**
     * \brief Rescue PHY layer model
     * \ingroup rescue
     *
     */
    class RescuePhy : public Object {
    public:

        enum State {
            IDLE, TX, RX, COLL
        };

        RescuePhy();
        virtual ~RescuePhy();
        void Clear();

        static TypeId GetTypeId(void);

        /**
         * \param dev the device this PHY is attached to.
         */
        void SetDevice(Ptr<RescueNetDevice> device);
        /**
         * \param mac the (high) MAC this PHY is attached to.
         */
        void SetMac(Ptr<RescueMac> mac);
        /**
         * \param mac the (low) CSMA MAC this PHY is attached to.
         */
        void SetMacCsma(Ptr<RescueMacCsma> csmaMac);
        /**
         * \param mac the (low) TDMA MAC this PHY is attached to.
         */
        void SetMacTdma(Ptr<RescueMacTdma> tdmaMac);
        /**
         * Method used to notify PHY about Contention Free Period
         */
        void NotifyCFP();
        /**
         * Method used to notify PHY about Contention Period
         */
        void NotifyCP();
        /**
         * \param channel the channel used by this PHY
         */
        void SetChannel(Ptr<RescueChannel> channel);
        /**
         * \param dBm the TX power used for transmission
         */
        void SetTxPower(double dBm);
        /**
         * \param rate the error rate model
         */
        //void SetErrorRateModel (Ptr<RescueErrorRateModel> rate);

        /**
         * \return channel the channel used by this PHY
         */
        Ptr<RescueChannel> GetChannel();
        /**
         * \return the RX power used for transmission
         */
        double GetTxPower();
        /**
         * \return the error rate model being used
         */
        //Ptr<RescueErrorRateModel> GetErrorRateModel (void) const;

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
         * Invoked to start frame transmission
         *
         * \param pkt the packet to send
         * \param mode the transmission mode to use to send this packet
         * \param il interleaver number
         * \return true if transmission was started
         */
        bool SendPacket(Ptr<Packet> pkt, RescueMode mode, uint16_t il);
        /**
         * Invoked to start RELAYED frame transmission
         *
         * \param relayedPkt pair of packet for transmission and associated PHY header
         * \param mode the transmission mode to use to send this packet
         * \return true if transmission was started
         */
        bool SendPacket(std::pair<Ptr<Packet>, RescuePhyHeader> relayedPkt, RescueMode mode);
        /**
         * This method is called to end frame transmission
         *
         * \param pkt transmitted packet
         */
        void SendPacketDone(Ptr<Packet> pkt);
        /**
         * Invoked to start frame reception
         *
         * \param pkt the arriving packet
         * \param mode the transmission mode of the arriving packet
         * \param txDuration duration of this transmission
         * \param rxPower the receive power in dBm
         */
        void ReceivePacket(Ptr<Packet> pkt, RescueMode mode, Time txDuration, double_t rxPower);
        /**
         * This method is called to end frame reception
         *
         * \param pkt the received packet
         * \param mode the transmission mode of the arriving packet
         * \param rxPower the receive power in dBm
         */
        void ReceivePacketDone(Ptr<Packet> pkt, RescueMode mode, double rxPower);

        /**
         * \return true if PHY is idle
         */
        bool IsIdle();
        /**
         * Used for TX time calculation when PhyHdr is not constructed yet
         *
         * \param basicSize the number of bytes to send with basic mode (usually PHY header size)
         * \param dataSize the number of bytes in the packet to send with data rate (payload size)
         * \param basicMode the basic TX mode (RescueMode)
         * \param dataMode the data TX mode (RescueMode)
         * \param type type of the transmitted frame
         * \return the total amount of time this PHY will stay busy for
         *          the transmission of these bytes.
         */
        Time CalTxDuration(uint32_t basicSize, uint32_t dataSize, RescueMode basicMode, RescueMode dataMode, uint16_t type);
        /**
         * Used for TX time calculation when PhyHdr is alreade constructed
         *
         * \param basicSize the number of bytes to send with basic mode (usually PHY header size)
         * \param dataSize the number of bytes in the packet to send with data rate (payload size)
         * \param basicMode the basic TX mode (RescueMode)
         * \param dataMode the data TX mode (RescueMode)
         * \return the total amount of time this PHY will stay busy for
         *          the transmission of these bytes.
         */
        Time CalTxDuration(uint32_t basicSize, uint32_t dataSize, RescueMode basicMode, RescueMode dataMode);

        /**
         * \param mode data TX mode (RescueMode)
         * \return preamble duration associated with given data TX RescueMode
         */
        Time GetPhyPreambleDuration(RescueMode mode);
        /**
         * \param phyHdr PHY header
         * \param mode data TX mode (RescueMode)
         * \return PHY header TX duration (associated with given data TX RescueMode)
         */
        Time GetPhyHeaderDuration(RescuePhyHeader phyHdr, RescueMode mode);
        /**
         * \param pkt DATA packet for transmission
         * \param mode data TX mode (RescueMode)
         * \return DATA TX duration of this packet
         */
        Time GetDataDuration(Ptr<Packet> pkt, RescueMode mode);
        /**
         * \param payloadMode data TX mode (RescueMode)
         * \return RescueMode for preamble transmission (associated with given data TX RescueMode)
         */
        RescueMode GetPhyPreambleMode(RescueMode payloadMode);
        /**
         * \param payloadMode data TX mode (RescueMode)
         * \return RescueMode for PHY header transmission (associated with given data TX RescueMode)
         */
        RescueMode GetPhyHeaderMode(RescueMode payloadMode);

        /**
         * \return the number of transmission modes supported by this PHY
         */
        uint32_t GetNModes(void) const;
        /**
         * \param mode number of expected mode
         * \return the given RescueMode
         */
        RescueMode GetMode(uint32_t mode) const;
        /**
         * \param mode RescueMode to check
         * \return true if a given mode is supported by this PHY
         */
        bool IsModeSupported(RescueMode mode) const;

        /**
         * \return a RescueMode for OFDM at 3Mbps
         */
        static RescueMode GetOfdm3Mbps();
        /**
         * \return a RescueMode for OFDM at 6Mbps
         */
        static RescueMode GetOfdm6Mbps();
        /**
         * \return a RescueMode for OFDM at 9Mbps
         */
        static RescueMode GetOfdm9Mbps();
        /**
         * \return a RescueMode for OFDM at 12Mbps
         */
        static RescueMode GetOfdm12Mbps();
        /**
         * \return a RescueMode for OFDM at 18Mbps
         */
        static RescueMode GetOfdm18Mbps();
        /**
         * \return a RescueMode for OFDM at 24Mbps
         */
        static RescueMode GetOfdm24Mbps();
        /**
         * \return a RescueMode for OFDM at 36Mbps
         */
        static RescueMode GetOfdm36Mbps();

    private:
        /**
         * Calculate the success rate of the chunk given the SINR, duration, and Rescue mode.
         * The duration and mode are used to calculate how many bits are present in the chunk.
         *
         * \param snir SINR
         * \param duration
         * \param mode
         * \return the success rate
         */
        //double CalculateChunkSuccessRate (double snir, Time duration, RescueMode mode);
        /**
         * Calculate LLR
         * CURRENTLY NOT IMPLEMENTED
         */
        double CalculateLLR(double sinr);
        /**
         * Calculate CI
         * CURRENTLY NOT IMPLEMENTED
         */
        double CalculateCI(double prevCI, double llr, uint32_t symbols);

        /**
         * Removes stored fram copies.
         *
         * \param phyHdr PHY header associated with frame which copies should be removed
         */
        void RemoveFrameCopies(RescuePhyHeader phyHdr);
        /**
         * Stores a frame copy. Return true if previous copies of the frame were stored.
         *
         * \param pkt the arriving packet (without PHY header)
         * \param phyHdr the arriving PHY header
         * \param sinr signal-to-noise/interference ratio of received frame copy in dBm
         * \param per packet error rate of received frame copy in dBm
         * \param mode the PHY TX mode of received frame
         * \return true if previous copies of the frame were found
         */
        bool AddFrameCopy(Ptr<Packet> pkt, RescuePhyHeader phyHdr, double sinr, double per, double ber, RescueMode mode);
        /**
         * Checks if frame was succesfully restored from stored copies.
         *
         * \param phyHdr PHY header associated with frame to check
         * \return true if frame was succesfully restored
         */
        bool IsRestored(RescuePhyHeader phyHdr, bool useBB2);


        Ptr<RescueNetDevice> m_device; //!< Pointer to RescueNetDevice
        Ptr<RescueMac> m_mac; //!< Pointer to RescueMac
        Ptr<LowRescueMac> m_lowMac; //!< Pointer to current LowRescueMac (CSMA or TDMA)
        Ptr<RescueMacCsma> m_csmaMac; //!< Pointer to CSMA MAC
        Ptr<RescueMacTdma> m_tdmaMac; //!< Pointer to TDMA MAC
        Ptr<RescueChannel> m_channel; //!< Pointer to associated RescueChannel.
        //Ptr<RescueErrorRateModel> m_errorRateModel;   //!< Pointer to RescueErrorRateModel.
        Ptr<UniformRandomVariable> m_random; //!< Provides uniform random variables.

        // PHY parameters
        double m_txPower; //!< transmission power (dBm)
        double m_csThr; //!< carrier sense threshold (dBm)
        double m_rxThr; //!< rx power threshold (dBm)
        Time m_preambleDuration; //!< duration of preamble
        uint32_t m_trailerSize; //!< size of trailer (B)
        RescueModeList m_deviceRateSet; //!< List of supported TX/RX modes (RescueMode)

        double m_berThr; //!< PER threshold - frames with higher PER are unusable for reconstruction purposes and should not be stored or forwarded
        bool m_useBB2; //!< Use BlackBox #2 for error calculation? (if it is possible)
        bool m_useLOTF; //!< Use Rescue links-on-the-fly

        State m_state; //!< Current state of this PHY
        bool m_csBusy; //!< Busy channel indicator (true = channel busy)
        Time m_csBusyEnd; //!< Expected time when channel become idle

        Ptr<Packet> m_pktRx; //!< Currently received packet

        /*
         * Structures to keep information about stored frames' copies
         */
        struct RxFrameCopies;

        typedef std::list<struct RxFrameCopies> RxFrameAccumulator;
        typedef std::list<struct RxFrameCopies>::reverse_iterator RxFrameAccumulatorRI;
        typedef std::list<struct RxFrameCopies>::iterator RxFrameAccumulatorI;

        /**
         * A struct to keep information about stored frames' copies
         *
         * \param phyHdr PHY header associated with transmitted packet
         * \param pkt transmitted packet
         * \param snr_db vector of SNRs (instantaneous Es/N0 at the receiver side,
         *               includes the channel fading state) per complex symbol (for each frame copy) in dB
         * \param linkPER vector of packet error probability (for each frame copy), range 0.0...0.5
         * \param constellationSize constellation size associated with transmission: 2=BPSK, 4=QPSK, 16=16QAM
         * \param spectralEfficiency number of information bits per complex symbol (affected by code rate
         *                           and constellation size)
         * \param packetLength length of packet in bytes
         * \param tstamp time stamp of first arrived copy
         */
        struct RxFrameCopies {
            RxFrameCopies(RescuePhyHeader phyHdr,
                    Ptr<Packet> pkt,
                    std::vector<double> snr_db,
                    std::vector<double> linkPER,
                    std::vector<double> linkBER,
                    //int constellationSize,
                    std::vector<int> constellationSizes,
                    //double spectralEfficiency,
                    std::vector<double> spectralEfficiencies,
                    int packetLength,
                    Time tstamp);
            RescuePhyHeader phyHdr;
            Ptr<Packet> pkt;
            std::vector<double> snr_db;
            std::vector<double> linkPER;
            std::vector<double> linkBER;
            //int constellationSize;
            std::vector<int> constellationSizes;
            //double spectralEfficiency;
            std::vector<double> spectralEfficiencies;
            int packetLength;
            Time tstamp;
        };

        RxFrameAccumulator m_rxFrameAccumulator; //!< Received frames' copies buffer

    protected:
        TracedCallback<Ptr<const Packet>> m_traceSend; //<! Trace Hookup for enqueue a DATA
        TracedCallback<Ptr<const Packet>, double> m_traceRecv; //<! Trace Hookup for DATA RX (by final station)

    };

} // namespace ns3

#endif // RESCUE_PHY_H

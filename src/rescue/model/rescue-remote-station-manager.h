/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef RESCUE_REMOTE_STATION_MANAGER_H
#define RESCUE_REMOTE_STATION_MANAGER_H

#include <vector>
#include <utility>
#include "ns3/mac48-address.h"
#include "ns3/traced-callback.h"
#include "ns3/packet.h"
#include "ns3/object.h"
#include "ns3/nstime.h"
#include "rescue-mode.h"

namespace ns3 {

    struct RescueRemoteStation;
    struct RescueRemoteStationState;
    class RescuePhy;
    class RescueMac;

    /**
     * \brief Tid independent remote station statistics
     * \ingroup rescue
     *
     * Structure is similar to struct sta_info in Linux kernel (see
     * net/mac80211/sta_info.h)
     */
    class RescueRemoteStationInfo {
    public:
        RescueRemoteStationInfo();
        /**
         * \brief Updates average frame error rate when data
         * was transmitted successfully.
         * \param retryCounter is src value at the moment of
         * success transmission.
         */
        void NotifyTxSuccess(uint32_t retryCounter);
        void NotifyTxSuccessSNR(uint32_t retryCounter, double snr);
        /// Updates average frame error rate when final data has failed.
        void NotifyTxFailed();
        /// Return frame error rate (probability that frame is corrupted due to transmission error).
        double GetFrameErrorRate() const;

    private:
        /**
         * \brief Calculate averaging coefficient for frame error rate. Depends on time of the last update.
         * \attention Calling this method twice gives different results,
         * because it resets time of last update.
         *
         * \return average coefficient for frame error rate
         */
        double CalculateAveragingCoefficient();
        /// averaging coefficient depends on the memory time
        Time m_memoryTime;
        /// when last update has occured
        Time m_lastUpdate;
        /// moving percentage of failed frames
        double m_failAvg;
        /// average SNR(?)
        double m_snrAvg;
    };

    /**
     * \ingroup rescue
     * \brief hold a list of per-remote-station state.
     *
     * \attention This class is just adaptation and simplification of WifiRemoteStationManager
     *            Some methods (e.g. related for AP operation) were left for possible future use
     *            (e.g. centralised MAC implementation)
     *
     * \sa ns3::RescueRemoteStation.
     */
    class RescueRemoteStationManager : public Object {
    public:
        static TypeId GetTypeId(void);

        RescueRemoteStationManager();
        virtual ~RescueRemoteStationManager();

        /**
         * Set up PHY associated with this device since it is the object that
         * knows the full set of transmit rates that are supported.
         *
         * \param phy the PHY of this device
         */
        virtual void SetupPhy(Ptr<RescuePhy> phy);

        /**
         * \param mac the MAC of this device
         */
        void SetupMac(Ptr<RescueMac> mac);

        /**
         * \param cw the minimum contetion window
         */
        void SetCwMin(uint32_t cw);
        /**
         * \param cw the maximal contetion window
         */
        void SetCwMax(uint32_t cw);

        /**
         * \return minimal contention window
         */
        uint32_t GetCwMin(void);
        /**
         * \return maximal contention window
         */
        uint32_t GetCwMax(void);

        /**
         * Return the maximum STA retry count (SRC).
         *
         * \return the maximum SRC
         */
        //uint32_t GetMaxSrc (void) const;
        /**
         * Sets the maximum STA retry count (SRC).
         *
         * \param maxSrc the maximum SRC
         */
        //void SetMaxSrc (uint32_t maxSrc);

        /**
         * \return the current address of associated MAC layer.
         */
        Mac48Address GetAddress(void) const;

        /**
         * Reset the station, invoked in a STA upon dis-association or in an AP upon reboot.
         */
        void Reset(void);
        /**
         * Invoked in a STA upon association to store the set of rates which belong to the
         * BSSBasicRateSet of the associated AP and which are supported locally.
         * Invoked in an AP to configure the BSSBasicRateSet.
         *
         * \param mode the RescueMode to be added to the basic mode set
         */
        void AddBasicMode(RescueMode mode);

        /**
         * Return the default transmission mode.
         *
         * \return RescueMode the default transmission mode
         */
        RescueMode GetDefaultMode(void) const;
        /**
         * Return the number of basic modes we support.
         *
         * \return the number of basic modes we support
         */
        uint32_t GetNBasicModes(void) const;
        /**
         * Return a basic mode from the set of basic modes.
         *
         * \param i index of the basic mode in the basic mode set
         * \return the basic mode at the given index
         */
        RescueMode GetBasicMode(uint32_t i) const;

        /**
         * Return a mode for non-unicast packets.
         *
         * \return RescueMode for non-unicast packets
         */
        RescueMode GetNonUnicastMode(void) const;

        /**
         * Return a contention window for non-unicast packets.
         *
         * \return contention window for non-unicast packets
         */
        uint32_t GetNonUnicastCw(void) const;

        /**
         * Invoked in an AP upon disassociation of a
         * specific STA.
         *
         * \param address the address of the STA
         */
        void Reset(Mac48Address address);
        /**
         * Invoked in a STA or AP to store the set of
         * modes supported by a destination which is
         * also supported locally.
         * The set of supported modes includes
         * the BSSBasicRateSet.
         *
         * \param address the address of the station being recorded
         * \param mode the RescueMode supports by the station
         */
        void AddSupportedMode(Mac48Address address, RescueMode mode);

        /**
         * Return whether the station state is brand new.
         *
         * \param address the address of the station
         * \return true if the state of the station is brand new,
         *          false otherwise
         */
        bool IsBrandNew(Mac48Address address) const;
        /**
         * Return whether the station associated.
         *
         * \param address the address of the station
         * \return true if the station is associated,
         *          false otherwise
         */
        bool IsAssociated(Mac48Address address) const;
        /**
         * Return whether we are waiting for an ACK for
         * the association response we sent.
         *
         * \param address the address of the station
         * \return true if the station is associated,
         *          false otherwise
         */
        bool IsWaitAssocTxOk(Mac48Address address) const;
        /**
         * Records that we are waiting for an ACK for
         * the association response we sent.
         *
         * \param address the address of the station
         */
        void RecordWaitAssocTxOk(Mac48Address address);
        /**
         * Records that we got an ACK for
         * the association response we sent.
         *
         * \param address the address of the station
         */
        void RecordGotAssocTxOk(Mac48Address address);
        /**
         * Records that we missed an ACK for
         * the association response we sent.
         *
         * \param address the address of the station
         */
        void RecordGotAssocTxFailed(Mac48Address address);
        /**
         * Records that the STA was disassociated.
         *
         * \param address the address of the station
         */
        void RecordDisassociated(Mac48Address address);

        /**
         * \param address remote address
         * \param packet the packet to queue
         * \param fullPacketSize the size of the packet after its Rescue MAC header has been added.
         *
         * This method is typically invoked just before queuing a packet for transmission.
         * It is a no-op unless the IsLowLatency attribute of the attached ns3::RescueRemoteStationManager
         * is set to false, in which case, the tx parameters of the packet are calculated and stored in
         * the packet as a tag. These tx parameters are later retrieved from GetDataMode.
         */
        void PrepareForQueue(Mac48Address address, Ptr<const Packet> packet, uint32_t fullPacketSize);

        /**
         * \param address remote address
         * \param packet the packet to send
         * \param fullPacketSize the size of the packet after its Rescue MAC header has been added.
         * \return the transmission mode to use to send this packet
         */
        RescueMode GetDataTxMode(Mac48Address address, Ptr<const Packet> packet, uint32_t fullPacketSize);

        /**
         * \param address remote address
         * \param packet the packet to send
         * \param fullPacketSize the size of the packet after its Rescue MAC header has been added.
         * \return the contention window
         */
        uint32_t GetDataCw(Mac48Address address, Ptr<const Packet> packet, uint32_t fullPacketSize);

        /**
         * Should be invoked whenever the AckTimeout associated to a transmission
         * attempt expires.
         *
         * \param address the address of the receiver
         */
        void ReportDataFailed(Mac48Address address);
        /**
         * Should be invoked whenever we receive the Ack associated to a data packet
         * we just sent.
         *
         * \param address the address of the receiver
         * \param ackSnr the SNR of the ACK we received
         * \param ackMode the RescueMode the receiver used to send the ACK
         * \param dataSnr the SNR of the DATA we sent
         */
        void ReportDataOk(Mac48Address address, double ackSnr, RescueMode ackMode, double dataSnr, double dataPer, uint32_t retryCounter);
        /**
         * Should be invoked after calling ReportDataFailed if
         * NeedDataRetransmission returns false
         *
         * \param address the address of the receiver
         */
        void ReportFinalDataFailed(Mac48Address address);

        /**
         * \param senderAddress sender station address
         * \param sourceAddress source station address
         * \param rxSnr the snr of the packet received
         * \param txMode the transmission mode used for the packet received
         * \param wasReconstructed true if the frame was received using joint decoding
         *
         * Should be invoked whenever a packet is successfully received.
         */
        void ReportRxOk(Mac48Address senderAddress, Mac48Address sourceAddress,
                double rxSnr, RescueMode txMode, bool wasReconstructed);
        /**
         * \param senderAddress sender station address
         * \param sourceAddress source station address
         * \param rxSnr the snr of the packet received
         * \param txMode the transmission mode used for the packet received
         * \param wasReconstructed true if the frame was received using joint decoding
         *
         * Should be invoked whenever a packet is successfully received.
         */
        void ReportRxFail(Mac48Address senderAddress, Mac48Address sourceAddress,
                double rxSnr, RescueMode txMode, bool wasReconstructed);

        /**
         * \param address remote address
         * \param packet the packet to send
         * \return true if we want to resend a packet
         *          after a failed transmission attempt, false otherwise.
         */
        //bool NeedDataRetransmission (Mac48Address address, Ptr<const Packet> packet);

        /**
         * \param address
         * \param dataMode the transmission mode used to send an ACK we just received
         * \return the transmission mode to use for the ACK to complete the data/ACK
         *          handshake.
         */
        RescueMode GetAckTxMode(Mac48Address address, RescueMode dataMode);
        /**
         * \param address
         * \return the transmission mode to use for the ACK to complete the data/ACK
         *          handshake.
         */
        RescueMode GetAckTxMode(Mac48Address address);
        /**
         * \param address
         * \return the CW for ACK forwarding
         */
        uint32_t GetAckCw(Mac48Address address);
        /**
         * \param address
         * \return the transmission mode for the CTRL frame transmission.
         */
        RescueMode GetCtrlTxMode(Mac48Address address);

        RescueMode GetCtrlTxMode();

        /**
         * \param address
         * \return the CW for CTRL frame tx
         */
        uint32_t GetCtrlCw(Mac48Address address);
        /**
         * \param address of the remote station
         * \return information regarding the remote station associated with the given address
         */
        RescueRemoteStationInfo GetInfo(Mac48Address address);

        /**
         * Return the retry counter for the given station.
         *
         * \param station the station being queried
         * \return current retry counter value for this station
         */
        uint32_t GetRetryCount(const RescueRemoteStation *station) const;
        /**
         * Return the retry counter for the given station.
         *
         * \param address of the remote station being queried
         * \return current retry counter value for this station
         */
        uint32_t GetRetryCount(const Mac48Address address) const;

    protected:
        virtual void DoDispose(void);
        /**
         * Return whether mode associated with the specified station at the specified index.
         *
         * \param station the station being queried
         * \param i the index
         * \return RescueMode at the given index of the specified station
         */
        RescueMode GetSupported(const RescueRemoteStation *station, uint32_t i) const;
        /**
         * Return the number of modes supported by the given station.
         *
         * \param station the station being queried
         * \return the number of modes supported by the given station
         */
        uint32_t GetNSupported(const RescueRemoteStation *station) const;

    private:
        /**
         * \param station the station that we need to communicate
         * \param packet the packet to send
         * \param normally indicates whether the normal Rescue data retransmission mechanism
         *        would request that the data is retransmitted or not.
         * \return true if we want to resend a packet
         *          after a failed transmission attempt, false otherwise.
         *
         * Note: This method is called after a unicast packet transmission has been attempted
         *       and has failed.
         */
        //virtual bool DoNeedDataRetransmission (RescueRemoteStation *station,
        //                                       Ptr<const Packet> packet, bool normally);

        /**
         * \return whether this manager is a manager designed to work in low-latency
         *          environments.
         *
         * Note: In this context, low vs high latency is defined in <i>IEEE 802.11 Rate Adaptation:
         * A Practical Approach</i>, by M. Lacage, M.H. Manshaei, and T. Turletti.
         */
        virtual bool IsLowLatency(void) const = 0;
        /**
         * \return a new station data structure
         */
        virtual RescueRemoteStation* DoCreateStation(void) const = 0;
        /**
         * \param station the station that we need to communicate
         * \param size size of the packet or fragment we want to send
         * \return the transmission mode to use to send a packet to the station
         *
         * Note: This method is called before sending a unicast packet or a fragment
         *       of a unicast packet to decide which transmission mode to use.
         */
        virtual RescueMode DoGetDataTxMode(RescueRemoteStation *station,
                Ptr<const Packet> packet,
                uint32_t size) = 0;
        /**
         * \param station the station that we need to communicate
         * \param size size of the packet or fragment we want to send
         * \return the transmission mode to use to send a packet to the station
         *
         * Note: This method can be called before starting the backoff for a unicast packet or a fragment
         *       of a unicast packet to set specific contention window size.
         */
        virtual uint32_t DoGetDataCw(RescueRemoteStation *station,
                Ptr<const Packet> packet,
                uint32_t size) = 0;

        virtual uint32_t DoGetAckCw(RescueRemoteStation *station) = 0;
        /**
         * \param station the station that we need to send the acknowledgment
         * \return the transmission mode to use to send a packet to the station
         *
         * Note: This method is called before sending a unicast packet or a fragment
         *       of a unicast packet to decide which transmission mode to use.
         */
        virtual RescueMode DoGetAckTxMode(RescueRemoteStation *station, RescueMode reqMode) = 0;
        virtual RescueMode DoGetAckTxMode_(RescueRemoteStation *station) = 0;

        virtual RescueMode DoGetCtrlTxMode() = 0;

        /**
         * This method is a pure virtual method that must be implemented by the sub-class.
         * This allows different types of RescueRemoteStationManager to respond differently,
         *
         * \param station the station that we failed to send DATA
         */
        virtual void DoReportDataFailed(RescueRemoteStation *station) = 0;
        /**
         * This method is a pure virtual method that must be implemented by the sub-class.
         * This allows different types of RescueRemoteStationManager to respond differently,
         *
         * \param station the station that we successfully sent DATA
         * \param ackSnr the SNR of the ACK we received
         * \param ackMode the RescueMode the receiver used to send the ACK
         * \param dataSnr the SNR of the DATA we sent
         */
        virtual void DoReportDataOk(RescueRemoteStation *station,
                double ackSnr, RescueMode ackMode,
                double dataSnr, double dataPer) = 0;
        /**
         * This method is a pure virtual method that must be implemented by the sub-class.
         * This allows different types of RescueRemoteStationManager to respond differently,
         *
         * \param station the station that we failed to send DATA
         */
        virtual void DoReportFinalDataFailed(RescueRemoteStation *station) = 0;
        /**
         * This method is a pure virtual method that must be implemented by the sub-class.
         * This allows different types of RescueRemoteStationManager to respond differently,
         *
         * \param senderStation the station that sent the DATA
         * \param sourceStation the station that originated the DATA
         * \param rxSnr the SNR of the DATA we received
         * \param txMode the RescueMode the sender used to send the DATA
         * \param wasReconstructed true if the frame was received using joint decoding
         */
        virtual void DoReportRxOk(RescueRemoteStation *senderStation,
                RescueRemoteStation *sourceStation,
                double rxSnr, RescueMode txMode, bool wasReconstructed) = 0;
        /**
         * This method is a pure virtual method that must be implemented by the sub-class.
         * This allows different types of RescueRemoteStationManager to respond differently,
         *
         * \param senderStation the station that sent the DATA
         * \param sourceStation the station that originated the DATA
         * \param rxSnr the SNR of the DATA we received
         * \param txMode the RescueMode the sender used to send the DATA
         * \param wasReconstructed true if the frame was received using joint decoding
         */
        virtual void DoReportRxFail(RescueRemoteStation *senderStation,
                RescueRemoteStation *sourceStation,
                double rxSnr, RescueMode txMode, bool wasReconstructed) = 0;

        /**
         * Return the state of the station associated with the given address.
         *
         * \param address the address of the station
         * \return RescueRemoteStationState corresponding to the address
         */
        RescueRemoteStationState* LookupState(Mac48Address address) const;
        /**
         * Return the station associated with the given address.
         *
         * \param address the address of the station
         * \return RescueRemoteStation corresponding to the address
         */
        RescueRemoteStation* Lookup(Mac48Address address) const;

        RescueMode GetControlAnswerMode(Mac48Address address, RescueMode reqMode);
        RescueMode GetControlAnswerMode(Mac48Address address);

        uint32_t GetControlCw(Mac48Address address);

        /**
         * A vector of RescueRemoteStations
         */
        typedef std::vector <RescueRemoteStation *> Stations;
        /**
         * A vector of RescueRemoteStationStates
         */
        typedef std::vector <RescueRemoteStationState *> StationStates;

        StationStates m_states; //!< States of known stations
        Stations m_stations; //!< Information for each known stations
        /**
         * This is a pointer to the RescuePhy associated with this
         * RescueRemoteStationManager that is set on call to
         * RescueRemoteStationManager::SetupPhy(). Through this pointer the
         * station manager can determine PHY characteristics, such as the
         * set of all transmission rates that may be supported (the
         * "DeviceRateSet").
         */
        Ptr<RescuePhy> m_phy;
        Ptr<RescueMac> m_mac; //!< Pointer to associated RescueMac
        RescueMode m_defaultTxMode; //!< The default transmission mode

        /**
         * This member is the list of RescueMode objects that comprise the
         * BSSBasicRateSet parameter. This list is constructed through calls
         * to RescueRemoteStationManager::AddBasicMode(), and an API that
         * allows external access to it is available through
         * RescueRemoteStationManager::GetNBasicModes() and
         * RescueRemoteStationManager::GetBasicMode().
         */
        RescueModeList m_bssBasicRateSet;

        //uint32_t m_maxSrc;    //!< Maximum STA retry count (SRC)
        RescueMode m_nonUnicastMode; //!< Transmission mode for non-unicast DATA frames

        uint16_t m_cwMin; //!< Minimal value of contention window
        uint16_t m_cwMax; //!< Maximal value of contention window
        uint16_t m_cwNonUnicast; //!< Contention window value for non unicast transmissions

        /**
         * The trace source fired when the transmission of a single data packet has failed
         */
        //TracedCallback<Mac48Address> m_macTxDataFailed;
        /**
         * The trace source fired when the transmission of a data packet has
         * exceeded the maximum number of attempts
         */
        //TracedCallback<Mac48Address> m_macTxFinalDataFailed;

    };

    /**
     * A struct that holds information about each remote station.
     */
    struct RescueRemoteStationState {

        /**
         * State of the station
         */
        enum {
            BRAND_NEW,
            DISASSOC,
            WAIT_ASSOC_TX_OK,
            GOT_ASSOC_TX_OK
        } m_state;

        /**
         * This member is the list of RescueMode objects that comprise the
         * OperationalRateSet parameter for this remote station. This list
         * is constructed through calls to
         * RescueRemoteStationManager::AddSupportedMode(), and an API that
         * allows external access to it is available through
         * RescueRemoteStationManager::GetNSupported() and
         * RescueRemoteStationManager::GetSupported().
         */
        RescueModeList m_operationalRateSet;
        Mac48Address m_address; //!< Mac48Address of the remote station
        RescueRemoteStationInfo m_info;

    };

    /**
     * \brief hold per-remote-station state.
     *
     * The state in this class is used to keep track
     * of association status if we are in an infrastructure
     * network and to perform the selection of tx parameters
     * on a per-packet basis.
     */
    struct RescueRemoteStation {
        RescueRemoteStationState *m_state; //!< Remote station state
        //uint32_t m_src; //!< STA short retry count
    };


} // namespace ns3

#endif /* RESCUE_REMOTE_STATION_MANAGER_H */

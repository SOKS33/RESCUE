/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
 * Copyright (c) 2015 AGH University of Science nad Technology
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
 *
 * Adapted for Rescue by: Lukasz Prasnal <prasnal@kt.agh.edu.pl>
 */

#include "rescue-remote-station-manager.h"
#include "ns3/simulator.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/tag.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "rescue-phy.h"
#include "rescue-mac.h"
#include "rescue-mac-header.h"

NS_LOG_COMPONENT_DEFINE("RescueRemoteStationManager");


/***************************************************************
 *           Packet Mode Tagger
 ***************************************************************/

namespace ns3 {

    class HighLatencyDataTxModeTag : public Tag {
    public:
        HighLatencyDataTxModeTag();
        HighLatencyDataTxModeTag(RescueMode dataTxMode);
        HighLatencyDataTxModeTag(uint32_t dataCw);
        HighLatencyDataTxModeTag(RescueMode dataTxMode, uint32_t dataCw);
        /**
         * \returns the transmission mode to use to send this packet
         */
        RescueMode GetDataTxMode(void) const;
        uint32_t GetDataCw(void) const;

        static TypeId GetTypeId(void);
        virtual TypeId GetInstanceTypeId(void) const;
        virtual uint32_t GetSerializedSize(void) const;
        virtual void Serialize(TagBuffer i) const;
        virtual void Deserialize(TagBuffer i);
        virtual void Print(std::ostream &os) const;
    private:
        RescueMode m_dataTxMode;
        uint16_t m_dataCw;
    };

    HighLatencyDataTxModeTag::HighLatencyDataTxModeTag() {
    }

    HighLatencyDataTxModeTag::HighLatencyDataTxModeTag(RescueMode dataTxMode)
    : m_dataTxMode(dataTxMode) {
    }

    HighLatencyDataTxModeTag::HighLatencyDataTxModeTag(uint32_t dataCw)
    : m_dataCw(dataCw) {
    }

    HighLatencyDataTxModeTag::HighLatencyDataTxModeTag(RescueMode dataTxMode, uint32_t dataCw)
    : m_dataTxMode(dataTxMode),
    m_dataCw(dataCw) {
    }

    RescueMode
    HighLatencyDataTxModeTag::GetDataTxMode(void) const {
        return m_dataTxMode;
    }

    uint32_t
    HighLatencyDataTxModeTag::GetDataCw(void) const {
        return m_dataCw;
    }

    TypeId
    HighLatencyDataTxModeTag::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::HighLatencyDataTxModeTag")
                .SetParent<Tag> ()
                .AddConstructor<HighLatencyDataTxModeTag> ()
                ;
        return tid;
    }

    TypeId
    HighLatencyDataTxModeTag::GetInstanceTypeId(void) const {
        return GetTypeId();
    }

    uint32_t
    HighLatencyDataTxModeTag::GetSerializedSize(void) const {
        return sizeof (RescueMode);
    }

    void
    HighLatencyDataTxModeTag::Serialize(TagBuffer i) const {
        i.Write((uint8_t *) & m_dataTxMode, sizeof (RescueMode));
        i.WriteU16(m_dataCw);
    }

    void
    HighLatencyDataTxModeTag::Deserialize(TagBuffer i) {
        i.Read((uint8_t *) & m_dataTxMode, sizeof (RescueMode));
        m_dataCw = i.ReadU16();
    }

    void
    HighLatencyDataTxModeTag::Print(std::ostream &os) const {
        os << "Data=" << m_dataTxMode << " cw=" << m_dataCw;
    }

} // namespace ns3

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now().GetSeconds() << "] [addr=" << ((m_mac != 0) ? compressMac(m_mac->GetAddress ()) : 0) << "] [REM STA] "

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(RescueRemoteStationManager);

    TypeId
    RescueRemoteStationManager::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::RescueRemoteStationManager")
                .SetParent<Object> ()
                .AddAttribute("IsLowLatency", "If true, we attempt to modelize a so-called low-latency device: a device"
                " where decisions about tx parameters can be made on a per-packet basis and feedback about the"
                " transmission of each packet is obtained before sending the next. Otherwise, we modelize a "
                " high-latency device, that is a device where we cannot update our decision about tx parameters"
                " after every packet transmission.",
                BooleanValue(true), // this value is ignored because there is no setter
                MakeBooleanAccessor(&RescueRemoteStationManager::IsLowLatency),
                MakeBooleanChecker())
                .AddAttribute("MaxSrc", "The maximum number of retransmission attempts for a DATA packet. This value"
                " will not have any effect on some rate control algorithms.",
                UintegerValue(7),
                MakeUintegerAccessor(&RescueRemoteStationManager::m_maxSrc),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("NonUnicastMode", "Rescue mode used for non-unicast transmissions.",
                RescueModeValue(),
                MakeRescueModeAccessor(&RescueRemoteStationManager::m_nonUnicastMode),
                MakeRescueModeChecker())
                .AddTraceSource("MacTxDataFailed",
                "The transmission of a data packet by the MAC layer has failed",
                MakeTraceSourceAccessor(&RescueRemoteStationManager::m_macTxDataFailed))
                .AddTraceSource("MacTxFinalDataFailed",
                "The transmission of a data packet has exceeded the maximum number of attempts",
                MakeTraceSourceAccessor(&RescueRemoteStationManager::m_macTxFinalDataFailed))
                ;
        return tid;
    }

    RescueRemoteStationManager::RescueRemoteStationManager() {
    }

    RescueRemoteStationManager::~RescueRemoteStationManager() {
    }

    void
    RescueRemoteStationManager::DoDispose(void) {
        for (StationStates::const_iterator i = m_states.begin(); i != m_states.end(); i++) {
            delete (*i);
        }
        m_states.clear();
        for (Stations::const_iterator i = m_stations.begin(); i != m_stations.end(); i++) {
            delete (*i);
        }
        m_stations.clear();
    }

    void
    RescueRemoteStationManager::SetupPhy(Ptr<RescuePhy> phy) {
        // We need to track our PHY because it is the object that knows the
        // full set of transmit rates that are supported. We need to know
        // this in order to find the relevant rates when chosing a
        // transmit rate for automatic control responses like
        // acknowledgements.
        m_phy = phy;
        m_defaultTxMode = phy->GetMode(0);
        Reset();
    }

    void
    RescueRemoteStationManager::SetupMac(Ptr<RescueMac> mac) {
        m_mac = mac;
    }

    void
    RescueRemoteStationManager::SetCwMin(uint32_t cw) {
        m_cwMin = cw;
    }

    void
    RescueRemoteStationManager::SetCwMax(uint32_t cw) {
        m_cwMax = cw;
    }

    uint32_t
    RescueRemoteStationManager::GetCwMin(void) {
        return m_cwMin;
    }

    uint32_t
    RescueRemoteStationManager::GetCwMax(void) {
        return m_cwMax;
    }

    uint32_t
    RescueRemoteStationManager::GetMaxSrc(void) const {
        return m_maxSrc;
    }

    void
    RescueRemoteStationManager::SetMaxSrc(uint32_t maxSrc) {
        m_maxSrc = maxSrc;
    }

    Mac48Address
    RescueRemoteStationManager::GetAddress() const {
        return m_mac->GetAddress();
    }

    void
    RescueRemoteStationManager::Reset(Mac48Address address) {
        NS_ASSERT(!address.IsGroup());
        RescueRemoteStationState *state = LookupState(address);
        state->m_operationalRateSet.clear();
        AddSupportedMode(address, GetDefaultMode());
    }

    void
    RescueRemoteStationManager::AddSupportedMode(Mac48Address address, RescueMode mode) {
        NS_ASSERT(!address.IsGroup());
        RescueRemoteStationState *state = LookupState(address);
        for (RescueModeListIterator i = state->m_operationalRateSet.begin(); i != state->m_operationalRateSet.end(); i++) {
            if ((*i) == mode) {
                // already in.
                return;
            }
        }
        state->m_operationalRateSet.push_back(mode);
    }

    bool
    RescueRemoteStationManager::IsBrandNew(Mac48Address address) const {
        if (address.IsGroup()) {
            return false;
        }
        return LookupState(address)->m_state == RescueRemoteStationState::BRAND_NEW;
    }

    bool
    RescueRemoteStationManager::IsAssociated(Mac48Address address) const {
        if (address.IsGroup()) {
            return true;
        }
        return LookupState(address)->m_state == RescueRemoteStationState::GOT_ASSOC_TX_OK;
    }

    bool
    RescueRemoteStationManager::IsWaitAssocTxOk(Mac48Address address) const {
        if (address.IsGroup()) {
            return false;
        }
        return LookupState(address)->m_state == RescueRemoteStationState::WAIT_ASSOC_TX_OK;
    }

    void
    RescueRemoteStationManager::RecordWaitAssocTxOk(Mac48Address address) {
        NS_ASSERT(!address.IsGroup());
        LookupState(address)->m_state = RescueRemoteStationState::WAIT_ASSOC_TX_OK;
    }

    void
    RescueRemoteStationManager::RecordGotAssocTxOk(Mac48Address address) {
        NS_ASSERT(!address.IsGroup());
        LookupState(address)->m_state = RescueRemoteStationState::GOT_ASSOC_TX_OK;
    }

    void
    RescueRemoteStationManager::RecordGotAssocTxFailed(Mac48Address address) {
        NS_ASSERT(!address.IsGroup());
        LookupState(address)->m_state = RescueRemoteStationState::DISASSOC;
    }

    void
    RescueRemoteStationManager::RecordDisassociated(Mac48Address address) {
        NS_ASSERT(!address.IsGroup());
        LookupState(address)->m_state = RescueRemoteStationState::DISASSOC;
    }

    void
    RescueRemoteStationManager::PrepareForQueue(Mac48Address address, Ptr<const Packet> packet, uint32_t fullPacketSize) {
        if (IsLowLatency() || address.IsGroup()) {
            return;
        }
        RescueRemoteStation *station = Lookup(address);
        RescueMode data = DoGetDataTxMode(station, packet, fullPacketSize);
        HighLatencyDataTxModeTag datatag;
        // first, make sure that the tag is not here anymore.
        ConstCast<Packet> (packet)->RemovePacketTag(datatag);
        datatag = HighLatencyDataTxModeTag(data);
        // and then, add it back
        packet->AddPacketTag(datatag);
    }

    RescueMode
    RescueRemoteStationManager::GetDataTxMode(Mac48Address address, Ptr<const Packet> packet, uint32_t fullPacketSize) {
        if (address.IsGroup()) {
            NS_LOG_DEBUG("Group adress " << address);
            return GetNonUnicastMode();
        }
        if (!IsLowLatency()) {
            HighLatencyDataTxModeTag datatag;
            bool found;
            found = ConstCast<Packet> (packet)->PeekPacketTag(datatag);
            NS_ASSERT(found);
            // cast found to void, to suppress 'found' set but not used
            // compiler warning
            (void) found;
            return datatag.GetDataTxMode();
        }
        return DoGetDataTxMode(Lookup(address), packet, fullPacketSize);
    }

    uint32_t
    RescueRemoteStationManager::GetDataCw(Mac48Address address, Ptr<const Packet> packet, uint32_t fullPacketSize) {
        if (address.IsGroup()) {
            NS_LOG_DEBUG("Group adress " << address);
            return GetNonUnicastCw();
        }
        if (!IsLowLatency()) {
            HighLatencyDataTxModeTag datatag;
            bool found;
            found = ConstCast<Packet> (packet)->PeekPacketTag(datatag);
            NS_ASSERT(found);
            // cast found to void, to suppress 'found' set but not used
            // compiler warning
            (void) found;
            return datatag.GetDataCw();
        }
        return DoGetDataCw(Lookup(address), packet, fullPacketSize);
    }

    void
    RescueRemoteStationManager::ReportDataFailed(Mac48Address address) {
        NS_ASSERT(!address.IsGroup());
        RescueRemoteStation *station = Lookup(address);
        station->m_src++;
        m_macTxDataFailed(address);
        DoReportDataFailed(station);
    }

    void
    RescueRemoteStationManager::ReportDataOk(Mac48Address address, double ackSnr, RescueMode ackMode, double dataSnr, double dataPer) {
        NS_ASSERT(!address.IsGroup());
        RescueRemoteStation *station = Lookup(address);
        station->m_state->m_info.NotifyTxSuccess(station->m_src);
        station->m_src = 0;
        DoReportDataOk(station, ackSnr, ackMode, dataSnr, dataPer);
    }

    void
    RescueRemoteStationManager::ReportFinalDataFailed(Mac48Address address) {
        NS_ASSERT(!address.IsGroup());
        RescueRemoteStation *station = Lookup(address);
        station->m_state->m_info.NotifyTxFailed();
        station->m_src = 0;
        m_macTxFinalDataFailed(address);
        DoReportFinalDataFailed(station);
    }

    void
    RescueRemoteStationManager::ReportRxOk(Mac48Address senderAddress, Mac48Address sourceAddress,
            double rxSnr, RescueMode txMode, bool wasReconstructed) {
        /*if (address.IsGroup ())
          {
            return;
          }*/
        RescueRemoteStation *senderStation = Lookup(senderAddress);
        RescueRemoteStation *sourceStation = Lookup(sourceAddress);
        DoReportRxOk(senderStation, sourceStation,
                rxSnr, txMode, wasReconstructed);
    }

    void
    RescueRemoteStationManager::ReportRxFail(Mac48Address senderAddress, Mac48Address sourceAddress,
            double rxSnr, RescueMode txMode, bool wasReconstructed) {
        /*if (address.IsGroup ())
          {
            return;
          }*/
        RescueRemoteStation *senderStation = Lookup(senderAddress);
        RescueRemoteStation *sourceStation = Lookup(sourceAddress);
        DoReportRxFail(senderStation, sourceStation,
                rxSnr, txMode, wasReconstructed);
    }

    bool
    RescueRemoteStationManager::NeedDataRetransmission(Mac48Address address, Ptr<const Packet> packet) {
        NS_ASSERT(!address.IsGroup());
        RescueRemoteStation *station = Lookup(address);
        bool normally = station->m_src < GetMaxSrc();
        return DoNeedDataRetransmission(station, packet, normally);
    }

    RescueMode
    RescueRemoteStationManager::GetControlAnswerMode(Mac48Address address, RescueMode reqMode) {
        NS_LOG_FUNCTION(this << address);

        if (&RescueRemoteStationManager::DoGetAckTxMode != 0) {
            RescueRemoteStation *station = Lookup(address);
            return DoGetAckTxMode(station, reqMode);
        }

        /**
         * The standard has relatively unambiguous rules for selecting a
         * control response rate (the below is quoted from IEEE 802.11-2012,
         * Section 9.7):
         *
         *   To allow the transmitting STA to calculate the contents of the
         *   Duration/ID field, a STA responding to a received frame shall
         *   transmit its Control Response frame (either CTS or ACK), other
         *   than the BlockAck control frame, at the highest rate in the
         *   BSSBasicRateSet parameter that is less than or equal to the
         *   rate of the immediately previous frame in the frame exchange
         *   sequence (as defined in Annex G) and that is of the same
         *   modulation class (see Section 9.7.8) as the received frame...
         */
        RescueMode mode = GetDefaultMode();
        bool found = false;

        // First, search the BSS Basic Rate set
        for (RescueModeListIterator i = m_bssBasicRateSet.begin();
                i != m_bssBasicRateSet.end(); i++) {
            if ((!found || i->GetPhyRate() > mode.GetPhyRate())
                    && i->GetPhyRate() <= reqMode.GetPhyRate()
                    && i->GetModulationClass() == reqMode.GetModulationClass()) {
                mode = *i;
                // We've found a potentially-suitable transmit rate, but we
                // need to continue and consider all the basic rates before
                // we can be sure we've got the right one.
                found = true;
            }
        }

        // If we found a suitable rate in the BSSBasicRateSet, then we are
        // done and can return that mode.
        if (found) {
            return mode;
        }

        /**
         * If no suitable basic rate was found, we search the mandatory
         * rates. The standard (IEEE 802.11-2007, Section 9.6) says:
         *
         *   ...If no rate contained in the BSSBasicRateSet parameter meets
         *   these conditions, then the control frame sent in response to a
         *   received frame shall be transmitted at the highest mandatory
         *   rate of the PHY that is less than or equal to the rate of the
         *   received frame, and that is of the same modulation class as the
         *   received frame. In addition, the Control Response frame shall
         *   be sent using the same PHY options as the received frame,
         *   unless they conflict with the requirement to use the
         *   BSSBasicRateSet parameter.
         *
         * \todo Note that we're ignoring the last sentence for now, because
         * there is not yet any manipulation here of PHY options.
         */
        for (uint32_t idx = 0; idx < m_phy->GetNModes(); idx++) {
            RescueMode thismode = m_phy->GetMode(idx);

            /* If the rate:
             *
             *  - is equal to or faster than our current best choice, and
             *  - is less than or equal to the rate of the received frame, and
             *  - is of the same modulation class as the received frame
             *
             * ...then it's our best choice so far.
             */
            if ((!found || thismode.GetPhyRate() > mode.GetPhyRate())
                    && thismode.GetPhyRate() <= reqMode.GetPhyRate()
                    && thismode.GetModulationClass() == reqMode.GetModulationClass()) {
                mode = thismode;
                // As above; we've found a potentially-suitable transmit
                // rate, but we need to continue and consider all the
                // rates before we can be sure we've got the right one.
                found = true;
            }
        }

        /**
         * If we still haven't found a suitable rate for the response then
         * someone has messed up the simulation config. This probably means
         * that the RescuePhyStandard is not set correctly, or that a rate that
         * is not supported by the PHY has been explicitly requested in a
         * RescueRemoteStationManager (or descendant) configuration.
         *
         * Either way, it is serious - we can either disobey the standard or
         * fail, and I have chosen to do the latter...
         */
        if (!found) {
            NS_FATAL_ERROR("Can't find response rate for " << reqMode
                    << ". Check standard and selected rates match.");
        }

        return mode;
    }

    RescueMode
    RescueRemoteStationManager::GetControlAnswerMode(Mac48Address address) {
        NS_LOG_FUNCTION(this << address);

        if (&RescueRemoteStationManager::DoGetAckTxMode_ != 0) {
            RescueRemoteStation *station = Lookup(address);
            return DoGetAckTxMode_(station);
        }

        return GetDefaultMode();
    }

    uint32_t
    RescueRemoteStationManager::GetControlCw(Mac48Address address) {
        return GetCwMin();
    }

    RescueMode
    RescueRemoteStationManager::GetAckTxMode(Mac48Address address, RescueMode dataMode) {
        NS_ASSERT(!address.IsGroup());
        return GetControlAnswerMode(address, dataMode);
    }

    RescueMode
    RescueRemoteStationManager::GetAckTxMode(Mac48Address address) {
        NS_ASSERT(!address.IsGroup());
        return GetControlAnswerMode(address);
    }

    uint32_t
    RescueRemoteStationManager::GetAckCw(Mac48Address address) {
        NS_ASSERT(!address.IsGroup());
        return GetControlCw(address);
    }

    RescueMode
    RescueRemoteStationManager::GetCtrlTxMode(Mac48Address address) {
        //NS_ASSERT (!address.IsGroup ());
        return GetControlAnswerMode(address);
    }

    uint32_t
    RescueRemoteStationManager::GetCtrlCw(Mac48Address address) {
        //NS_ASSERT (!address.IsGroup ());
        return GetControlCw(address);
    }

    RescueRemoteStationInfo
    RescueRemoteStationManager::GetInfo(Mac48Address address) {
        RescueRemoteStationState *state = LookupState(address);
        return state->m_info;
    }

    RescueRemoteStationState *
    RescueRemoteStationManager::LookupState(Mac48Address address) const {
        for (StationStates::const_iterator i = m_states.begin(); i != m_states.end(); i++) {
            if ((*i)->m_address == address) {
                return (*i);
            }
        }
        RescueRemoteStationState *state = new RescueRemoteStationState();
        state->m_state = RescueRemoteStationState::BRAND_NEW;
        state->m_address = address;
        state->m_operationalRateSet.push_back(GetDefaultMode());
        const_cast<RescueRemoteStationManager *> (this)->m_states.push_back(state);
        return state;
    }

    RescueRemoteStation *
    RescueRemoteStationManager::Lookup(Mac48Address address) const {
        for (Stations::const_iterator i = m_stations.begin(); i != m_stations.end(); i++) {
            if ((*i)->m_state->m_address == address) {
                return (*i);
            }
        }
        RescueRemoteStationState *state = LookupState(address);

        RescueRemoteStation *station = DoCreateStation();
        station->m_state = state;
        station->m_src = 0;
        // XXX
        const_cast<RescueRemoteStationManager *> (this)->m_stations.push_back(station);
        return station;
    }

    RescueMode
    RescueRemoteStationManager::GetDefaultMode(void) const {
        return m_defaultTxMode;
    }

    void
    RescueRemoteStationManager::Reset(void) {
        for (Stations::const_iterator i = m_stations.begin(); i != m_stations.end(); i++) {
            delete (*i);
        }
        m_stations.clear();
        m_bssBasicRateSet.clear();
        m_bssBasicRateSet.push_back(m_defaultTxMode);
    }

    void
    RescueRemoteStationManager::AddBasicMode(RescueMode mode) {
        for (uint32_t i = 0; i < GetNBasicModes(); i++) {
            if (GetBasicMode(i) == mode) {
                return;
            }
        }
        m_bssBasicRateSet.push_back(mode);
    }

    uint32_t
    RescueRemoteStationManager::GetNBasicModes(void) const {
        return m_bssBasicRateSet.size();
    }

    RescueMode
    RescueRemoteStationManager::GetBasicMode(uint32_t i) const {
        NS_LOG_UNCOND(" i=" << i << " bsssize=" << m_bssBasicRateSet.size());
        NS_ASSERT(i < m_bssBasicRateSet.size());
        return m_bssBasicRateSet[i];
    }

    RescueMode
    RescueRemoteStationManager::GetNonUnicastMode(void) const {
        if (m_nonUnicastMode == RescueMode()) {
            return GetBasicMode(0);
        } else {
            return m_nonUnicastMode;
        }
    }

    uint32_t
    RescueRemoteStationManager::GetNonUnicastCw(void) const {
        if (m_cwNonUnicast == 0) {
            return m_cwMin;
        } else {
            return m_cwNonUnicast;
        }
    }

    bool
    RescueRemoteStationManager::DoNeedDataRetransmission(RescueRemoteStation *station,
            Ptr<const Packet> packet, bool normally) {
        return normally;
    }

    RescueMode
    RescueRemoteStationManager::GetSupported(const RescueRemoteStation *station, uint32_t i) const {
        NS_ASSERT(i < GetNSupported(station));
        return station->m_state->m_operationalRateSet[i];
    }

    uint32_t
    RescueRemoteStationManager::GetRetryCount(const RescueRemoteStation *station) const {
        return station->m_src;
    }

    uint32_t
    RescueRemoteStationManager::GetRetryCount(const Mac48Address address) const {
        RescueRemoteStation *station = Lookup(address);
        return station->m_src;
    }

    uint32_t
    RescueRemoteStationManager::GetNSupported(const RescueRemoteStation *station) const {
        return station->m_state->m_operationalRateSet.size();
    }

    //RescueRemoteStationInfo constructor

    RescueRemoteStationInfo::RescueRemoteStationInfo()
    : m_memoryTime(Seconds(1.0)),
    m_lastUpdate(Seconds(0.0)),
    m_failAvg(0.0) {
    }

    double
    RescueRemoteStationInfo::CalculateAveragingCoefficient() {
        double retval = std::exp((double) (m_lastUpdate.GetMicroSeconds() - Simulator::Now().GetMicroSeconds())
                / (double) m_memoryTime.GetMicroSeconds());
        m_lastUpdate = Simulator::Now();
        return retval;
    }

    void
    RescueRemoteStationInfo::NotifyTxSuccess(uint32_t retryCounter) {
        double coefficient = CalculateAveragingCoefficient();
        m_failAvg = (double) retryCounter / (1 + (double) retryCounter) * (1.0 - coefficient) + coefficient * m_failAvg;
    }

    void
    RescueRemoteStationInfo::NotifyTxSuccessSNR(uint32_t retryCounter, double snr) {
        double coefficient = CalculateAveragingCoefficient();
        m_failAvg = (double) retryCounter / (1 + (double) retryCounter) * (1.0 - coefficient) + coefficient * m_failAvg;

        m_snrAvg = snr / (1 + snr) * (1.0 - coefficient) + coefficient * m_snrAvg; //?
    }

    void
    RescueRemoteStationInfo::NotifyTxFailed() {
        double coefficient = CalculateAveragingCoefficient();
        m_failAvg = (1.0 - coefficient) + coefficient * m_failAvg;
    }

    double
    RescueRemoteStationInfo::GetFrameErrorRate() const {
        return m_failAvg;
    }
} // namespace ns3

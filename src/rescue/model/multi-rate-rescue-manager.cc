/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2004,2005 INRIA
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

#include "multi-rate-rescue-manager.h"

#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/assert.h"
#include "ns3/log.h"

#include "rescue-mac-header.h"

NS_LOG_COMPONENT_DEFINE("MultiRateRescueManager");

#define Min(a,b) ((a < b) ? a : b)






namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(MultiRateRescueManager);

    TypeId
    MultiRateRescueManager::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::MultiRateRescueManager")
                .SetParent<RescueRemoteStationManager> ()
                .AddConstructor<MultiRateRescueManager> ()

                /*
                    .AddAttribute ("DataMode", "The transmission mode to use for every data packet transmission",
                                   StringValue ("OfdmRate6Mbps"),
                                   MakeRescueModeAccessor (&MultiRateRescueManager::m_dataMode),
                                   MakeRescueModeChecker ())
                    .AddAttribute ("ControlMode", "The transmission mode to use for every control packet transmission.",
                                   StringValue ("OfdmRate6Mbps"),
                                   MakeRescueModeAccessor (&MultiRateRescueManager::m_ctlMode),
                                   MakeRescueModeChecker ())
                 */
                .AddAttribute("DataCw", "The contention window to use for every data packet transmission",
                UintegerValue(15),
                MakeUintegerAccessor(&MultiRateRescueManager::m_dataCw),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("ControlCw", "The contention window to use for every control packet transmission.",
                UintegerValue(8),
                MakeUintegerAccessor(&MultiRateRescueManager::m_ctlCw),
                MakeUintegerChecker<uint32_t> ())
                ;
        return tid;
    }

    MultiRateRescueManager::MultiRateRescueManager() {
        m_dataModeHi = RescueModeFactory::CreateRescueMode("Ofdm36Mbps", RESCUE_MOD_CLASS_OFDM, 20000000, 36000000, RESCUE_CODE_RATE_3_4, 16);
        m_dataModeLo = RescueModeFactory::CreateRescueMode("Ofdm12Mbps", RESCUE_MOD_CLASS_OFDM, 20000000, 12000000, RESCUE_CODE_RATE_1_2, 4);
        m_ctlMode = RescueModeFactory::CreateRescueMode("Ofdm12Mbps", RESCUE_MOD_CLASS_OFDM, 20000000, 12000000, RESCUE_CODE_RATE_1_2, 4);
        NS_LOG_FUNCTION(this);
    }

    MultiRateRescueManager::~MultiRateRescueManager() {
        NS_LOG_FUNCTION(this);
    }

    RescueRemoteStation *
    MultiRateRescueManager::DoCreateStation(void) const {
        NS_LOG_FUNCTION(this);
        RescueRemoteStation *station = new RescueRemoteStation();
        return station;
    }

    void
    MultiRateRescueManager::DoReportRxOk(RescueRemoteStation *senderStation,
            RescueRemoteStation *sourceStation,
            double rxSnr, RescueMode txMode, bool wasReconstructed) {
        NS_LOG_FUNCTION(this << senderStation << sourceStation << rxSnr << txMode << ((wasReconstructed) ? "joint decoder was used" : ""));
    }

    void
    MultiRateRescueManager::DoReportRxFail(RescueRemoteStation *senderStation,
            RescueRemoteStation *sourceStation,
            double rxSnr, RescueMode txMode, bool wasReconstructed) {
        NS_LOG_FUNCTION(this << senderStation << sourceStation << rxSnr << txMode << ((wasReconstructed) ? "joint decoder was used" : ""));
    }

    void
    MultiRateRescueManager::DoReportDataFailed(RescueRemoteStation *station) {
        NS_LOG_FUNCTION(this << station);
    }

    void
    MultiRateRescueManager::DoReportDataOk(RescueRemoteStation *st,
            double ackSnr, RescueMode ackMode,
            double dataSnr, double dataPer) {
        NS_LOG_FUNCTION(this << st << ackSnr << ackMode << dataSnr);
    }

    void
    MultiRateRescueManager::DoReportFinalDataFailed(RescueRemoteStation *station) {
        NS_LOG_FUNCTION(this << station);
    }

    RescueMode
    MultiRateRescueManager::DoGetDataTxMode(RescueRemoteStation *st, Ptr<const Packet> packet, uint32_t size) {
        NS_LOG_FUNCTION(GetAddress() << "get data tx mode" << st->m_state->m_address);
        return m_dataModeLo;
    }

    uint32_t
    MultiRateRescueManager::DoGetDataCw(RescueRemoteStation *st, Ptr<const Packet> packet, uint32_t size) {
        RescueMacHeader hdr;
        packet->PeekHeader(hdr);
        double snrSR = 20, snrRD = 20, snrAll; //zakladamy ze jak nie mamy zadnych danych to trzeba je probowac zdobyc czyli normalny priorytet
        double newCW = 0;
        relayMap::iterator iRel = m_relays.find(st->m_state->m_address);


        if (hdr.GetSource() != GetAddress()) {//nasza rola to relay tylko tutaj przeliczamy CW
            relayMap::iterator iRel = m_relays.find(st->m_state->m_address);
            if (iRel != m_relays.end()) {
                if (iRel->second.count(st->m_state->m_address)) snrRD = iRel->second[st->m_state->m_address];
            }

            iRel = m_relays.find(hdr.GetSource());
            if (iRel != m_relays.end()) {
                if (iRel->second.count(hdr.GetSource())) snrSR = iRel->second[hdr.GetSource()];
            }

            //dokonjemy przeliczenia z dB na wartosc liniowa
            snrSR = pow(10.0, -0.1 * snrSR);
            snrRD = pow(10.0, -0.1 * snrRD);
            snrAll = 1.0 / (snrSR + snrRD);
            newCW = ceil(36.0e6 * (m_dataCw + 1) / (2.0e7 * log2(1.0 + 0.29 * snrAll))); //dobrane tak aby dla snr=9.31 lacze osiagalo 36e6mbps
            newCW = newCW <= m_dataCw ? m_dataCw : newCW;
            NS_LOG_FUNCTION("calculated new CW at " << GetAddress() << ":" << newCW);


        }


        // NS_LOG_FUNCTION (this << st << size);
        return m_dataCw;
    }

    RescueMode
    MultiRateRescueManager::DoGetAckTxMode(RescueRemoteStation *st, RescueMode reqMode) {
        NS_LOG_FUNCTION(this << st);
        return m_ctlMode;
    }

    RescueMode
    MultiRateRescueManager::DoGetAckTxMode_(RescueRemoteStation *st) {
        NS_LOG_FUNCTION(this << st);
        return m_ctlMode;
    }

    uint32_t
    MultiRateRescueManager::DoGetAckCw(RescueRemoteStation *st) {
        NS_LOG_FUNCTION(this << st);
        return m_ctlCw;
    }

    RescueMode
    MultiRateRescueManager::DoGetCtrlTxMode() {
        NS_LOG_FUNCTION(this);
        return m_ctlMode;
    }

    bool
    MultiRateRescueManager::IsLowLatency(void) const {
        NS_LOG_FUNCTION(this);
        return true;
    }

} // namespace ns3

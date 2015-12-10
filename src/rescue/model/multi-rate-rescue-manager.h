/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
#ifndef MULTI_RATE_RESCUE_MANAGER_H
#define MULTI_RATE_RESCUE_MANAGER_H

#include <stdint.h>

#include "rescue-remote-station-manager.h"



//-------------------------------sikor
#include <string>
#include <iostream>
#include <map>
#include "ns3/mac48-address.h"
//-------------------------------sikor







namespace ns3 {




    //-------------------------------sikor

    struct macComp {

        bool operator()(const Mac48Address& a, const Mac48Address& b) const {
            return a < b;
        }
    };


    typedef std::map<Mac48Address, double, macComp> snrMap;
    typedef std::map<Mac48Address, snrMap, macComp> relayMap;



    //-------------------------------sikor

    /**
     * \ingroup rescue
     * \brief use multi rates for data and control transmissions
     *
     * This class uses always the same transmission rate for every
     * packet sent.
     */
    class MultiRateRescueManager : public RescueRemoteStationManager {
    public:
        static TypeId GetTypeId(void);
        MultiRateRescueManager();
        virtual ~MultiRateRescueManager();

    private:
        // overriden from base class
        virtual RescueRemoteStation* DoCreateStation(void) const;
        virtual void DoReportRxOk(RescueRemoteStation *senderStation,
                RescueRemoteStation *sourceStation,
                double rxSnr, RescueMode txMode, bool wasReconstructed);
        virtual void DoReportRxFail(RescueRemoteStation *senderStation,
                RescueRemoteStation *sourceStation,
                double rxSnr, RescueMode txMode, bool wasReconstructed);
        virtual void DoReportDataFailed(RescueRemoteStation *station);
        virtual void DoReportDataOk(RescueRemoteStation *station,
                double ackSnr, RescueMode ackMode,
                double dataSnr, double dataPer);
        virtual void DoReportFinalDataFailed(RescueRemoteStation *station);
        virtual RescueMode DoGetDataTxMode(RescueRemoteStation *station,
                Ptr<const Packet> packet,
                uint32_t size);
        virtual uint32_t DoGetDataCw(RescueRemoteStation *station,
                Ptr<const Packet> packet,
                uint32_t size);
        virtual RescueMode DoGetAckTxMode(RescueRemoteStation *station,
                RescueMode reqMode);
        virtual RescueMode DoGetAckTxMode_(RescueRemoteStation *station);
        virtual RescueMode DoGetCtrlTxMode();
        virtual uint32_t DoGetAckCw(RescueRemoteStation *station);
        virtual bool IsLowLatency(void) const;

        RescueMode m_dataModeHi, m_dataModeLo; //!< Rescue mode for unicast DATA frames
        RescueMode m_ctlMode; //!< Rescue mode for control frames and ACK
        uint32_t m_dataCw; //!< Contention window for unicast DATA frames
        uint32_t m_ctlCw; //!< Contention window for control frames and ACK
        relayMap m_relays;
        Mac48Address m_lastDest;
    };

} // namespace ns3



#endif /* MULTI_RATE_RESCUE_MANAGER_H */

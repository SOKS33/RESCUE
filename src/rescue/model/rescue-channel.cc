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

#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/object-factory.h"
#include "ns3/double.h"
#include "ns3/propagation-loss-model.h"

#include "rescue-channel.h"

NS_LOG_COMPONENT_DEFINE("RescueChannel");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now().GetMicroSeconds() << "] [CHA] "

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(RescueChannel);

    TypeId
    RescueChannel::GetTypeId() {
        static TypeId tid = TypeId("ns3::RescueChannel")
                .SetParent<Object> ()
                .AddConstructor<RescueChannel> ()
                .AddAttribute("PropagationLossModel", "A pointer to the propagation loss model attached to this channel.",
                PointerValue(CreateObject<LogDistancePropagationLossModel> ()),
                MakePointerAccessor(&RescueChannel::m_loss),
                MakePointerChecker<PropagationLossModel> ())
                .AddAttribute("PropagationDelayModel", "A pointer to the propagation delay model attached to this channel.",
                PointerValue(CreateObject<ConstantSpeedPropagationDelayModel> ()),
                MakePointerAccessor(&RescueChannel::m_delay),
                MakePointerChecker<PropagationDelayModel> ())
                .AddAttribute("InsertTxFlowEarlier",
                "Insert tx-flow earlier a certain time",
                TimeValue(NanoSeconds(10)),
                MakeTimeAccessor(&RescueChannel::m_addNoiseEntryEarlier),
                MakeTimeChecker())
                .AddAttribute("DeleteTxFlowLater",
                "Delete tx-flow later a certain time",
                TimeValue(NanoSeconds(100)),
                MakeTimeAccessor(&RescueChannel::m_delNoiseEntryLater),
                MakeTimeChecker())
                .AddAttribute("NoiseFloor",
                "Noise Floor (dBm)",
                DoubleValue(-120.0),
                MakeDoubleAccessor(&RescueChannel::m_noiseFloor),
                MakeDoubleChecker<double> ())
                ;
        return tid;
    }

    RescueChannel::RescueChannel()
    : Channel() {
    }

    RescueChannel::~RescueChannel() {
    }

    void
    RescueChannel::Clear() {
        m_devList.clear();
        m_noiseEntry.clear();
    }

    uint32_t
    RescueChannel::GetNDevices() const {
        return m_devList.size();
    }

    Ptr<NetDevice>
    RescueChannel::GetDevice(uint32_t i) const {
        return m_devList[i].first;
    }

    void
    RescueChannel::AddDevice(Ptr<RescueNetDevice> dev, Ptr<RescuePhy> phy) {
        NS_LOG_INFO("Adding dev/phy pair number " << m_devList.size() + 1);
        m_devList.push_back(std::make_pair(dev, phy));
    }

    bool
    RescueChannel::SendPacket(Ptr<RescuePhy> phy, Ptr<Packet> packet, double txPower, RescueMode mode, Time txDuration) {
        NS_LOG_FUNCTION("");
        Ptr<MobilityModel> senderMobility = 0;
        Ptr<MobilityModel> recvMobility = 0;

        // NoiseEntry stores information, how much signal a node will get and how long that signal will exist.
        // This information will be used by PHY layer to obtain SINR value.
        NoiseEntry ne;
        ne.packet = packet;
        ne.txDuration = txDuration;
        ne.mode = mode;

        RescueDeviceList::const_iterator it = m_devList.begin();
        for (; it != m_devList.end(); it++) {
            if (phy == it->second) {
                senderMobility = it->first->GetNode()->GetObject<MobilityModel> ();
                break;
            }
        }
        NS_ASSERT(senderMobility != 0);

        Simulator::Schedule(txDuration, &RescueChannel::SendPacketDone, this, phy, packet);

        uint32_t j = 0;
        it = m_devList.begin();
        for (; it != m_devList.end(); it++) {
            if (phy != it->second) {
                recvMobility = it->first->GetNode()->GetObject<MobilityModel> ();
                Time delay = m_delay->GetDelay(senderMobility, recvMobility); // propagation delay
                double rxPower = m_loss->CalcRxPower(txPower, senderMobility, recvMobility); // receive power (dBm)

                uint32_t dstNodeId = it->first->GetNode()->GetId();
                Ptr<Packet> copy = packet->Copy();

                ne.packet = copy;
                ne.phy = it->second;
                ne.rxPower = rxPower;
                ne.txEnd = Simulator::Now() + txDuration + delay;
                ne.procDelay = NanoSeconds(0);

                //MODIF 2
                phy->NotifyMacDelay(delay);

                //Simulator::ScheduleWithContext (dstNodeId, delay - m_addNoiseEntryEarlier, &RescueChannel::AddNoiseEntry, this, j, ne);
                Simulator::ScheduleWithContext(dstNodeId, delay, &RescueChannel::AddNoiseEntry, this, j, ne);
                //Simulator::ScheduleWithContext (dstNodeId, delay, &RescueChannel::ReceivePacket, this, j, ne);
            }
            j++;
        }

        return true;
    }

    void
    RescueChannel::SendPacketDone(Ptr<RescuePhy> phy, Ptr<Packet> packet) {
        NS_LOG_FUNCTION("");
        phy->SendPacketDone(packet);
    }

    void
    RescueChannel::AddNoiseEntry(uint32_t i, NoiseEntry ne) {
        NS_LOG_FUNCTION("dev:" << i);

        double maxRxPower = ne.rxPower;
        double minRxPower = ne.rxPower;
        Time maxTxStart;
        Time minTxStart;
        Time preambleSimbolDuration = NanoSeconds(1000000000 / ne.phy->GetPhyPreambleMode(ne.mode).GetDataRate() + 1);
        Time delay = 2 * preambleSimbolDuration;

        //check stronger noise entries starting within the first premble simbol period
        //or weaker transmissions that has started earlier but the first preamble simbol is not ended
        std::list<NoiseEntry>::iterator it = m_noiseEntry.begin();
        for (; it != m_noiseEntry.end(); ++it) {
            if ((it->packet != ne.packet)
                    && (it->phy == ne.phy)) {
                //other frame transmission detected by this phy
                Time txStart = it->txEnd - it->txDuration;

                if ((it->rxPower > maxRxPower) //is stronger transmission
                        && (txStart >= Simulator::Now()) //transmission starts after this transmission (or at the same time)
                        && (txStart <= (Simulator::Now() + preambleSimbolDuration))) //transmission starts before end of first preamble simbol for this transmission
                {
                    //stronger transmission starts within the first slot, delay this tx after the stronger one
                    //NS_LOG_DEBUG ("found stronger noise entry!");
                    maxRxPower = it->rxPower;
                    maxTxStart = txStart + it->procDelay;
                } else if ((it->rxPower < minRxPower) //is weaker transmission
                        && (txStart <= Simulator::Now()) //transmission has started before this transmission (or at the same time)
                        && (txStart > (Simulator::Now() - preambleSimbolDuration))) //this transmission starts within the first preamble simbol of the weakest one tx
                {
                    //weaker transmission has started but the first slot is not ended, should process this tx before the weaker one
                    //NS_LOG_DEBUG ("found weaker noise entry!");
                    minRxPower = it->rxPower;
                    minTxStart = txStart + it->procDelay;
                }
            }
        }
        NS_LOG_DEBUG("found: maxTxPower: " << maxRxPower << ", start: " << maxTxStart << ", minTxPower: " << minRxPower << ", start: " << minTxStart);
        if (maxRxPower > ne.rxPower) {
            //delay this transmission processing after the found noise entry starts
            delay = maxTxStart - Simulator::Now() + NanoSeconds(1);
            NS_LOG_DEBUG("STRONGER RX: start: " << maxTxStart << ", delay: " << delay);
        } else if (minRxPower < ne.rxPower) {
            //start this transmission processing before processing of the found noise entry starts
            delay = minTxStart - Simulator::Now() - NanoSeconds(1);
            NS_LOG_DEBUG("WEAKER RX: start: " << minTxStart << ", delay: " << delay);
        }

        ne.procDelay = delay;
        m_noiseEntry.push_back(ne);
        Simulator::Schedule(delay, &RescueChannel::ReceivePacket, this, i, ne);
    }

    void
    RescueChannel::ReceivePacket(uint32_t i, NoiseEntry ne) {
        NS_LOG_FUNCTION("dev:" << i << ", rxPower: " << ne.rxPower);
        //m_noiseEntry.push_back (ne);
        m_devList[i].second->ReceivePacket(ne.packet, ne.mode, ne.txDuration - ne.procDelay, ne.rxPower);
        Simulator::Schedule(ne.txDuration, &RescueChannel::ReceivePacketDone, this, i, ne);
    }

    void
    RescueChannel::ReceivePacketDone(uint32_t i, NoiseEntry ne) {
        NS_LOG_FUNCTION("dev:" << i);
        m_devList[i].second->ReceivePacketDone(ne.packet, ne.mode, ne.rxPower);
        // If concurrent transmissions end at the same time, some of them can be missed from SINR calculation
        // So, delete a noise entry a few seconds later
        Simulator::Schedule(m_delNoiseEntryLater, &RescueChannel::DeleteNoiseEntry, this, ne);
    }

    void
    RescueChannel::DeleteNoiseEntry(NoiseEntry ne) {
        NS_LOG_FUNCTION(this);
        std::list<NoiseEntry>::iterator it = m_noiseEntry.begin();
        for (; it != m_noiseEntry.end(); ++it) {
            if (it->packet == ne.packet && it->phy == ne.phy) {
                m_noiseEntry.erase(it);
                break;
            }
        }
    }

    double
    RescueChannel::GetNoiseW(Ptr<RescuePhy> phy, Ptr<Packet> signal) {
        Time now = Simulator::Now();
        double noiseW = DbmToW(m_noiseFloor);
        std::list<NoiseEntry>::iterator it = m_noiseEntry.begin();
        // calculate the cumulative noise power
        for (; it != m_noiseEntry.end(); ++it) {
            if (it->phy == phy && it->packet != signal && it->txEnd + NanoSeconds(1) >= now) {
                NS_LOG_DEBUG("add noise: " << DbmToW(it->rxPower));
                noiseW += DbmToW(it->rxPower);
            }
        }
        return noiseW;
    }

    double
    RescueChannel::DbmToW(double dbm) {
        double mw = pow(10.0, dbm / 10.0);
        return mw / 1000.0;
    }

    double
    RescueChannel::WToDbm(double w) {
        double db = log10(1000.0 * w);
        return db * 10.0;
    }

} // namespace ns3

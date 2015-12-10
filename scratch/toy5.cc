/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 AGH University of Science and Technology
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

/* Node 0 periodically transmits UDP packet to Node 1 through Nodes 2 and 3 (they both act as relays)
 *
 *                  -> Node 2 (Relay) --------> Node 4 (Relay) ----
 *                 /    (x2, y2, 0)               (x4, y4, 0)      \
 *                /             \                      ^            \
 *               /               \                    /              \
 *              /                 \                  /                \
 *             /                   \                /                  v
 *   Node 0 (Source)                \              /              Node 1 (Destination)
 *    (0, 0, 0) \                    \            /                   (x1, 0, 0)
 *               \                    \          /                     ^
 *                \                    \        /                     /
 *                 \                    \      /                     /
 *                  \                    v    /                     /
 *                   \---------------> Node 3 (Relay) --------------
 *                                      (x3, y3, 0)
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/rng-seed-manager.h"

#include "ns3/rescue-channel.h"
#include "ns3/adhoc-rescue-mac-helper.h"
#include "ns3/rescue-phy-basic-helper.h"
#include "ns3/rescue-mac-header.h"
#include "ns3/rescue-phy-header.h"

#include "ns3/rescue-utils.h"

#include <vector>
#include <bitset>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RescueToyScenario5");

class SimulationHelper {
private:

public:
    SimulationHelper();
    static void PopulateArpCache();
    static std::string MapPhyRateToString(uint32_t phyRate);
};

Time start = Seconds(10);
Time duration = Seconds(20);

uint32_t m_send = 0;
uint32_t m_received = 0;
uint64_t m_send_bytes = 0;
uint64_t m_received_bytes = 0;
Time m_lastDelay = Seconds(0);
Time m_delaySum = Seconds(0);
Time m_jitterSum = Seconds(0);

typedef std::map<uint64_t, Time> Packets;
typedef std::map<uint64_t, Time>::reverse_iterator PacketsRI;
typedef std::map<uint64_t, Time>::iterator PacketsI;

Packets m_sentFrames;

std::string
SimulationHelper::MapPhyRateToString(uint32_t phyRate) {
    switch (phyRate) {
        case 6:
            return "Ofdm6Mbps";
            break;
        case 9:
            return "Ofdm9Mbps";
            break;
        case 12:
            return "Ofdm12Mbps";
            break;
        case 18:
            return "Ofdm18Mbps";
            break;
        case 24:
            return "Ofdm24Mbps";
            break;
        case 36:
            return "Ofdm36Mbps";
            break;
        default:
            std::cout << "Unsuported TX rate !!! Valid values are: 6, 9, 12, 18, 24, 36!" << std::endl;
            NS_ASSERT(false);
            return "";
            break;
    }
}

void
SimulationHelper::PopulateArpCache() {

    Ptr<ArpCache> arp = CreateObject<ArpCache> ();
    arp->SetAliveTimeout(Seconds(3600 * 24 * 365));

    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {

        Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
        NS_ASSERT(ip != 0);
        ObjectVectorValue interfaces;
        ip->GetAttribute("InterfaceList", interfaces);

        for (ObjectVectorValue::Iterator j = interfaces.Begin(); j != interfaces.End(); j++) {

            Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
            NS_ASSERT(ipIface != 0);
            Ptr<NetDevice> device = ipIface->GetDevice();
            NS_ASSERT(device != 0);
            Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress());

            for (uint32_t k = 0; k < ipIface->GetNAddresses(); k++) {

                Ipv4Address ipAddr = ipIface->GetAddress(k).GetLocal();

                if (ipAddr == Ipv4Address::GetLoopback()) continue;

                ArpCache::Entry * entry = arp->Add(ipAddr);
                entry->MarkWaitReply(0);
                entry->MarkAlive(addr);
            }
        }
    }

    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {

        Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
        NS_ASSERT(ip != 0);
        ObjectVectorValue interfaces;
        ip->GetAttribute("InterfaceList", interfaces);

        for (ObjectVectorValue::Iterator j = interfaces.Begin(); j != interfaces.End(); j++) {

            Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
            ipIface->SetAttribute("ArpCache", PointerValue(arp));
        }
    }
}

/* =========== Trace functions =========== */

std::string COLOR_RED = "\x1b[31m";
std::string COLOR_GREEN = "\x1b[32m";
std::string COLOR_YELLOW = "\x1b[33m";
std::string COLOR_BLUE = "\x1b[34m";
std::string COLOR_DEFAULT = "\x1b[0m";

void
EnqueueOkTrace(std::string context, uint32_t node, uint32_t iff, Ptr<const Packet> pkt, const RescueMacHeader &hdr) {
    NS_LOG_UNCOND(COLOR_RED << "TRACE ENQUEUE" << COLOR_DEFAULT);
    m_sentFrames.insert(std::pair<uint64_t, Time> (pkt->GetUid(), Simulator::Now()));
    if (Simulator::Now() > start) {
        m_send++;
        //        std::cout << Simulator::Now().GetSeconds() << "\tENQUEUE, TX to: \t" << hdr.GetDestination() << "\t" << m_send << " SIZE=" << pkt->GetSize() << std::endl;

        m_send_bytes += pkt->GetSize();
    }
}

void
DataRxOkTrace(std::string context, uint32_t node, uint32_t iff, Ptr<const Packet> pkt, const RescuePhyHeader &hdr) {
    NS_LOG_UNCOND(COLOR_RED << "TRACE DATARX OK" << COLOR_DEFAULT);
    Packets::iterator it = m_sentFrames.find(pkt->GetUid());
    if (it != m_sentFrames.end()) {
        if (Simulator::Now() > start) {
            Time delay = Simulator::Now() - it->second;
            m_received++;
            //            std::cout << Simulator::Now().GetSeconds() << "\tRX OK, \t\t from: \t" << hdr.GetSource() << "\t" << m_received << std::endl;

            m_received_bytes += pkt->GetSize();
            m_delaySum += delay;
            m_jitterSum += ((m_lastDelay > delay) ? (m_lastDelay - delay) : (delay - m_lastDelay));
            m_lastDelay = delay;
        }
        m_sentFrames.erase(it);
    }
}

uint32_t phy_send = 0, phy_recv = 0;

void
PhySendTrace(std::string context, Ptr<const Packet> pkt) {
    if (pkt->GetSize() > 500)
        phy_send++;
}

double minSNR = 12.0;

void
PhyRecvTrace(std::string context, Ptr<const Packet> pkt, double snr) {
    if (pkt->GetSize() > 500)
        phy_recv++;
    minSNR = (minSNR > snr ? snr : minSNR);
    //    NS_LOG_UNCOND("SNR = " << snr << " min=" << minSNR);
}

double SNR = 120.0;

void
ComputeMinSNR(std::string context, double snr) {
    SNR = (SNR > snr ? snr : SNR);
    //    NS_LOG_UNCOND("COMPUTE SNR = " << snr << " min=" << SNR);
}

/* =========== Trace functions =========== */

int main(int argc, char *argv[]) {
    float calcStart = 10;
    float simTime = 25;
    uint32_t seed = 3;

    float xD = 1200.0;
    float yD = 0.0;

    float xR1 = 300.0;
    float yR1 = 500.0;

    float xR2 = 300.0;
    float yR2 = -500.0;

    float xR3 = 900.0;
    float yR3 = 500.0;

    float xR4 = 900.0;
    float yR4 = -500.0;

    double snrSR1 = 0;
    double snrSR2 = 0;
    double snrR2D = 0;
    double snrR3D = 0;
    double snrR1R2 = 0;
    double snrR1R3 = 0;
    double snrR2R3 = 0;

    double noise = -100.0;
    double CsPowerThr = -110;
    double RxPowerThr = -108;
    double TxPower = 20;
    double BerThr = 0.2;
    bool useLOTF = true;

    uint32_t cwMin = 16;
    uint32_t cwMax = 1024;
    double slotTime = 15;
    double sifsTime = 30;
    double lifsTime = 60;
    uint32_t queueLimit = 100;
    uint16_t retryLimit = 2;
    double ackTimeout = 7000;

    uint32_t basicPhyRate = 6;
    uint32_t dataPhyRate = 6;
    uint32_t relayDataPhyRate1 = 6;
    uint32_t relayDataPhyRate2 = 6;
    uint32_t relayDataPhyRate3 = 6;
    uint32_t relayDataPhyRate4 = 6;

    float Mbps = 2;
    uint16_t packetSize = 1000;
    uint32_t packetLimit = 0;


    //MODIF 3
    float xR5 = 1500.0;
    float yR5 = 500.0;
    xD = 1500.0;
    yD = -100.0;
    uint32_t relayDataPhyRate5 = 6;

    /* =============== Logging =============== */

    //    LogComponentEnable("RescueArqManager", LOG_LEVEL_INFO); //comment to switch off logging

    //    LogComponentEnable("RescueMacCsma", LOG_LEVEL_INFO); //comment to switch off logging

    //    LogComponentEnable("RescueMacCsma", LOG_LEVEL_DEBUG); //comment to switch off logging
    //    LogComponentEnable("RescueMacCsma", LOG_LEVEL_ALL); //comment to switch off logging

    //    LogComponentEnable("RescuePhy", LOG_LEVEL_INFO); //comment to switch off logging
    //    LogComponentEnable("RescuePhy", LOG_LEVEL_DEBUG); //comment to switch off logging
    //LogComponentEnable("RescuePhy", LOG_LEVEL_ALL); //comment to switch off logging

    //    LogComponentEnable("RescueChannel", LOG_LEVEL_DEBUG); //comment to switch off logging
    //LogComponentEnable("RescueChannel", LOG_LEVEL_ALL); //comment to switch off logging

    //LogComponentEnable("PropagationLossModel", LOG_LEVEL_ALL); //comment to switch off logging
    //LogComponentEnable("RescueRemoteStationManager", LOG_LEVEL_ALL); //comment to switch off logging
    //LogComponentEnable("ConstantRateRescueManager", LOG_LEVEL_ALL); //comment to switch off logging
    //LogComponentEnable("RescueHelper", LOG_LEVEL_ALL); //comment to switch off logging

    //LogComponentEnable("BlackBox", LOG_LEVEL_ALL); //comment to switch off logging
    //LogComponentEnable("BlackBox_no2", LOG_LEVEL_ALL); //comment to switch off logging



    /* ======= Command Line parameters ======= */

    CommandLine cmd;
    cmd.AddValue("simTime", "Time of simulation", simTime);
    cmd.AddValue("calcStart", "Begin of results analysis", calcStart);
    cmd.AddValue("seed", "Seed", seed);

    cmd.AddValue("xD", "X-coordinate [m] of node 1 (destination)", xD);
    cmd.AddValue("yD", "Y-coordinate [m] of node 1 (destination)", yD);
    cmd.AddValue("xR1", "X-coordinate [m] of node 2 (relay 1)", xR1);
    cmd.AddValue("yR1", "Y-coordinate [m] of node 2 (relay 1)", yR1);
    cmd.AddValue("xR2", "X-coordinate [m] of node 3 (relay 2)", xR2);
    cmd.AddValue("yR2", "Y-coordinate [m] of node 3 (relay 2)", yR2);

    cmd.AddValue("snrSR1", "SNR [dB] for direct transmission between node 0 (source) and node 2 (relay 1)", snrSR1);
    cmd.AddValue("snrSR2", "SNR [dB] for direct transmission between node 0 (source) and node 3 (relay 2)", snrSR2);
    cmd.AddValue("snrR2D", "SNR [dB] for direct transmission between node 3 (relay 2) and node 1 (destination)", snrR2D);

    cmd.AddValue("noise", "Noise floor for channel [dBm]", noise);
    cmd.AddValue("CsPowerThr", "Carrier Sense Threshold [dBm]", CsPowerThr);
    cmd.AddValue("RxPowerThr", "Reception Power Threshold [dBm]", RxPowerThr);
    cmd.AddValue("TxPower", "Transmission Power [dBm]", TxPower);
    cmd.AddValue("BerThr", "BER threshold - frames with higher BER are unusable for reconstruction purposes and should not be stored or forwarded (default value = 0.5)", BerThr);
    cmd.AddValue("useLOTF", "Use Links-on-the-Fly", useLOTF);

    cmd.AddValue("cwMin", "Minimum value of CW", cwMin);
    cmd.AddValue("cwMax", "Maximum value of CW", cwMax);
    cmd.AddValue("slotTime", "Time slot duration [us] for MAC backoff", slotTime);
    cmd.AddValue("sifsTime", "Short Inter-frame Space [us]", sifsTime);
    cmd.AddValue("lifsTime", "Long Inter-frame Space [us]", lifsTime);
    cmd.AddValue("queueLimit", "Maximum packets to queue at MAC", queueLimit);
    cmd.AddValue("retryLimit", "Maximum Limit for DATA Retransmission", retryLimit);
    cmd.AddValue("ackTimeout", "ACK awaiting time [us]", ackTimeout);

    cmd.AddValue("basicPhyRate", "Transmission Rate [Mbps] for Control Packets", basicPhyRate);
    cmd.AddValue("dataPhyRate", "Transmission Rate [Mbps] for Data Packets", dataPhyRate);
    cmd.AddValue("relayDataPhyRate1", "Transmission Rate [Mbps] for Data Packets used by relay 1 node", relayDataPhyRate1);
    cmd.AddValue("relayDataPhyRate2", "Transmission Rate [Mbps] for Data Packets used by relay 2 node", relayDataPhyRate2);

    cmd.AddValue("dataRate", "Data Rate [Mbps]", Mbps);
    cmd.AddValue("packetSize", "Packet Size [B]", packetSize);
    cmd.AddValue("packetLimit", "Max number of transmitted packets (0 - no limit)", packetLimit);

    double expo = 3.0;
    cmd.AddValue("exponent", "ThreeLog exponent", expo);


    CC_ENABLED = true;
    CC_LOG_ENABLED = false;
    ACK_ENABLED = false;

    bool SIMPLE = false;
    bool DISTRIBUTED = false;
    bool CLASSIC = false;
    bool DISTRIBUTED_ACK = false;
    cmd.AddValue("CC", "True if Control Channel is enabled", CC_ENABLED);
    cmd.AddValue("CC_LOG", "true if Control Channel LOGS are enabled", CC_LOG_ENABLED);
    cmd.AddValue("ACK", "TRUE if ACK are enabled", ACK_ENABLED);
    cmd.AddValue("Simple", "True if SimpleMAC is used (without ACK)", SIMPLE);
    cmd.AddValue("Distributed", "True if DistributedMAC is used", DISTRIBUTED);
    cmd.AddValue("Classic", "True if SimpleMAC is used", CLASSIC);
    cmd.AddValue("DistributedACK", "True if DistributedMAC + E2E ACK is used", DISTRIBUTED_ACK);
    cmd.Parse(argc, argv);

    if (!SIMPLE && !CLASSIC && !DISTRIBUTED && !DISTRIBUTED_ACK)
        CLASSIC = true;

    if (CC_LOG_ENABLED && !CC_ENABLED)
        CC_LOG_ENABLED = false;

    if ((SIMPLE && DISTRIBUTED)
            || (SIMPLE && CLASSIC)
            || (DISTRIBUTED && CLASSIC)) {
        DISTRIBUTED = false;
        SIMPLE = false;
        CLASSIC = true;
    }

    if (SIMPLE) {
        CC_ENABLED = false;
        CC_LOG_ENABLED = false;
        ACK_ENABLED = false;

        DISTRIBUTED_ACK_ENABLED = false;

    } else if (DISTRIBUTED) {
        CC_ENABLED = true;
        ACK_ENABLED = false;

        DISTRIBUTED_ACK_ENABLED = false;

    } else if (CLASSIC) {
        CC_ENABLED = false;
        CC_LOG_ENABLED = false;
        ACK_ENABLED = true;

        DISTRIBUTED_ACK_ENABLED = false;

    } else if (DISTRIBUTED_ACK) {
        CC_ENABLED = false;
        CC_LOG_ENABLED = false;
        ACK_ENABLED = false;
        DISTRIBUTED_ACK_ENABLED = true;

    }

    ns3::RngSeedManager::SetSeed(seed);
    duration = Seconds(simTime);
    start = Seconds(calcStart);



    /* ============ Nodes creation =========== */
    NodeContainer nodes;
    nodes.Create(2);
    NodeContainer relayNode1;
    relayNode1.Create(1);
    NodeContainer relayNode2;
    relayNode2.Create(1);
    NodeContainer relayNode3;
    relayNode3.Create(1);
    NodeContainer relayNode4;
    relayNode4.Create(1);

    //MODIF 3
    NodeContainer relayNode5;
    relayNode5.Create(1);

    /* ======== Positioning / Mobility ======= */

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(xD, yD, 0.0));
    positionAlloc->Add(Vector(xR1, yR1, 0.0));
    positionAlloc->Add(Vector(xR2, yR2, 0.0));
    positionAlloc->Add(Vector(xR3, yR3, 0.0));
    positionAlloc->Add(Vector(xR4, yR4, 0.0));
    //MODIF 3
    positionAlloc->Add(Vector(xR5, yR5, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    //MODIF 3

    //    mobility.Install(NodeContainer(nodes, relayNode1, relayNode2, relayNode3, relayNode4));
    mobility.Install(NodeContainer(nodes, relayNode1, relayNode2, relayNode3, NodeContainer(relayNode4, relayNode5)));



    /* ===== Channel / Propagation Model ===== */

    //    Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
    //    //Unavailable links
    //    lossModel->SetLoss(nodes.Get(0)->GetObject<MobilityModel>(), nodes.Get(1)->GetObject<MobilityModel>(), 1000);
    //    lossModel->SetLoss(nodes.Get(0)->GetObject<MobilityModel>(), relayNode3.Get(0)->GetObject<MobilityModel>(), 1000);
    //    lossModel->SetLoss(nodes.Get(1)->GetObject<MobilityModel>(), relayNode1.Get(0)->GetObject<MobilityModel>(), 1000);
    //    //Source --> Relays
    //    lossModel->SetLoss(nodes.Get(0)->GetObject<MobilityModel>(), relayNode1.Get(0)->GetObject<MobilityModel>(), TxPower - noise - snrSR1); //Loss = TxPower - noise - snr
    //    lossModel->SetLoss(nodes.Get(0)->GetObject<MobilityModel>(), relayNode2.Get(0)->GetObject<MobilityModel>(), TxPower - noise - snrSR2); //Loss = TxPower - noise - snr
    //    //Relays --> Restination
    //    lossModel->SetLoss(relayNode3.Get(0)->GetObject<MobilityModel>(), nodes.Get(1)->GetObject<MobilityModel>(), TxPower - noise - snrR3D); //Loss = TxPower - noise - snr
    //    lossModel->SetLoss(relayNode2.Get(0)->GetObject<MobilityModel>(), nodes.Get(1)->GetObject<MobilityModel>(), TxPower - noise - snrR2D); //Loss = TxPower - noise - snr
    //    //Relays --> Relays
    //    lossModel->SetLoss(relayNode1.Get(0)->GetObject<MobilityModel>(), relayNode2.Get(0)->GetObject<MobilityModel>(), TxPower - noise - snrR1R2); //Loss = TxPower - CsPowerThr
    //    lossModel->SetLoss(relayNode1.Get(0)->GetObject<MobilityModel>(), relayNode3.Get(0)->GetObject<MobilityModel>(), TxPower - noise - snrR1R3); //Loss = TxPower - CsPowerThr
    //    lossModel->SetLoss(relayNode2.Get(0)->GetObject<MobilityModel>(), relayNode3.Get(0)->GetObject<MobilityModel>(), TxPower - noise - snrR2R3); //Loss = TxPower - CsPowerThr


    //    Ptr<LogDistancePropagationLossModel> log = CreateObject<LogDistancePropagationLossModel> ();
    //    log->SetAttribute("Exponent", DoubleValue(2.8));

    //    Ptr<RangePropagationLossModel> range = CreateObject<RangePropagationLossModel> ();
    //    range->SetAttribute("MaxRange", DoubleValue(800.0));

    //    Ptr<NakagamiPropagationLossModel> nakagami = CreateObject<NakagamiPropagationLossModel> ();
    //    nakagami->SetAttribute("Distance2", DoubleValue(800.0));
    //    nakagami->SetAttribute("m2", DoubleValue(0.0));
    //    nakagami->SetAttribute("Distance1", DoubleValue(10.0));
    //    nakagami->SetAttribute("m1", DoubleValue(0.12));

    Ptr<ThreeLogDistancePropagationLossModel> threelog = CreateObject<ThreeLogDistancePropagationLossModel> ();
    threelog->SetAttribute("Distance2", DoubleValue(800.0));
    threelog->SetAttribute("Exponent2", DoubleValue(10e9));
    threelog->SetAttribute("Distance1", DoubleValue(10.0));
    threelog->SetAttribute("Exponent1", DoubleValue(expo));



    Ptr<RescueChannel> rescueChan = CreateObject<RescueChannel> ();
    rescueChan->SetAttribute("NoiseFloor", DoubleValue(noise));
    //    rescueChan->SetAttribute("PropagationLossModel", PointerValue(lossModel));
    //    rescueChan->SetAttribute("PropagationLossModel", PointerValue(range));
    //    rescueChan->SetAttribute("PropagationLossModel", PointerValue(nakagami));
    rescueChan->SetAttribute("PropagationLossModel", PointerValue(threelog));

    /* ======= PHY & MAC configuration ======= */



    RescuePhyBasicHelper rescuePhy = RescuePhyBasicHelper::Default();
    rescuePhy.SetType("ns3::RescuePhy",
            "CsPowerThr", DoubleValue(CsPowerThr),
            "RxPowerThr", DoubleValue(RxPowerThr),
            "TxPower", DoubleValue(TxPower),
            "BerThr", DoubleValue(BerThr),
            "UseLOTF", BooleanValue(useLOTF));

    AdhocRescueMacHelper rescueMac = AdhocRescueMacHelper::Default();
    rescueMac.SetType("ns3::AdhocRescueMac",
            "CwMin", UintegerValue(cwMin),
            "CwMax", UintegerValue(cwMax),
            "SlotTime", TimeValue(MicroSeconds(slotTime)),
            "SifsTime", TimeValue(MicroSeconds(sifsTime)),
            "LifsTime", TimeValue(MicroSeconds(lifsTime)),
            "QueueLimits", UintegerValue(queueLimit));

    RescueHelper rescue;
    rescue.SetRemoteStationManager("ns3::ConstantRateRescueManager",
            "DataMode", StringValue(SimulationHelper::MapPhyRateToString(dataPhyRate)),
            "ControlMode", StringValue(SimulationHelper::MapPhyRateToString(basicPhyRate)));
    rescue.SetArqManager("ns3::RescueArqManager",
            "BasicAckTimeout", TimeValue(MicroSeconds(ackTimeout)),
            "MaxRetryCount", UintegerValue(++retryLimit));
    NetDeviceContainer devices = rescue.Install(nodes, rescueChan, rescuePhy, rescueMac);

    RescueHelper relayRescue1;
    relayRescue1.SetRemoteStationManager("ns3::ConstantRateRescueManager",
            "DataMode", StringValue(SimulationHelper::MapPhyRateToString(relayDataPhyRate1)),
            "ControlMode", StringValue(SimulationHelper::MapPhyRateToString(basicPhyRate)));
    relayRescue1.SetArqManager("ns3::RescueArqManager",
            "BasicAckTimeout", TimeValue(MicroSeconds(ackTimeout)),
            "MaxRetryCount", UintegerValue(++retryLimit));
    NetDeviceContainer relayDevice1 = relayRescue1.Install(relayNode1, rescueChan, rescuePhy, rescueMac);

    RescueHelper relayRescue2;
    relayRescue2.SetRemoteStationManager("ns3::ConstantRateRescueManager",
            "DataMode", StringValue(SimulationHelper::MapPhyRateToString(relayDataPhyRate2)),
            "ControlMode", StringValue(SimulationHelper::MapPhyRateToString(basicPhyRate)));
    relayRescue2.SetArqManager("ns3::RescueArqManager",
            "BasicAckTimeout", TimeValue(MicroSeconds(ackTimeout)),
            "MaxRetryCount", UintegerValue(++retryLimit));
    NetDeviceContainer relayDevice2 = relayRescue2.Install(relayNode2, rescueChan, rescuePhy, rescueMac);

    RescueHelper relayRescue3;
    relayRescue3.SetRemoteStationManager("ns3::ConstantRateRescueManager",
            "DataMode", StringValue(SimulationHelper::MapPhyRateToString(relayDataPhyRate3)),
            "ControlMode", StringValue(SimulationHelper::MapPhyRateToString(basicPhyRate)));
    relayRescue3.SetArqManager("ns3::RescueArqManager",
            "BasicAckTimeout", TimeValue(MicroSeconds(ackTimeout)),
            "MaxRetryCount", UintegerValue(++retryLimit));
    NetDeviceContainer relayDevice3 = relayRescue3.Install(relayNode3, rescueChan, rescuePhy, rescueMac);

    RescueHelper relayRescue4;
    relayRescue4.SetRemoteStationManager("ns3::ConstantRateRescueManager",
            "DataMode", StringValue(SimulationHelper::MapPhyRateToString(relayDataPhyRate4)),
            "ControlMode", StringValue(SimulationHelper::MapPhyRateToString(basicPhyRate)));
    relayRescue4.SetArqManager("ns3::RescueArqManager",
            "BasicAckTimeout", TimeValue(MicroSeconds(ackTimeout)),
            "MaxRetryCount", UintegerValue(++retryLimit));
    NetDeviceContainer relayDevice4 = relayRescue4.Install(relayNode4, rescueChan, rescuePhy, rescueMac);

    //MODIF 3
    RescueHelper relayRescue5;
    relayRescue5.SetRemoteStationManager("ns3::ConstantRateRescueManager",
            "DataMode", StringValue(SimulationHelper::MapPhyRateToString(relayDataPhyRate5)),
            "ControlMode", StringValue(SimulationHelper::MapPhyRateToString(basicPhyRate)));
    relayRescue5.SetArqManager("ns3::RescueArqManager",
            "BasicAckTimeout", TimeValue(MicroSeconds(ackTimeout)),
            "MaxRetryCount", UintegerValue(++retryLimit));
    NetDeviceContainer relayDevice5 = relayRescue5.Install(relayNode5, rescueChan, rescuePhy, rescueMac);


    nodes.Add(relayNode1);
    nodes.Add(relayNode2);
    nodes.Add(relayNode3);
    nodes.Add(relayNode4);
    //MODIF 3
    nodes.Add(relayNode5);

    devices.Add(relayDevice1);
    devices.Add(relayDevice2);
    devices.Add(relayDevice3);
    devices.Add(relayDevice4);
    //MODIF 3
    devices.Add(relayDevice5);


    /* ============ Internet stack =========== */

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer iface = ipv4.Assign(devices);



    /* ============= Applications ============ */

    uint16_t dataSize = packetSize - (8 + 20 + 8 + 15 + 4); //UDP hdr, IP hdr, LLC hdr, MAC hdr, MAC FCS
    DataRate dataRate = DataRate(int(1000000 * Mbps));

    OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(iface.GetAddress(1), 1000));
    onOffHelper.SetConstantRate(dataRate, dataSize);
    onOffHelper.SetAttribute("MaxBytes", UintegerValue(packetLimit * dataSize));
    onOffHelper.SetAttribute("StartTime", TimeValue(start));
    onOffHelper.SetAttribute("StopTime", TimeValue(start + duration));
    // packets generation times modified by random value between -50% and + 50% of
    // constant time step between packets
    //onOffHelper.SetAttribute ("RandSendTimeLimit", UintegerValue (random));
    onOffHelper.Install(nodes.Get(0));

    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(iface.GetAddress(1), 1000));
    sink.Install(nodes.Get(1));


    /* =========== Trace Connections ========= */

    Config::Connect("/NodeList/*/DeviceList/*/Mac/EnqueueOk", MakeCallback(&EnqueueOkTrace));
    Config::Connect("/NodeList/*/DeviceList/*/Mac/DataRxOk", MakeCallback(&DataRxOkTrace));

    Config::Connect("/NodeList/*/DeviceList/*/Phy/SendOk", MakeCallback(&PhySendTrace));
    Config::Connect("/NodeList/*/DeviceList/*/Phy/RecvOk", MakeCallback(&PhyRecvTrace));
    Config::Connect("/NodeList/*/DeviceList/*/Phy/UpdateSNR", MakeCallback(&ComputeMinSNR));

    /* ============= Flow Monitor ============ */

    FlowMonitorHelper flowmon_helper;
    Ptr<FlowMonitor> monitor = flowmon_helper.InstallAll();
    monitor->SetAttribute("StartTime", TimeValue(start));
    monitor->SetAttribute("DelayBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("JitterBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("PacketSizeBinWidth", DoubleValue(20));

    /* ========== Running simulation ========= */

    SimulationHelper::PopulateArpCache();
    Simulator::Stop(start + duration + Seconds(1));
    Simulator::Run();
    Simulator::Destroy();

    /* =========== Printing Results ========== */

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon_helper.GetClassifier());
    std::string proto;

    std::cout << std::endl;

    std::map< FlowId, FlowMonitor::FlowStats > stats = monitor->GetFlowStats();
    for (std::map< FlowId, FlowMonitor::FlowStats >::iterator flow = stats.begin(); flow != stats.end(); flow++) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow->first);
        switch (t.protocol) {
            case(6):
                proto = "TCP";
                break;
            case(17):
                proto = "UDP";
                break;
            default:
                exit(1);
        }
        std::cout << "\n====================================================\n"
                << "FlowID: " << flow->first << "(" << proto << " "
                << t.sourceAddress << "/" << t.sourcePort << " --> "
                << t.destinationAddress << "/" << t.destinationPort << ")" <<
                std::endl;

        std::cout << "timeFirstTxPacket:" << flow->second.timeFirstTxPacket.GetSeconds() << std::endl;
        std::cout << "timeFirstRxPacket:" << flow->second.timeFirstRxPacket.GetSeconds() << std::endl;
        std::cout << "timeLastTxPacket:" << flow->second.timeLastTxPacket.GetSeconds() << std::endl;
        std::cout << "timeLastRxPacket:" << flow->second.timeLastRxPacket.GetSeconds() << std::endl;

        std::cout << std::endl;

        std::cout << "  Tx Packets: " << flow->second.txPackets << " " << m_send << std::endl;
        std::cout << "  Tx Bytes: " << flow->second.txBytes << std::endl;
        std::cout << "  Enqueued Packets: " << m_send << std::endl;
        std::cout << "  Enqueued Bytes: " << m_send_bytes << std::endl;


        std::cout << "  Rx Packets: " << flow->second.rxPackets << " " << m_received << std::endl;
        std::cout << "  Rx Bytes: " << flow->second.rxBytes << std::endl;
        std::cout << "  Received Packets: " << m_received << std::endl;
        std::cout << "  Received Bytes: " << m_received_bytes << std::endl;

        std::cout << std::endl;
        //        std::cout << "  Lost Packets: " << flow->second.lostPackets << std::endl;
        std::cout << "  Lost Packets: " << m_send - m_received << std::endl;

        //        double lost = 100.0 * flow->second.lostPackets / flow->second.txPackets;
        double lost = (m_send > 0) ? (100.0 * (m_send - m_received) / m_send) : 100.0;
        double delay = (double) ((m_received > 0) ? ((flow->second.delaySum / m_received).GetMicroSeconds() / 1000) : 0);
        double jitter = (double) ((m_received > 0) ? ((flow->second.jitterSum / m_received).GetMicroSeconds() / 1000) : 0);
        //        double throughput = 8000 * (double) flow->second.rxBytes / (flow->second.timeLastRxPacket - flow->second.timeFirstTxPacket).GetNanoSeconds();
        double throughput = 8000 * (double) m_received_bytes / (flow->second.timeLastRxPacket - flow->second.timeFirstTxPacket).GetNanoSeconds();

        std::cout << "  Lost Packets [%]: " << lost << std::endl;
        std::cout << "  Mean Delay [ms]: " << delay << std::endl;
        std::cout << "  Mean Jitter [ms]: " << jitter << std::endl;
        std::cout << "  Throughput [Mbps]: " << throughput << std::endl;

        std::cout << std::endl;
        std::cout << "  PHY send : " << phy_send << std::endl;
        std::cout << "  PHY recv : " << phy_recv << std::endl;
        std::cout << std::endl;

        std::cout << "SIM_STATS " << lost << " " << delay << " " << jitter << " " << throughput
                << " " << phy_send << " " << phy_recv
                << " " << ((phy_send > 0) ? (1.0 * m_send / phy_send) : 0)
                << " " << minSNR << std::endl;
    }

    //    std::cout << "SEND " << phy_send << " RECV " << phy_recv << std::endl;

    //    std::cout << "  Tx Bytes [B]: " << m_send_bytes << std::endl;
    //    std::cout << "  Rx Bytes [B]: " << m_received_bytes << std::endl;
    //    std::cout << "  Tx Packets: " << m_send << std::endl;
    //    std::cout << "  Rx Packets: " << m_received << std::endl;
    //    std::cout << "  Throughput [Mbps]: " << 8 * (double) m_received_bytes / (double) (simTime - calcStart) / 1000000 << std::endl;
    //    std::cout << "  Lost Packets [%]: " << ((m_send > m_received) ? (100 * ((double) m_send - (double) m_received) / (double) m_send) : 0) << std::endl;
    //    std::cout << "  Mean Delay [ms]: " << (double) (m_delaySum / m_received).GetMicroSeconds() / 1000 << std::endl;
    //    std::cout << "  Mean Jitter [ms]: " << (double) (m_jitterSum / m_received).GetMicroSeconds() / 1000 << std::endl;

    //    std::cout << 8 * (double) m_received_bytes / (double) (simTime - calcStart) / 1000000 << "\t"
    //            << ((m_send > m_received) ? (100 * ((double) m_send - (double) m_received) / (double) m_send) : 0) << "\t"
    //            << (double) (m_delaySum / m_received).GetMicroSeconds() / 1000 << "\t"
    //            << (double) (m_jitterSum / m_received).GetMicroSeconds() / 1000 << std::endl;

    std::cout << "PROPER END" << std::endl;
    std::cout << SIMPLE << CLASSIC << DISTRIBUTED << CC_ENABLED << CC_LOG_ENABLED << ACK_ENABLED << DISTRIBUTED_ACK_ENABLED << std::endl;


    //    std::cout << CC_ENABLED << CC_LOG_ENABLED << ACK_ENABLED << SIMPLE << DISTRIBUTED << std::endl;

    return 0;
}


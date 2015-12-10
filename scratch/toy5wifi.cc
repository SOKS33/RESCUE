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

#include "ns3/wifi-module.h"
#include "ns3/olsr-module.h"

#include <vector>
#include <bitset>

using namespace ns3;


NS_LOG_COMPONENT_DEFINE("RescueToyScenario5wifi");

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
            return "OfdmRate6Mbps";
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

//void
//EnqueueOkTrace(std::string context, uint32_t node, uint32_t iff, Ptr<const Packet> pkt, const RescueMacHeader &hdr) {
//    std::cout << Simulator::Now().GetSeconds() << "\tENQUEUE, TX to: \t" << hdr.GetDestination() << "\t" << *pkt << std::endl;
//    m_sentFrames.insert(std::pair<uint64_t, Time> (pkt->GetUid(), Simulator::Now()));
//    if (Simulator::Now() > start) {
//        m_send++;
//        m_send_bytes += pkt->GetSize();
//    }
//}

//void
//DataRxOkTrace(std::string context, uint32_t node, uint32_t iff, Ptr<const Packet> pkt, const RescuePhyHeader &hdr) {
//    std::cout << Simulator::Now().GetSeconds() << "\tRX OK, \t\t from: \t" << hdr.GetSource() << "\t" << *pkt << std::endl;
//    Packets::iterator it = m_sentFrames.find(pkt->GetUid());
//    if (it != m_sentFrames.end()) {
//        if (Simulator::Now() > start) {
//            Time delay = Simulator::Now() - it->second;
//            m_received++;
//            m_received_bytes += pkt->GetSize();
//            m_delaySum += delay;
//            m_jitterSum += ((m_lastDelay > delay) ? (m_lastDelay - delay) : (delay - m_lastDelay));
//            m_lastDelay = delay;
//        }
//        m_sentFrames.erase(it);
//    }
//}


std::string COLOR_RED = "\x1b[31m";
std::string COLOR_GREEN = "\x1b[32m";
std::string COLOR_YELLOW = "\x1b[33m";
std::string COLOR_BLUE = "\x1b[34m";
std::string COLOR_DEFAULT = "\x1b[0m";

void EnqueueOkTrace2(std::string context, Ptr<const Packet> pkt) {
    //    //    WifiMacHeader hdr;
    //    //    pkt->PeekHeader(hdr);
    //    //
    //    std::ostringstream os;
    //    //    hdr.Print(os);
    //    pkt->Print(os);
    //    std::cout << os.str() << std::endl;
    if (pkt->GetSize() > 500) {

        m_sentFrames.insert(std::pair<uint64_t, Time> (pkt->GetUid(), Simulator::Now()));
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << COLOR_RED << "TRACE ENQUEUE 2 " << COLOR_DEFAULT
                << m_sentFrames.size());

        if (Simulator::Now() > start) {
            m_send++;
            m_send_bytes += pkt->GetSize();
        }
        //        std::cout << Simulator::Now().GetSeconds() << "TX OKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK TTTTTTTTTTTTTTTTT " << pkt->GetSize() << std::endl;

    }
}

void DataRxOkTrace2(std::string context, Ptr<const Packet> pkt) {
    //    WifiMacHeader hdr;
    //    pkt->PeekHeader(hdr);
    //    std::ostringstream os;
    //    os << "datarxoktrace2";
    //    hdr.Print(os);
    //    pkt->Print(os);
    //    std::cout << os.str() << std::endl;
    if (pkt->GetSize() > 500) {
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << COLOR_GREEN << "TRACE DATARXOK 2" << COLOR_DEFAULT);

        Packets::iterator it = m_sentFrames.find(pkt->GetUid());
        if (it != m_sentFrames.end()) {
            if (Simulator::Now() > start) {
                Time delay = Simulator::Now() - it->second;
                m_received++;
                m_received_bytes += pkt->GetSize() + 8;
                m_delaySum += delay;
                m_jitterSum += ((m_lastDelay > delay) ? (m_lastDelay - delay) : (delay - m_lastDelay));
                m_lastDelay = delay;
            }
            m_sentFrames.erase(it);
            //            std::cout << Simulator::Now().GetSeconds() << "RX OKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK RRRRRRRRRRRRRRRR " << pkt->GetSize() << std::endl;
        }
    }
}

void DataRxOkTrace3(std::string context, Ptr<const Packet> pkt) {
    NS_LOG_UNCOND(Simulator::Now().GetSeconds() << COLOR_GREEN << "TRACE DATARXOK 3" << COLOR_DEFAULT);

    //    //    WifiMacHeader hdr;
    //    //    pkt->PeekHeader(hdr);
    //    //
    //    std::ostringstream os;
    //    //    hdr.Print(os);
    //    pkt->Print(os);
    //    std::cout << os.str() << std::endl;

    std::cout << Simulator::Now().GetSeconds() << "RX OKKKKKKKKKKKKKKKKKKKKK PRIOMISCCCCCCCCCC RRRRRRRRR" << std::endl;


}

uint32_t ip_send = 0, ip_recv = 0;

void TxTraceIP(std::string context, const Ipv4Header &hdr, Ptr<const Packet> pkt, uint32_t foo) {

    std::ostringstream os;
    hdr.Print(os);
    if (pkt->GetSize() > 800) {
        //                std::cout << Simulator::Now().GetSeconds() << os.str() << " size=" << pkt->GetSize() << " " << ip_send << std::endl;
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << COLOR_RED << "TRACE TX TRACE IP" << COLOR_DEFAULT);
        ip_send++;
    }
}

void RxTraceIP(std::string context, const Ipv4Header &hdr, Ptr<const Packet> pkt, uint32_t foo) {

    if (pkt->GetSize() > 800) {
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << COLOR_RED << "TRACE RX TRACE IP" << COLOR_DEFAULT);
        ip_recv++;
    }
}


uint32_t phy_send = 0, phy_recv = 0, app_send = 0, phy_send_src = 0;

void
PhySendTrace(std::string context, Ptr<const Packet> pkt) {

    if (pkt->GetSize() > 500) {
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << COLOR_YELLOW << "TRACE PHY TX" << COLOR_DEFAULT);
        phy_send++;
    }
}

void
PhySendTraceSrc(std::string context, Ptr<const Packet> pkt) {
    if (pkt->GetSize() > 500) {
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << COLOR_YELLOW << "TRACE PHY TX SRC" << COLOR_DEFAULT);
        phy_send_src++;
    }
}

void
PhyRecvTrace(std::string context, Ptr<const Packet> pkt) {
    if (pkt->GetSize() > 500) {
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << COLOR_YELLOW << "TRACE PHY RX" << COLOR_DEFAULT);
        phy_recv++;
    }
}

void
TxApplication(std::string context, Ptr<const Packet> pkt) {
    if (pkt->GetSize() > 500) {
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << COLOR_BLUE << "TRACE TX APP" << COLOR_DEFAULT);
        app_send++;
    }
}

/* =========== Trace functions =========== */

int main(int argc, char *argv[]) {
    float calcStart = 10;
    float simTime = 25;
    uint32_t seed = 3;

    float xD = 150.0;
    float yD = 0.0;

    float xR1 = 0.0;
    float yR1 = 50.0;
    //
    //    float xR2 = 300.0;
    //    float yR2 = -500.0;

    float xR3 = 100.0;
    float yR3 = 0.0;
    //
    //    float xR4 = 900.0;
    //    float yR4 = -500.0;

    double noise = -100.0;
    double CsPowerThr = -110;
    double RxPowerThr = -108;
    double TxPower = 20;


    uint32_t basicPhyRate = 6;
    uint32_t dataPhyRate = 6;
    uint32_t relayDataPhyRate1 = 6;
    uint32_t relayDataPhyRate2 = 6;
    //    uint32_t relayDataPhyRate3 = 6;
    //    uint32_t relayDataPhyRate4 = 6;

    float Mbps = 2;
    uint16_t packetSize = 1000;
    uint32_t packetLimit = 0;

    double exponentThreeLog = 3.0;

    /* ======= Command Line parameters ======= */

    CommandLine cmd;
    cmd.AddValue("simTime", "Time of simulation", simTime);
    cmd.AddValue("calcStart", "Begin of results analysis", calcStart);
    cmd.AddValue("seed", "Seed", seed);

    //    cmd.AddValue("xD", "X-coordinate [m] of node 1 (destination)", xD);
    //    cmd.AddValue("yD", "Y-coordinate [m] of node 1 (destination)", yD);
    //    cmd.AddValue("xR1", "X-coordinate [m] of node 2 (relay 1)", xR1);
    //    cmd.AddValue("yR1", "Y-coordinate [m] of node 2 (relay 1)", yR1);
    //    cmd.AddValue("xR2", "X-coordinate [m] of node 3 (relay 2)", xR2);
    //    cmd.AddValue("yR2", "Y-coordinate [m] of node 3 (relay 2)", yR2);

    cmd.AddValue("noise", "Noise floor for channel [dBm]", noise);
    cmd.AddValue("CsPowerThr", "Carrier Sense Threshold [dBm]", CsPowerThr);
    cmd.AddValue("RxPowerThr", "Reception Power Threshold [dBm]", RxPowerThr);
    cmd.AddValue("TxPower", "Transmission Power [dBm]", TxPower);


    cmd.AddValue("basicPhyRate", "Transmission Rate [Mbps] for Control Packets", basicPhyRate);
    cmd.AddValue("dataPhyRate", "Transmission Rate [Mbps] for Data Packets", dataPhyRate);
    cmd.AddValue("relayDataPhyRate1", "Transmission Rate [Mbps] for Data Packets used by relay 1 node", relayDataPhyRate1);
    cmd.AddValue("relayDataPhyRate2", "Transmission Rate [Mbps] for Data Packets used by relay 2 node", relayDataPhyRate2);

    cmd.AddValue("packetSize", "Packet Size [B]", packetSize);
    cmd.AddValue("packetLimit", "Max number of transmitted packets (0 - no limit)", packetLimit);

    cmd.AddValue("exponent", "ThreeLog exponent", exponentThreeLog);

    cmd.AddValue("dataRate", "Data Rate [Mbps]", Mbps);
    cmd.Parse(argc, argv);


    ns3::RngSeedManager::SetSeed(seed);
    duration = Seconds(simTime);
    start = Seconds(calcStart);

    /* ============ Nodes creation =========== */
    NodeContainer nodes;
    nodes.Create(2);
    NodeContainer relayNode1;
    relayNode1.Create(1);
    //    NodeContainer relayNode2;
    //    relayNode2.Create(1);
    NodeContainer relayNode3;
    relayNode3.Create(1);
    //    NodeContainer relayNode4;
    //    relayNode4.Create(1);

    /* ======== Positioning / Mobility ======= */

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(xD, yD, 0.0));
    positionAlloc->Add(Vector(xR1, yR1, 0.0));
    //    positionAlloc->Add(Vector(xR2, yR2, 0.0));
    positionAlloc->Add(Vector(xR3, yR3, 0.0));
    //    positionAlloc->Add(Vector(xR4, yR4, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    //    mobility.Install(NodeContainer(nodes, relayNode1, relayNode2, relayNode3, relayNode4));
    mobility.Install(NodeContainer(nodes, relayNode1, relayNode3));


    /* ======= PHY & MAC configuration ======= */


    WifiHelper wifi;

    //    wifi.EnableLogComponents();
    bool log = true;
    if (log) {
        //        LogComponentEnable("Aarfcd", LOG_LEVEL_DEBUG);
        //        LogComponentEnable("AdhocWifiMac", LOG_LEVEL_DEBUG);
        //        LogComponentEnable("AmrrWifiRemoteStation", LOG_LEVEL_DEBUG);
        //        LogComponentEnable("ApWifiMac", LOG_LEVEL_DEBUG);
        //    LogComponentEnable("ns3::ArfWifiManager", LOG_LEVEL_DEBUG);
        //        LogComponentEnable("Cara", LOG_LEVEL_DEBUG);
        //        LogComponentEnable("DcaTxop", LOG_LEVEL_DEBUG);
        //        LogComponentEnable("DcfManager", LOG_LEVEL_DEBUG);
        //    LogComponentEnable("DsssErrorRateModel", LOG_LEVEL_DEBUG);
        //        LogComponentEnable("EdcaTxopN", LOG_LEVEL_DEBUG);
        //        LogComponentEnable("InterferenceHelper", LOG_LEVEL_DEBUG);
        //        LogComponentEnable("Jakes", LOG_LEVEL_DEBUG);
        LogComponentEnable("MacLow", LOG_LEVEL_DEBUG);
        LogComponentEnable("MacRxMiddle", LOG_LEVEL_DEBUG);
        //    LogComponentEnable("MsduAggregator", LOG_LEVEL_DEBUG);
        //    LogComponentEnable("MsduStandardAggregator", LOG_LEVEL_DEBUG);
        //    LogComponentEnable("NistErrorRateModel", LOG_LEVEL_DEBUG);
        //    LogComponentEnable("OnoeWifiRemoteStation", LOG_LEVEL_DEBUG);
        LogComponentEnable("PropagationLossModel", LOG_LEVEL_DEBUG);
        LogComponentEnable("RegularWifiMac", LOG_LEVEL_DEBUG);
        //    LogComponentEnable("RraaWifiManager", LOG_LEVEL_DEBUG);
        LogComponentEnable("StaWifiMac", LOG_LEVEL_DEBUG);
        //    LogComponentEnable("SupportedRates", LOG_LEVEL_DEBUG);
        LogComponentEnable("WifiChannel", LOG_LEVEL_DEBUG);
        //    LogComponentEnable("WifiPhyStateHelper", LOG_LEVEL_DEBUG);
        LogComponentEnable("WifiPhy", LOG_LEVEL_DEBUG);
        LogComponentEnable("WifiRemoteStationManager", LOG_LEVEL_DEBUG);
        //    LogComponentEnable("YansErrorRateModel", LOG_LEVEL_DEBUG);
        LogComponentEnable("YansWifiChannel", LOG_LEVEL_DEBUG);
        LogComponentEnable("YansWifiPhy", LOG_LEVEL_DEBUG);
    }

    wifi.SetStandard(WIFI_PHY_STANDARD_80211a);

    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    wifiPhy.Set("TxPowerStart", DoubleValue(TxPower));
    wifiPhy.Set("TxPowerEnd", DoubleValue(TxPower));
    wifiPhy.Set("TxGain", DoubleValue(0.0));
    wifiPhy.Set("RxGain", DoubleValue(0.0));

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::ThreeLogDistancePropagationLossModel",
            "Exponent1", DoubleValue(exponentThreeLog),
            "Exponent2", DoubleValue(10e9),
            "Distance1", DoubleValue(10.0),
            "Distance2", DoubleValue(800.0));
    wifiPhy.SetChannel(wifiChannel.Create());

    // Add a non-QoS upper mac, and disable rate control (i.e. ConstantRateWifiManager)
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
    wifiMac.SetType("ns3::AdhocWifiMac");

    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
            "DataMode", StringValue(SimulationHelper::MapPhyRateToString(dataPhyRate)),
            "ControlMode", StringValue(SimulationHelper::MapPhyRateToString(basicPhyRate)));

    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);
    NetDeviceContainer relayDevice1 = wifi.Install(wifiPhy, wifiMac, relayNode1);
    //    NetDeviceContainer relayDevice2 = wifi.Install(wifiPhy, wifiMac, relayNode2);
    NetDeviceContainer relayDevice3 = wifi.Install(wifiPhy, wifiMac, relayNode3);
    //    NetDeviceContainer relayDevice4 = wifi.Install(wifiPhy, wifiMac, relayNode4);


    nodes.Add(relayNode1);
    //    nodes.Add(relayNode2);
    nodes.Add(relayNode3);
    //    nodes.Add(relayNode4);

    devices.Add(relayDevice1);
    //    devices.Add(relayDevice2);
    devices.Add(relayDevice3);
    //    devices.Add(relayDevice4);


    /* ============ Internet stack =========== */

    OlsrHelper olsr;
    Ipv4StaticRoutingHelper staticRouting;

    Ipv4ListRoutingHelper list;
    list.Add(staticRouting, 0);
    list.Add(olsr, 10);

    InternetStackHelper internet;
    internet.SetRoutingHelper(list);
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
    //onOffHelper.SetAttribute ("RandSendTimeLimit", UintegerValue (random)); //packets generation times modified by random value between -50% and + 50% of constant time step between packets
    onOffHelper.Install(nodes.Get(0));

    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(iface.GetAddress(1), 1000));
    sink.Install(nodes.Get(1));

    /* =========== Trace Connections ========= */

    Config::Connect("/NodeList/0/DeviceList/*/Mac/MacTx", MakeCallback(&EnqueueOkTrace2));
    Config::Connect("/NodeList/*/DeviceList/*/Mac/MacRx", MakeCallback(&DataRxOkTrace2));
    Config::Connect("/NodeList/*/DeviceList/*/Mac/MacPromiscRx", MakeCallback(&DataRxOkTrace3));

    Config::Connect("/NodeList/*/DeviceList/*/Phy/PhyTxBegin", MakeCallback(&PhySendTrace));
    Config::Connect("/NodeList/0/DeviceList/*/Phy/PhyTxBegin", MakeCallback(&PhySendTraceSrc));
    Config::Connect("/NodeList/*/DeviceList/*/Phy/PhyRxEnd", MakeCallback(&PhyRecvTrace));

    Config::Connect("/NodeList/*/$ns3::Ipv4L3Protocol/SendOutgoing", MakeCallback(&TxTraceIP));
    Config::Connect("/NodeList/*/$ns3::Ipv4L3Protocol/LocalDeliver", MakeCallback(&RxTraceIP));

    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/Tx", MakeCallback(&TxApplication));
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

        std::cout << "  Tx Packets: " << flow->second.txPackets << std::endl;
        std::cout << "  Tx Bytes: " << flow->second.txBytes << std::endl;
        std::cout << "  Enqueued Packets: " << m_send << std::endl;
        std::cout << "  Enqueued Bytes: " << m_send_bytes << std::endl;

        std::cout << "  Rx Packets: " << flow->second.rxPackets << std::endl;
        std::cout << "  Rx Bytes: " << flow->second.rxBytes << std::endl;
        std::cout << "  Received Packets: " << m_received << std::endl;
        std::cout << "  Received Bytes: " << m_received_bytes << std::endl;

        std::cout << "IP : " << ip_send << " " << ip_recv << std::endl;

        std::cout << "  Lost Packets: " << flow->second.lostPackets << std::endl;

        double lost2 = (m_send > 0) ? (100.0 * (m_send - m_received) / m_send) : 100.0;
        double lost = (flow->second.txPackets > 0) ? (100.0 * flow->second.lostPackets / flow->second.txPackets) : 100.0;
        double delay = (double) (m_received > 0) ? ((flow->second.delaySum / m_received).GetMicroSeconds() / 1000) : 0.0;
        double jitter = (double) (m_received > 0) ? ((flow->second.jitterSum / m_received).GetMicroSeconds() / 1000) : 0.0;
        double throughput = 8000 * (double) flow->second.rxBytes / (flow->second.timeLastRxPacket - flow->second.timeFirstTxPacket).GetNanoSeconds();
        std::cout << "  Lost Packets [%]: " << lost << std::endl;
        std::cout << "  Mean Delay [ms]: " << delay << std::endl;
        std::cout << "  Mean Jitter [ms]: " << jitter << std::endl;
        std::cout << "  Throughput [Mbps]: " << throughput << std::endl;

        std::cout << "SIM_STATS " << lost2 << " " << delay << " " << jitter << " " << throughput
                << " " << phy_send << " " << phy_recv << " "
                << ((phy_send > 0) ? (1.0 * m_send / phy_send) : 0) << std::endl;

    }
    //    std::cout << "APP SEND " << app_send << " " << phy_send_src << std::endl;

    //    std::cout << "SEND " << phy_send << " RECV " << phy_recv << std::endl;
    //
    //    std::cout << "  Tx Bytes [B]: " << m_send_bytes << std::endl;
    //    std::cout << "  Rx Bytes [B]: " << m_received_bytes << std::endl;
    //    std::cout << "  Tx Packets: " << m_send << std::endl;
    //    std::cout << "  Rx Packets: " << m_received << std::endl;
    //    std::cout << "  Throughput [Mbps]: " << 8 * (double) m_received_bytes / (double) (simTime - calcStart) / 1000000 << std::endl;
    //    std::cout << "  Lost Packets [%]: " << ((m_send > m_received) ? (100 * ((double) m_send - (double) m_received) / (double) m_send) : 0) << std::endl;
    //    std::cout << "  Mean Delay [ms]: " << (double) (m_delaySum / m_received).GetMicroSeconds() / 1000 << std::endl;
    //    std::cout << "  Mean Jitter [ms]: " << (double) (m_jitterSum / m_received).GetMicroSeconds() / 1000 << std::endl;
    //
    //    std::cout << 8 * (double) m_received_bytes / (double) (simTime - calcStart) / 1000000 << "\t"
    //            << ((m_send > m_received) ? (100 * ((double) m_send - (double) m_received) / (double) m_send) : 0) << "\t"
    //            << (double) (m_delaySum / m_received).GetMicroSeconds() / 1000 << "\t"
    //            << (double) (m_jitterSum / m_received).GetMicroSeconds() / 1000 << std::endl;

    std::cout << "PROPER END" << std::endl;
    return 0;
}


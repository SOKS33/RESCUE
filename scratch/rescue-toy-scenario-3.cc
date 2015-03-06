/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 University of Arizona
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
 * basing on Simple CSMA/CA Protocol module Junseok Kim <junseok@email.arizona.edu> <engr.arizona.edu/~junseok>
 */

/* Node 0 periodically transmits UDP packet to Node 1 through Nodes 2 and 3 (they act as relays)
 *
 *               Node 2 (Relay)
 *                (x2, y2, 0)
 *                 ^        \  
 *                /          \ 
 *               /            \
 *              /              v
 *      Node 0  --------------> Node 1              
 *    (0, 0, 0) \             (x1, 0, 0)         
 *               \             ^
 *                \           /
 *                 \         /
 *                  v       /
 *               Node 3 (Relay)
 *                (x3, y3, 0)
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/rng-seed-manager.h"

#include "ns3/rescue-channel.h"
#include "ns3/rescue-mac-csma-helper.h"
#include "ns3/rescue-phy-basic-helper.h"
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RescueToyScenario3");




class SimulationHelper {
	
private:

public:
	SimulationHelper ();
	static void PopulateArpCache ();
    static std::string MapPhyRateToString (uint32_t phyRate);
};

std::string
SimulationHelper::MapPhyRateToString (uint32_t phyRate)
{
  switch (phyRate)
	{
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
        NS_ASSERT (false);
		return "";
        break;
	}
}

void
SimulationHelper::PopulateArpCache () {

	Ptr<ArpCache> arp = CreateObject<ArpCache> ();
	arp->SetAliveTimeout (Seconds(3600 * 24 * 365));
	
	for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
		
		Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
		NS_ASSERT(ip !=0);
		ObjectVectorValue interfaces;
		ip->GetAttribute("InterfaceList", interfaces);
    
			for(ObjectVectorValue::Iterator j = interfaces.Begin(); j != interfaces.End (); j ++) {
				
				Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
				NS_ASSERT(ipIface != 0);
				Ptr<NetDevice> device = ipIface->GetDevice();
				NS_ASSERT(device != 0);
				Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress ());
      
					for(uint32_t k = 0; k < ipIface->GetNAddresses (); k ++) {
						
						Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal();
						
						if(ipAddr == Ipv4Address::GetLoopback()) continue;
        
						ArpCache::Entry * entry = arp->Add(ipAddr);
						entry->MarkWaitReply(0);
						entry->MarkAlive(addr);
					}
			}
	}
	
	for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
		
		Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
		NS_ASSERT(ip !=0);
		ObjectVectorValue interfaces;
		ip->GetAttribute("InterfaceList", interfaces);
    
		for(ObjectVectorValue::Iterator j = interfaces.Begin(); j != interfaces.End (); j ++) {
			
			Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
			ipIface->SetAttribute("ArpCache", PointerValue(arp));
		}
	}
}



int main (int argc, char *argv[])
{
  float simTime = 1.5;
  float calcStart = 1;
  uint32_t seed = 1;

  float x1 = 700.0;
  float x2 = 350.0;
  float y2 = 550.0;
  float x3 = 350.0;
  float y3 = -550.0;
  double noise = -120.0;
  double CsPowerThr = -130;
  double TxPower = 20;
  double PerThr = 0.5;

  uint32_t cwMin = 8;
  uint32_t cwMax = 1024;
  double slotTime = 15;
  double sifsTime = 30;
  double lifsTime = 60;
  uint32_t basicPhyRate = 6;
  uint32_t dataPhyRate = 18;
  uint32_t queueLimit = 20;
  uint16_t retryLimit = 2;
  double ackTimeout = 3500;

  float Mbps = 1;
  uint16_t packetSize = 1000;
  uint32_t packetLimit = 1;



/* =============== Logging =============== */

  LogComponentEnable("RescueMacCsma", LOG_LEVEL_INFO); //comment to switch off logging
  //LogComponentEnable("RescueMacCsma", LOG_LEVEL_DEBUG); //comment to switch off logging
  //LogComponentEnable("RescueMacCsma", LOG_LEVEL_ALL); //comment to switch off logging

  LogComponentEnable("RescuePhy", LOG_LEVEL_INFO); //comment to switch off logging
  //LogComponentEnable("RescuePhy", LOG_LEVEL_DEBUG); //comment to switch off logging
  //LogComponentEnable("RescuePhy", LOG_LEVEL_ALL); //comment to switch off logging

  LogComponentEnable("RescueChannel", LOG_LEVEL_ALL); //comment to switch off logging
  LogComponentEnable("RescueRemoteStationManager", LOG_LEVEL_ALL); //comment to switch off logging
  LogComponentEnable("ConstantRateRescueManager", LOG_LEVEL_ALL); //comment to switch off logging
  LogComponentEnable("RescueHelper", LOG_LEVEL_ALL); //comment to switch off logging
  LogComponentEnable("BlackBox", LOG_LEVEL_ALL); //comment to switch off logging
  


/* ======= Command Line parameters ======= */

  CommandLine cmd;
  cmd.AddValue ("simTime", "Time of simulation", simTime);
  cmd.AddValue ("calcStart", "Begin of results analysis", calcStart);
  cmd.AddValue("seed", "Seed", seed);

  cmd.AddValue ("x1", "Distance [m] between nodes 0 (sender; [x0,y0]=[0,0]) and 1 (X-coordinate of node 1)", x1);
  cmd.AddValue ("x2", "X-coordinate [m] of node 2 (relay)", x2);
  cmd.AddValue ("y2", "Y-coordinate [m] of node 2 (relay)", y2);
  cmd.AddValue ("x3", "X-coordinate [m] of node 3 (relay)", x3);
  cmd.AddValue ("y3", "Y-coordinate [m] of node 3 (relay)", y3);
  cmd.AddValue ("noise", "Noise floor for channel [dBm]", noise);
  cmd.AddValue ("CsPowerThr", "Carrier Sense Threshold [dBm]", CsPowerThr);
  cmd.AddValue ("TxPower", "Transmission Power [dBm]", TxPower);
  cmd.AddValue ("PerThr", "PER threshold - frames with higher PER are unusable for reconstruction purposes and should not be stored or forwarded (default value = 0.5)", PerThr);

  cmd.AddValue ("cwMin", "Minimum value of CW", cwMin);
  cmd.AddValue ("cwMax", "Maximum value of CW", cwMax);
  cmd.AddValue ("slotTime", "Time slot duration [us] for MAC backoff", slotTime);
  cmd.AddValue ("sifsTime", "Short Inter-frame Space [us]", sifsTime);
  cmd.AddValue ("lifsTime", "Long Inter-frame Space [us]", lifsTime);
  cmd.AddValue ("basicPhyRate", "Transmission Rate [Mbps] for Control Packets", basicPhyRate);
  cmd.AddValue ("dataPhyRate", "Transmission Rate [Mbps] for Data Packets", dataPhyRate);
  cmd.AddValue ("queueLimit", "Maximum packets to queue at MAC", queueLimit);
  cmd.AddValue ("retryLimit", "Maximum Limit for DATA Retransmission", retryLimit);
  cmd.AddValue ("ackTimeout", "ACK awaiting time [us]", ackTimeout);

  cmd.AddValue ("dataRate", "Data Rate [Mbps]", Mbps);
  cmd.AddValue ("packetSize", "Packet Size [B]", packetSize);
  cmd.AddValue ("packetLimit", "Max number of transmitted packets (0 - no limit)", packetLimit);
  cmd.Parse (argc, argv);


  ns3::RngSeedManager::SetSeed (seed);
  


/* ===== Channel / Propagation Model ===== */

  Ptr<RescueChannel> rescueChan = CreateObject<RescueChannel> ();
  rescueChan->SetAttribute ("NoiseFloor", DoubleValue (noise)); 



/* ============ Nodes creation =========== */  

  NodeContainer nodes;
  nodes.Create (4);
  RescuePhyBasicHelper rescuePhy = RescuePhyBasicHelper::Default ();
  rescuePhy.SetType ("ns3::RescuePhy",
                     "CsPowerThr", DoubleValue (CsPowerThr),
                     "TxPower", DoubleValue (TxPower),
                     "PerThr", DoubleValue (PerThr));
  RescueMacCsmaHelper rescueMac = RescueMacCsmaHelper::Default ();
  rescueMac.SetType ("ns3::RescueMacCsma",
                     "CwMin", UintegerValue (cwMin),
                     "CwMax", UintegerValue (cwMax),
                     "SlotTime", TimeValue (MicroSeconds (slotTime)),
                     "SifsTime", TimeValue (MicroSeconds (sifsTime)),
                     "LifsTime", TimeValue (MicroSeconds (lifsTime)),
                     "QueueLimit", UintegerValue (queueLimit),
                     "AckTimeout", TimeValue (MicroSeconds (ackTimeout)));
  RescueHelper rescue;
  rescue.SetRemoteStationManager ("ns3::ConstantRateRescueManager",
							      "DataMode", StringValue (SimulationHelper::MapPhyRateToString (dataPhyRate)),
							      "ControlMode", StringValue (SimulationHelper::MapPhyRateToString (basicPhyRate)),
								  "MaxSrc", UintegerValue (retryLimit));
  NetDeviceContainer devices = rescue.Install (nodes, rescueChan, rescuePhy, rescueMac);
  


/* ======== Positioning / Mobility ======= */

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (x1, 0.0, 0.0));
  positionAlloc->Add (Vector (x2, y2, 0.0));
  positionAlloc->Add (Vector (x3, y3, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);
  


/* ============ Internet stack =========== */

  InternetStackHelper internet;
  internet.Install (nodes);
  
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer iface = ipv4.Assign (devices);
  


/* ============= Applications ============ */

  uint16_t dataSize = packetSize - (8 + 20 + 8);
  DataRate dataRate = DataRate (int(1000000 * Mbps * dataSize/packetSize));

  OnOffHelper onOffHelper ("ns3::UdpSocketFactory", InetSocketAddress (iface.GetAddress (1), 1000));
  onOffHelper.SetConstantRate (dataRate, dataSize);
  onOffHelper.SetAttribute ("MaxBytes", UintegerValue (packetLimit*dataSize));
  //onOffHelper.SetAttribute ("UserPriority", UintegerValue (up));
  onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.0)));
  onOffHelper.SetAttribute ("StopTime", TimeValue (Seconds (simTime)));
  //onOffHelper.SetAttribute ("RandSendTimeLimit", UintegerValue (random)); //packets generation times modified by random value between -50% and + 50% of constant time step between packets
  onOffHelper.Install (nodes.Get (0));

  PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (iface.GetAddress (1), 1000));
  sink.Install (nodes.Get (1));

  /*UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (5.0));

  UdpEchoClientHelper echoClient (iface.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.5)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (500));

  ApplicationContainer clientApps;
  clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (5.0));*/



/* ============= Flow Monitor ============ */

  FlowMonitorHelper flowmon_helper;
  Ptr<FlowMonitor> monitor = flowmon_helper.InstallAll ();
  monitor->SetAttribute ("StartTime", TimeValue (Seconds (calcStart))); //czas od którego będa zbierane statystyki
  monitor->SetAttribute ("DelayBinWidth", DoubleValue (0.001));
  monitor->SetAttribute ("JitterBinWidth", DoubleValue (0.001));
  monitor->SetAttribute ("PacketSizeBinWidth", DoubleValue (20));

  

/* ========== Running simulation ========= */

  SimulationHelper::PopulateArpCache ();
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();



/* =========== Printing Results ========== */

  monitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon_helper.GetClassifier ());
  std::string proto;
  
  std::cout << std::endl;
  
  std::map< FlowId, FlowMonitor::FlowStats > stats = monitor->GetFlowStats();
  for (std::map< FlowId, FlowMonitor::FlowStats >::iterator flow = stats.begin (); flow != stats.end (); flow++)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (flow->first);
      switch(t.protocol)
        {
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

      std::cout << "  Tx Bytes: " << flow->second.txBytes << std::endl;
      std::cout << "  Rx Bytes: " << flow->second.rxBytes << std::endl;
      std::cout << "  Tx Packets: " << flow->second.txPackets << std::endl;
      std::cout << "  Rx Packets: " << flow->second.rxPackets << std::endl;
      std::cout << "  Lost Packets: " << flow->second.lostPackets << std::endl;
    }

  return 0;
}


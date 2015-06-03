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

/* Node 0 periodically transmits UDP packet to Node 1 and Node 2 acts as a relay
 *
 *   Node 0 (Source) ---------> Node 1 (Destination)              
 *    (0, 0, 0) \                (x1, 0, 0)         
 *               \             ^
 *                \           /
 *                 \         /
 *                  v       /
 *                 Node 2 (Relay)
 *                  (x2, y2, 0)
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
//#include "ns3/flow-monitor-module.h"
#include "ns3/rng-seed-manager.h"

#include "ns3/rescue-channel.h"
#include "ns3/adhoc-rescue-mac-helper.h"
#include "ns3/rescue-phy-basic-helper.h"
#include "ns3/rescue-mac-header.h"
#include "ns3/rescue-phy-header.h"

#include <vector>
#include <bitset>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RescueToyScenario1");




class SimulationHelper {
	
private:

public:
	SimulationHelper ();
	static void PopulateArpCache ();
    static std::string MapPhyRateToString (uint32_t phyRate);
};

Time start = Seconds (5);
Time end = Seconds (25);

uint32_t m_send = 0;
uint32_t m_received = 0;
uint64_t m_send_bytes = 0;
uint64_t m_received_bytes = 0;
Time m_lastDelay = Seconds (0);
Time m_delaySum = Seconds (0);
Time m_jitterSum = Seconds (0);

typedef std::map<uint64_t, Time> Packets;
typedef std::map<uint64_t, Time>::reverse_iterator PacketsRI;
typedef std::map<uint64_t, Time>::iterator PacketsI;

Packets m_sentFrames;

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



/* =========== Trace functions =========== */

void
EnqueueOkTrace (std::string context, uint32_t node, uint32_t iff, Ptr<const Packet> pkt, const RescueMacHeader &hdr)
{
  //std::cout << Simulator::Now ().GetSeconds () << "\tENQUEUE, TX to: \t" << hdr.GetDestination () << "\t" << *pkt << std::endl;
  m_sentFrames.insert (std::pair<uint64_t, Time> (pkt->GetUid (), Simulator::Now ()) );
  if (Simulator::Now () > start)
    {
      m_send++;
      m_send_bytes += pkt->GetSize ();
    }
}

void
DataRxOkTrace (std::string context, uint32_t node, uint32_t iff, Ptr<const Packet> pkt, const RescuePhyHeader &hdr)
{
  //std::cout << Simulator::Now ().GetSeconds () << "\tRX OK, from: \t" << hdr.GetSource () << "\t" << *pkt << std::endl;
  Packets::iterator it = m_sentFrames.find (pkt->GetUid ());
  if (it != m_sentFrames.end ())
    {
      if (Simulator::Now () > start)
        {
          Time delay = Simulator::Now () - it->second;
          m_received++;
          m_received_bytes += pkt->GetSize ();
          m_delaySum += delay;
          m_jitterSum += ( (m_lastDelay > delay) ? (m_lastDelay - delay) : (delay - m_lastDelay) );
          m_lastDelay = delay;
        }
      m_sentFrames.erase (it);
    }
}



/* =========== Trace functions =========== */

int main (int argc, char *argv[])
{
  float calcStart = 5;
  float simTime = 25;
  uint32_t seed = 3;

  float xD = 680.0;
  float yD = 0.0;
  float xR = 340.0;
  float yR = -560.0;

  double snrSD = 0;
  double snrSR = 0;
  double snrRD = 0;

  double noise = -100.0;
  double CsPowerThr = -110;
  double RxPowerThr = -108;
  double TxPower = 20;
  double BerThr = 0.2;
  bool useLOTF = true;
  bool useBB2 = false;

  uint32_t cwMin = 16;
  uint32_t cwMax = 1024;
  double slotTime = 15;
  double sifsTime = 30;
  double lifsTime = 60;
  uint32_t queueLimit = 100;
  uint16_t retryLimit = 2;
  double ackTimeout = 3500;

  uint32_t basicPhyRate = 6;
  uint32_t dataPhyRate = 6;
  uint32_t relayDataPhyRate = 6;

  float Mbps = 15;
  uint16_t packetSize = 1000;
  uint32_t packetLimit = 0;



/* =============== Logging =============== */

  //LogComponentEnable("RescueMacCsma", LOG_LEVEL_INFO); //comment to switch off logging
  //LogComponentEnable("RescueMacCsma", LOG_LEVEL_DEBUG); //comment to switch off logging
  //LogComponentEnable("RescueMacCsma", LOG_LEVEL_ALL); //comment to switch off logging

  //LogComponentEnable("RescuePhy", LOG_LEVEL_INFO); //comment to switch off logging
  //LogComponentEnable("RescuePhy", LOG_LEVEL_DEBUG); //comment to switch off logging
  //LogComponentEnable("RescuePhy", LOG_LEVEL_ALL); //comment to switch off logging

  //LogComponentEnable("RescueChannel", LOG_LEVEL_ALL); //comment to switch off logging
  //LogComponentEnable("RescueRemoteStationManager", LOG_LEVEL_ALL); //comment to switch off logging
  //LogComponentEnable("ConstantRateRescueManager", LOG_LEVEL_ALL); //comment to switch off logging
  //LogComponentEnable("RescueHelper", LOG_LEVEL_ALL); //comment to switch off logging
  //LogComponentEnable("BlackBox_no1", LOG_LEVEL_ALL); //comment to switch off logging
  


/* ======= Command Line parameters ======= */

  CommandLine cmd;
  cmd.AddValue ("simTime", "Time of simulation", simTime);
  cmd.AddValue ("calcStart", "Begin of results analysis", calcStart);
  cmd.AddValue ("seed", "Seed", seed);

  cmd.AddValue ("xD", "X-coordinate [m] of node 1 (destination)", xD);
  cmd.AddValue ("yD", "Y-coordinate [m] of node 2 (destination)", yD);
  cmd.AddValue ("xR", "X-coordinate [m] of node 2 (relay)", xR);
  cmd.AddValue ("yR", "Y-coordinate [m] of node 2 (relay)", yR);

  cmd.AddValue ("snrSD", "SNR [dB] for direct transmission between node 0 (source) and node 1 (destination)", snrSD);
  cmd.AddValue ("snrSR", "SNR [dB] for direct transmission between node 0 (source) and node 2 (relay)", snrSR);
  cmd.AddValue ("snrRD", "SNR [dB] for direct transmission between node 2 (relay) and node 1 (destination)", snrRD);

  cmd.AddValue ("noise", "Noise floor for channel [dBm]", noise);
  cmd.AddValue ("CsPowerThr", "Carrier Sense Threshold [dBm]", CsPowerThr);
  cmd.AddValue ("RxPowerThr", "Reception Power Threshold [dBm]", RxPowerThr);
  cmd.AddValue ("TxPower", "Transmission Power [dBm]", TxPower);
  cmd.AddValue ("BerThr", "BER threshold - frames with higher BER are unusable for reconstruction purposes and should not be stored or forwarded (default value = 0.5)", BerThr);
  cmd.AddValue ("useLOTF", "Use Links-on-the-Fly", useLOTF);
  cmd.AddValue ("useBB2", "Use BlackBox #2 for error calculations", useBB2);

  cmd.AddValue ("cwMin", "Minimum value of CW", cwMin);
  cmd.AddValue ("cwMax", "Maximum value of CW", cwMax);
  cmd.AddValue ("slotTime", "Time slot duration [us] for MAC backoff", slotTime);
  cmd.AddValue ("sifsTime", "Short Inter-frame Space [us]", sifsTime);
  cmd.AddValue ("lifsTime", "Long Inter-frame Space [us]", lifsTime);
  cmd.AddValue ("queueLimit", "Maximum packets to queue at MAC", queueLimit);
  cmd.AddValue ("retryLimit", "Maximum Limit for DATA Retransmission", retryLimit);
  cmd.AddValue ("ackTimeout", "ACK awaiting time [us]", ackTimeout);

  cmd.AddValue ("basicPhyRate", "Transmission Rate [Mbps] for Control Packets", basicPhyRate);
  cmd.AddValue ("dataPhyRate", "Transmission Rate [Mbps] for Data Packets", dataPhyRate);
  cmd.AddValue ("relayDataPhyRate", "Transmission Rate [Mbps] for Data Packets used by relay node", relayDataPhyRate);

  cmd.AddValue ("dataRate", "Data Rate [Mbps]", Mbps);
  cmd.AddValue ("packetSize", "Packet Size [B]", packetSize);
  cmd.AddValue ("packetLimit", "Max number of transmitted packets (0 - no limit)", packetLimit);
  cmd.Parse (argc, argv);
  

  ns3::RngSeedManager::SetSeed (seed);
  end = Seconds (simTime);
  start = Seconds (calcStart);



/* ============ Nodes creation =========== */  

  NodeContainer nodes;
  nodes.Create (2);
  NodeContainer relayNode;
  relayNode.Create (1);



/* ======== Positioning / Mobility ======= */

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (xD, yD, 0.0));
  positionAlloc->Add (Vector (xR, yR, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (NodeContainer (nodes, relayNode));



/* ===== Channel / Propagation Model ===== */

  Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
  lossModel->SetLoss (nodes.Get(0)->GetObject<MobilityModel>(), nodes.Get(1)->GetObject<MobilityModel>(), TxPower - noise - snrSD); //Loss = TxPower - noise - snr
  lossModel->SetLoss (nodes.Get(0)->GetObject<MobilityModel>(), relayNode.Get(0)->GetObject<MobilityModel>(), TxPower - noise - snrSR); //Loss = TxPower - noise - snr
  lossModel->SetLoss (relayNode.Get(0)->GetObject<MobilityModel>(), nodes.Get(1)->GetObject<MobilityModel>(), TxPower - noise - snrRD); //Loss = TxPower - noise - snr

  Ptr<RescueChannel> rescueChan = CreateObject<RescueChannel> ();
  rescueChan->SetAttribute ("NoiseFloor", DoubleValue (noise)); 
  rescueChan->SetAttribute ("PropagationLossModel", PointerValue (lossModel));



/* ======= PHY & MAC configuration ======= */  

  RescuePhyBasicHelper rescuePhy = RescuePhyBasicHelper::Default ();
  rescuePhy.SetType ("ns3::RescuePhy",
                     "CsPowerThr", DoubleValue (CsPowerThr),
                     "RxPowerThr", DoubleValue (RxPowerThr),
                     "TxPower", DoubleValue (TxPower),
                     "BerThr", DoubleValue (BerThr),
                     "UseLOTF", BooleanValue (useLOTF),
                     "UseBB2", BooleanValue (useBB2));

  AdhocRescueMacHelper rescueMac = AdhocRescueMacHelper::Default ();
  rescueMac.SetType ("ns3::AdhocRescueMac",
                     "CwMin", UintegerValue (cwMin),
                     "CwMax", UintegerValue (cwMax),
                     "SlotTime", TimeValue (MicroSeconds (slotTime)),
                     "SifsTime", TimeValue (MicroSeconds (sifsTime)),
                     "LifsTime", TimeValue (MicroSeconds (lifsTime)),
                     "QueueLimits", UintegerValue (queueLimit),
                     "BasicAckTimeout", TimeValue (MicroSeconds (ackTimeout)));

  RescueHelper rescue;
  rescue.SetRemoteStationManager ("ns3::ConstantRateRescueManager",
							      "DataMode", StringValue (SimulationHelper::MapPhyRateToString (dataPhyRate)),
							      "ControlMode", StringValue (SimulationHelper::MapPhyRateToString (basicPhyRate)),
								  "MaxSrc", UintegerValue (++retryLimit));
  NetDeviceContainer devices = rescue.Install (nodes, rescueChan, rescuePhy, rescueMac);
  
  RescueHelper relayRescue;
  relayRescue.SetRemoteStationManager ("ns3::ConstantRateRescueManager",
							           "DataMode", StringValue (SimulationHelper::MapPhyRateToString (relayDataPhyRate)),
							           "ControlMode", StringValue (SimulationHelper::MapPhyRateToString (basicPhyRate)),
								       "MaxSrc", UintegerValue (++retryLimit));
  NetDeviceContainer relayDevice = relayRescue.Install (relayNode, rescueChan, rescuePhy, rescueMac);
  
  nodes.Add (relayNode);
  devices.Add (relayDevice);
  


/* ============ Internet stack =========== */

  InternetStackHelper internet;
  internet.Install (nodes);
  
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer iface = ipv4.Assign (devices);
  


/* ============= Applications ============ */

  uint16_t dataSize = packetSize - (8 + 20 + 8 + 15 + 4); //UDP hdr, IP hdr, LLC hdr, MAC hdr, MAC FCS
  DataRate dataRate = DataRate (int(1000000 * Mbps * dataSize/packetSize));

  OnOffHelper onOffHelper ("ns3::UdpSocketFactory", InetSocketAddress (iface.GetAddress (1), 1000));
  onOffHelper.SetConstantRate (dataRate, dataSize);
  onOffHelper.SetAttribute ("MaxBytes", UintegerValue (packetLimit*dataSize));
  onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.0)));
  onOffHelper.SetAttribute ("StopTime", TimeValue (end));
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



/* =========== Trace Connections ========= */

  Config::Connect ("/NodeList/*/DeviceList/*/Mac/EnqueueOk", MakeCallback (&EnqueueOkTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/Mac/DataRxOk", MakeCallback (&DataRxOkTrace));



/* ============= Flow Monitor ============ */

  /*FlowMonitorHelper flowmon_helper;
  Ptr<FlowMonitor> monitor = flowmon_helper.InstallAll ();
  monitor->SetAttribute ("StartTime", TimeValue (Seconds (calcStart))); //czas od którego będa zbierane statystyki
  monitor->SetAttribute ("DelayBinWidth", DoubleValue (0.001));
  monitor->SetAttribute ("JitterBinWidth", DoubleValue (0.001));
  monitor->SetAttribute ("PacketSizeBinWidth", DoubleValue (20));*/



/* ========== Running simulation ========= */

  SimulationHelper::PopulateArpCache ();
  Simulator::Stop (end);
  Simulator::Run ();
  Simulator::Destroy ();



/* =========== Printing Results ========== */

  /*monitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon_helper.GetClassifier ());
  std::string proto;
  
  //std::cout << std::endl;
  
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
      std::cout << "  Mean Delay: " << flow->second.delaySum / m_received << std::endl;
      std::cout << "  Mean Delay: " << flow->second.jitterSum / m_received << std::endl;

      std::cout << "  Throughput [Mbps]: " << 8000 * (double)flow->second.rxBytes / (flow->second.timeLastRxPacket - flow->second.timeFirstTxPacket).GetNanoSeconds () << std::endl;*/

      //std::cout << "snr01 [dBm]:\tsnr02 [dBm]:\tsnr12 [dBm]:\tthr [Mbps]:\tloss [%]:\tdel [ms]:\tjitter [ms]:" << std::endl;
      //std::cout << snr01 << "\t" << snr02 << "\t" << snr12 << "\t" << 8000 * (double)flow->second.rxBytes / (flow->second.timeLastRxPacket - flow->second.timeFirstTxPacket).GetNanoSeconds () << std::endl;
    //}

      //std::cout << "  Tx Bytes [B]: " << m_send_bytes << std::endl;
      //std::cout << "  Rx Bytes [B]: " << m_received_bytes << std::endl;
      //std::cout << "  Tx Packets: " << m_send << std::endl;
      //std::cout << "  Rx Packets: " << m_received << std::endl;
      //std::cout << "  Throughput [Mbps]: " << 8 * (double)m_received_bytes / (double)(simTime - calcStart) / 1000000 << std::endl;
      //std::cout << "  Lost Packets [%]: " <<  ((m_send > m_received) ? (100 * ((double)m_send - (double)m_received) / (double)m_send) : 0) << std::endl;
      //std::cout << "  Mean Delay [ms]: " << (double)(m_delaySum / m_received).GetMicroSeconds () / 1000 << std::endl;
      //std::cout << "  Mean Jitter [ms]: " << (double)(m_jitterSum / m_received).GetMicroSeconds () / 1000 << std::endl;

      //std::cout << "snrSD [dBm]:\tsnrSR [dBm]:\tsnrRD [dBm]:\tthr [Mbps]:\tloss [%]:\tdel [ms]:\tjitter [ms]:" << std::endl;
      std::cout << snrSD << "\t" << snrSR << "\t" << snrRD << "\t"
                << 8 * (double)m_received_bytes / (double)(simTime - calcStart) / 1000000 << "\t"
                << ((m_send > m_received) ? (100 * ((double)m_send - (double)m_received) / (double)m_send) : 0) << "\t"
                << (double)(m_delaySum / m_received).GetMicroSeconds () / 1000 << "\t"
                << (double)(m_jitterSum / m_received).GetMicroSeconds () / 1000 << std::endl;

  return 0;
}


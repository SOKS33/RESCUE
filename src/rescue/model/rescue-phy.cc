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

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/mac48-address.h"

#include "rescue-mac.h"
#include "rescue-phy.h"
#include "rescue-mac-header.h"
#include "rescue-phy-header.h"
#include "snr-per-tag.h"
#include "blackbox.h"

NS_LOG_COMPONENT_DEFINE ("RescuePhy");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now() << "] [addr=" << ((m_mac != 0) ? m_mac->GetAddress () : 0) << "] [PHY] "

#define Min(a,b) ((a < b) ? a : b)
#define Max(a,b) ((a > b) ? a : b)

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RescuePhy);

RescuePhy::RescuePhy ()
 : m_device (0), 
   m_mac (0),
   m_channel (0),
   m_pktRx (0)
{
  m_csBusy = false;
  m_csBusyEnd = Seconds (0);
  m_random = CreateObject<UniformRandomVariable> ();

  m_deviceRateSet.push_back (RescuePhy::GetOfdm6Mbps ());
  m_deviceRateSet.push_back (RescuePhy::GetOfdm9Mbps ());
  m_deviceRateSet.push_back (RescuePhy::GetOfdm12Mbps ());
  m_deviceRateSet.push_back (RescuePhy::GetOfdm18Mbps ());
  m_deviceRateSet.push_back (RescuePhy::GetOfdm24Mbps ());
  m_deviceRateSet.push_back (RescuePhy::GetOfdm36Mbps ());
}

RescuePhy::~RescuePhy ()
{
  m_deviceRateSet.clear ();
  Clear ();
}

void
RescuePhy::Clear ()
{
  m_pktRx = 0;
}

RescuePhy::RxFrameCopies::RxFrameCopies (RescuePhyHeader phyHdr,
                                         Ptr<Packet> pkt,
                                         std::vector<double> snr_db,
                                         std::vector<double> linkPER,
                                         double constellationSize, 
                                         double spectralEfficiency, 
                                         int packetLength,
                                         Time tstamp)
  : phyHdr (phyHdr),
    pkt (pkt),
    snr_db (snr_db),
    linkPER (linkPER),
    constellationSize (constellationSize),
    spectralEfficiency (spectralEfficiency),
    packetLength (packetLength),
    tstamp (tstamp)
{
}

TypeId
RescuePhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RescuePhy")
    .SetParent<Object> ()
    .AddConstructor<RescuePhy> ()
    .AddAttribute ("PreambleDuration",
                   "Duration [us] of Preamble of PHY Layer",
                   TimeValue (MicroSeconds (16)),
                   MakeTimeAccessor (&RescuePhy::m_preambleDuration),
                   MakeTimeChecker ())
    .AddAttribute ("TrailerSize",
                   "Size of Trailer (e.g. FCS) [B]",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RescuePhy::m_trailerSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("CsPowerThr",
                   "Carrier Sense Threshold [dBm]",
                   DoubleValue (-110),
                   MakeDoubleAccessor (&RescuePhy::m_csThr),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxPower",
                   "Transmission Power [dBm]",
                   DoubleValue (10),
                   MakeDoubleAccessor (&RescuePhy::SetTxPower),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("PerThr",
                   "PER threshold - frames with higher PER are unusable for reconstruction purposes and should not be stored or forwarded",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&RescuePhy::m_perThr),
                   MakeDoubleChecker<double> ())
    ;
  return tid;
}



// ------------------------ Set Functions -----------------------------
void
RescuePhy::SetDevice (Ptr<RescueNetDevice> device)
{
  m_device = device;
}

void
RescuePhy::SetMac (Ptr<RescueMac> mac)
{
  m_mac = mac;
}

void
RescuePhy::SetChannel (Ptr<RescueChannel> channel)
{
  m_channel = channel;
}

void
RescuePhy::SetTxPower (double dBm)
{
  m_txPower = dBm;
}

void
RescuePhy::SetErrorRateModel (Ptr<RescueErrorRateModel> rate)
{
  m_errorRateModel = rate;
}



// ------------------------ Get Functions -----------------------------
Ptr<RescueChannel>
RescuePhy::GetChannel ()
{
  return m_channel;
}
double
RescuePhy::GetTxPower ()
{
  return m_txPower;
}

Ptr<RescueErrorRateModel>
RescuePhy::GetErrorRateModel (void) const
{
  return m_errorRateModel;
}



// ----------------------- Send Functions ------------------------------
bool
RescuePhy::SendPacket (Ptr<Packet> pkt, RescueMode mode, uint16_t il)
{
  NS_LOG_FUNCTION ("");
  // RX might be interrupted by TX, but not vice versa
  if (m_state == TX) 
    {
      NS_LOG_DEBUG ("Already in transmission mode");
      return false;
    }
  
  Ptr<Packet> p = pkt->Copy (); //copy packet stored in MAC layer

  m_state = TX;
  RescueMacHeader currentMacHdr;
  p->PeekHeader (currentMacHdr);
  NS_LOG_FUNCTION ("v1 src:" << currentMacHdr.GetSource () << 
                   "dst:" << currentMacHdr.GetDestination () << 
                   "seq:" << currentMacHdr.GetSequence ());
  RescuePhyHeader currentPhyHdr = RescuePhyHeader (currentMacHdr.GetSource (), 
                                                   currentMacHdr.GetSource (), 
                                                   currentMacHdr.GetDestination (), 
                                                   currentMacHdr.GetType (),
                                                   currentMacHdr.GetSequence (),
                                                   il);
  Time txDuration;

  txDuration = CalTxDuration (currentPhyHdr.GetSize (), p->GetSize (), 
                              GetPhyHeaderMode (mode), mode);
  
  NS_LOG_DEBUG ("Tx will finish at " << (Simulator::Now () + txDuration).GetSeconds () << 
                " (" << txDuration.GetNanoSeconds() << "ns), txPower: " << m_txPower);
  
  p->AddHeader (currentPhyHdr);

  // forward to CHANNEL
  m_channel->SendPacket (Ptr<RescuePhy> (this), p, m_txPower, mode, txDuration);
  
  return true;
}

bool
RescuePhy::SendPacket (std::pair<Ptr<Packet>, RescuePhyHeader> relayedPkt, RescueMode mode)
{
  NS_LOG_FUNCTION ("v2 src:" << relayedPkt.second.GetSource () << 
                   "dst:" << relayedPkt.second.GetDestination () << 
                   "seq:" << relayedPkt.second.GetSequence ());
  // RX might be interrupted by TX, but not vice versa
  if (m_state == TX) 
    {
      NS_LOG_DEBUG ("Already in transmission mode");
      return false;
    }
  
  m_state = TX;
  Time txDuration;
  
  txDuration = CalTxDuration (relayedPkt.second.GetSize (), relayedPkt.first->GetSize (), 
                              GetPhyHeaderMode (mode), mode);

  NS_LOG_DEBUG ("Tx will finish at " << (Simulator::Now () + txDuration).GetSeconds () << 
                " (" << txDuration.GetNanoSeconds() << "ns), txPower: " << m_txPower);
  
  relayedPkt.first->AddHeader (relayedPkt.second);

  // forward to CHANNEL
  m_channel->SendPacket (Ptr<RescuePhy> (this), relayedPkt.first, m_txPower, mode, txDuration);
  
  return true;
}

void 
RescuePhy::SendPacketDone (Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION ("");
  m_state = IDLE;
  m_mac->SendPacketDone (pkt);
}



// ---------------------- Receive Functions ----------------------------
void 
RescuePhy::ReceivePacket (Ptr<Packet> pkt, RescueMode mode, Time txDuration, double_t rxPower)
{
  NS_LOG_FUNCTION ("rxPower:" << rxPower << 
                   "mode:" << mode << 
                   "busyEnd:" << m_csBusyEnd);
  
  if (m_state == TX)
    {
      NS_LOG_INFO ("Drop packet due to half-duplex");
      return;
    }
  
  // Start RX when energy is bigger than carrier sense threshold 
  // 
  Time txEnd = Simulator::Now () + txDuration;
  if (rxPower > m_csThr && txEnd > m_csBusyEnd)
    {
      if (m_csBusy == false)
        {
          m_csBusy = true;
          m_pktRx = pkt;
          m_mac->ReceivePacket (this, pkt);
        }
      m_state = RX;
      m_csBusyEnd = txEnd;
    }
}

void 
RescuePhy::ReceivePacketDone (Ptr<Packet> pkt, RescueMode mode, double rxPower)
{
  NS_LOG_FUNCTION ("busyEnd:" << m_csBusyEnd);
  
  if (m_csBusyEnd <= Simulator::Now () + NanoSeconds (1))
    {
      m_csBusy = false;
    }
  
  if (m_state != RX)
    {
      NS_LOG_INFO ("Drop packet due to state");
      return;
    }
  
  // We do support SINR !!
  double noiseW = m_channel->GetNoiseW (this, pkt); // noise plus interference
  //double rxPowerW = m_channel->DbmToW (rxPower);
  double sinr = rxPower - m_channel->WToDbm (noiseW);

  if (pkt == m_pktRx)
    {

      RescuePhyHeader hdr;
      pkt->RemoveHeader (hdr);

      double hsr = 1.0; //header success rate

      //Calculate Preamble Success Rate
      hsr *= CalculateChunkSuccessRate (sinr,
                                        GetPhyPreambleDuration (mode),
                                        GetPhyPreambleMode (mode));

      //Calculate PHY Header Success Rate
      hsr *= CalculateChunkSuccessRate (sinr,
                                        GetPhyHeaderDuration (hdr, mode),
                                        GetPhyHeaderMode (mode));

      double her = 1 - hsr; //header error rate
      NS_LOG_DEBUG ("mode=" << mode << 
                    ", snr=" << sinr << 
                    ", her=" << her << 
                    ", size=" << pkt->GetSize ());

      if (m_random->GetValue () > her) //is the PHY header complete
        {
          //PHY HEADER CORRECTLY RECEIVED
          NS_LOG_INFO ("PHY HDR CORRECT");
          m_state = IDLE;

          NS_LOG_FUNCTION ("src:" << hdr.GetSource () << 
                           "sender:" << hdr.GetSender () <<
                           "dst:" << hdr.GetDestination () <<
                           "seq:" << hdr.GetSequence ());

          if (hdr.GetType () != RESCUE_PHY_PKT_TYPE_DATA)  //CONTROL FRAME - no payload processing
            {
              m_mac->ReceivePacketDone (this, pkt, hdr, sinr, mode, true, true, false);
            }
          else //PER (and SNR?) tag must be updated
            {
              SnrPerTag tag;
              double per = (pkt->PeekPacketTag (tag) ? tag.GetPER () : 0); //packet (payload) error rate
              double snr = (pkt->PeekPacketTag (tag) ? tag.GetSNR () : (m_txPower - m_channel->WToDbm (noiseW))); //packet SNR (if not recorded, use maximal possible value)
              NS_LOG_DEBUG ("SNR from previous hops: " << snr << ", PER from previous hops: " << per);

              double psr = 1 - per; //packet (payload) success rate
              psr *= CalculateChunkSuccessRate (sinr,
                                                GetDataDuration (pkt, mode),
                                                mode); //should we modify PER value with by the result of classic error-rate-model?
              per = 1 - psr;
              NS_LOG_DEBUG ("PER after last hop: " << per);

              //update packet tag
              //tag.SetSNR (sinr); //not shure what to do - store last hp snr?!?
              tag.SetSNR (Min (sinr, snr)); //not shure what to do - store minimal snr?!?
              tag.SetPER (per);
              pkt->ReplacePacketTag (tag);

              //NS_LOG_DEBUG ("data rate: " << mode.GetDataRate () << " phy rate: " << mode.GetPhyRate () << " constellation size: " << (int)mode.GetConstellationSize () << " spectralEfficiency: " << mode.GetSpectralEfficiency ());

              /*std::vector<double> snr_db;
              std::vector<double> linkPER;
              snr_db.push_back (sinr);
              linkPER.push_back (per);
              per = BlackBox::CalculateRescuePER (snr_db, linkPER, mode.GetConstellationSize (), mode.GetSpectralEfficiency (), 8 * pkt->GetSize ());
              NS_LOG_DEBUG ("PER from BlackBox: " << per);*/  
          
              if (hdr.GetDestination () != m_mac->GetAddress ())   //RELAY THIS FRAME? - no payload processing but PER tag must be updated
                {
                  m_mac->ReceivePacketDone (this, pkt, hdr, sinr, mode, true, (per <= m_perThr), false); //(per < 0.5) - inform MAC about PER - not forward if PER > PER THRESHOLD - unusable frame
                }
              else  //DATA FRAME DESTINED FOR THIS DEVICE
                {
                  if (m_random->GetValue () > per) //is the payload complete?
                    {
                      //no errors - just forward up
                      RemoveFrameCopies (hdr);
                      m_mac->ReceivePacketDone (this, pkt, hdr, sinr, mode, true, true, false);
                    }
                  else if (per <= m_perThr) //PER > PER THRESHOLD - unusable frame
                    {
                      NS_LOG_INFO ("PHY HDR OK!, DATA: DAMAGED!");
                      if (AddFrameCopy (pkt, hdr, tag.GetSNR (), tag.GetPER (), mode)) //STOR FRAME COPY - check: is it the first one?
                        {
                          //it is NOT first copy - try to restore data using previously stored frame copies and notify
                          m_mac->ReceivePacketDone (this, pkt, hdr, sinr, mode, true, IsRestored (hdr), true);
                        }
                      else
                        {
                          //it is first copy - just notify MAC
                          m_mac->ReceivePacketDone (this, pkt, hdr, sinr, mode, true, false, true);
                        }
                    }
                  else
                    {
                      NS_LOG_INFO ("PHY HDR OK!, DATA: UNUSABLE (PER >= 0.5) - DROP IT!");
                    }
                }
            }
          return;
        }
      else
        {
          //to notify unusable frame
          m_mac->ReceivePacketDone (this, pkt, hdr, sinr, mode, false, false, false);
          NS_LOG_INFO ("PHY HDR DAMAGED!");
        }
    }
  else if (! m_csBusy)
    {
      RescuePhyHeader hdr;
      pkt->RemoveHeader (hdr);
      m_mac->ReceivePacketDone (this, pkt, hdr, sinr, mode, false, false, false); //to notify unusable frame
    }
  
  if (! m_csBusy) // set MAC state IDLE
    {
      m_state = IDLE;
    }
}

void
RescuePhy::RemoveFrameCopies (RescuePhyHeader phyHdr)
{
  NS_LOG_FUNCTION ("");
  for (RxFrameAccumulatorI it = m_rxFrameAccumulator.begin (); it != m_rxFrameAccumulator.end (); it++)
    {
      //NS_LOG_DEBUG("queue iterator = " << ++i << " source: " << it->src << " destination: " << it->dst << " seq. number: " << it->seq_no << " tstamp: " << it->tstamp);
      if ((it->phyHdr.GetSource () == phyHdr.GetSource ())
          && (it->phyHdr.GetSequence () == phyHdr.GetSequence ()))
        {
          it = m_rxFrameAccumulator.erase (it);
          break;
        }
    }
}

bool
RescuePhy::AddFrameCopy (Ptr<Packet> pkt, RescuePhyHeader phyHdr, double sinr, double per, RescueMode mode)
{
  NS_LOG_INFO ("STORE FRAME COPY");

  for (RxFrameAccumulatorI it = m_rxFrameAccumulator.begin (); it != m_rxFrameAccumulator.end (); it++)
    {
      //NS_LOG_DEBUG("queue iterator = " << ++i << " source: " << it->src << " destination: " << it->dst << " seq. number: " << it->seq_no << " tstamp: " << it->tstamp);
      if ((it->phyHdr.GetSource () == phyHdr.GetSource ())
          && (it->phyHdr.GetSequence () == phyHdr.GetSequence ()))
            {
                  NS_LOG_INFO (it->snr_db.size () << " PREVIOUS COPY/IES FOUND, ADD THIS COPY");
                  it->snr_db.push_back (sinr);
                  it->linkPER.push_back (per);
                  return true;
            }
    }

  NS_LOG_INFO ("NO PREVIOUS COPIES, STORE FIRST COPY");
  std::vector<double> snr_db;
  snr_db.push_back (sinr);

  std::vector<double> linkPER;
  linkPER.push_back (per);

  m_rxFrameAccumulator.push_back (RxFrameCopies (phyHdr, pkt, 
                                                 snr_db, linkPER, 
                                                 mode.GetConstellationSize (), 
                                                 mode.GetSpectralEfficiency (), 
                                                 pkt->GetSize (), 
                                                 ns3::Simulator::Now () ));
  return false;
}

double
RescuePhy::CalculateChunkSuccessRate (double snir, Time duration, RescueMode mode)
{
  NS_LOG_FUNCTION ("snir: " << snir << "duration: " << duration << "mode " << mode);
  if (duration == NanoSeconds (0))
    {
      return 1.0;
    }
  uint32_t rate = mode.GetPhyRate ();
  uint64_t nbits = (uint64_t)(rate * duration.GetSeconds ());
  double csr = m_errorRateModel->GetChunkSuccessRate (mode, snir, (uint32_t)nbits);
  NS_LOG_FUNCTION ("csr: " << csr);
  return csr;
}

double 
RescuePhy::CalculateLLR (double sinr)
{
  //for future use
  return 0;
}

double 
RescuePhy::CalculateCI (double prevCI, double llr, uint32_t symbols)
{
  //for future use
  return 0;
}

bool
RescuePhy::IsRestored (RescuePhyHeader phyHdr)
{
  NS_LOG_FUNCTION ("");
  bool restored = false;

  //try to reconstruct data if there is any copy of this frame in the buffer?
  for (RxFrameAccumulatorI it = m_rxFrameAccumulator.begin (); it != m_rxFrameAccumulator.end (); it++)
    {
      //NS_LOG_DEBUG("queue iterator = " << ++i << " source: " << it->src << " destination: " << it->dst << " seq. number: " << it->seq_no << " tstamp: " << it->tstamp);
      if ((it->phyHdr.GetSource () == phyHdr.GetSource ())
          && (it->phyHdr.GetSequence () == phyHdr.GetSequence ()))
            {
              double blackBoxPER = BlackBox::CalculateRescuePER (it->snr_db, 
                                                                 it->linkPER, 
                                                                 it->constellationSize, 
                                                                 it->spectralEfficiency, 
                                                                 8 * it->packetLength);
              NS_LOG_DEBUG ("BlackBox PER = " << blackBoxPER);
              if (m_random->GetValue () > blackBoxPER)
                {
                  //MAGIC THINGS HAPPENS HERE !!!
                  restored = true;
                  it = m_rxFrameAccumulator.erase (it);
                  break;
                }
            }
    }
              
  if (restored)
    {
      NS_LOG_INFO ("FRAME RESTORED!");
    }
  else
    {
      NS_LOG_INFO ("UNABLE TO RESTORE FRAME!");
    }
  return restored;
}



// --------------------------- ETC -------------------------------------
bool 
RescuePhy::IsIdle ()
{
  if (m_state == IDLE && !m_csBusy) { return true; }
  return false;
}

Time
RescuePhy::CalTxDuration (uint32_t basicSize, uint32_t dataSize, RescueMode basicMode, RescueMode dataMode, uint16_t type)
{
  //for TX time calculation when PhyHdr is not constructed yet
  RescuePhyHeader hdr = RescuePhyHeader (type);
  double_t txHdrTime = (double)(hdr.GetSize () + basicSize + m_trailerSize) * 8.0 / basicMode.GetDataRate ();
  double_t txMpduTime = (double)dataSize * 8.0 / dataMode.GetDataRate ();
  return m_preambleDuration + Seconds (txHdrTime) + Seconds (txMpduTime);
}

Time
RescuePhy::CalTxDuration (uint32_t basicSize, uint32_t dataSize, RescueMode basicMode, RescueMode dataMode)
{
  //for TX time calculation when PhyHdr is alreade constructed
  double_t txHdrTime = (double)(basicSize + m_trailerSize) * 8.0 / basicMode.GetDataRate ();
  double_t txMpduTime = (double)dataSize * 8.0 / dataMode.GetDataRate ();
  return m_preambleDuration + Seconds (txHdrTime) + Seconds (txMpduTime);
}

int64_t
RescuePhy::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_random->SetStream (stream);
  return 1;
}



// ------------------------ TX MODES -----------------------------------
Time
RescuePhy::GetPhyPreambleDuration (RescueMode mode)
{
  return m_preambleDuration;
}

Time
RescuePhy::GetPhyHeaderDuration (RescuePhyHeader phyHdr, RescueMode mode)
{
  NS_LOG_FUNCTION ("size: " << phyHdr.GetSize () << "rate: " << mode.GetDataRate ());
  return MicroSeconds (1000000 * 8 * phyHdr.GetSize () / mode.GetDataRate ());
}

Time
RescuePhy::GetDataDuration (Ptr<Packet> pkt, RescueMode mode)
{
  NS_LOG_FUNCTION ("size: " << pkt->GetSize () << "rate: " << mode.GetDataRate ());
  return MicroSeconds (1000000 * 8 * pkt->GetSize () / mode.GetDataRate ());
}

RescueMode
RescuePhy::GetPhyPreambleMode (RescueMode payloadMode)
{
  return RescuePhy::GetOfdm6Mbps ();
}

RescueMode
RescuePhy::GetPhyHeaderMode (RescueMode payloadMode)
{
  return RescuePhy::GetOfdm6Mbps ();
}

uint32_t
RescuePhy::GetNModes (void) const
{
  return m_deviceRateSet.size ();
}

RescueMode
RescuePhy::GetMode (uint32_t mode) const
{
  return m_deviceRateSet[mode];
}

bool
RescuePhy::IsModeSupported (RescueMode mode) const
{
  for (uint32_t i = 0; i < GetNModes (); i++)
    {
      if (mode == GetMode (i))
        {
          return true;
        }
    }
  return false;
}

RescueMode
RescuePhy::GetOfdm6Mbps ()
{
  static RescueMode mode =
    RescueModeFactory::CreateRescueMode ("Ofdm6Mbps",
                                         RESCUE_MOD_CLASS_OFDM,
                                         20000000, 6000000,
                                         RESCUE_CODE_RATE_1_2, 2);
  return mode;
}

RescueMode
RescuePhy::GetOfdm9Mbps ()
{
  static RescueMode mode =
    RescueModeFactory::CreateRescueMode ("Ofdm9Mbps",
                                         RESCUE_MOD_CLASS_OFDM,
                                         20000000, 9000000,
                                         RESCUE_CODE_RATE_3_4, 2);
  return mode;
}

RescueMode
RescuePhy::GetOfdm12Mbps ()
{
  static RescueMode mode =
    RescueModeFactory::CreateRescueMode ("Ofdm12Mbps",
                                         RESCUE_MOD_CLASS_OFDM,
                                         20000000, 12000000,
                                         RESCUE_CODE_RATE_1_2, 4);
  return mode;
}

RescueMode
RescuePhy::GetOfdm18Mbps ()
{
  static RescueMode mode =
    RescueModeFactory::CreateRescueMode ("Ofdm18Mbps",
                                         RESCUE_MOD_CLASS_OFDM,
                                         20000000, 18000000,
                                         RESCUE_CODE_RATE_3_4, 4);
  return mode;
}

RescueMode
RescuePhy::GetOfdm24Mbps ()
{
  static RescueMode mode =
    RescueModeFactory::CreateRescueMode ("Ofdm24Mbps",
                                         RESCUE_MOD_CLASS_OFDM,
                                         20000000, 24000000,
                                         RESCUE_CODE_RATE_1_2, 16);
  return mode;
}

RescueMode
RescuePhy::GetOfdm36Mbps ()
{
  static RescueMode mode =
    RescueModeFactory::CreateRescueMode ("Ofdm36Mbps",
                                         RESCUE_MOD_CLASS_OFDM,
                                         20000000, 36000000,
                                         RESCUE_CODE_RATE_3_4, 16);
  return mode;
}

} // namespace ns3


namespace {

static class Constructor
{
public:
  Constructor ()
  {
    ns3::RescuePhy::GetOfdm6Mbps ();
    ns3::RescuePhy::GetOfdm9Mbps ();
    ns3::RescuePhy::GetOfdm12Mbps ();
    ns3::RescuePhy::GetOfdm18Mbps ();
    ns3::RescuePhy::GetOfdm24Mbps ();
    ns3::RescuePhy::GetOfdm36Mbps ();

  }
} g_constructor;
}

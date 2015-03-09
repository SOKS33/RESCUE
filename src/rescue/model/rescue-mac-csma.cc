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

#include "ns3/attribute.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/nstime.h"
#include "ns3/random-variable.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/node.h"

#include "rescue-mac-header.h"
#include "rescue-mac-csma.h"
#include "snr-per-tag.h"

NS_LOG_COMPONENT_DEFINE ("RescueMacCsma");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now() << "] [addr=" << m_address << "] [MAC] ";

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RescueMacCsma);

RescueMacCsma::RescueMacCsma ()
  : RescueMac (),
    m_phy (0),
    m_ccaTimeoutEvent (),
    m_backoffTimeoutEvent (),
    m_ackTimeoutEvent (),
    m_sendAckEvent (),
    m_sendDataEvent (),
    m_state (IDLE),
    m_pktTx (0),
    m_pktData (0)
{
  m_cw = m_cwMin;
  m_backoffRemain = Seconds (0);
  m_backoffStart = Seconds (0);
  m_sequence = 0;
  m_interleaver = 0;
  m_random = CreateObject<UniformRandomVariable> ();
}

RescueMacCsma::~RescueMacCsma ()
{
  Clear ();
  m_remoteStationManager = 0;
}

void
RescueMacCsma::Clear ()
{
  m_pktTx = 0;
  NS_LOG_DEBUG ("RESET PACKET");
  m_pktData = 0;
  m_pktQueue.clear ();
  m_seqList.clear ();
}

RescueMacCsma::FrameInfo::FrameInfo (Mac48Address dst,
                                     Mac48Address src,
                                     uint16_t seq_no,
                                     uint16_t il,
                                     bool ACKed,
                                     Time tstamp)
  : dst (dst),
    src (src),
    seq_no (seq_no),
    il (il),
    ACKed (ACKed),
    tstamp (tstamp)
{
}

RescueMacCsma::FrameInfo::FrameInfo (Mac48Address dst,
                                     Mac48Address src,
                                     uint16_t seq_no,
                                     uint16_t il,
                                     Time tstamp)
  : dst (dst),
    src (src),
    seq_no (seq_no),
    il (il),
    ACKed (false),
    tstamp (tstamp)
{
}

RescueMacCsma::FrameInfo::FrameInfo (Mac48Address dst,
                                     uint16_t seq_no,
                                     bool ACKed,
                                     Time tstamp)
  : dst (dst),
    seq_no (seq_no),
    ACKed (ACKed),
    tstamp (tstamp)
{
}

RescueMacCsma::FrameInfo::FrameInfo (Mac48Address src,
                                     uint16_t seq_no,
                                     Time tstamp)
  : src (src),
    seq_no (seq_no),
    tstamp (tstamp)
{
}

TypeId
RescueMacCsma::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RescueMacCsma")
    .SetParent<Object> ()
    .AddConstructor<RescueMacCsma> ()
    .AddAttribute ("CwMin",
                   "Minimum value of CW",
                   UintegerValue (8),
                   MakeUintegerAccessor (&RescueMacCsma::m_cwMin),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("CwMax",
                   "Maximum value of CW",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&RescueMacCsma::m_cwMax),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("SlotTime",
                   "Time slot duration [us] for MAC backoff",
                   TimeValue (MicroSeconds (15)),
                   MakeTimeAccessor (&RescueMacCsma::m_slotTime),
                   MakeTimeChecker ())
    .AddAttribute ("SifsTime",
                   "Short Inter-frame Space [us]",
                   TimeValue (MicroSeconds (30)),
                   MakeTimeAccessor (&RescueMacCsma::m_sifs),
                   MakeTimeChecker ())
    .AddAttribute ("LifsTime",
                   "Long Inter-frame Space [us]",
                   TimeValue (MicroSeconds (60)),
                   MakeTimeAccessor (&RescueMacCsma::m_lifs),
                   MakeTimeChecker ())
    .AddAttribute ("QueueLimit",
                   "Maximum packets to queue at MAC",
                   UintegerValue (20),
                   MakeUintegerAccessor (&RescueMacCsma::m_queueLimit),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("AckTimeout",
                   "ACK awaiting time",
                   TimeValue (MicroSeconds (3000)),
                   MakeTimeAccessor (&RescueMacCsma::m_ackTimeout),
                   MakeTimeChecker ())

    .AddTraceSource ("AckTimeout",
                     "Trace Hookup for ACK Timeout",
                     MakeTraceSourceAccessor (&RescueMacCsma::m_traceAckTimeout))
    .AddTraceSource ("SendDataDone",
                     "Trace Hookup for sending a DATA",
                     MakeTraceSourceAccessor (&RescueMacCsma::m_traceSendDataDone))
    .AddTraceSource ("Enqueue",
                     "Trace Hookup for enqueue a DATA",
                     MakeTraceSourceAccessor (&RescueMacCsma::m_traceEnqueue))

    .AddTraceSource ("DataTx",
                     "Trace Hookup for originated DATA TX",
                     MakeTraceSourceAccessor (&RescueMacCsma::m_traceDataTx))
    .AddTraceSource ("DataRelay",
                     "Trace Hookup for DATA relay TX",
                     MakeTraceSourceAccessor (&RescueMacCsma::m_traceDataRelay))
    .AddTraceSource ("DataRx",
                     "Trace Hookup for DATA RX by final node",
                     MakeTraceSourceAccessor (&RescueMacCsma::m_traceDataRx))
    .AddTraceSource ("AckTx",
                     "Trace Hookup for ACK TX",
                     MakeTraceSourceAccessor (&RescueMacCsma::m_traceAckTx))
    .AddTraceSource ("ForwardAck",
                     "Trace Hookup for ACK Forward",
                     MakeTraceSourceAccessor (&RescueMacCsma::m_traceAckForward))

  ;
  return tid;
}



// ------------------------ Set Functions -----------------------------
void
RescueMacCsma::AttachPhy (Ptr<RescuePhy> phy)
{
  m_phy = phy;
}

void
RescueMacCsma::SetDevice (Ptr<RescueNetDevice> dev)
{
  m_device = dev;
  SetCw (m_cwMin);
}

void
RescueMacCsma::SetRemoteStationManager (Ptr<RescueRemoteStationManager> manager)
{
  m_remoteStationManager = manager;
}

void
RescueMacCsma::SetAddress (Mac48Address addr)
{
  NS_LOG_FUNCTION (addr);
  m_address = addr;
}

void
RescueMacCsma::SetForwardUpCb (Callback<void, Ptr<Packet>, Mac48Address, Mac48Address> cb)
{
  m_forwardUpCb = cb;
}

void
RescueMacCsma::SetCwMin (uint32_t cw)
{
  m_cwMin = cw;
}
void
RescueMacCsma::SetCw (uint32_t cw)
{
  m_cw = cw;
}

void
RescueMacCsma::SetSlotTime (Time duration)
{
  m_slotTime = duration;
}

int64_t
RescueMacCsma::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_random->SetStream (stream);
  return 1;
}


// ------------------------ Get Functions -----------------------------
uint32_t
RescueMacCsma::GetCw (void)
{
  return m_cw;
}

Time
RescueMacCsma::GetSlotTime (void)
{
  return m_slotTime;
}

Time
RescueMacCsma::GetSifs (void) const
{
  return m_sifs;
}

Time
RescueMacCsma::GetLifs (void) const
{
  return m_lifs;
}

Mac48Address
RescueMacCsma::GetAddress () const
{
  return this->m_address;
}

Mac48Address
RescueMacCsma::GetBroadcast (void) const
{
  return Mac48Address::GetBroadcast ();
}

Time
RescueMacCsma::GetCtrlDuration (uint16_t type, RescueMode mode)
{
  RescueMacHeader hdr = RescueMacHeader (m_address, m_address, type);
  return m_phy->CalTxDuration (hdr.GetSize (), 0, m_phy->GetPhyHeaderMode (mode), mode, type);
}

Time
RescueMacCsma::GetDataDuration (Ptr<Packet> pkt, RescueMode mode)
{
  return m_phy->CalTxDuration (0, pkt->GetSize (), m_phy->GetPhyHeaderMode (mode), mode, 0);
}

std::string
RescueMacCsma::StateToString (State state)
{
  switch (state)
    {
    case IDLE:
      return "IDLE";
    case BACKOFF:
      return "BACKOFF";
    case WAIT_TX:
      return "WAIT_TX";
    case TX:
      return "TX";
    case WAIT_RX:
      return "WAIT_RX";
    case RX:
      return "RX";
    case COLL:
      return "COLL";
    default:
      return "??";
    }
}



// ----------------------- Queue Functions -----------------------------
bool
RescueMacCsma::Enqueue (Ptr<Packet> pkt, Mac48Address dst)
{
  NS_LOG_FUNCTION ("dst:" << dst << 
                   "size [B]:" << pkt->GetSize () <<
                   "#queue:" << m_pktQueue.size () << 
                   "#relay queue:" << m_pktRelayQueue.size () << 
                   "#ack queue:" << m_ackForwardQueue.size () << 
                   "state:" << StateToString (m_state));
  if (m_pktQueue.size () >= m_queueLimit)
    {
      return false;
    }  

  RescueMacHeader hdr = RescueMacHeader (m_address, dst, RESCUE_MAC_PKT_TYPE_DATA);
  pkt->AddHeader (hdr);
  m_traceEnqueue (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt, hdr);
  m_pktQueue.push_back (pkt);
  
  if (m_state == IDLE)
    { 
      CcaForLifs ();
    }
  
  return false;
}

void
RescueMacCsma::Dequeue ()
{
  NS_LOG_FUNCTION ("#queue:" << m_pktQueue.size () <<
                   "#relay queue:" << m_pktRelayQueue.size () <<
                   "#ack queue:" << m_ackForwardQueue.size ());
  m_pktQueue.remove(m_pktData);
}



// ------------------ Channel Access Functions -------------------------
void
RescueMacCsma::CcaForLifs ()
{
  NS_LOG_FUNCTION ("#queue:" << m_pktQueue.size () <<
                   "#relay queue:" << m_pktRelayQueue.size () << 
                   "#ack queue:" << m_ackForwardQueue.size () <<
                   "state:" << StateToString (m_state) <<
                   "phy idle:" << (m_phy->IsIdle () ? "TRUE" : "FALSE"));
  Time now = Simulator::Now ();
  
  if ( ((m_pktQueue.size () == 0) && (m_pktRelayQueue.size () == 0) && (m_ackForwardQueue.size () == 0))
       || m_ccaTimeoutEvent.IsRunning ())
    {
      return;
    }
  if (m_state != IDLE || !m_phy->IsIdle ())
    {
      m_ccaTimeoutEvent = Simulator::Schedule (GetLifs (), &RescueMacCsma::CcaForLifs, this);
      return;
    }
  m_ccaTimeoutEvent = Simulator::Schedule (GetLifs (), &RescueMacCsma::BackoffStart, this);
}

void
RescueMacCsma::BackoffStart ()
{
  NS_LOG_FUNCTION ("B-OFF remain:" << m_backoffRemain << 
                   "state:" << StateToString (m_state) << 
                   "phy idle:" << (m_phy->IsIdle () ? "TRUE" : "FALSE"));
  if (m_state != IDLE || !m_phy->IsIdle ())
    {
      CcaForLifs ();
      return;
    }
  if (m_backoffRemain == Seconds (0))
    {
      uint32_t slots = m_random->GetInteger (0, m_cw - 1);
      m_backoffRemain = Seconds ((double)(slots) * GetSlotTime().GetSeconds ());
      NS_LOG_DEBUG ("Select a random number (0, " << m_cw - 1 << "): " << slots << 
                    ", backoffRemain " << m_backoffRemain << 
                    ", will finish " << m_backoffRemain + Simulator::Now ());
    }
  m_backoffStart = Simulator::Now ();
  m_backoffTimeoutEvent = Simulator::Schedule (m_backoffRemain, &RescueMacCsma::ChannelAccessGranted, this);
}

void
RescueMacCsma::ChannelBecomesBusy ()
{
  NS_LOG_FUNCTION ("");
  if (m_backoffTimeoutEvent.IsRunning ())
    {
      m_backoffTimeoutEvent.Cancel ();
      Time elapse;
      if (Simulator::Now () > m_backoffStart)
        {
          elapse = Simulator::Now () - m_backoffStart;
        }
      if (elapse < m_backoffRemain)
        {
          m_backoffRemain = m_backoffRemain - elapse;
          m_backoffRemain = RoundOffTime (m_backoffRemain);
        }
      NS_LOG_DEBUG ("Freeze backoff! Remain " << m_backoffRemain);
    }
  CcaForLifs ();
}

void
RescueMacCsma::ChannelAccessGranted ()
{
  NS_LOG_FUNCTION ("");

  for (RelayQueueI it = m_pktRelayQueue.begin (); it != m_pktRelayQueue.end ();)
    {
      if (m_resendAck) break;
      if (CheckRelayedFrame (*it))
        {
    	  it = m_pktRelayQueue.erase (it);
          //NS_LOG_DEBUG("erase!");
        }
      else
        {
		  it++;
        }
    }

  if (m_ackForwardQueue.size () != 0)
    {
      m_backoffStart = Seconds (0);
      m_backoffRemain = Seconds (0);
      m_state = WAIT_TX;

      m_pktRelay = m_ackForwardQueue.front ();
      m_ackForwardQueue.pop_front ();
      if (!m_resendAck) m_ackCache.push_front (m_pktRelay); //record ACK in cache

      if (m_pktRelay.first == 0)
        NS_ASSERT ("Null packet for ack forward tx");

      NS_LOG_INFO ("FORWARD ACK!");
      m_traceAckForward (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), m_pktRelay.first, m_pktRelay.second);

      SendRelayedData ();
      m_resendAck = false;
    }
  else if (m_pktRelayQueue.size () != 0)
    {    
      m_backoffStart = Seconds (0);
      m_backoffRemain = Seconds (0);
      m_state = WAIT_TX;

      m_pktRelay = m_pktRelayQueue.front ();
      m_pktRelayQueue.pop_front ();

      if (m_pktRelay.first == 0)
        NS_ASSERT ("Null packet for relay tx");

      NS_LOG_INFO ("RELAY DATA FRAME!");
      m_traceDataRelay (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), m_pktRelay.first, m_pktRelay.second);

      SendRelayedData ();
    }
  else if ((m_pktQueue.size () != 0) && !m_ackTimeoutEvent.IsRunning ())
    {
      NS_LOG_DEBUG ("SEND QUEUED PACKET!");
  
      m_backoffStart = Seconds (0);
      m_backoffRemain = Seconds (0);
      m_state = WAIT_TX;
  
      m_pktData = m_pktQueue.front ();
      m_pktQueue.pop_front ();
      NS_LOG_INFO ("dequeue packet from TX queue, size: " << m_pktData->GetSize());
  
      if (m_pktData == 0)
        NS_ASSERT ("Queue has null packet");
    
      m_traceDataTx (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), m_pktData);
      SendData ();
    }
  else
    {
      if (m_ackTimeoutEvent.IsRunning ())
        {
          NS_LOG_DEBUG ("Awaiting for ACK");
        }
      else
        {
          NS_LOG_DEBUG ("No queued frames for TX");
        }
    }
}



// ----------------------- Send Functions ------------------------------
void
RescueMacCsma::SendData ()
{
  NS_LOG_FUNCTION ("");
  RescueMacHeader hdr;
  uint32_t pktSize = m_pktData->GetSize ();
  m_pktData->RemoveHeader (hdr);

  m_interleaver++; //increment interleaver counter

  NS_LOG_FUNCTION ("dst:" << hdr.GetDestination () << 
                   "seq:" << m_sequence << 
                   "IL:" << m_interleaver << 
                   "size:" << pktSize <<
                   "#queue:" << m_pktQueue.size() <<
                   "#relay queue:" << m_pktRelayQueue.size () <<
                   "#ack queue:" << m_ackForwardQueue.size ());

  m_sentFrames.push_back (FrameInfo (hdr.GetDestination (), 
                                     hdr.GetSource (), 
                                     m_sequence, 
                                     m_interleaver, 
                                     ns3::Simulator::Now ())); //remember to prevent loops

  m_createdFrames.push_back (FrameInfo (hdr.GetDestination (), 
                                        m_sequence, false, 
                                        ns3::Simulator::Now ())); //remember for ARQ                             
  
  RescueMode mode = m_remoteStationManager->GetDataTxMode (hdr.GetDestination (), m_pktData, pktSize);

  if (hdr.GetDestination () != GetBroadcast ()) // Unicast
    {
      hdr.SetSequence (m_sequence);
      m_pktData->AddHeader (hdr);
      NS_LOG_DEBUG ("pktData Size: " << m_pktData->GetSize ());
      if (SendPacket (m_pktData, mode, m_interleaver))
        {
          //Time ackTimeout = GetDataDuration (m_pktData, mode) + GetSifs () + GetCtrlDuration (RESCUE_MAC_PKT_TYPE_ACK, mode) + GetSlotTime ();
          Ptr<Packet> p = m_pktData->Copy ();
          m_ackTimeoutEvent = Simulator::Schedule (m_ackTimeout, &RescueMacCsma::AckTimeout, this, p, hdr);
          NS_LOG_DEBUG ("ACK TIMEOUT TIMER START");
        }
      else
        {
          StartOver ();
        }
    }
  else // Broadcast
    {
      /*hdr.SetSequence (m_sequence);
      m_pktData->AddHeader (hdr);
      if (SendPacket (m_pktData, mode, 0))
        {
        }
      else
        {
          StartOver ();
        }*/
    }
}

void
RescueMacCsma::SendRelayedData ()
{
  NS_LOG_FUNCTION ("src:" << m_pktRelay.second.GetSource () << 
                   "dst:" << m_pktRelay.second.GetDestination () << 
                   "seq:" << m_pktRelay.second.GetSequence ());
  
  RescueMode mode = m_remoteStationManager->GetDataTxMode (m_pktRelay.second.GetDestination (), m_pktRelay.first, m_pktRelay.first->GetSize ());

  if (m_pktRelay.second.GetDestination () != GetBroadcast ()) // Unicast
    {
      m_pktRelay.second.SetSender (m_address);
      if (SendPacket (m_pktRelay, mode))
        {
        }
      else
        {
          StartOver ();
        }
      m_pktRelay.first = 0;
      m_pktRelay.second = 0;
    }
  else // Broadcast
    {
      if (SendPacket (m_pktRelay, mode))
        {
        }
      else
        {
          StartOver ();
        }
      m_pktRelay.first = 0;
      m_pktRelay.second = 0;
    }
}

void
RescueMacCsma::SendAck (Mac48Address dest, uint16_t seq, RescueMode dataTxMode, SnrPerTag tag)
{
  NS_LOG_INFO ("SEND ACK");
  NS_LOG_FUNCTION ("to:" << dest);
  
  Ptr<Packet> pkt = Create<Packet> (0);
  RescuePhyHeader ackHdr = RescuePhyHeader (m_address, dest, RESCUE_MAC_PKT_TYPE_ACK);
  ackHdr.SetSequence (seq);
  pkt->AddPacketTag (tag);

  RescueMode mode = m_remoteStationManager->GetAckTxMode (m_pktRelay.second.GetDestination (), dataTxMode);  
  std::pair<Ptr<Packet>, RescuePhyHeader> ackPkt (pkt, ackHdr);  

  m_traceAckTx (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt, ackHdr);      
                     
  SendPacket (ackPkt, mode);
}

bool 
RescueMacCsma::SendPacket (Ptr<Packet> pkt, RescueMode mode, uint16_t il)
{
  NS_LOG_FUNCTION ("state:" << StateToString (m_state) << "packet size: " << pkt->GetSize ());
  
  if (m_state == IDLE || m_state == WAIT_TX) 
    {
      if (m_phy->SendPacket (pkt, mode, il)) 
        {
          m_state = TX;
          m_pktTx = pkt;
          return true;
        }
      else
        {
          m_state = IDLE;
        }
    }
  return false;
}

bool 
RescueMacCsma::SendPacket (std::pair<Ptr<Packet>, RescuePhyHeader> relayedPkt, RescueMode mode)
{
  NS_LOG_FUNCTION ("state:" << StateToString (m_state));
  
  if (m_state == IDLE || m_state == WAIT_TX) 
    {
      if (m_phy->SendPacket (relayedPkt, mode)) 
        {
          m_state = TX;
          m_pktTx = relayedPkt.first;
          return true;
        }
      else
        {
          m_state = IDLE;
        }
    }
  return false;
}

void 
RescueMacCsma::SendPacketDone (Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION ("state:" << StateToString (m_state));
  
  if (m_state != TX /*|| m_pktTx != pkt*/)
    {
      NS_LOG_DEBUG ("Something is wrong!");
      return;
    }
  m_state = IDLE;
  RescueMacHeader hdr;
  pkt->PeekHeader (hdr);
  switch (hdr.GetType ())
    {
    case RESCUE_MAC_PKT_TYPE_DATA:
      if (hdr.GetDestination () == GetBroadcast ())
        {
          //don't know what to do
          SendDataDone ();
          CcaForLifs ();
          return;
        }
	  break;
    case RESCUE_MAC_PKT_TYPE_ACK:
      CcaForLifs ();
      break;
    case RESCUE_MAC_PKT_TYPE_RR:
    case RESCUE_MAC_PKT_TYPE_B:
      break;
    default:
      CcaForLifs ();
      break;
    }
}

void 
RescueMacCsma::SendDataDone ()
{
  NS_LOG_FUNCTION ("");
  m_sequence++;
  NS_LOG_DEBUG ("RESET PACKET");
  m_pktData = 0;
  m_backoffStart = Seconds (0);
  m_backoffRemain = Seconds (0);
  // According to IEEE 802.11-2007 std (p261)., CW should be reset to minimum value 
  // when retransmission reaches limit or when DATA is transmitted successfully
  SetCw (m_cwMin);
  CcaForLifs ();
}

void
RescueMacCsma::StartOver ()
{
  NS_LOG_FUNCTION ("");
  NS_LOG_INFO ("return packet to the front of TX queue");
  m_pktQueue.push_front (m_pktData);
  m_backoffStart = Seconds (0);
  m_backoffRemain = Seconds (0);
  CcaForLifs ();
}



// ---------------------- Receive Functions ----------------------------
void 
RescueMacCsma::ReceiveData (Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode, bool correctData)
{
  NS_LOG_FUNCTION ("");
  if (correctData)
    {
      ReceiveCorrectData (pkt, phyHdr, mode);
    }
  else //uncorrect data
    {
      //check - have I already received the correct/complete frame?
      bool rx = false;
      for (FramesInfoI it = m_receivedFrames.begin (); it != m_receivedFrames.end (); it++)
        {
          //int i; NS_LOG_DEBUG ("queue iterator = " << ++i << " source: " << it->src << " destination: " << it->dst << " seq. number: " << it->seq_no << " tstamp: " << it->tstamp);
          if ( (rx = ((it->src == phyHdr.GetSource ())
                      && (it->seq_no == phyHdr.GetSequence ())))
                == true ) break;
        }
      if (!rx)
        {
          NS_LOG_INFO ("THE FRAME COPY WAS STORED!");
          //m_phy->StoreFrameCopy (pkt, phyHdr);
        }
      else
        {
          NS_LOG_INFO ("(unnecessary DATA frame copy) DROP!");
        }
    }
}

void 
RescueMacCsma::ReceiveCorrectData (Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode)
{
  NS_LOG_FUNCTION ("");

  RescueMacHeader hdr;
  pkt->RemoveHeader (hdr);
  NS_LOG_FUNCTION ("src:" << phyHdr.GetSource () << 
                   "sequence: " << phyHdr.GetSequence ());
  	
  m_state = WAIT_TX;
  // forward upper layer
  if (IsNewSequence (phyHdr.GetSource (), phyHdr.GetSequence ()))
    {
      NS_LOG_INFO ("DATA RX OK!");
      SnrPerTag tag;
      pkt->PeekPacketTag (tag); //to send back SnrPerTag - for possible use in the future (multi rate control etc.)
      m_sendAckEvent = Simulator::Schedule (GetSifs (), &RescueMacCsma::SendAck, this, phyHdr.GetSource (), phyHdr.GetSequence (), mode, tag);
      m_receivedFrames.push_front (FrameInfo (phyHdr.GetSource (), 
                                              phyHdr.GetSequence (), 
                                              ns3::Simulator::Now ()));
      m_forwardUpCb (pkt, phyHdr.GetSource (), phyHdr.GetDestination ());
    }
  else
    {
      NS_LOG_INFO ("(duplicate) DROP!");
    }
}

void 
RescueMacCsma::ReceiveRelayData (Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode)
{
  NS_LOG_FUNCTION ("src:" << phyHdr.GetSource () << 
                   "dst:" << phyHdr.GetDestination () << 
                   "seq:" << phyHdr.GetSequence () <<
                   "mode:" << mode);

  //check - have I recently transmitted this frame? have I transmitted an ACK for it?
  bool tx = true;
  for (FramesInfoI it = m_sentFrames.begin (); it != m_sentFrames.end (); it++)
    {
      //int i; NS_LOG_DEBUG ("queue iterator = " << ++i << " source: " << it->src << " destination: " << it->dst << " seq. number: " << it->seq_no << " tstamp: " << it->tstamp);
      if ( (it->src == phyHdr.GetSource ())
           && (it->dst == phyHdr.GetDestination ())
           && (it->seq_no == phyHdr.GetSequence ()) )
        {
          if ( (it->ACKed)
               && (it->il < phyHdr.GetInterleaver ()) )
            {
              //frame was already ACKed and unnecessary retransmission is detected (new interleaver)
              ResendAckFor (std::pair<Ptr<Packet>, RescuePhyHeader> (pkt, phyHdr));
              tx = false;
            }
          else if (it->il == phyHdr.GetInterleaver ()) 
            {
              //frame was not already ACKed but we transmitted copy of given TX try, just break
              tx = false;
              break; 
            }
          //otherwise - it is new TX try, forward it!
        }

    }

  //ask routing - should I forward it?
    /*if (tx) 
        { 
    //====== PLACE FOR ROUTING INTERACTION ======

        }*/

  if (tx) 
    {
      NS_LOG_INFO ("FRAME TO RELAY!");
      
      //phyHdr.SetSender (m_address);
      std::pair<Ptr<Packet>, RescuePhyHeader> pktRelay (pkt, phyHdr);
      m_pktRelayQueue.push_back (pktRelay);

      //remember in SentFrames list
      m_sentFrames.push_back (FrameInfo (phyHdr.GetDestination (), 
                                         phyHdr.GetSource (), 
                                         phyHdr.GetSequence (), 
                                         phyHdr.GetInterleaver (), 
                                         ns3::Simulator::Now ()));                                   
    }
  else
    {
      NS_LOG_INFO ("(DATA already TX-ed) DROP!");
    }

  m_state = IDLE;
  CcaForLifs ();
  return;
}

void 
RescueMacCsma::ReceiveBroadcastData (Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode, bool correctData)
{
  NS_LOG_FUNCTION ("");

  //don't know what to do
}

void 
RescueMacCsma::ReceiveAck (Ptr<Packet> ackPkt, RescuePhyHeader phyHdr, double ackSnr, RescueMode ackMode)
{
  NS_LOG_FUNCTION ("src:" << phyHdr.GetSource () << 
                   "dst:" << phyHdr.GetDestination () << 
                   "seq:" << phyHdr.GetSequence ());
    
  m_state = IDLE;

  if (phyHdr.GetDestination () == m_address)
    {
      //check - have I recently transmitted frame which is acknowledged?
      bool wasTXed = false;
      for (FramesInfoI it = m_createdFrames.begin (); it != m_createdFrames.end (); it++)
        {
          //int i; NS_LOG_DEBUG ("queue iterator = " << ++i << " source: " << it->src << " destination: " << it->dst << " seq. number: " << it->seq_no << " tstamp: " << it->tstamp);
          if ( (wasTXed = ( (it->dst == phyHdr.GetSource ())
                            && (it->seq_no == phyHdr.GetSequence ()) ) ) == true ) 
            {
              if (!it->ACKed)
                {
                  NS_LOG_INFO ("GOT ACK!");
                  it->ACKed = true;
                  m_ackTimeoutEvent.Cancel ();
                  SnrPerTag tag;
                  ackPkt->PeekPacketTag (tag);
                  m_remoteStationManager->ReportDataOk (phyHdr.GetSource (), 
                                                        ackSnr, ackMode, 
                                                        tag.GetSNR (), tag.GetPER ());
                  NS_LOG_INFO ("DATA TX OK!");
                  m_traceSendDataDone (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), phyHdr.GetSource (), true);
                  SendDataDone ();
                }
              else
                {
                  NS_LOG_INFO ("DUPLICATED ACK");
                }
              break;
            }
        }

      if (!wasTXed)
        {
          NS_LOG_INFO ("ACK ERROR (acknowledged frame was not originated here)");
        }
      return;
    }
  else
    {
      //check - have I recently transmitted frame which is acknowledgded here?
      bool found = false;
      for (FramesInfoRI it = m_sentFrames.rbegin (); it != m_sentFrames.rend (); it++)
        {
          //int i; NS_LOG_DEBUG ("queue iterator = " << ++i << " source: " << it->src << " destination: " << it->dst << " seq. number: " << it->seq_no << " tstamp: " << it->tstamp);
          if ( (it->dst == phyHdr.GetSource ())
               && (it->src == phyHdr.GetDestination ())
               && (it->seq_no == phyHdr.GetSequence ()) )
            {
              if (!it->ACKed) //did we get an ACK for this frame?
                {
                  NS_LOG_INFO ("ACK to forward!");
                  it->ACKed = true;
                  std::pair<Ptr<Packet>, RescuePhyHeader> forwardAck (ackPkt, phyHdr);
                  m_ackForwardQueue.push_back (forwardAck);      
                }
              else
                {
                  NS_LOG_INFO ("(ACK already received) DROP!");
                }
              found = true;
              break;
            }
        }
      if (!found)
        {
          NS_LOG_INFO ("(ACK for frame which was not relayed here) DROP!");
        }

      CcaForLifs ();
      return;
    }
}

void 
RescueMacCsma::ReceiveResourceReservation (Ptr<Packet> pkt, RescuePhyHeader phyHdr) 
{
  NS_LOG_FUNCTION ("");
}

void 
RescueMacCsma::ReceiveBeacon (Ptr<Packet> pkt, RescuePhyHeader phyHdr)
{
  NS_LOG_FUNCTION ("");
}

void
RescueMacCsma::ReceivePacket (Ptr<RescuePhy> phy, Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION ("");
  ChannelBecomesBusy ();
  switch (m_state)
    {
    case WAIT_TX:
    case RX:
    case WAIT_RX:
    case BACKOFF:
    case IDLE:
      m_state = RX;
      break;
    case TX:
    case COLL:
      break;
    }
}

void 
RescueMacCsma::ReceivePacketDone (Ptr<RescuePhy> phy, Ptr<Packet> pkt, RescuePhyHeader phyHdr, 
                                  double snr, RescueMode mode, 
                                  bool correctPhyHdr, bool correctData, bool wasReconstructed)
{
  NS_LOG_FUNCTION ("");
  m_state = IDLE;

  if (!correctPhyHdr)
    {
      NS_LOG_INFO ("PHY HDR DAMAGED - DROP!");
      CcaForLifs ();
      return;
    }
  else if (phyHdr.GetSource () == m_address)
    {
      NS_LOG_INFO ("FRAME RETURNED TO SOURCE - DROP!");
      CcaForLifs ();
      return;
    }
  else
    {
      switch (phyHdr.GetType ())
        {
          case RESCUE_PHY_PKT_TYPE_DATA:
           if (phyHdr.GetDestination () == GetBroadcast ())
             {
               NS_LOG_INFO ("PHY HDR OK!" << 
                            ", DATA: " << (correctData ? "OK!" : "DAMAGED!") << 
                            ", BROADCASTED FRAME!");
               ReceiveBroadcastData (pkt, phyHdr, mode, correctData);
             }
           else if (phyHdr.GetDestination () !=  m_address)
             {
               NS_LOG_INFO ("PHY HDR OK!" << 
                            ", DATA: " << (correctData ? "PER OK!, FRAME TO RELAY ?!" : "UNUSABLE - DROP!"));
               if (correctData) 
                 {
                    ReceiveRelayData (pkt, phyHdr, mode);
                 }
               else
                 {
                   CcaForLifs ();
                   return;
                 }
             }
           else
             {
               NS_LOG_INFO ("PHY HDR OK!" << 
                            ", DATA: " << (correctData ? "OK!" : "DAMAGED!") << 
                            ", RECONSTRUCTION STATUS: " << ((wasReconstructed) ? (correctData ? "SUCCESS" : "MORE COPIES NEEDED") : "NOT NEEDED"));
               ReceiveData (pkt, phyHdr, mode, correctData);
             }
            break;
          case RESCUE_PHY_PKT_TYPE_ACK:
            ReceiveAck (pkt, phyHdr, snr, mode);
            break;
          case RESCUE_PHY_PKT_TYPE_RR:
            ReceiveResourceReservation (pkt, phyHdr);
            break;
          case RESCUE_PHY_PKT_TYPE_B:
            ReceiveBeacon (pkt, phyHdr);
            break;
          default:
            CcaForLifs ();
            break;
        }
    }
}



// -------------------------- Timeout ----------------------------------

void
RescueMacCsma::AckTimeout (Ptr<Packet> pkt, RescueMacHeader hdr)
{
  NS_LOG_FUNCTION ("try: " << m_remoteStationManager->GetRetryCount (hdr.GetDestination ()));
  NS_LOG_INFO ("!!! ACK TIMEOUT !!!");
  //m_state = IDLE;
  m_remoteStationManager->ReportDataFailed (hdr.GetDestination ());
  m_traceAckTimeout (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt, hdr);

  if (m_remoteStationManager->NeedDataRetransmission (hdr.GetDestination (), pkt))
    {
      NS_LOG_INFO ("RETRANSMISSION");
      StartOver ();
      //SendData ();
    }
  else
    {
      // Retransmission is over the limit. Drop it!
      NS_LOG_INFO ("DATA TX FAIL!");
      m_remoteStationManager->ReportFinalDataFailed (hdr.GetDestination ());
      m_traceSendDataDone (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), hdr.GetDestination (), false);
      SendDataDone ();
    }
}



// --------------------------- ETC -------------------------------------
bool
RescueMacCsma::CheckRelayedFrame (std::pair<Ptr<Packet>, RescuePhyHeader> relayedPkt)
{
  NS_LOG_FUNCTION ("");
  bool rx;
  //check - have I recently received ACK for this DATA fame copy?
  for (FramesInfoI it = m_sentFrames.begin (); it != m_sentFrames.end (); it++)
    {
      //int i; NS_LOG_DEBUG ("queue iterator = " << ++i << " source: " << it->src << " destination: " << it->dst << " seq. number: " << it->seq_no << " tstamp: " << it->tstamp);
      if ( (rx = ( (it->src == relayedPkt.second.GetSource ())
                   && (it->dst == relayedPkt.second.GetDestination ())
                   && (it->seq_no == relayedPkt.second.GetSequence ())
                   && (it->ACKed) ) == true) ) 
        {
          NS_LOG_INFO ("RELAYED FRAME (src: " << it->src <<
                       ", dst: " << it->dst <<
                       ", seq: " << it->seq_no << 
                       ") was already ACKed, DROP!");

          if (it->il < relayedPkt.second.GetInterleaver ())
            {
              ResendAckFor (relayedPkt);
            }
          break;
        }
    }
  return rx;
}

void
RescueMacCsma::ResendAckFor (std::pair<Ptr<Packet>, RescuePhyHeader> relayedPkt)
{
  NS_LOG_INFO ("UNNECESSARY RETRANSMISSION DETECTED!");
  NS_LOG_FUNCTION ("");
     
  for (RelayQueueI it = m_ackCache.begin (); it != m_ackCache.end (); it++)
    {
      if ( (it->second.GetSource () == relayedPkt.second.GetDestination ())
           && (it->second.GetDestination () == relayedPkt.second.GetSource ())
           && (it->second.GetSequence () == relayedPkt.second.GetSequence ()) )
        {
          NS_LOG_INFO ("ACK found in cache, RESEND!");
          m_ackForwardQueue.push_front (*it);
          m_resendAck = true;
        }
     }
}

bool
RescueMacCsma::IsNewSequence (Mac48Address addr, uint16_t seq)
{
  NS_LOG_FUNCTION ("");
  std::list<std::pair<Mac48Address, uint16_t> >::iterator it = m_seqList.begin ();
  for (; it != m_seqList.end (); ++it)
    {
      if (it->first == addr)
        {
          if (it->second == 65536 && seq < it->second)
            {
              it->second = seq;
              return true;
            }
          else if (seq > it->second)
            {
              it->second = seq;
              return true;
            }
          else
            {
              return false;
            }
         }
    }
  std::pair<Mac48Address, uint16_t> newEntry (addr, seq);
  m_seqList.push_back (newEntry);
  return true;
}

void
RescueMacCsma::DoubleCw ()
{
  if (m_cw * 2 > m_cwMax)
    {
      m_cw = m_cwMax;
    }
  else
    {
      m_cw = m_cw * 2;
    }
}

// Nodes can start backoff procedure at different time because of propagation 
// delay and processing jitter (it's very small but matter in simulation), 
Time
RescueMacCsma::RoundOffTime (Time time)
{
  int64_t realTime = time.GetMicroSeconds ();
  int64_t slotTime = GetSlotTime ().GetMicroSeconds ();
  if (realTime % slotTime >= slotTime / 2)
    {
      return Seconds (GetSlotTime().GetSeconds () * (double)(realTime / slotTime + 1));
    }
  else
    {
      return Seconds (GetSlotTime().GetSeconds () * (double)(realTime / slotTime));
    }
}

} // namespace ns3

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

#ifndef RESCUE_MAC_CSMA_H
#define RESCUE_MAC_CSMA_H

#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/event-id.h"
#include "ns3/traced-value.h"
#include "ns3/random-variable-stream.h"
#include "ns3/mac48-address.h"

#include "low-rescue-mac.h"
#include "rescue-mac-header.h"
#include "rescue-phy-header.h"
#include "rescue-mode.h"
#include "snr-per-tag.h"

#include <list>
#include <map>

namespace ns3 {

class RescuePhy;
class RescueChannel;
class RescueMac;
class RescueRemoteStationManager;
class RescueArqManager;
class RescueNetDevice;

/**
 * \brief base class for Rescue MAC
 * \ingroup rescue
 */
class RescueMacCsma : public LowRescueMac
{
public:
  RescueMacCsma ();
  virtual ~RescueMacCsma ();
  static TypeId GetTypeId (void);
 
  /**
   * Clear the transmission queues and pending packets, reset counters.
   */
  virtual void Clear (void);

  /**
   * \param phy the physical layer to attach to this MAC.
   */
  virtual void AttachPhy (Ptr<RescuePhy> phy);
  /**
   * \param hiMac the HI-MAC sublayer to attach to this (lower) MAC.
   */
  virtual void SetHiMac (Ptr<RescueMac> hiMac);
  /**
   * \param dev the device this MAC is attached to.
   */
  virtual void SetDevice (Ptr<RescueNetDevice> dev);
  /**
   * \param manager RescueRemoteStationManager associated with this MAC
   */
  void SetRemoteStationManager (Ptr<RescueRemoteStationManager> manager);

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  virtual int64_t AssignStreams (int64_t stream);

  /**
   * \param cw the minimum contetion window
   */ 
  void SetCwMin (uint32_t cw);
  /**
   * \param cw the maximal contetion window
   */ 
  void SetCwMax (uint32_t cw);
  /**
   * \param duration the slot duration
   */
  void SetSlotTime (Time duration);
  /**
   * \param duration the slot duration
   */
  void SetSifsTime (Time duration);
  /**
   * \param duration the slot duration
   */
  void SetLifsTime (Time duration);
  /**
   * \param length the length of queues used by this MAC
   */
  void SetQueueLimits (uint32_t length);
  /**
   * \param duration the Basic ACK Timeout duration
   */
  void SetBasicAckTimeout (Time duration);

  /**
   * \return minimal contention window
   */ 
  uint32_t GetCwMin (void) const;
  /**
   * \return maximal contention window
   */ 
  uint32_t GetCwMax (void) const;
  /**
   * \return current contention window
   */ 
  uint32_t GetCw (void) const;
  /**
   * \return the current slot duration.
   */
  Time GetSlotTime (void) const;
  /**
   * \return the current slot duration.
   */
  Time GetSifsTime (void) const;
  /**
   * \return the current slot duration.
   */
  Time GetLifsTime (void) const;
  /**
   * \return length the length of queues used by this MAC
   */
  uint32_t GetQueueLimits (void) const;
  /**
   * \return duration the Basic ACK Timeout duration
   */
  Time GetBasicAckTimeout (void) const;
  
  /**
   * \param pkt the packet to send.
   * \param dest the address to which the packet should be sent.
   *
   * The packet should be enqueued in a tx queue, and should be
   * dequeued as soon as the CSMA determines that
   * access is granted to this MAC.
   */
  virtual bool Enqueue (Ptr<Packet> pkt, Mac48Address dest);
  /**
   * \param pkt the control packet to send.
   * \param phyHdr the phy header associated with the packet.
   */
  virtual bool EnqueueCtrl (Ptr<Packet> pkt, RescuePhyHeader phyHdr);

  /**
   * Method used to start operation of this lower MAC entity 
   * (eg. start CSMA-CA MAC at the beginning of Contention Period)
   *
   * /return MAC was started succesfully.
   */
  virtual bool StartOperation ();
  /**
   * Method used to start operation of this lower MAC entity 
   * (eg. start CSMA-CA MAC at the beginning of Contention Period)
   *
   * \param duration the duration of operation period
   *
   * /return MAC was started succesfully.
   */
  virtual bool StartOperation (Time duration);
  /**
   * Method used to stop operation of this lower MAC entity 
   * (eg. stop CSMA-CA MAC at the and of Contention Period)
   *
   * /return MAC was stopped succesfully.
   */
  virtual bool StopOperation ();
  /**
   * Method used to stop operation of this lower MAC entity 
   * (eg. stop CSMA-CA MAC at the and of Contention Period)
   *
   * \param duration the duration of inactivity period
   *
   * /return MAC was stopped succesfully.
   */
  virtual bool StopOperation (Time duration);

  /**
   * This method is called after end of packet transmission
   *
   * \param pkt transmitted packet
   */
  virtual void SendPacketDone (Ptr<Packet> pkt);
  /**
   * Method used by PHY to notify MAC about begin of frame reception
   *
   * \param phy receiving phy
   * \param pkt the received packet
   */
  virtual void ReceivePacket (Ptr<RescuePhy> phy, Ptr<Packet> pkt);
  /**
   * Method used by PHY to notify MAC about end of frame reception
   *
   * \param phy receiving phy
   * \param pkt the received packet
   * \param phyHdr PHY header associated with the received packet
   * \param snr SNR of this transmission
   * \param mode the mode (RescueMode) of this transmission
   * \param correctPhyHdr true if the PHY header was correctly received
   * \param correctData true if the payload was correctly received
   * \param wasReconstructed true if the frame was reconstructed basing on almost two received frame copies
   */
  virtual void ReceivePacketDone (Ptr<RescuePhy> phy, Ptr<Packet> pkt, RescuePhyHeader phyHdr, 
                                  double snr, RescueMode mode, 
                                  bool correctPhyHdr, bool correctData, bool wasReconstructed);
  
private:
  typedef enum {
    IDLE, BACKOFF, WAIT_TX, TX, WAIT_RX, RX, COLL
  } State;
  std::string StateToString (State state);
  
  /**
   * Invoked to calculate control packet (e.g. ACK) TX duration
   *
   * \param type the type of the frame
   * \param mode the TX mode (RescueMode) of associated DATA frames
   * \return the duration of control packet TX
   */
  Time GetCtrlDuration (uint16_t type, RescueMode mode);
  /**
   * Invoked to calculate DATA packet TX duration
   *
   * \param pkt packet
   * \param mode the TX mode for this packet
   * \return the duration of DATA packet TX
   */
  Time GetDataDuration (Ptr<Packet> pkt, RescueMode mode);

  /**
   * \param cw the new congestion window
   */ 
  void SetCw (uint32_t cw);
  
  /**
   * starts CCA during waiting for LIFS expiration
   */ 
  void CcaForLifs ();
  /**
   * runs backoff counter
   */ 
  void BackoffStart ();
  /**
   * invoked when channel becomes busy to freeze backoff counter
   */ 
  void ChannelBecomesBusy ();
  /**
   * invoked at the end of backoff procedure to start TX
   */ 
  void ChannelAccessGranted ();
  /**
   * invoked to remove current packet from TX queue
   */ 
  void Dequeue ();
  /**
   * invoked to remove current control packet from CTRL TX queue
   */ 
  //void DequeueCtrl ();
  /**
   * invoked to transmit DATA frame
   */ 
  void SendData ();
  /**
   * invoked by relay station to forward DATA frame
   */ 
  void SendRelayedData ();
  /**
   * invoked to transmit ACK frame
   *
   * \param dest address of acknowledged station
   * \param seq sequence number of acknowledged DATA frame
   * \param dataTxMode acknowledged DATA frame TX mode (RescueMode)
   * \param tag SnrPerTag to notify source about final SNR/PER values of packet 
   *            - for possible further use for multi rate control etc.
   */ 
  void SendAck (Mac48Address dest, uint16_t seq, RescueMode dataTxMode, SnrPerTag tag);
  /**
   * invoked to pass the frame to PHY for transmission
   *
   * \param pkt packet for transmission
   * \param mode frame TX mode (RescueMode)
   * \param il interleaver number
   * \return true if transmission starts
   */ 
  bool SendPacket (Ptr<Packet> pkt, RescueMode mode, uint16_t il);
  /**
   * invoked to pass the relayed frame to PHY for transmission
   *
   * \param relayedPkt pair of packet for transmission and associated PHY header
   * \param mode frame TX mode (RescueMode)
   * \return true if transmission starts
   */ 
  bool SendPacket (std::pair<Ptr<Packet>, RescuePhyHeader> relayedPkt, RescueMode mode);
  /**
   * invoked when transmission was not successful to try again after LIFS period
   *
   * \param pkt packet for which transmission was not successful
   */
  void StartOver (Ptr<Packet> pkt);
  /**
   * invoked when transmission was not successful to try again after LIFS period
   *
   * \param pktHdr pair of packet and PHY header for which transmission was not successful
   */
  void StartOver (std::pair<Ptr<Packet>, RescuePhyHeader> pktHdr);
  /**
   * invoked after data transmission to prepare for new one 
   * (reset CW, increase sequence counter etc.)
   */
  void SendDataDone ();
 
  /**
   * invoked by ReceivePacketDone to process any received DATA frame
   * destined for this station
   *
   * \param pkt the received packet
   * \param phyHdr PHY header associated with the received packet
   * \param mode the mode (RescueMode) of this transmission
   * \param correctData true if the payload was correctly received
   */
  void ReceiveData (Ptr<Packet> pkt, RescuePhyHeader phyHdr, 
                    RescueMode mode, bool correctData);
  /**
   * invoked by ReceiveData to process correctly received DATA frame
   * destined to this station
   *
   * \param pkt the received packet
   * \param phyHdr PHY header associated with the received packet
   * \param mode the mode (RescueMode) of this transmission
   */
  void ReceiveCorrectData (Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode);
  /**
   * invoked by ReceivePacketDone of RELAY station to process received DATA frame
   * destined to another tation
   *
   * \param pkt the received packet
   * \param phyHdr PHY header associated with the received packet
   * \param mode the mode (RescueMode) of this transmission
   */
  void ReceiveRelayData (Ptr<Packet> pkt, RescuePhyHeader phyHdr, RescueMode mode);
  /**
   * invoked by ReceivePacketDone to process received BROADCAST frame
   * currently NOR SUPPORTED
   *
   * \param pkt the received packet
   * \param phyHdr PHY header associated with the received packet
   * \param mode the mode (RescueMode) of this transmission
   * \param correctData true if the payload was correctly received
   */
  void ReceiveBroadcastData (Ptr<Packet> pkt, RescuePhyHeader phyHdr, 
                             RescueMode mode, bool correctData);
  /**
   * invoked by ReceivePacketDone to process any received ACK frame
   *
   * \param ackPkt the received ACK packet
   * \param phyHdr PHY header associated with the received ACK packet
   * \param ackSnr SNR of this ACK frame reception (for further rate control)
   * \param ackMode the mode (RescueMode) of this ACK transmission
   */
  void ReceiveAck (Ptr<Packet> ackPkt, RescuePhyHeader phyHdr, 
                   double ackSnr, RescueMode ackMode);
  /**
   * invoked by ReceivePacketDone to process RESOURCE RESERVATION frame
   * currently NOT SUPPORTED
   *
   * \param pkt the received RESOURCE RESERVATION frame
   * \param phyHdr PHY header associated with the received RESOURCE RESERVATION frame
   */
  void ReceiveResourceReservation (Ptr<Packet> pkt, RescuePhyHeader phyHdr);
  /**
   * invoked by ReceivePacketDone to process BEACON frame
   * currently NOT SUPPORTED
   *
   * \param pkt the received BEACON frame
   * \param phyHdr PHY header associated with the received BEACON frame
   */
  void ReceiveBeacon (Ptr<Packet> pkt, RescuePhyHeader phyHdr);
  
  /**
   * invoked to check if given frame was already acknowledged
   * if unnecessary retransmission detected forces lost ACK retransmission
   *
   * \param relayedPkt pair of packet for transmission and associated PHY header
   * \return true if packet was already acknowledged
   */
  bool CheckRelayedFrame (std::pair<Ptr<Packet>, RescuePhyHeader> relayedPkt);
  /**
   * lost ACK retransmission
   *
   * \param relayedPkt pair of packet for transmission and associated PHY header
   */
  void ResendAckFor (std::pair<Ptr<Packet>, RescuePhyHeader> relayedPkt);

  /**
   * invoked when ACK Timoeut counter expires
   *
   * \param pkt packet for which the timeout has expired (including header)
   */
  void AckTimeout (Ptr<Packet> pkt);
  /**
   * invoked to expand contention window value
   */
  void DoubleCw ();
  /**
   * used to round given time to value being a multiplication of slot duartion
   *
   * \param time the time value to round
   * \return rounded time value
   */
  Time RoundOffTime (Time time);
  /**
   * used to check sequence number of incoming frame to prevent
   * duplicated frame reception
   *
   * \param addr address of frame originator
   * \param seq sequence number of a given frame
   * \return false if frame is duplicated
   */
  bool IsNewSequence (Mac48Address addr, uint16_t seq);
  
  bool m_enabled; //!< True for this CSMA MAC during Contention Period

  Ptr<RescuePhy> m_phy;                                     //!< Pointer to RescuePhy (actually send/receives frames)
  Ptr<RescueMac> m_hiMac;                                   //!< Pointer to (higher) RescueMac
  Ptr<RescueNetDevice> m_device;                            //!< Pointer to RescueNetDevice
  Ptr<RescueRemoteStationManager> m_remoteStationManager;   //!< Pointer to RescueRemoteStationManager (rate control)
  Ptr<RescueArqManager> m_arqManager;                       //!< Pointer to RescueArqManager (ARQ control)
  Ptr<UniformRandomVariable> m_random;                      //!< Provides uniform random variables.
    
  EventId m_ccaTimeoutEvent;        //!< CCA procedure timeout event
  EventId m_backoffTimeoutEvent;    //!< BACKOFF procedure timeout event
  EventId m_ackTimeoutEvent;        //!< End-to-end ACK timeout event
  EventId m_sendAckEvent;           //!< Event to send ACK
  EventId m_sendDataEvent;          //!< Event to send DATA
  
  // MAC parameters
  uint16_t m_cwMin;     //!< Minimal value of contention window
  uint16_t m_cwMax;     //!< Maximal value of contention window
  Time m_slotTime;      //!< Slot duration
  Time m_sifs;          //!< SIFS duration
  Time m_lifs;          //!< LIFS duration

  State m_state;            //!< Current state of this MAC
  uint16_t m_cw;            //!< Current contention window
  uint16_t m_sequence;      //!< Sequence counter of this MAC (used if ARQ manager is not specified)
  uint8_t m_interleaver;    //!< Counter to set interlever
  uint32_t m_queueLimit;    //!< Maximal queue(s) size

  Time m_backoffRemain;     //!< Remaining BACKOFF time
  Time m_backoffStart;      //!< The time of last BACKOFF counter start
  Time m_basicAckTimeout;   //!< The maximal duration of ACK awaiting
  Time m_opEnd;             //!< The end of current operation period
  Time m_nopBegin;          //!< The beginning of next operation period

  Ptr<Packet> m_pktTx;                                  //!< Currently transmitted packet (any)
  Ptr<Packet> m_pktData;                                //!< Currently transmitted DATA packet
  std::pair<Ptr<Packet>, RescuePhyHeader> m_pktRelay;   //!< Currently RELAYED DATA packet
  

  //Relay Queue stores MAC frames and PHY headers
  typedef std::list<std::pair<Ptr<Packet>, RescuePhyHeader> > RelayQueue;
  typedef std::list<std::pair<Ptr<Packet>, RescuePhyHeader> >::reverse_iterator RelayQueueRI;
  typedef std::list<std::pair<Ptr<Packet>, RescuePhyHeader> >::iterator RelayQueueI;

  std::list<Ptr<Packet> > m_pktQueue;   //!< The queue for newly originated frames
  RelayQueue m_pktRelayQueue;           //!< The queue for frames to forward
  RelayQueue m_ctrlPktQueue;            //!< The queue for control frames to transmit
  RelayQueue m_ackForwardQueue;         //!< The queue for ACK frames to forward
  RelayQueue m_ackCache;                //!< The memory to store recently forwarded ACK in case of retransmission need

  bool m_resendAck; //!< to notify that pending ACK is retransmitted

  std::list<std::pair<Mac48Address, uint16_t> > m_seqList;  //!< The list of MAC addresses and associated sequence numbers

  /*
   * List to keep information about transmitted frames in order to prevent loops during flooding
   */
  struct FrameInfo
    {
      FrameInfo (uint8_t il,
                 bool ACKed,
                 bool retried);
      uint8_t il;
      bool ACKed;
      bool retried;
    };
  typedef std::map<uint16_t, struct FrameInfo> SeqIlList;
  struct seqIlAck
    {
      seqIlAck (SeqIlList::iterator& it) 
        : seq (it->first), il (it->second.il), 
          ACKed (it->second.ACKed), retried (it->second.retried) {}
      const uint16_t& seq;
      uint8_t& il;
      bool& ACKed;
      bool& retried;
    };
  typedef std::map<std::pair<Mac48Address, Mac48Address>, SeqIlList> RecvSendSeqList;
  struct srcDstSeqList
    {
      srcDstSeqList (RecvSendSeqList::iterator& it) 
        : src (it->first.first), dst (it->first.second), seqList (it->second) {}
      const Mac48Address& src;
      const Mac48Address& dst;
      SeqIlList& seqList;
    };
  RecvSendSeqList m_sentFrames; //!< List of recently transmitted / relayed frames - to prevent loops

  /*
   * List to keep information about transmitted frames in order to perform ARQ
   */ 
  typedef std::map<uint16_t, bool> SeqList;
  struct seqAck
    {
      seqAck (SeqList::iterator& it) 
        : seq (it->first), ACKed (it->second) {}
      const uint16_t& seq;
      bool& ACKed;
    };
  typedef std::map<Mac48Address, SeqList> SendSeqList;
  struct dstSeqList
    {
      dstSeqList (SendSeqList::iterator& it) 
        : dst (it->first), seqList (it->second) {}
      const Mac48Address& dst;
      SeqList& seqList;
    };
  SendSeqList m_createdFrames; //!< List of originated frames - for ARQ

  /*
   * List to keep information about transmitted frames in order to eliminate duplicates
   */ 
  typedef std::map<Mac48Address, uint16_t> RecvSeqList;
  struct srcSeq
    {
      srcSeq (RecvSeqList::iterator& it) 
        : src (it->first), seq (it->second) {}
      const Mac48Address& src;
      uint16_t& seq;
    };
  RecvSeqList m_receivedFrames; //!< List of received frames seq. numbers - to eliminate duplicates
 


  // for trace and performance evaluation
  /*TracedCallback<uint32_t, uint32_t, Ptr<const Packet>, const RescueMacHeader &> m_traceEnqueue;            //<! Trace Hookup for enqueue a DATA
  TracedCallback<uint32_t, uint32_t, Ptr<const Packet>, const RescueMacHeader &> m_traceAckTimeout;         //<! Trace Hookup for ACK Timeout
  TracedCallback<uint32_t, uint32_t, Mac48Address, bool> m_traceSendDataDone;                               //!< Trace Hookup for succesfull DATA TX (after ACK RX)

  TracedCallback<uint32_t, uint32_t, Ptr<const Packet> > m_traceDataTx;                                     //<! Trace Hookup for DATA TX (originated)
  TracedCallback<uint32_t, uint32_t, Ptr<const Packet>, const RescuePhyHeader &> m_traceDataRelay;          //<! Trace Hookup for DATA relay TX
  TracedCallback<uint32_t, uint32_t, Ptr<const Packet> > m_traceDataRx;                                     //<! Trace Hookup for DATA RX (by final station)
  TracedCallback<uint32_t, uint32_t, Ptr<const Packet>, const RescuePhyHeader &> m_traceAckTx;              //<! Trace Hookup for ACK TX (originated)
  TracedCallback<uint32_t, uint32_t, Ptr<const Packet>, const RescuePhyHeader &> m_traceAckForward;         //<! Trace Hookup for ACK forwarding
  TracedCallback<uint32_t, uint32_t, Ptr<const Packet>, const RescuePhyHeader &> m_traceCtrlTx;             //<! Trace Hookup for CTRL TX*/
 
protected:
  virtual void DoDispose ();
};

}

#endif // RESCUE_MAC_CSMA_H

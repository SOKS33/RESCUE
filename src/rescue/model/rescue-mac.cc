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

#include "rescue-mac.h"
#include "rescue-net-device.h"
#include "rescue-remote-station-manager.h"
#include "rescue-arq-manager.h"
#include "low-rescue-mac.h"
#include "rescue-mac-csma.h"
#include "rescue-mac-tdma.h"
#include "rescue-phy.h"
#include "rescue-channel.h"

NS_LOG_COMPONENT_DEFINE("RescueMac");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now() << "] [addr=" << m_address << "] [MAC] "

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(RescueMac);

    uint8_t compressMac(ns3::Mac48Address addr) {
        uint8_t* buf = new uint8_t[6];
        addr.CopyTo(buf);
        uint8_t tmp = buf[5];
        delete[] buf;
        return tmp;
    }

    RescueMac::RescueMac()
    : m_phy(0),
    m_tdmaMac(0) {
        NS_LOG_INFO("");
        m_random = CreateObject<UniformRandomVariable> ();
        m_csmaMac = CreateObject<RescueMacCsma> ();
        m_csmaMac->SetHiMac(this);
    }

    RescueMac::~RescueMac() {
        NS_LOG_INFO("");
    }

    void
    RescueMac::DoInitialize() {
        NS_LOG_INFO("");
    }

    void
    RescueMac::DoDispose() {
        Clear();
        //m_remoteStationManager = 0;
        m_csmaMac = 0;
    }

    void
    RescueMac::Clear() {
    }

    TypeId
    RescueMac::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::RescueMac")
                .SetParent<Object> ()
                .AddConstructor<RescueMac> ()
                .AddAttribute("CwMin",
                "Minimum value of CW",
                UintegerValue(15),
                MakeUintegerAccessor(&RescueMac::SetCwMin),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("CwMax",
                "Maximum value of CW",
                UintegerValue(1024),
                MakeUintegerAccessor(&RescueMac::SetCwMax),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("SlotTime",
                "Time slot duration [us] for MAC backoff",
                TimeValue(MicroSeconds(15)),
                MakeTimeAccessor(&RescueMac::SetSlot),
                MakeTimeChecker())
                .AddAttribute("SifsTime",
                "Short Inter-frame Space [us]",
                TimeValue(MicroSeconds(30)),
                MakeTimeAccessor(&RescueMac::SetSifs),
                MakeTimeChecker())
                .AddAttribute("LifsTime",
                "Long Inter-frame Space [us]",
                TimeValue(MicroSeconds(60)),
                MakeTimeAccessor(&RescueMac::SetLifs),
                MakeTimeChecker())
                .AddAttribute("QueueLimits",
                "Maximum packets to queue at MAC",
                UintegerValue(20),
                MakeUintegerAccessor(&RescueMac::SetQueueLimits),
                MakeUintegerChecker<uint32_t> ())
                /*.AddAttribute ("BasicAckTimeout",
                               "ACK awaiting time",
                               TimeValue (MicroSeconds (3000)),
                               MakeTimeAccessor (&RescueMac::SetBasicAckTimeout),
                               MakeTimeChecker ())*/

                .AddAttribute("CsmaMac", "The CsmaMac object",
                PointerValue(),
                MakePointerAccessor(&RescueMac::GetCsmaMac),
                MakePointerChecker<RescueMacCsma> ())
                .AddAttribute("TdmaMac", "The TdmaMac object",
                PointerValue(),
                MakePointerAccessor(&RescueMac::GetTdmaMac),
                MakePointerChecker<RescueMacTdma> ())

                .AddTraceSource("EnqueueOk",
                "Trace Hookup for enqueue a DATA",
                MakeTraceSourceAccessor(&RescueMac::m_traceEnqueue))
                .AddTraceSource("DataRxOk",
                "Trace Hookup for receive a DATA",
                MakeTraceSourceAccessor(&RescueMac::m_traceDataRxOk))
                ;
        return tid;
    }



    // ------------------------ Set Functions -----------------------------

    void
    RescueMac::AttachPhy(Ptr<RescuePhy> phy) {
        NS_LOG_FUNCTION(this << phy);
        m_phy = phy;
        if (m_csmaMac != 0) {
            m_phy->SetMacCsma(m_csmaMac);
            m_csmaMac->AttachPhy(m_phy);
        }
        if (m_tdmaMac != 0) {
            m_phy->SetMacTdma(m_tdmaMac);
            m_tdmaMac->AttachPhy(m_phy);
        }
    }

    void
    RescueMac::SetDevice(Ptr<RescueNetDevice> dev) {
        NS_LOG_FUNCTION(this << dev);
        m_device = dev;
        if (m_device != 0) {
            m_csmaMac->SetDevice(m_device);
        }
        if (m_tdmaMac != 0) {
            m_tdmaMac->SetDevice(m_device);
        }
    }

    void
    RescueMac::SetRemoteStationManager(Ptr<RescueRemoteStationManager> manager) {
        //NS_LOG_FUNCTION (this << manager);
        m_remoteStationManager = manager;
        if (m_csmaMac != 0) {
            m_csmaMac->SetRemoteStationManager(m_remoteStationManager);
        }
        if (m_tdmaMac != 0) {
            m_tdmaMac->SetRemoteStationManager(m_remoteStationManager);
        }
    }

    void
    RescueMac::SetArqManager(Ptr<RescueArqManager> arqManager) {
        //NS_LOG_FUNCTION (this << arqManager);
        m_arqManager = arqManager;
        if (m_csmaMac != 0) {
            m_csmaMac->SetArqManager(m_arqManager);
        }
        if (m_tdmaMac != 0) {
            m_tdmaMac->SetArqManager(m_arqManager);
        }
    }

    void
    RescueMac::SetForwardUpCb(Callback<void, Ptr<Packet>, Mac48Address, Mac48Address> cb) {
        m_forwardUpCb = cb;
    }

    int64_t
    RescueMac::AssignStreams(int64_t stream) {
        NS_LOG_FUNCTION(this << stream);
        m_random->SetStream(stream);
        return 1;
    }

    void
    RescueMac::SetAddress(Mac48Address addr) {
        NS_LOG_FUNCTION(this << addr);
        m_address = addr;
    }

    void
    RescueMac::SetTypeOfStation(StationType type) {
        //NS_LOG_FUNCTION (this << type);
        m_type = type;
    }

    void
    RescueMac::SetCwMin(uint32_t cw) {
        if (m_csmaMac != 0)
            m_csmaMac->SetCwMin(cw);
    }

    void
    RescueMac::SetCwMax(uint32_t cw) {
        if (m_csmaMac != 0)
            m_csmaMac->SetCwMax(cw);
    }

    void
    RescueMac::SetSlot(Time duration) {
        if (m_csmaMac != 0)
            m_csmaMac->SetSlotTime(duration);
    }

    void
    RescueMac::SetSifs(Time duration) {
        if (m_csmaMac != 0)
            m_csmaMac->SetSifsTime(duration);
        if (m_tdmaMac != 0)
            m_tdmaMac->SetSifsTime(duration);
    }

    void
    RescueMac::SetLifs(Time duration) {
        if (m_csmaMac != 0)
            m_csmaMac->SetLifsTime(duration);
        //if (m_tdmaMac != 0)
        //  m_tdmaMac->SetLifsTime (duration);
    }

    void
    RescueMac::SetQueueLimits(uint32_t length) {
        if (m_csmaMac != 0)
            m_csmaMac->SetQueueLimits(length);
        if (m_tdmaMac != 0)
            m_tdmaMac->SetQueueLimits(length);
    }

    /*void
    RescueMac::SetBasicAckTimeout (Time duration)
    {
      if (m_csmaMac != 0)
        m_csmaMac->SetBasicAckTimeout (duration);
      //if (m_tdmaMac != 0)
      //  m_tdmaMac->SetBasicAckTimeout (duration);
    }*/

    // ------------------------ Get Functions -----------------------------

    Mac48Address
    RescueMac::GetAddress() const {
        //NS_LOG_FUNCTION (this);
        return m_address;
    }

    Mac48Address
    RescueMac::GetBroadcast(void) const {
        return Mac48Address::GetBroadcast();
    }

    Ptr<RescueMacCsma>
    RescueMac::GetCsmaMac(void) const {
        return m_csmaMac;
    }

    Ptr<RescueMacTdma>
    RescueMac::GetTdmaMac(void) const {
        return m_tdmaMac;
    }

    uint32_t
    RescueMac::GetCwMin(void) const {
        return m_csmaMac->GetCwMin();
    }

    uint32_t
    RescueMac::GetCwMax(void) const {
        return m_csmaMac->GetCwMax();
    }

    Time
    RescueMac::GetSlot(void) const {
        return m_csmaMac->GetSlotTime();
    }

    Time
    RescueMac::GetSifs(void) const {
        return m_csmaMac->GetSifsTime();
    }

    Time
    RescueMac::GetLifs(void) const {
        return m_csmaMac->GetLifsTime();
    }

    uint32_t
    RescueMac::GetQueueLimits(void) const {
        if (m_csmaMac != 0)
            return m_csmaMac->GetQueueLimits();
            //else if (m_tdmaMac != 0)
            //  return m_tdmaMac->GetQueueLimits ()
        else return 0;
    }

    /*Time
    RescueMac::GetBasicAckTimeout (void) const
    {
      if (m_csmaMac != 0)
        return m_csmaMac->GetBasicAckTimeout ();
      //else if (m_tdmaMac != 0)
      //  return m_tdmaMac->GetBasicAckTimeout ()
      else return Seconds (0);
    }*/



    // ----------------------- Queue/Send Functions -----------------------------

    void
    RescueMac::Enqueue(Ptr<Packet> pkt, Mac48Address dst) {
        // We expect RegularRescueMac subclasses which do support forwarding (e.g.,
        // AP) to override this method. Therefore, we throw a fatal error if
        // someone tries to invoke this method on a class which has not done
        // this.
        NS_FATAL_ERROR("This MAC entity (" << this << ", " << GetAddress()
                << ") does not support Enqueue ()");

        /*NS_LOG_FUNCTION ("dst:" << dst <<
                         "size [B]:" << pkt->GetSize ());

        RescueMacHeader hdr = RescueMacHeader (m_address, dst, RESCUE_MAC_PKT_TYPE_DATA);
        pkt->AddHeader (hdr);
        m_traceEnqueue (m_device->GetNode ()->GetId (), m_device->GetIfIndex (), pkt, hdr);
        m_csmaMac->Enqueue (pkt, dst);*/
    }

    void
    RescueMac::EnqueueRetry(Ptr<Packet> pkt, Mac48Address dst) {
        NS_FATAL_ERROR("This MAC entity (" << this << ", " << GetAddress()
                << ") does not support EnqueueRetry ()");
    }

    void
    //RescueMac::NotifyEnqueueRelay (Ptr<Packet> pkt, Mac48Address dst)
    RescueMac::NotifyEnqueuedPacket(Ptr<Packet> pkt, Mac48Address dst) {
        NS_FATAL_ERROR("This MAC entity (" << this << ", " << GetAddress()
                << ") does not support NotifyEnqueueRelay ()");
    }

    void
    RescueMac::EnqueueAck(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_FATAL_ERROR("This MAC entity (" << this << ", " << GetAddress()
                << ") does not support EnqueueAck ()");
    }

    void
    RescueMac::EnqueueOk(Ptr<const Packet> pkt, const RescueMacHeader &hdr) {
        NS_LOG_FUNCTION(this << hdr);
        m_traceEnqueue(m_device->GetNode()->GetId(), m_device->GetIfIndex(), pkt, hdr);
    }

    void
    RescueMac::NotifySendPacketDone() {
        NS_LOG_FUNCTION("");
    }

    // ---------------------- Receive Functions ----------------------------

    void
    RescueMac::ReceivePacket(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        NS_LOG_INFO("Forward up the stack!");
        RxOk(pkt, phyHdr);
        m_forwardUpCb(pkt, phyHdr.GetSource(), phyHdr.GetDestination());
    }

    void
    RescueMac::RxOk(Ptr<const Packet> pkt, const RescuePhyHeader &hdr) {
        NS_LOG_FUNCTION(this << hdr);
        m_traceDataRxOk(m_device->GetNode()->GetId(), m_device->GetIfIndex(), pkt, hdr);
    }

    void
    RescueMac::ReceiveResourceReservation(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        //This method should be overriden by RegularRescueMac subclasses.
        NS_LOG_FUNCTION("");
    }

    void
    RescueMac::ReceiveBeacon(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        //This method should be overriden by RegularRescueMac subclasses.
        NS_LOG_FUNCTION("");
    }

    bool
    RescueMac::ShouldBeForwarded(Ptr<Packet> pkt, RescuePhyHeader phyHdr) {
        return true;
    }



    // ---------------------- Receive Functions ----------------------------

    void
    RescueMac::NotifyTxAllowed(void) {
        NS_LOG_FUNCTION("");
    }


} // namespace ns3

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
 * and ns-3 wifi module by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/mac48-address.h"

#include "rescue-mac.h"
#include "rescue-mac-csma.h"
#include "rescue-mac-tdma.h"
#include "rescue-phy.h"
#include "rescue-mac-header.h"
#include "rescue-phy-header.h"
#include "snr-per-tag.h"
#include "blackbox_no1.h"
#include "blackbox_no1.h"
#include "blackbox_no2.h"
#include "src/core/model/object.h"
#include "ns3/node.h"

NS_LOG_COMPONENT_DEFINE("RescuePhy");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[time=" << ns3::Simulator::Now().GetSeconds() << "] [addr=" << ((m_mac != 0) ? compressMac(m_mac->GetAddress ()) : 0) << "] [PHY] "

#define Min(a,b) ((a < b) ? a : b)
#define Max(a,b) ((a > b) ? a : b)

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(RescuePhy);

    RescuePhy::RescuePhy()
    : m_device(0),
    m_mac(0),
    m_csmaMac(0),
    m_channel(0),
    m_pktRx(0) {
        m_csBusy = false;
        m_csBusyEnd = Seconds(0);
        m_random = CreateObject<UniformRandomVariable> ();

        m_deviceRateSet.push_back(RescuePhy::GetOfdm3Mbps());
        m_deviceRateSet.push_back(RescuePhy::GetOfdm6Mbps());
        m_deviceRateSet.push_back(RescuePhy::GetOfdm9Mbps());
        m_deviceRateSet.push_back(RescuePhy::GetOfdm12Mbps());
        m_deviceRateSet.push_back(RescuePhy::GetOfdm18Mbps());
        m_deviceRateSet.push_back(RescuePhy::GetOfdm24Mbps());
        m_deviceRateSet.push_back(RescuePhy::GetOfdm36Mbps());
    }

    RescuePhy::~RescuePhy() {
        m_deviceRateSet.clear();
        Clear();
    }

    void
    RescuePhy::Clear() {
        m_pktRx = 0;
    }

    RescuePhy::RxFrameCopies::RxFrameCopies(RescuePhyHeader phyHdr,
            Ptr<Packet> pkt,
            std::vector<double> snr_db,
            std::vector<double> linkPER,
            std::vector<double> linkBER,
            //double constellationSize,
            std::vector<int> constellationSizes,
            //double spectralEfficiency,
            std::vector<double> spectralEfficiencies,
            int packetLength,
            Time tstamp)
    : phyHdr(phyHdr),
    pkt(pkt),
    snr_db(snr_db),
    linkPER(linkPER),
    linkBER(linkBER),
    //constellationSize (constellationSize),
    constellationSizes(constellationSizes),
    //spectralEfficiency (spectralEfficiency),
    spectralEfficiencies(spectralEfficiencies),
    packetLength(packetLength),
    tstamp(tstamp) {
    }

    TypeId
    RescuePhy::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::RescuePhy")
                .SetParent<Object> ()
                .AddConstructor<RescuePhy> ()
                .AddAttribute("PreambleDuration",
                "Duration [us] of Preamble of PHY Layer",
                TimeValue(MicroSeconds(16)),
                MakeTimeAccessor(&RescuePhy::m_preambleDuration),
                MakeTimeChecker())
                .AddAttribute("TrailerSize",
                "Size of Trailer (e.g. FCS) [B]",
                UintegerValue(0),
                MakeUintegerAccessor(&RescuePhy::m_trailerSize),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("CsPowerThr",
                "Carrier Sense Threshold [dBm]",
                DoubleValue(-110),
                MakeDoubleAccessor(&RescuePhy::m_csThr),
                MakeDoubleChecker<double> ())
                .AddAttribute("RxPowerThr",
                "Receive Power Threshold [dBm]",
                DoubleValue(-108),
                MakeDoubleAccessor(&RescuePhy::m_rxThr),
                MakeDoubleChecker<double> ())
                .AddAttribute("TxPower",
                "Transmission Power [dBm]",
                DoubleValue(10),
                MakeDoubleAccessor(&RescuePhy::SetTxPower),
                MakeDoubleChecker<double> ())
                .AddAttribute("BerThr",
                "BER threshold - frames with higher BER are unusable for reconstruction purposes and should not be stored or forwarded",
                DoubleValue(0.5),
                MakeDoubleAccessor(&RescuePhy::m_berThr),
                MakeDoubleChecker<double> ())
                .AddAttribute("UseBB2",
                "Use BlackBox #2 for error calculation? (if it is possible)",
                BooleanValue(false),
                MakeBooleanAccessor(&RescuePhy::m_useBB2),
                MakeBooleanChecker())
                .AddAttribute("UseLOTF",
                "Use links-on-the-fly",
                BooleanValue(true),
                MakeBooleanAccessor(&RescuePhy::m_useLOTF),
                MakeBooleanChecker())
                .AddTraceSource("SendOk",
                "Trace Hookup for enqueue a DATA",
                MakeTraceSourceAccessor(&RescuePhy::m_traceSend))
                .AddTraceSource("RecvOk",
                "Trace Hookup for enqueue a DATA",
                MakeTraceSourceAccessor(&RescuePhy::m_traceRecv))
                ;
        return tid;
    }



    // ------------------------ Set Functions -----------------------------

    void
    RescuePhy::SetDevice(Ptr<RescueNetDevice> device) {
        m_device = device;
    }

    void
    RescuePhy::SetMac(Ptr<RescueMac> mac) {
        //NS_LOG_FUNCTION (this << mac);
        m_mac = mac;
    }

    void
    RescuePhy::SetMacCsma(Ptr<RescueMacCsma> csmaMac) {
        //NS_LOG_FUNCTION (this << csmaMac);
        m_csmaMac = csmaMac;
        m_lowMac = m_csmaMac;
    }

    void
    RescuePhy::SetMacTdma(Ptr<RescueMacTdma> tdmaMac) {
        m_tdmaMac = tdmaMac;
    }

    void
    RescuePhy::NotifyCFP() {
        NS_LOG_FUNCTION("");
        m_lowMac = m_tdmaMac;
    }

    void
    RescuePhy::NotifyCP() {
        NS_LOG_FUNCTION("");
        m_lowMac = m_csmaMac;
    }

    void
    RescuePhy::SetChannel(Ptr<RescueChannel> channel) {
        m_channel = channel;
    }

    void
    RescuePhy::SetTxPower(double dBm) {
        m_txPower = dBm;
    }

    /*void
    RescuePhy::SetErrorRateModel (Ptr<RescueErrorRateModel> rate)
    {
      m_errorRateModel = rate;
    }*/



    // ------------------------ Get Functions -----------------------------

    Ptr<RescueChannel>
    RescuePhy::GetChannel() {
        return m_channel;
    }

    double
    RescuePhy::GetTxPower() {
        return m_txPower;
    }

    /*Ptr<RescueErrorRateModel>
    RescuePhy::GetErrorRateModel (void) const
    {
      return m_errorRateModel;
    }*/



    // ----------------------- Send Functions ------------------------------

    bool
    RescuePhy::SendPacket(Ptr<Packet> pkt, RescueMode mode, uint16_t il) {
        NS_LOG_FUNCTION("");
        // RX might be interrupted by TX, but not vice versa
        if (m_state == TX) {
            NS_LOG_DEBUG("Already in transmission mode");
            return false;
        }

        Ptr<Packet> p = pkt->Copy(); //copy packet stored in MAC layer

        m_state = TX;
        RescueMacHeader currentMacHdr;
        p->PeekHeader(currentMacHdr);
        NS_LOG_FUNCTION("v1 src:" << currentMacHdr.GetSource() <<
                "dst:" << currentMacHdr.GetDestination() <<
                "seq:" << currentMacHdr.GetSequence());
        RescuePhyHeader currentPhyHdr = RescuePhyHeader(currentMacHdr.GetSource(),
                currentMacHdr.GetSource(),
                currentMacHdr.GetDestination(),
                currentMacHdr.GetType(),
                currentMacHdr.GetSequence(),
                il);

        currentMacHdr.IsRetry() ? currentPhyHdr.SetRetry() : currentPhyHdr.SetNoRetry();

        Time txDuration = CalTxDuration(currentPhyHdr.GetSize(), p->GetSize(),
                GetPhyHeaderMode(mode), mode);

        NS_LOG_DEBUG("Tx will finish at " << (Simulator::Now() + txDuration).GetSeconds() <<
                " (" << txDuration.GetNanoSeconds() << "ns), txPower: " << m_txPower);

        p->AddHeader(currentPhyHdr);

        // forward to CHANNEL
        m_channel->SendPacket(Ptr<RescuePhy> (this), p, m_txPower, mode, txDuration);

        return true;
    }

    bool
    RescuePhy::SendPacket(std::pair<Ptr<Packet>, RescuePhyHeader> relayedPkt, RescueMode mode) {
        NS_LOG_FUNCTION("v2 src:" << relayedPkt.second.GetSource() <<
                "dst:" << relayedPkt.second.GetDestination() <<
                "seq:" << relayedPkt.second.GetSequence());
        // RX might be interrupted by TX, but not vice versa
        if (m_state == TX) {
            NS_LOG_DEBUG("Already in transmission mode");
            return false;
        }

        m_state = TX;
        Time txDuration = CalTxDuration(relayedPkt.second.GetSize(), relayedPkt.first->GetSize(),
                GetPhyHeaderMode(mode), mode);

        NS_LOG_DEBUG("Tx will finish at " << (Simulator::Now() + txDuration).GetSeconds() <<
                " (" << txDuration.GetNanoSeconds() << "ns), txPower: " << m_txPower);

        relayedPkt.first->AddHeader(relayedPkt.second);

        // forward to CHANNEL
        m_channel->SendPacket(Ptr<RescuePhy> (this), relayedPkt.first, m_txPower, mode, txDuration);

        return true;
    }

    void
    RescuePhy::SendPacketDone(Ptr<Packet> pkt) {
        NS_LOG_FUNCTION("");
        m_state = IDLE;
        //MODIF
        m_traceSend(1);
        m_lowMac->SendPacketDone(pkt);
    }



    // ---------------------- Receive Functions ----------------------------

    //MODIF
    bool doubleReception = false;

    void
    RescuePhy::ReceivePacket(Ptr<Packet> pkt, RescueMode mode, Time txDuration, double_t rxPower) {
        NS_LOG_FUNCTION("rxPower:" << rxPower <<
                "mode:" << mode <<
                "busyEnd:" << m_csBusyEnd);

        NS_LOG_DEBUG("Node " << (uint16_t) compressMac(m_device->GetMac()->GetAddress()) << " \t THRE" << m_csThr << "\t RX POWEEEER " << rxPower);


        if (m_state == TX) {
            NS_LOG_INFO("Drop packet due to half-duplex");
            return;
        }

        // Start RX when energy is bigger than carrier sense threshold
        //
        Time txEnd = Simulator::Now() + txDuration;
        if (rxPower > m_csThr && txEnd > m_csBusyEnd) {
            if (m_csBusy == false) {
                m_csBusy = true;
                m_pktRx = pkt;
                m_lowMac->ReceivePacket(this, pkt);
                //MODIF
            } else {
                NS_LOG_DEBUG("PHY BUSYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY");
                doubleReception = true;
            }
            m_state = RX;
            m_csBusyEnd = txEnd;
        }
    }

    void
    RescuePhy::ReceivePacketDone(Ptr<Packet> pkt, RescueMode mode, double rxPower) {
        NS_LOG_FUNCTION("busyEnd:" << m_csBusyEnd);

        if (m_csBusyEnd <= Simulator::Now() + NanoSeconds(1)) {
            m_csBusy = false;
        }

        if (m_state != RX) {
            NS_LOG_INFO("Drop packet due to state");
            return;
        }

        //MODIF
        //        if (doubleReception) {
        //            rxPower -= 20.0;
        //            NS_LOG_UNCOND(this->m_device->GetNode()->GetId() << " Receiving multiple packets at once, lowering RXPOWER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        //        }

        // We do support SINR !!
        double noiseW = m_channel->GetNoiseW(this, pkt); // noise plus interference
        //double rxPowerW = m_channel->DbmToW (rxPower);
        double sinr = rxPower - m_channel->WToDbm(noiseW);

        NS_LOG_INFO("rxPower: " << rxPower << " ,noise: " << m_channel->WToDbm(noiseW) << " ,sinr: " << sinr);

        if ((rxPower > m_rxThr) && (pkt == m_pktRx)) {

            //MODIF
            m_traceRecv(1);

            RescuePhyHeader hdr;
            pkt->RemoveHeader(hdr);

            std::vector<double> snr_db;
            std::vector<double> linkPER;
            snr_db.push_back(sinr);
            linkPER.push_back(0.0);

            double hsr = 1.0; //header success rate

            //Calculate Preamble Success Rate
            /*hsr *= CalculateChunkSuccessRate (sinr,
                                              GetPhyPreambleDuration (mode),
                                              GetPhyPreambleMode (mode));*/
            int preambleBits = GetPhyPreambleDuration(mode).GetMicroSeconds() * GetPhyPreambleMode(mode).GetDataRate() / 1000000;
            hsr *= (1 - BlackBox_no1::CalculateRescuePacketErrorRate(snr_db, linkPER,
                    GetPhyPreambleMode(mode).GetConstellationSize(),
                    GetPhyPreambleMode(mode).GetSpectralEfficiency(),
                    preambleBits));

            //Calculate PHY Header Success Rate
            /*hsr *= CalculateChunkSuccessRate (sinr,
                                              GetPhyHeaderDuration (hdr, mode),
                                              GetPhyHeaderMode (mode));*/
            hsr *= (1 - BlackBox_no1::CalculateRescuePacketErrorRate(snr_db, linkPER,
                    GetPhyHeaderMode(mode).GetConstellationSize(),
                    GetPhyHeaderMode(mode).GetSpectralEfficiency(),
                    8 * hdr.GetSize()));
            /*hsr *= (1 - BlackBox_no1::CalculateRescuePacketErrorRate (snr_db, linkPER,
                                                                      GetPhyHeaderMode (mode).GetConstellationSize (),
                                                                      GetPhyHeaderMode (mode).GetSpectralEfficiency (),
                                                                      8 * hdr.GetSize () ) );*/
            double her = 1 - hsr; //header error rate*/

            bool correct = true;
            /*if (m_useBB2)
               correct = (0 == (BlackBox_no2::CalculateRescueBitErrorNumber (snr_db,
                                                                             BlackBox_no1::PERtoBER (linkPER, 272),
                                                                             GetPhyHeaderMode (mode).GetConstellationSize (),
                                                                             GetPhyHeaderMode (mode).GetSpectralEfficiency ()) ) );
            else
              {*/
            //correct = (m_random->GetValue () > her);
            //}

            NS_LOG_DEBUG("mode=" << mode <<
                    ", snr=" << sinr <<
                    ", her=" << her <<
                    ", size=" << pkt->GetSize());

            if (correct) //is the PHY header complete
            {
                //PHY HEADER CORRECTLY RECEIVED
                NS_LOG_INFO("PHY HDR CORRECT");
                m_state = IDLE;
                //MODIF
                doubleReception = false;

                NS_LOG_FUNCTION("src:" << hdr.GetSource() <<
                        "sender:" << hdr.GetSender() <<
                        "dst:" << hdr.GetDestination() <<
                        "seq:" << hdr.GetSequence());

                if (hdr.GetType() != RESCUE_PHY_PKT_TYPE_DATA) //CONTROL FRAME - no payload processing
                {
                    m_lowMac->ReceivePacketDone(this, pkt, hdr, sinr, mode, true, true, false);
                } else //PER (and SNR?) tag must be updated
                {
                    SnrPerTag tag;
                    //double per = (pkt->PeekPacketTag (tag) ? tag.GetPER () : 0); //packet (payload) error rate
                    double ber = (pkt->PeekPacketTag(tag) ? tag.GetBER() : 0); //packet (payload) bit error rate
                    double per = BlackBox_no1::BERtoPER(ber, 8 * pkt->GetSize());
                    double snr = (pkt->PeekPacketTag(tag) ? tag.GetSNR() : (m_txPower - m_channel->WToDbm(noiseW))); //packet SNR (if not recorded, use maximal possible value)
                    //                    NS_LOG_UNCOND("PER from previous hops: " << per << ", BER: " << ber << ", min SNR on route: " << snr);

                    /*double psr = 1 - per; //packet (payload) success rate
                    psr *= CalculateChunkSuccessRate (sinr,
                                                      GetDataDuration (pkt, mode),
                                                      mode); //should we modify PER value with by the result of classic error-rate-model?
                    per = 1 - psr;
                    NS_LOG_DEBUG ("PER after last hop: " << per);*/

                    //snr_db.push_back (sinr);
                    //linkPER[0] = per;
                    std::vector<double> linkBER;
                    linkBER.push_back(ber);
                    ber = BlackBox_no1::CalculateRescueBitErrorRate(snr_db, linkBER, //linkPER,
                            //BlackBox_no1::PERtoBER (linkPER, 8 * pkt->GetSize ()),
                            mode.GetConstellationSize(),
                            mode.GetSpectralEfficiency());
                    per = BlackBox_no1::BERtoPER(ber, 8 * pkt->GetSize());
                    NS_LOG_DEBUG("PER after last hop (from BlackBox_no1): " << per << ", BER: " << ber);

                    //update packet tag
                    //tag.SetSNR (sinr); //not shure what to do - store last hp snr?!?
                    tag.SetSNR(Min(sinr, snr)); //not shure what to do - store minimal snr?!?
                    //tag.SetPER (per);
                    tag.SetBER(ber);
                    pkt->ReplacePacketTag(tag);

                    //NS_LOG_DEBUG ("data rate: " << mode.GetDataRate () << " phy rate: " << mode.GetPhyRate () << " constellation size: " << (int)mode.GetConstellationSize () << " spectralEfficiency: " << mode.GetSpectralEfficiency ());

                    //should BlackBox #2 be used?
                    bool useBB2 = (m_useBB2 //BlackBox #2 enabled?
                            && ((mode.GetConstellationSize() == 2) || (mode.GetConstellationSize() == 4))); //supported modulations
                    //&& (snr_db.size () <= 2) //supported number of links
                    //&& !((snr_db.size () == 1) && (linkPER[0] > 0)) ); //bug in BlackBox #2 for single link calculation

                    if (hdr.GetDestination() != m_mac->GetAddress()) //RELAY THIS FRAME? - no payload processing but PER tag must be updated
                    {
                        if (m_useLOTF) //LOTF used, is the BER sufficient?
                        {
                            correct = (ber <= m_berThr);
                            NS_LOG_INFO("FRAME " << (correct ? "CORRECT" : "DAMAGED") << " [because ber <= m_berThr]");
                        } else if (useBB2) //no LOTF, is the payload correct
                        {
                            correct = (0 == (BlackBox_no2::CalculateRescueBitErrorNumber(snr_db, linkBER,
                                    //BlackBox_no1::PERtoBER (linkPER, 8000),
                                    mode.GetConstellationSize(),
                                    mode.GetSpectralEfficiency())));
                            NS_LOG_INFO("FRAME " << (correct ? "CORRECT" : "DAMAGED") << " [BlackBox_no2 " << (correct ? "does not detect" : "detects") << " errors]");
                        } else {
                            correct = (m_random->GetValue() > per);
                            NS_LOG_INFO("FRAME " << (correct ? "CORRECT" : "DAMAGED") << " [test: m_random->GetValue () > per]");
                        }

                        m_lowMac->ReceivePacketDone(this, pkt, hdr,
                                sinr, mode, true,
                                correct,
                                false); //(ber < BER THRESHOLD) - inform MAC about BER - not forward if BER > BER THRESHOLD - unusable frame
                    } else //DATA FRAME DESTINED FOR THIS DEVICE
                    {
                        //is the payload complete?
                        if (useBB2) {
                            correct = (0 == (BlackBox_no2::CalculateRescueBitErrorNumber(snr_db, linkBER,
                                    //BlackBox_no1::PERtoBER (linkPER, 8000),
                                    mode.GetConstellationSize(),
                                    mode.GetSpectralEfficiency())));
                            NS_LOG_INFO("FRAME " << (correct ? "CORRECT" : "DAMAGED") << " [BlackBox_no2 " << (correct ? "does not detect" : "detects") << " errors]");
                        } else {
                            correct = (m_random->GetValue() > per);
                            NS_LOG_INFO("FRAME " << (correct ? "CORRECT" : "DAMAGED") << " [test: m_random->GetValue () > per]");
                        }

                        if (correct) {
                            //no errors - just forward up
                            RemoveFrameCopies(hdr);
                            m_lowMac->ReceivePacketDone(this, pkt, hdr, sinr, mode, true, true, false);
                        } else if ((m_useLOTF)
                                && (ber <= m_berThr)) //BER > BER THRESHOLD - unusable frame
                        {
                            NS_LOG_INFO("PHY HDR OK!, DATA: DAMAGED!");
                            ber = tag.GetBER();
                            per = BlackBox_no1::BERtoPER(ber, 8 * pkt->GetSize());
                            if (AddFrameCopy(pkt, hdr, sinr, per, ber, mode)) //STOR FRAME COPY - check: is it the first one?
                            {
                                //it is NOT first copy - try to restore data using previously stored frame copies and notify
                                m_lowMac->ReceivePacketDone(this, pkt, hdr, sinr, mode, true, IsRestored(hdr, useBB2), true);
                            } else {
                                //it is first copy - just notify MAC
                                m_lowMac->ReceivePacketDone(this, pkt, hdr, sinr, mode, true, false, true);
                            }
                        } else {
                            NS_LOG_INFO("PHY HDR OK!, DATA: UNUSABLE (BER >=" << m_berThr << ") - DROP IT!");
                        }
                    }
                }
                return;
            } else {
                //to notify unusable frame
                m_lowMac->ReceivePacketDone(this, pkt, hdr, sinr, mode, false, false, false);
                NS_LOG_INFO("PHY HDR DAMAGED!");
            }
        } else if (!m_csBusy) {
            RescuePhyHeader hdr;
            pkt->RemoveHeader(hdr);
            m_lowMac->ReceivePacketDone(this, pkt, hdr, sinr, mode, false, false, false); //to notify unusable frame
        }

        if (!m_csBusy) // set MAC state IDLE
        {
            m_state = IDLE;
        }
    }

    void
    RescuePhy::RemoveFrameCopies(RescuePhyHeader phyHdr) {
        NS_LOG_FUNCTION("");
        for (RxFrameAccumulatorI it = m_rxFrameAccumulator.begin(); it != m_rxFrameAccumulator.end(); it++) {
            //NS_LOG_DEBUG("queue iterator = " << ++i << " source: " << it->src << " destination: " << it->dst << " seq. number: " << it->seq_no << " tstamp: " << it->tstamp);
            if ((it->phyHdr.GetSource() == phyHdr.GetSource())
                    && (it->phyHdr.GetSequence() == phyHdr.GetSequence())) {
                it = m_rxFrameAccumulator.erase(it);
                break;
            }
        }
    }

    bool
    RescuePhy::AddFrameCopy(Ptr<Packet> pkt, RescuePhyHeader phyHdr, double sinr, double per, double ber, RescueMode mode) {
        NS_LOG_INFO("STORE FRAME COPY");

        for (RxFrameAccumulatorI it = m_rxFrameAccumulator.begin(); it != m_rxFrameAccumulator.end(); it++) {
            //NS_LOG_DEBUG("queue iterator = " << ++i << " source: " << it->src << " destination: " << it->dst << " seq. number: " << it->seq_no << " tstamp: " << it->tstamp);
            if ((it->phyHdr.GetSource() == phyHdr.GetSource())
                    && (it->phyHdr.GetSequence() == phyHdr.GetSequence())) {
                if ((it->snr_db.size() < 3) || !m_useBB2) //BlackBox #2 accepts only 3 copies, so store the best of them
                {
                    NS_LOG_INFO(it->snr_db.size() << " PREVIOUS COPY/IES FOUND, ADD THIS COPY");
                    it->snr_db.push_back(sinr);
                    it->linkPER.push_back(per);
                    it->linkBER.push_back(ber);
                    it->constellationSizes.push_back(mode.GetConstellationSize());
                    it->spectralEfficiencies.push_back(mode.GetSpectralEfficiency());
                } else {
                    //find the worst copy and store new copy if it is better [because BB #2 process only 3 copies]
                    uint32_t max_i = 0;
                    for (uint32_t i = 0; i < it->linkBER.size(); ++i)
                        if (it->linkBER[i] > it->linkBER[max_i])
                            max_i = i;

                    if (it->linkBER[max_i] > ber) {
                        NS_LOG_INFO(it->snr_db.size() << " PREVIOUS COPIES FOUND, STORE THIS COPY (BER=" << ber << ") INSTEAD OF " << max_i << " (BER=" << it->linkBER[max_i] << ")");
                        it->snr_db[max_i] = sinr;
                        it->linkPER[max_i] = per;
                        it->linkBER[max_i] = ber;
                        it->constellationSizes[max_i] = mode.GetConstellationSize();
                        it->spectralEfficiencies[max_i] = mode.GetSpectralEfficiency();
                    } else
                        NS_LOG_INFO(it->snr_db.size() << " PREVIOUS BETTER COPIES FOUND, DON'T STORE THIS COPY");
                }
                return true;
            }
        }

        NS_LOG_INFO("NO PREVIOUS COPIES, STORE FIRST COPY");
        std::vector<double> snr_db;
        snr_db.push_back(sinr);

        std::vector<double> linkPER;
        linkPER.push_back(per);

        std::vector<double> linkBER;
        linkBER.push_back(ber);

        std::vector<int> constellationSizes;
        constellationSizes.push_back(mode.GetConstellationSize());

        std::vector<double> spectralEfficiencies;
        spectralEfficiencies.push_back(mode.GetSpectralEfficiency());

        m_rxFrameAccumulator.push_back(RxFrameCopies(phyHdr, pkt,
                snr_db, linkPER, linkBER,
                //mode.GetConstellationSize (),
                constellationSizes,
                //mode.GetSpectralEfficiency (),
                spectralEfficiencies,
                pkt->GetSize(),
                ns3::Simulator::Now()));
        return false;
    }

    /*double
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
    }*/

    double
    RescuePhy::CalculateLLR(double sinr) {
        //for future use
        return 0;
    }

    double
    RescuePhy::CalculateCI(double prevCI, double llr, uint32_t symbols) {
        //for future use
        return 0;
    }

    bool
    RescuePhy::IsRestored(RescuePhyHeader phyHdr, bool useBB2) {
        NS_LOG_FUNCTION("");
        bool restored = false;

        //try to reconstruct data if there is any copy of this frame in the buffer?
        for (RxFrameAccumulatorI it = m_rxFrameAccumulator.begin(); it != m_rxFrameAccumulator.end(); it++) {
            //NS_LOG_DEBUG("queue iterator = " << ++i << " source: " << it->src << " destination: " << it->dst << " seq. number: " << it->seq_no << " tstamp: " << it->tstamp);
            if ((it->phyHdr.GetSource() == phyHdr.GetSource())
                    && (it->phyHdr.GetSequence() == phyHdr.GetSequence())) {
                if (useBB2) {
                    restored = (0 == (BlackBox_no2::CalculateRescueBitErrorNumber(it->snr_db,
                            it->linkBER,
                            //BlackBox_no1::PERtoBER (it->linkPER, 8 * it->packetLength),
                            it->constellationSizes,
                            it->spectralEfficiencies)));
                    NS_LOG_INFO("FRAME " << (restored ? "RESTORED" : "NOT RESTORED") << " [BlackBox_no2 " << (restored ? "does not detect" : "detects") << " errors]");
                } else {
                    double blackBoxPER = BlackBox_no1::CalculateRescuePacketErrorRate(it->snr_db,
                            it->linkPER,
                            //it->linkBER,
                            it->constellationSizes,
                            it->spectralEfficiencies,
                            8 * it->packetLength);
                    NS_LOG_DEBUG("BlackBox_no1 PER = " << blackBoxPER);
                    restored = (m_random->GetValue() > blackBoxPER);
                    NS_LOG_INFO("FRAME " << (restored ? "RESTORED" : "NOT RESTORED") << " [test: m_random->GetValue () > per]");
                }
                if (restored) {
                    //MAGIC THINGS HAPPENS HERE !!!
                    it = m_rxFrameAccumulator.erase(it);
                    break;
                }
            }
        }

        return restored;
    }



    // --------------------------- ETC -------------------------------------

    bool
    RescuePhy::IsIdle() {
        if (m_state == IDLE && !m_csBusy) {
            return true;
        }
        return false;
    }

    Time
    RescuePhy::CalTxDuration(uint32_t basicSize, uint32_t dataSize, RescueMode basicMode, RescueMode dataMode, uint16_t type) {
        //for TX time calculation when PhyHdr is not constructed yet
        RescuePhyHeader hdr = RescuePhyHeader(type);
        double_t txHdrTime = (double) (hdr.GetSize() + basicSize) * 8.0 / basicMode.GetDataRate();
        double_t txMpduTime = (double) (dataSize + m_trailerSize) * 8.0 / dataMode.GetDataRate();
        return m_preambleDuration + Seconds(txHdrTime) + Seconds(txMpduTime);
    }

    Time
    RescuePhy::CalTxDuration(uint32_t basicSize, uint32_t dataSize, RescueMode basicMode, RescueMode dataMode) {
        //for TX time calculation when PhyHdr is alreade constructed
        double_t txHdrTime = (double) basicSize * 8.0 / basicMode.GetDataRate();
        double_t txMpduTime = (double) (dataSize + m_trailerSize) * 8.0 / dataMode.GetDataRate();
        return m_preambleDuration + Seconds(txHdrTime) + Seconds(txMpduTime);
    }

    int64_t
    RescuePhy::AssignStreams(int64_t stream) {
        NS_LOG_FUNCTION(this << stream);
        m_random->SetStream(stream);
        return 1;
    }



    // ------------------------ TX MODES -----------------------------------

    Time
    RescuePhy::GetPhyPreambleDuration(RescueMode mode) {
        return m_preambleDuration;
    }

    Time
    RescuePhy::GetPhyHeaderDuration(RescuePhyHeader phyHdr, RescueMode mode) {
        NS_LOG_FUNCTION("size: " << phyHdr.GetSize() << "rate: " << mode.GetDataRate());
        return MicroSeconds(1000000 * 8 * phyHdr.GetSize() / mode.GetDataRate());
    }

    Time
    RescuePhy::GetDataDuration(Ptr<Packet> pkt, RescueMode mode) {
        NS_LOG_FUNCTION("size: " << pkt->GetSize() << "rate: " << mode.GetDataRate());
        return MicroSeconds(1000000 * 8 * pkt->GetSize() / mode.GetDataRate());
    }

    RescueMode
    RescuePhy::GetPhyPreambleMode(RescueMode payloadMode) {
        //return RescuePhy::GetOfdm6Mbps ();
        return RescuePhy::GetOfdm3Mbps();
    }

    RescueMode
    RescuePhy::GetPhyHeaderMode(RescueMode payloadMode) {
        //return RescuePhy::GetOfdm6Mbps ();
        return RescuePhy::GetOfdm3Mbps();
    }

    uint32_t
    RescuePhy::GetNModes(void) const {
        return m_deviceRateSet.size();
    }

    RescueMode
    RescuePhy::GetMode(uint32_t mode) const {
        return m_deviceRateSet[mode];
    }

    bool
    RescuePhy::IsModeSupported(RescueMode mode) const {
        for (uint32_t i = 0; i < GetNModes(); i++) {
            if (mode == GetMode(i)) {
                return true;
            }
        }
        return false;
    }

    RescueMode
    RescuePhy::GetOfdm3Mbps() {
        static RescueMode mode =
                RescueModeFactory::CreateRescueMode("Ofdm3Mbps",
                RESCUE_MOD_CLASS_OFDM,
                20000000, 3000000,
                RESCUE_CODE_RATE_1_4, 2);
        return mode;
    }

    RescueMode
    RescuePhy::GetOfdm6Mbps() {
        static RescueMode mode =
                RescueModeFactory::CreateRescueMode("Ofdm6Mbps",
                RESCUE_MOD_CLASS_OFDM,
                20000000, 6000000,
                RESCUE_CODE_RATE_1_2, 2);
        return mode;
    }

    RescueMode
    RescuePhy::GetOfdm9Mbps() {
        static RescueMode mode =
                RescueModeFactory::CreateRescueMode("Ofdm9Mbps",
                RESCUE_MOD_CLASS_OFDM,
                20000000, 9000000,
                RESCUE_CODE_RATE_3_4, 2);
        return mode;
    }

    RescueMode
    RescuePhy::GetOfdm12Mbps() {
        static RescueMode mode =
                RescueModeFactory::CreateRescueMode("Ofdm12Mbps",
                RESCUE_MOD_CLASS_OFDM,
                20000000, 12000000,
                RESCUE_CODE_RATE_1_2, 4);
        return mode;
    }

    RescueMode
    RescuePhy::GetOfdm18Mbps() {
        static RescueMode mode =
                RescueModeFactory::CreateRescueMode("Ofdm18Mbps",
                RESCUE_MOD_CLASS_OFDM,
                20000000, 18000000,
                RESCUE_CODE_RATE_3_4, 4);
        return mode;
    }

    RescueMode
    RescuePhy::GetOfdm24Mbps() {
        static RescueMode mode =
                RescueModeFactory::CreateRescueMode("Ofdm24Mbps",
                RESCUE_MOD_CLASS_OFDM,
                20000000, 24000000,
                RESCUE_CODE_RATE_1_2, 16);
        return mode;
    }

    RescueMode
    RescuePhy::GetOfdm36Mbps() {
        static RescueMode mode =
                RescueModeFactory::CreateRescueMode("Ofdm36Mbps",
                RESCUE_MOD_CLASS_OFDM,
                20000000, 36000000,
                RESCUE_CODE_RATE_3_4, 16);
        return mode;
    }

} // namespace ns3


namespace {

    static class Constructor {
    public:

        Constructor() {
            ns3::RescuePhy::GetOfdm3Mbps();
            ns3::RescuePhy::GetOfdm6Mbps();
            ns3::RescuePhy::GetOfdm9Mbps();
            ns3::RescuePhy::GetOfdm12Mbps();
            ns3::RescuePhy::GetOfdm18Mbps();
            ns3::RescuePhy::GetOfdm24Mbps();
            ns3::RescuePhy::GetOfdm36Mbps();

        }
    } g_constructor;
}

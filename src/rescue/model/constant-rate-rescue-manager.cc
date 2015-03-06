/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2004,2005 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *
 * Adapted for Rescue by: Lukasz Prasnal <prasnal@kt.agh.edu.pl>
 */

#include "constant-rate-rescue-manager.h"

#include "ns3/string.h"
#include "ns3/assert.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("ConstantRateRescueManager");

#define Min(a,b) ((a < b) ? a : b)

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ConstantRateRescueManager);

TypeId
ConstantRateRescueManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ConstantRateRescueManager")
    .SetParent<RescueRemoteStationManager> ()
    .AddConstructor<ConstantRateRescueManager> ()
    .AddAttribute ("DataMode", "The transmission mode to use for every data packet transmission",
                   StringValue ("OfdmRate6Mbps"),
                   MakeRescueModeAccessor (&ConstantRateRescueManager::m_dataMode),
                   MakeRescueModeChecker ())
    .AddAttribute ("ControlMode", "The transmission mode to use for every control packet transmission.",
                   StringValue ("OfdmRate6Mbps"),
                   MakeRescueModeAccessor (&ConstantRateRescueManager::m_ctlMode),
                   MakeRescueModeChecker ())
  ;
  return tid;
}

ConstantRateRescueManager::ConstantRateRescueManager ()
{
  NS_LOG_FUNCTION (this);
}
ConstantRateRescueManager::~ConstantRateRescueManager ()
{
  NS_LOG_FUNCTION (this);
}


RescueRemoteStation *
ConstantRateRescueManager::DoCreateStation (void) const
{
  NS_LOG_FUNCTION (this);
  RescueRemoteStation *station = new RescueRemoteStation ();
  return station;
}


void
ConstantRateRescueManager::DoReportRxOk (RescueRemoteStation *station,
                                       double rxSnr, RescueMode txMode)
{
  NS_LOG_FUNCTION (this << station << rxSnr << txMode);
}
void
ConstantRateRescueManager::DoReportDataFailed (RescueRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}
void
ConstantRateRescueManager::DoReportDataOk (RescueRemoteStation *st,
                                           double ackSnr, RescueMode ackMode, 
                                           double dataSnr, double dataPer)
{
  NS_LOG_FUNCTION (this << st << ackSnr << ackMode << dataSnr);
}
void
ConstantRateRescueManager::DoReportFinalDataFailed (RescueRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

RescueMode
ConstantRateRescueManager::DoGetDataTxMode (RescueRemoteStation *st, uint32_t size)
{
  NS_LOG_FUNCTION (this << st << size);
  return m_dataMode;
}

RescueMode
ConstantRateRescueManager::DoGetAckTxMode (RescueRemoteStation *st, RescueMode reqMode)
{
  NS_LOG_FUNCTION (this << st);
  return m_ctlMode;
}

bool
ConstantRateRescueManager::IsLowLatency (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

} // namespace ns3

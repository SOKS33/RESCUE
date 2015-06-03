/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
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
 */
#include "propagation-delay-model.h"
#include "ns3/mobility-model.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PropagationDelayModel);

TypeId 
PropagationDelayModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PropagationDelayModel")
    .SetParent<Object> ()
  ;
  return tid;
}

PropagationDelayModel::~PropagationDelayModel ()
{
}

int64_t
PropagationDelayModel::AssignStreams (int64_t stream)
{
  return DoAssignStreams (stream);
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (RandomPropagationDelayModel);

TypeId
RandomPropagationDelayModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RandomPropagationDelayModel")
    .SetParent<PropagationDelayModel> ()
    .AddConstructor<RandomPropagationDelayModel> ()
    .AddAttribute ("Variable",
                   "The random variable which generates random delays (s).",
                   StringValue ("ns3::UniformRandomVariable"),
                   MakePointerAccessor (&RandomPropagationDelayModel::m_variable),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}

RandomPropagationDelayModel::RandomPropagationDelayModel ()
{
}
RandomPropagationDelayModel::~RandomPropagationDelayModel ()
{
}
Time
RandomPropagationDelayModel::GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  return Seconds (m_variable->GetValue ());
}

int64_t
RandomPropagationDelayModel::DoAssignStreams (int64_t stream)
{
  m_variable->SetStream (stream);
  return 1;
}

NS_OBJECT_ENSURE_REGISTERED (ConstantSpeedPropagationDelayModel);

TypeId
ConstantSpeedPropagationDelayModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ConstantSpeedPropagationDelayModel")
    .SetParent<PropagationDelayModel> ()
    .AddConstructor<ConstantSpeedPropagationDelayModel> ()
    .AddAttribute ("Speed", "The speed (m/s)",
                   DoubleValue (300000000.0),
                   MakeDoubleAccessor (&ConstantSpeedPropagationDelayModel::m_speed),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

ConstantSpeedPropagationDelayModel::ConstantSpeedPropagationDelayModel ()
{
}
Time
ConstantSpeedPropagationDelayModel::GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  double distance = a->GetDistanceFrom (b);
  double seconds = distance / m_speed;
  return Seconds (seconds);
}
void
ConstantSpeedPropagationDelayModel::SetSpeed (double speed)
{
  m_speed = speed;
}
double
ConstantSpeedPropagationDelayModel::GetSpeed (void) const
{
  return m_speed;
}

int64_t
ConstantSpeedPropagationDelayModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

NS_OBJECT_ENSURE_REGISTERED (NoPropagationDelayModel);

TypeId
NoPropagationDelayModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NoPropagationDelayModel")
    .SetParent<PropagationDelayModel> ()
    .AddConstructor<NoPropagationDelayModel> ()
  ;
  return tid;
}

NoPropagationDelayModel::NoPropagationDelayModel ()
{
}
Time
NoPropagationDelayModel::GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  return Seconds (0);
}

int64_t
NoPropagationDelayModel::DoAssignStreams (int64_t stream)
{
  return 0;
}


} // namespace ns3

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

#ifndef RESCUE_MAC_CSMA_HELPER_H
#define RESCUE_MAC_CSMA_HELPER_H

#include <string>
#include "rescue-helper.h"

namespace ns3 {

class RescueMacCsmaHelper : public RescueMacHelper
{
public:
  RescueMacCsmaHelper ();

  virtual ~RescueMacCsmaHelper ();
  static RescueMacCsmaHelper Default (void);
  void SetType (std::string type,
                std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
                std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
                std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
                std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue (),
                std::string n8 = "", const AttributeValue &v8 = EmptyAttributeValue (),
                std::string n9 = "", const AttributeValue &v9 = EmptyAttributeValue (),
                std::string n10 = "", const AttributeValue &v10 = EmptyAttributeValue (),
                std::string n11 = "", const AttributeValue &v11 = EmptyAttributeValue (),
                std::string n12 = "", const AttributeValue &v12 = EmptyAttributeValue ());
  
  void Set (std::string n = "", const AttributeValue &v = EmptyAttributeValue ());
  
private:
  virtual Ptr<RescueMac> Create (void) const;

  ObjectFactory m_mac;
};

} //namespace ns3

#endif /* RESCUE_MAC_CSMA_HELPER_H */

// -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*-
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
//         Gustavo Carneiro <gjc@inescporto.pt>

#define NS_LOG_APPEND_CONTEXT                                   \
  if (m_ipv4 && m_ipv4->GetObject<Node> ()) { \
      std::clog << Simulator::Now ().GetSeconds () \
                << " [node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; }

#include <iomanip>
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-route.h"
#include "ns3/output-stream-wrapper.h"
#include "ipv4-static-routing.h"
#include "ipv4-routing-table-entry.h"

NS_LOG_COMPONENT_DEFINE ("Ipv4DceRouting");

using std::make_pair;

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Ipv4StaticRouting);

TypeId
Ipv4StaticRouting::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4DceRouting")
    .SetParent<Ipv4StaticRouting> ()
    .AddConstructor<Ipv4DceRouting> ()
  ;
  return tid;
}

Ipv4DceRouting::Ipv4StaticRouting () 
{
  NS_LOG_FUNCTION (this);
}

void 
Ipv4DceRouting::NotifyInterfaceUp (uint32_t i)
{
  NS_LOG_FUNCTION (this << i);
}

void 
Ipv4DceRouting::NotifyInterfaceDown (uint32_t i)
{
  NS_LOG_FUNCTION (this << i);

  m_netlink->
}

void 
Ipv4DceRouting::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << interface << " " << address.GetLocal ());
}
void 
Ipv4DceRouting::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << interface << " " << address.GetLocal ());
}

void 
Ipv4DceRouting::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_LOG_FUNCTION (this << ipv4);
  NS_ASSERT (m_ipv4 == 0 && ipv4 != 0);
  Ipv4StaticRouting::SetIpv4 (ipv4);

  // do some other stuff
  m_netlink = CreateObject<NetlinkSocket> ();
  m_netlink->Bind (); // not really necessary to do this
}


} // namespace ns3

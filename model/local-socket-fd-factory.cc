/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 INRIA
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
 * Author: Frederic Urbani <frederic.urbani@inria.fr>
 *
 */
#include "local-socket-fd-factory.h"
#include "local-socket-fd.h"
#include "unix-fd.h"
#include "ns3/log.h"
#include <map>

NS_LOG_COMPONENT_DEFINE("LocalSocketFdFactory");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(LocalSocketFdFactory);

TypeId
LocalSocketFdFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LocalSocketFdFactory") .SetParent<SocketFdFactory> ()
      .AddConstructor<LocalSocketFdFactory> ();

  return tid;
}

LocalSocketFdFactory::LocalSocketFdFactory () :
  m_totalBuffersSize (0)
{
}

LocalSocketFdFactory::~LocalSocketFdFactory ()
{
}

void
LocalSocketFdFactory::DoDispose (void)
{
  m_bindByPath.clear ();
  // chain up
  SocketFdFactory::DoDispose ();
}

UnixFd *
LocalSocketFdFactory::CreateSocket (int domain, int type, int protocol)
{
  NS_ASSERT (domain == AF_UNIX);

  return new LocalSocketFd (this);
}

Ptr<LocalSocketFd>
LocalSocketFdFactory::FindBinder (std::string path) const
{
  BindMap::const_iterator i = m_bindByPath.find (path);
  if (m_bindByPath.end () == i) return 0;
  return i->second;
}

void
LocalSocketFdFactory::RegisterBinder (std::string path, Ptr<LocalSocketFd> socket)
{
  m_bindByPath[path] = socket;
}

void
LocalSocketFdFactory::UnRegisterBinder (std::string path)
{
  if (0 == m_bindByPath.count (path)) return;
  m_bindByPath.erase (path);
}

} // namespace ns3

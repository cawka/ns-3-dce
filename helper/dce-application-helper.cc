#include "dce-application-helper.h"
#include "dce-application.h"
#include "ns3/log.h"
#include <stdarg.h>

NS_LOG_COMPONENT_DEFINE ("DceApplicationHelper");

namespace ns3 {

DceApplicationHelper::DceApplicationHelper ()
  : m_stackSize (0)
{}
void 
DceApplicationHelper::SetBinary (std::string filename)
{
  m_filename = filename;
}
void 
DceApplicationHelper::SetStackSize (uint32_t stackSize)
{
  m_stackSize = stackSize;
}
void 
DceApplicationHelper::SetStdinFile (std::string filename)
{
  m_stdinFilename = filename;
}
void
DceApplicationHelper::AddArgument (std::string arg)
{
  NS_LOG_FUNCTION (this << arg);
  m_args.push_back (arg);
}
void DceApplicationHelper::AddArguments (std::string a0, std::string a1)
{
  AddArgument (a0);
  AddArgument (a1);
}
void 
DceApplicationHelper::AddArguments (std::string a0, std::string a1, std::string a2)
{
  AddArgument (a0);
  AddArgument (a1);
  AddArgument (a2);
}
void 
DceApplicationHelper::AddArguments (std::string a0, std::string a1, std::string a2, std::string a3)
{
  AddArgument (a0);
  AddArgument (a1);
  AddArgument (a2);
  AddArgument (a3);
}
void 
DceApplicationHelper::AddArguments (std::string a0, std::string a1, std::string a2, std::string a3,
			     std::string a4)
{
  AddArgument (a0);
  AddArgument (a1);
  AddArgument (a2);
  AddArgument (a3);
  AddArgument (a4);
}
void 
DceApplicationHelper::AddArguments (std::string a0, std::string a1, std::string a2, std::string a3,
			     std::string a4, std::string a5)
{
  AddArgument (a0);
  AddArgument (a1);
  AddArgument (a2);
  AddArgument (a3);
  AddArgument (a4);
  AddArgument (a5);
}
void 
DceApplicationHelper::AddArguments (std::string a0, std::string a1, std::string a2, std::string a3,
		     std::string a4, std::string a5, std::string a6)
{
  AddArgument (a0);
  AddArgument (a1);
  AddArgument (a2);
  AddArgument (a3);
  AddArgument (a4);
  AddArgument (a5);
  AddArgument (a6);
}
void 
DceApplicationHelper::AddArguments (std::string a0, std::string a1, std::string a2, std::string a3,
			     std::string a4, std::string a5, std::string a6, std::string a7)
{
  AddArgument (a0);
  AddArgument (a1);
  AddArgument (a2);
  AddArgument (a3);
  AddArgument (a4);
  AddArgument (a5);
  AddArgument (a6);
  AddArgument (a7);
}
void DceApplicationHelper::ParseArguments (std::string args)
{
  std::string::size_type cur = 0, next = 0;
  cur = args.find_first_not_of (" ", 0); // skip initial spaces
  next = args.find (" ", cur); // next space
  while (next != std::string::npos)
    {
      AddArgument (args.substr (cur, next - cur));
      cur = args.find_first_not_of (" ", next); // skip spaces
      next = args.find (" ", cur); // next space
    }
  AddArgument (args.substr (cur, args.size () - cur));
}
void
DceApplicationHelper::ResetArguments (void)
{
  m_args.clear ();
}
void 
DceApplicationHelper::AddEnvironment (std::string name, std::string value)
{
  m_envs.push_back (std::make_pair (name,value));
}
void 
DceApplicationHelper::ResetEnvironment (void)
{
  m_envs.clear ();
}

ApplicationContainer
DceApplicationHelper::Install (NodeContainer c)
{
  NS_LOG_FUNCTION (this);
  ApplicationContainer apps;
  for (NodeContainer::Iterator j = c.Begin (); j != c.End (); ++j)
    {
      Ptr<DceApplication> dce = CreateObject<DceApplication> ();
      dce->SetBinary (m_filename);
      dce->SetStackSize (m_stackSize);
      dce->SetArguments (m_args);
      dce->SetEnvironment (m_envs);
      dce->SetStdinFile (m_stdinFilename);
      (*j)->AddApplication (dce);
      apps.Add (dce);
    }
  return apps;
}


} // namespace ns3

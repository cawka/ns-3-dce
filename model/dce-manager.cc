/* -*-	Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2008 INRIA
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
#include "dce-manager.h"
#include "process.h"
#include "task-manager.h"
#include "libc-dce.h"
#include "unix-fd.h"
#include "unix-file-fd.h"
#include "utils.h"
#include "kingsley-alloc.h"
#include "dce-stdio.h"
#include "dce-unistd.h"
#include "dce-pthread.h"
#include "dce-fcntl.h"
#include "sys/dce-stat.h"
#include "loader-factory.h"
#include "waiter.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/enum.h"

#include <errno.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>



NS_LOG_COMPONENT_DEFINE ("DceManager");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DceManager);

class Waiter;

TypeId 
DceManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DceManager")
    .SetParent<Object> ()
    .AddConstructor<DceManager> ()
    .AddTraceSource ("Exit", "A process has exited",
                     MakeTraceSourceAccessor (&DceManager::m_processExit))
    .AddAttribute ("FirstPid", "The PID used by default when creating a process in this manager.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&DceManager::m_nextPid),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("MinimizeOpenFiles", "For each DCE virtual process running it exists 3 files opened (stdin,stdout and stderr),"
        " if you enable this flag, the 2 files stderr and stdout will be closed between each write in order to "
        " minimize the number of opened files and then maximize the number of DCE virtual process running at the same time.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DceManager::m_minimizeFiles),
                   MakeBooleanChecker ())
    ;
  return tid;
}

DceManager::DceManager ()
{
  NS_LOG_FUNCTION (this);
  // Allocate Init Process
  m_processes [1] = 0;
}
DceManager::~DceManager ()
{
  NS_LOG_FUNCTION (this);
}
void
DceManager::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  struct Process *tmp;
  std::map<uint16_t, Process*> mapCopy = m_processes;

  m_processes.clear ();

  for(std::map<uint16_t, Process*>::iterator it = mapCopy.begin (); it != mapCopy.end (); it++)
    {
      tmp = it->second;
      if ( 0 != tmp )
      {
          std::string statusWord = "Never ended.";
    	  AppendStatusFile (tmp->pid, tmp->nodeId, statusWord );
          DeleteProcess (tmp, 0);
      }
    }

  Object::DoDispose ();
}

struct ::Libc *
DceManager::GetLibc (void)
{
  static struct ::Libc *libc = 0;
  if (libc != 0)
    {
      return libc;
    }
  libc_dce (&libc);
  return libc;
}

void
DceManager::EnsureDirectoryExists (struct Thread *current, std::string dirName)
{
  int fd = dce_open (dirName.c_str(), O_DIRECTORY | O_RDONLY, 0);
  if (fd != -1)
    {
      dce_close (fd);
    }
  else if (current->err == ENOENT)
    {
      int status = dce_mkdir (dirName.c_str (), S_IRWXU | S_IRWXG);
      if (status == -1)
	{
	  NS_FATAL_ERROR ("Could not create directory " << dirName
			  << ": " << strerror (current->err));
	}
    }
  else
    {
      NS_FATAL_ERROR ("Could not open \"" << dirName << "\"");
    }
}

int
DceManager::CreatePidFile (struct Thread *current, std::string filename)
{
  EnsureDirectoryExists (current, "/var");
  EnsureDirectoryExists (current, "/var/log");
  std::ostringstream oss;
  oss << "/var/log/" << current->process->pid;
  std::string pidDirName = oss.str();
  EnsureDirectoryExists (current, oss.str());
  oss << "/" << filename;
  std::string s = oss.str();
  int fd = dce_creat (s.c_str (), S_IWUSR | S_IRUSR);
  return fd;
}

void
DceManager::DoStartProcess (void *context)
{
  struct ::Libc *libc = GetLibc ();

  Thread *current = (Thread *)context;
  UnixFd *unixFd = 0;

  if ( current->process->stdinFilename.length() > 0 )
    {
      std::string fullpath = UtilsGetRealFilePath (current->process->stdinFilename);
      int realFd = ::open (fullpath.c_str (), O_RDONLY, 0);

      if ( -1 == realFd )
        {
          NS_FATAL_ERROR ("Unable to open stdin file : " << current->process->stdinFilename );
        }
      unixFd = new UnixFileFd (realFd);
    }
  else
    {
      unixFd = new TermUnixFileFd (0);
    }
  // create fd 0
  current->process->openFiles.push_back (std::make_pair(0,unixFd));

  // create fd 1
  int fd = CreatePidFile (current, "stdout");
  NS_ASSERT (fd == 1);
  // create fd 2
  fd = CreatePidFile (current, "stderr");
  NS_ASSERT (fd == 2);

  fd = CreatePidFile (current, "cmdline");
  NS_ASSERT (fd == 3);
  for (int i = 0; i < current->process->originalArgc; i++)
    {
      char *cur = current->process->originalArgv[i];
      dce_write (fd, cur, strlen (cur));
      dce_write (fd, " ", 1);
    }
  dce_write (fd, "\n", 1);
  dce_close (fd);

  fd = CreatePidFile (current, "status");
  NS_ASSERT (fd == 3);
  std::ostringstream oss;
  oss << "Start Time: " << GetTimeStamp () << std::endl;
  const char *str = oss.str ().c_str();
  dce_write (fd, str, strlen (str));
  dce_close (fd);

  void *h = current->process->loader->Load ("libc-ns3.so", RTLD_GLOBAL);
  if (h == 0)
    {
      NS_FATAL_ERROR ("Unable to load libc-ns3.so");
    }
  void *symbol = current->process->loader->Lookup (h, "libc_setup");
  if (symbol == 0)
    {
      NS_FATAL_ERROR ("This is not our fake libc !");
    }
  // construct the libc now
  void (*libc_setup) (const struct Libc *fn);
  libc_setup = (void (*) (const struct Libc *))(symbol);
  libc_setup (libc);

  h = current->process->loader->Load ("libpthread-ns3.so", RTLD_GLOBAL);
  if (h == 0)
    {
      NS_FATAL_ERROR ("Unable to load libpthread-ns3.so");
    }
  symbol = current->process->loader->Lookup (h, "libpthread_setup");
  if (symbol == 0)
    {
      NS_FATAL_ERROR ("This is not our fake libpthread !");
    }
  // construct libpthread now
  void (*libpthread_setup) (const struct Libc *fn);
  libpthread_setup = (void (*) (const struct Libc *))(symbol);
  libpthread_setup (libc);

  // finally, call into 'main'.
  h = current->process->loader->Load (current->process->originalArgv[0], RTLD_GLOBAL);
  current->process->mainHandle = h;
  symbol = current->process->loader->Lookup (h, "main");
  int (*main) (int, char **);
  main = (int (*) (int, char **)) symbol;

  StartProcessDebugHook ();

  int retval = main (current->process->originalArgc, current->process->originalArgv);
  dce_exit (retval);
}

void
DceManager::StartProcessDebugHook (void)
{}

struct Process *
DceManager::CreateProcess (std::string name, std::string stdinfilename, std::vector<std::string> args,
                           std::vector<std::pair<std::string,std::string> > envs, int pid)
{
  struct Process *process = new Process ();
  process->euid = 0;
  process->ruid = 0;
  process->suid = 0;
  process->egid = 0;
  process->rgid = 0;
  process->sgid = 0;
  process->alloc = new KingsleyAlloc ();
  process->originalArgv = 0;
  process->originalArgc = 0;
  process->originalEnvp = 0;
  SetArgv (process, name, args);
  SetEnvp (process, envs);
  Ptr<LoaderFactory> loaderFactory = this->GetObject<LoaderFactory> ();
  process->loader = loaderFactory->Create (process->originalArgc, process->originalArgv,
                                           process->originalEnvp);
  process->exitValue = 0;
  process->name = name;
  process->ppid = 0;
  process->pid = pid?pid:AllocatePid ();
  process->manager = this;
  sigemptyset (&process->pendingSignals);
  // setup a signal handler for SIGKILL which calls dce_exit.
  struct SignalHandler handler;
  handler.signal = SIGKILL;
  handler.flags = 0;
  sigemptyset (&handler.mask);
  handler.handler = &DceManager::SigkillHandler;
  process->signalHandlers.push_back (handler);

  // setup a signal handler for SIGABRT which calls dce_exit.
  handler.signal = SIGABRT;
  handler.flags = 0;
  sigemptyset (&handler.mask);
  handler.handler = &DceManager::SigabrtHandler;
  process->signalHandlers.push_back (handler);


  // we reserve 0 for the mutex initialized with PTHREAD_MUTEX_INITIALIZER 
  // we reserve 2 for a destroyed mutex
  // 1 is such a common value that we try to avoid it.
  // we unfortunately cannot avoid the value 0 because we 
  // have to be compatible with the system PHREAD_MUTEX_INITIALIZER
  // which uses the value 0.
  process->nextMid = 3;
  // we reserve 2 for a destroyed mutex
  // 0 and 1 are such common values that we avoid them.
  process->nextSid = 3;
  // we reserve 2 for a destroyed condition variable
  // 0 and 1 are such common values that we avoid them.
  process->nextCid = 3;
  process->cwd = "/";
  process->pstdin = 0;
  process->pstdout = 0;
  process->pstderr = 0;
  process->penvp = 0;

  process->uMask = 022;

  process->stdinFilename = stdinfilename;

  //"seeding" random variable
  process->rndVarible = UniformVariable (0, RAND_MAX);

  process->nodeId = UtilsGetNodeId ();

  process->minimizeFiles = (m_minimizeFiles?1:0);

  if (!pid) m_processes[process->pid] = process;

  return process;
}

// Allocate new process with the same pid
struct Process *
DceManager::CopyExecProcess (struct Process *proc, std::string name, std::vector<std::string> args,
                         std::vector<std::pair<std::string,std::string> > envs)
{
  struct Process *process = CreateProcess (name, "",  args, envs, proc->pid);

  process->euid = proc->euid;
  process->ruid = proc->ruid;
  process->suid = proc->suid;
  process->egid = proc->egid;
  process->rgid = proc->rgid;
  process->sgid = proc->sgid;
  process->alloc = new KingsleyAlloc ();

  process->ppid = proc->pid; // Same as pid, to denote the current execking new stack and memory context...
  process->pid = proc->pid;
  process->cwd = proc->cwd;
  process->rndVarible = proc->rndVarible;
  process->minimizeFiles = proc->minimizeFiles;
  process->children = proc->children;

  return process;
}


void 
DceManager::TaskSwitch (enum Task::SwitchType type, void *context)
{
  Process *process = (Process *)context;
  switch (type)
    {
    case Task::TO:
      process->loader->NotifyStartExecute ();
      process->alloc->SwitchTo ();
      break;
    case Task::FROM:
      process->loader->NotifyEndExecute ();
      break;
    }
}


uint16_t
DceManager::Start (std::string name, std::string stdinfilename, std::vector<std::string> args,
                       std::vector<std::pair<std::string,std::string> > envs)
{
  NS_LOG_FUNCTION (this << name << args.size ());
  struct Process *process = CreateProcess (name, stdinfilename, args, envs, 0);
  struct Thread *thread = CreateThread (process);
  Task *task = TaskManager::Current ()->Start (&DceManager::DoStartProcess, thread);
  task->SetContext (thread);
  task->SetSwitchNotifier (&DceManager::TaskSwitch, process);
  thread->task = task;
  return process->pid;
}
uint16_t
DceManager::Start (std::string name, std::string stdinfilename, uint32_t stackSize,
                   std::vector<std::string> args, std::vector<std::pair<std::string,std::string> > envs)
{
  NS_LOG_FUNCTION (this << name << stackSize << args.size () << envs.size ());
  struct Process *process = CreateProcess (name, stdinfilename, args, envs, 0);
  struct Thread *thread = CreateThread (process);
  Task *task = TaskManager::Current ()->Start (&DceManager::DoStartProcess, thread, stackSize);
  task->SetContext (thread);
  task->SetSwitchNotifier (&DceManager::TaskSwitch, process);
  thread->task = task;
  return process->pid;
}


uint16_t
DceManager::AllocatePid (void)
{
  // Note that this function purposedly never allocates pid 0 
  // to make it useable in the kernel stack layer for kernel-level
  // special tasks (which thus all share the same pid)
  NS_LOG_FUNCTION (this);
  for (uint16_t i = 0; i < 0xffff; i++)
    {
      uint16_t candidatePid = (m_nextPid + i) & 0xffff;
      if (candidatePid != 0 && SearchProcess (candidatePid) == 0)
        {
          m_nextPid = (candidatePid + 1) & 0xffff;
          return candidatePid;
        }
    }
  NS_FATAL_ERROR ("Too many processes");
  return 0; // quiet compiler
}
uint16_t
DceManager::AllocateTid (const struct Process *process) const
{
  for (uint16_t tid = 0; tid < 0xffff; tid++)
    {
      bool found = false;
      for (std::vector<struct Thread *>::const_iterator i = process->threads.begin (); i != process->threads.end (); ++i)
        {
          struct Thread *tmp = *i;
          if (tmp->tid == tid)
            {
              found = true;
              break;
            }
        }
      if (!found)
        {
          return tid;
        }
    }
  NS_FATAL_ERROR ("We attempted to allocate a new tid for this process but none were available: "
                  "too many threads created at the same time.");
  // quiet compiler
  return 0;
}
struct Thread *
DceManager::CreateThread (struct Process *process)
{
  NS_LOG_FUNCTION (this << process);
  struct Thread *thread = new Thread ();
  NS_LOG_DEBUG ("Create " << thread);
  thread->err = 0;
  thread->tid = AllocateTid (process);
  thread->process = process;
  thread->isDetached = false;
  thread->hasExitValue = false;
  thread->joinWaiter = 0;
  thread->lastTime = Time(0);
  thread->childWaiter = 0;

  sigemptyset (&thread->signalMask);
  if (!process->threads.empty ())
    {
      // copy all key values.
      Thread *tmp = process->threads.front ();
      thread->keyValues = tmp->keyValues;
      // reset all values to NULL
      for (std::list<struct ThreadKeyValue>::iterator i = thread->keyValues.begin ();
           i != thread->keyValues.end (); ++i)
        {
          i->value = 0;
        }
    }
  process->threads.push_back (thread);
  return thread;
}

void 
DceManager::Exit (void)
{
  TaskManager::Current ()->Exit ();
}

bool
DceManager::ThreadExists (Thread *thread)
{
  if ( !thread ) return false;
  if ( !thread->process ) return false;

  Process *process = thread->process;

  for (std::vector<Thread *>::const_iterator j = process->threads.begin ();
      j != process->threads.end (); ++j)
    {
      Thread *cur = *j;
      if (cur == thread)
        {
          return true;
        }
    }

  return false;
}

uint16_t 
DceManager::Clone (Thread *thread)
{
  NS_LOG_FUNCTION (this << thread);
  Process *clone = new Process ();
  clone->euid = 0;
  clone->ruid = 0;
  clone->suid = 0;
  clone->egid = 0;
  clone->rgid = 0;
  clone->sgid = 0;
  // XXX setup
  clone->originalArgv = 0;
  clone->originalArgc = 0;
  clone->originalEnvp = 0;
  clone->exitValue = 0;
  clone->name = thread->process->name;
  clone->ppid = thread->process->pid;
  clone->pid = AllocatePid ();
  thread->process->children.insert (clone->pid);
  // dup each file descriptor.
  for (uint32_t index = 0; index < thread->process->openFiles.size (); index++)
    {
      std::pair<int,UnixFd*> i = thread->process->openFiles[index];
      i.second->Ref ();
      i.second->FdUsageInc ();
      clone->openFiles.push_back (i);
    }
  // don't copy threads, semaphores, mutexes, condition vars
  // XXX: what about file streams ?
  clone->manager = this;
  sigemptyset (&clone->pendingSignals);
  // setup a signal handler for SIGKILL which calls dce_exit.
  struct SignalHandler handler;
  handler.signal = SIGKILL;
  handler.flags = 0;
  sigemptyset (&handler.mask);
  handler.handler = &DceManager::SigkillHandler;
  clone->signalHandlers.push_back (handler);

  // setup a signal handler for SIGABRT which calls dce_exit.
  handler.signal = SIGABRT;
  handler.flags = 0;
  sigemptyset (&handler.mask);
  handler.handler = &DceManager::SigabrtHandler;
  clone->signalHandlers.push_back (handler);

  clone->nextMid = thread->process->nextMid;
  clone->nextSid = thread->process->nextSid;
  clone->nextCid = thread->process->nextCid;
  clone->cwd = thread->process->cwd;
  clone->pstdin = thread->process->pstdin;
  clone->pstdout = thread->process->pstdout;
  clone->pstderr = thread->process->pstderr;
  clone->penvp = thread->process->penvp;

  //"seeding" random variable
  clone->rndVarible = UniformVariable (0, RAND_MAX);
  m_processes[clone->pid] = clone;
  Thread *cloneThread = CreateThread (clone);

  clone->loader = thread->process->loader->Clone ();
  clone->alloc = thread->process->alloc->Clone ();
  Task *task = TaskManager::Current ()->Clone (thread->task);
  if (task != 0)
    {
      // parent. complete setup. return.
      task->SetContext (cloneThread);
      task->SetSwitchNotifier (&DceManager::TaskSwitch, clone);
      cloneThread->task = task;
      return clone->pid;
    }
  // child.
  return 0;
}

void 
DceManager::Yield (void)
{
  TaskManager::Current ()->Yield ();
}

void 
DceManager::Wait (void)
{
  TaskManager::Current ()->Sleep ();
}

Time
DceManager::Wait (Time timeout)
{
  return TaskManager::Current ()->Sleep (timeout);
}

void 
DceManager::Wakeup (Thread *thread)
{
  NS_ASSERT (ThreadExists (thread));
  return TaskManager::Current ()->Wakeup (thread->task);
}

void 
DceManager::SetFinishedCallback (uint16_t pid, Callback<void,uint16_t,int> cb)
{
  Process *process = SearchProcess (pid);
  if (process == 0)
    {
      return;
    }
  process->finished = cb;
}
void 
DceManager::Stop (uint16_t pid)
{
  NS_LOG_FUNCTION (this << pid);
  while (true)
    {
      Process *process = SearchProcess (pid);
      if (process == 0)
        {
          return;
        }
      std::string statusWord = "Stopped by NS3.";
      AppendStatusFile (process->pid, process->nodeId, statusWord);
      DeleteProcess (process, 0);
    }
}

void 
DceManager::SigkillHandler (int signal)
{
  NS_ASSERT (signal == SIGKILL);
  dce_exit (-1);
}
void
DceManager::SigabrtHandler (int signal)
{
  NS_ASSERT (signal == SIGABRT);
  dce_exit (-2);
}
void
DceManager::DeleteThread (struct Thread *thread)
{
  NS_LOG_FUNCTION (this << thread);
  if (thread->task != 0)
    {
      // the task could be 0 if it was Exited by
      // pthread_exit and it was not pthread_detached.
      GetObject<TaskManager> ()->Stop (thread->task);
    }
  thread->task = 0;
  for (std::vector<Thread *>::iterator i = thread->process->threads.begin ();
       i != thread->process->threads.end (); ++i)
    {
      if (*i == thread)
        {
          thread->process->threads.erase (i);
          break;
        }
    }
  if ( 0 != thread->childWaiter)
    {
      Waiter *lb = thread->childWaiter;
      thread->childWaiter = 0;
      delete lb;
    }
  delete thread;
}

void
DceManager::DeleteProcess (struct Process *process, int type)
{
  NS_LOG_FUNCTION (this << process << "pid " << process->pid << "ppid" << process->ppid);

  // Close all streams opened
  for (uint32_t i =  0; i < process->openStreams.size (); i++)
    {
      // Note that, while this code might look straightforward,
      // it is not so. Specifically, we call here the system fclose
      // function which will call indirectly system_close in 
      // system-wrappers.cc which will call dce_close in dce-fd.cc
      // which will return -1 because it is called without a 'current'
      // thread
      dce_fclose_unconditional (process->openStreams[i]);
    }
  process->openStreams.clear ();

  if (!process->finished.IsNull ())
    {
      process->finished (process->pid, process->exitValue);
    }
  // stop itimer timers if there are any.
  process->itimer.Cancel ();
  // close all its fds.
  for (uint32_t index = 0; index < process->openFiles.size (); index++)
    {
      UnixFd *freeOne = process->openFiles[index].second;

      process->openFiles[index].first = -1;
      process->openFiles[index].second = 0;

      if ( 0 != freeOne )
        {
          freeOne->FdUsageDec();
          if ( freeOne->GetFdUsageCount() == 0 )
            {
              freeOne->Dispose();
            }
          freeOne->Unref ();
        }
    }
  process->openFiles.clear ();
  // finally, delete remaining threads
  struct Thread *tmp;
  while (!process->threads.empty ())
    {
      tmp = process->threads.back ();
      process->threads.pop_back ();
      DeleteThread (tmp);
    }
  // delete all mutexes
  for (uint32_t i = 0; i < process->mutexes.size (); ++i)
    {
      struct Mutex *mutex = process->mutexes[i];
      // XXX: do some error checking here to ensure that no thread is
      // blocked in a critical section.
      delete mutex;
    }
  process->mutexes.clear ();
  // delete all semaphores
  for (uint32_t i = 0; i < process->semaphores.size (); ++i)
    {
      struct Semaphore *semaphore = process->semaphores[i];
      // XXX: do some error checking here to ensure that no thread is
      // blocked in a critical section.
      delete semaphore;
    }
  // delete all extra buffers
  while (!process->allocated.empty ())
    {
      void *buffer = process->allocated.back ();
      process->allocated.pop_back ();
      free (buffer);
    }

  int ppid = process->ppid;

  if (!type)
    {
      // Re-parent children
      std::set<uint16_t> children = process->children;
      process->children.clear ();
      std::set<uint16_t>::iterator it;

      for (it = children.begin () ; it != children.end (); it++ )
        {
          Process *child = m_processes [*it];

          if (child)
            {
              child->ppid = 1;
              if ( ( child->pid > 1 ) && !child->loader && !child->alloc )
                {
                  m_processes.erase ( child->pid );
                  delete child;
                }
            }
        }
      m_processExit (process->pid, process->exitValue);
    }
  delete process->loader;
  process->loader = 0;
  delete process->alloc;
  process->alloc = 0;

  if (!type)
    {
      if ( ppid > 1 )
        {
          // Warn father
          ChildFinished (process->pid);
        }
      else
        { // ppid == 0 have no father, perhaps DCE, else ppid = 1 init : have lost it's real father.
          // remove ourselves from list of processes
          m_processes.erase ( process->pid );
          // delete process data structure.
          delete process;
        }
    }
  else
    {
      delete process;
    }
}

bool
DceManager::CheckProcessContext (void) const
{
  return TaskManager::Current () != 0 && TaskManager::Current ()->CurrentTask () != 0;
}

Thread *
DceManager::SearchThread (uint16_t pid, uint16_t tid)
{
  NS_LOG_FUNCTION (this << pid << tid);
  NS_ASSERT (CheckProcessContext ());
  Process *process = SearchProcess (pid);
  if (process == 0)
    {
      return 0;
    }
  for (std::vector<Thread *>::const_iterator j = process->threads.begin (); 
       j != process->threads.end (); ++j)
    {
      Thread *thread = *j;
      if (thread->tid == tid)
        {
          return thread;
        }
    }
  return 0;
}
Process *
DceManager::SearchProcess (uint16_t pid)
{
  NS_LOG_FUNCTION (this << pid);

  return m_processes [pid];
}

void 
DceManager::SetArgv (struct Process *process, std::string filename, std::vector<std::string> args)
{
  NS_ASSERT (process->originalArgv == 0);
  int argc = args.size () + 1;
  char **argv = (char **)malloc (sizeof (char *) * (argc + 1));
  process->allocated.push_back (argv);
  argv[0] = strdup (filename.c_str ());
  process->allocated.push_back (argv[0]);
  for (uint32_t i = 0; i < args.size (); ++i)
    {
      char *arg = strdup (args[i].c_str ());
      NS_LOG_DEBUG ("argc=" << argc << " i=" << i << " v=" << arg);
      argv[i+1] = arg;
      process->allocated.push_back (arg);
    }
  argv[argc] = 0;
  process->originalArgv = argv;
  process->originalArgc = argc;
  process->originalProgname = argv[0];
}

void 
DceManager::SetEnvp (struct Process *process, 
                         std::vector<std::pair<std::string,std::string> > envs)
{
  int envpc = envs.size ();
  char **envp = (char **)malloc (sizeof (char *) * (envpc + 1));
  process->allocated.push_back (envp);
  for (uint32_t i = 0; i < envs.size (); ++i)
    {
      int size = envs[i].first.size () + envs[i].second.size () + 1;
      envp[i] = (char*)malloc (size+1);
      process->allocated.push_back (envp[i]);
      memcpy (envp[i], envs[i].first.c_str (), envs[i].first.size ());
      envp[i][envs[i].first.size ()] = '=';
      memcpy (envp[i] + envs[i].first.size () + 1, envs[i].second.c_str (), envs[i].second.size ());
      envp[i][size] = 0;
      NS_LOG_DEBUG ("envpc=" << envpc << " i=" << i << " v=" << envp[i]);
    }
  envp[envpc] = 0;
  process->originalEnvp = envp;
}

void
DceManager::AppendStatusFile (uint16_t pid, uint32_t nodeId,  std::string &line)
{
  std::ostringstream oss;
  oss << "files-" << nodeId << "/var/log/" << pid << "/status" ;
  std::string s = oss.str();

  int fd = ::open (s.c_str (), O_WRONLY | O_APPEND, 0 );

  if (fd >= 0) // XXX: When fork is used the pid directory is not created, I plan to fix it when I will work on fork/exec/wait...
    {
      oss.str ("");
      oss.clear ();
      oss << "      Time: " << GetTimeStamp () << " --> " << line << std::endl;
      std::string wholeLine = oss.str ();
      int l =  wholeLine.length();
      const char *str = wholeLine.c_str();
      ::write (fd, str, l);
      ::close (fd);
    }
}

std::map<uint16_t, Process *>
DceManager::GetProcs ()
{
  return m_processes;
}

int
DceManager::Execve (Thread *threadOld, const char *path, char *const argv[], char *const envp[])
{
   NS_LOG_FUNCTION (this << path);
   struct ExecContext *con = new (struct ExecContext);

   con->caller = threadOld;
   con->execResult = -1;

   struct Process *newOne = StartExecProcess (con, path, argv, envp);

   if (! newOne ) return -1;

   while ( -1 == con->execResult ) // Wait exec result
     {
       Yield ();
     }
   int res = con->execResult;
   con->execResult = -2;

   if ( 0 == res )
     { // SUCCESS
       Current ()->process->ppid = 1;
       Current ()->process->children.clear ();
       m_processes.erase ( Current ()->process-> pid );
       m_processes [newOne->pid] = newOne;
       dce_fflush (0);
       dce_exit_exec (0, 1);
     }
   else
     { // ERROR
       Current ()->err = res;
       Yield ();
     }

   return -1;
}

struct Process *
DceManager::StartExecProcess (struct ExecContext *context, const char *path, char *const argv[], char *const envp[])
{
  NS_LOG_FUNCTION ( this << path );
  std::vector<std::string> args;
  std::vector<std::pair<std::string,std::string> > envs;
  // Copy argv
  if ( 0 != argv )
    {
      char **v = (char **) argv;
      if (*v) v++; // Ignore argv[0]
      while (*v)
        {
          char *s = *v++;
          args.push_back (s);
        }
    }
  // Copy Env
  if ( 0 != envp )
    {
      char **e = (char **) envp;

      while (*e)
        {
          char *s = *e++;
          char *c = s;

          while ( ( *c != 0 ) && ( *c != '=' ) ) c++;

          if (( '=' != *c ) || (s == c) )  {
              Current () -> err = EINVAL;
              return 0;
          }
          std::string key = std::string (s, c);
          std::string val = std::string (1 + c);
          envs.push_back(std::make_pair (key, val));
        }
    }
   struct Process *process = CopyExecProcess ( context->caller->process, path, args, envs);
   struct Thread *thread = CreateThread (process);
   uint32_t stackSize = TaskManager::Current ()->GetStackSize (context->caller->task);
   Task *task = TaskManager::Current ()->Start (&DceManager::DoExec, context, stackSize );
   task->SetContext (thread);
   task->SetSwitchNotifier (&DceManager::TaskSwitch, process);
   thread->task = task;
   return process;
}

void
DceManager::DoExec (void *context)
{
  struct ::Libc *libc = GetLibc ();
  struct ExecContext *con = (struct ExecContext *) context;
  Thread *current = Current ();
  UnixFd *unixFd = 0;
  struct Process *oldProc = con->caller->process;
  int ppid = oldProc->ppid;
  int cpt = 0;

  for (std::vector<std::pair<int,UnixFd *> >::iterator i = oldProc->openFiles.begin ();
       i != oldProc->openFiles.end (); ++i)
    {
      if ( ( (i->first) >= 0 ) && (  (i->first) < 3 ) )
      {
          cpt++;
          i->second->FdUsageInc ();
          i->second->Ref ();
          current->process->openFiles.push_back (std::make_pair( i->first, i->second ) );
          if ( cpt >= 3 ) { break; }
      }
    }

  std::ostringstream oss;
  oss << "EXEC: ";
  for (int i = 0; i < current->process->originalArgc; i++)
    {
      oss << current->process->originalArgv[i] << ' ';
    }
  std::string line = oss.str ();
  AppendStatusFile (oldProc->pid, oldProc->nodeId, line);

  void *h = current->process->loader->Load ("libc-ns3.so", RTLD_GLOBAL);
  void *symbol = 0;
  if (h == 0)
    {
      con->execResult = ENOMEM;
    }
  else
    {
      symbol = current->process->loader->Lookup (h, "libc_setup");
      if (symbol == 0)
        {
          NS_FATAL_ERROR ("This is not our fake libc !");
        }
      // construct the libc now
      void (*libc_setup) (const struct Libc *fn);
      libc_setup = (void (*) (const struct Libc *))(symbol);
      libc_setup (libc);

      h = current->process->loader->Load ("libpthread-ns3.so", RTLD_GLOBAL);
      if (h == 0)
        {
          con->execResult = ENOMEM;
        }
      else
        {
          symbol = current->process->loader->Lookup (h, "libpthread_setup");
          if (symbol == 0)
            {
              NS_FATAL_ERROR ("This is not our fake libpthread !");
            }
          // construct libpthread now
          void (*libpthread_setup) (const struct Libc *fn);
          libpthread_setup = (void (*) (const struct Libc *))(symbol);
          libpthread_setup (libc);

          // finally, call into 'main'.
          h = current->process->loader->Load (current->process->originalArgv[0], RTLD_GLOBAL);

          if ( h == 0 )
            {
              con->execResult = EACCES;
            }
          else
            {
              current->process->mainHandle = h;
              symbol = current->process->loader->Lookup (h, "main");
              con->execResult = ( 0 != symbol ) ? 0 : ENOEXEC;
            }
        }
    }

  while ( -2 != con->execResult )
    {
      current->process->manager->Yield (); // warn caller
    }
  delete con;
  current->process->ppid = ppid;

  if ( 0 != symbol )
    {
      int (*main) (int, char **);
      main = (int (*) (int, char **)) symbol;

      StartProcessDebugHook ();

      int retval = main (current->process->originalArgc, current->process->originalArgv);
      dce_exit (retval);
    }
  else
    {
      // Destroy me ....
      dce_exit_exec (0, 2);
    }
}
void
DceManager::ChildFinished (uint16_t pid)
{
  // I need to wakeup wait for wait queue threads ....
  // XXX: TODO

  // Get pid process.

  Process *child = m_processes [pid];

  NS_ASSERT (child);

  Process *p =  m_processes [child->ppid];

  NS_ASSERT (p);

  WakeupChildWaiters ( p );

  if ( ! true ) // IF WAIT DONE !
    {
      std::set<uint16_t>::iterator it = p->children.find (pid);

      if ( it != p->children.end () )
        {
          p->children.erase(it);
        }
      m_processes.erase(pid);
      delete child;
    }
}
bool
DceManager::WakeupChildWaiters (struct Process *p)
{
  bool ret = false;
  std::vector<Thread *> tt = p->threads;

  for (std::vector<Thread*>::iterator it = tt.begin() ; it != tt.end(); it++ )
    {
  //    ret !=
      Thread *t = *it;

      if ( t && t->childWaiter ) t->childWaiter->Wakeup();
    }

  return ret;
}
void
DceManager::FinishChild (uint16_t pid)
{
  // A wait success on this proc
  Process *child = m_processes [pid];

  if (child)
    {
      m_processes.erase(pid);
      delete child;
    }
}
} // namespace ns3

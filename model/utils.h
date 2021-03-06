#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <list>
#include <sys/time.h>
#include <sys/stat.h>
#include "ns3/nstime.h"

#define GET_CURRENT(x)					\
  Thread *current;					\
  current = Current ();					\
  do {							\
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << x);	\
  NS_ASSERT (current != 0);				\
  } while (false)
#define GET_CURRENT_NOLOG()				\
  Thread *current;					\
  current = Current ();					\
  do {							\
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());	\
  NS_ASSERT (current != 0);				\
  } while (false)


namespace ns3 {

class Thread;
class Process;

// Little hack in order to have a context usable when disposing the Task Manager and the hidden goal is to flush the open FILEs.
extern Thread *gDisposingThreadContext;

void UtilsEnsureDirectoryExists (std::string realPath);
void UtilsEnsureAllDirectoriesExist (std::string realPath);
std::string UtilsGetRealFilePath (std::string path);
std::string UtilsGetAbsRealFilePath (uint32_t node, std::string path);
std::string UtilsGetVirtualFilePath (std::string path);
uint32_t UtilsGetNodeId (void);
int UtilsSearchOpenFd (int fd);
Thread *Current (void);
bool HasPendingSignal (void);
Time UtilsTimeToSimulationTime (Time time);
Time UtilsSimulationTimeToTime (Time time);
struct timeval UtilsTimeToTimeval (Time time);
struct timespec UtilsTimeToTimespec (Time time);
Time UtilsTimespecToTime (struct timespec tm);
Time UtilsTimevalToTime (struct timeval tv);
Time UtilsTimevalToTime (const struct timeval *tv);
void UtilsSendSignal (Process *process, int signum);
void UtilsDoSignal (void);
int UtilsAllocateFd (void);
// Little hack to advance time when detecting a possible infinite loop.
void UtilsAdvanceTime (Thread *current);
std::string GetTimeStamp ();
bool CheckExeMode (struct stat *st, uid_t uid, gid_t gid);
std::string FindExecFile (std::string root, std::string envPath, std::string fileName, uid_t uid, gid_t gid, int *errNo);
std::list<std::string> Split (std::string input, std::string sep);

#define MAX_FDS 1024
} // namespace ns3

#endif /* UTILS_H */

#define _GNU_SOURCE 1
#include "libc-dce.h"
#include "libc.h"
using namespace libc;

#include "arpa/dce-inet.h"
#include "sys/dce-socket.h"
#include "sys/dce-time.h"
#include "sys/dce-ioctl.h"
#include "sys/dce-mman.h"
#include "sys/dce-stat.h"
#include "sys/dce-select.h"
#include "sys/dce-timerfd.h"
#include "dce-unistd.h"
#include "dce-netdb.h"
#include "dce-pthread.h"
#include "dce-stdio.h"
#include "dce-stdarg.h"
#include "dce-errno.h"
#include "dce-fcntl.h"
#include "dce-sched.h"
#include "dce-poll.h"
#include "dce-signal.h"
#include "dce-stdlib.h"
#include "dce-time.h"
#include "dce-semaphore.h"
#include "dce-cxa.h"
#include "dce-string.h"
#include "dce-global-variables.h"
#include "dce-random.h"
#include "dce-umask.h"
#include "dce-misc.h"
#include "dce-wait.h"
#include "net/dce-if.h"

extern "C" {

void libc_dce (struct Libc **libc)
{
  *libc = new Libc;

//#define DCE(name)				\
//  (*libc)->name ## _fn = dce_ ## name;
#define DCE NATIVE
#define NATIVE(name) \
  (*libc)->name ## _fn = name;

  #include "libc-ns3.h"
}

} // extern "C"


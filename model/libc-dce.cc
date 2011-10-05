#define _GNU_SOURCE 1
#undef __OPTIMIZE__
#define _LARGEFILE64_SOURCE 1

#include "libc-dce.h"
#include "libc.h"

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

#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <ifaddrs.h>
#include <sys/uio.h>
#include <locale.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>
#include <xlocale.h>
#include <errno.h>

extern void __cxa_finalize (void *d);
extern int __cxa_atexit (void (*func) (void *), void *arg, void *d);
// extern int __gxx_personality_v0 (int a, int b,
// 								 unsigned c, struct _Unwind_Exception *d, struct _Unwind_Context *e);

typedef void (*func_t) (...);

extern "C" {

void libc_dce (struct Libc **libc)
{
  *libc = new Libc;

#define DCE(name)							\
  (*libc)->name ## _fn = (func_t)(__typeof(&name))dce_ ## name;

#define NATIVE(name)							\
  (*libc)->name ## _fn = (func_t)name;

#include "libc-ns3.h"
}

} // extern "C"


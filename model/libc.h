
#ifndef LIBC_H
#define LIBC_H

#define _GNU_SOURCE 1
#undef __OPTIMIZE__
#define _LARGEFILE64_SOURCE 1

struct SimuGlobalVariables;

namespace libc
{
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

  extern void __cxa_finalize (void *d);
  extern int __cxa_atexit (void (*func) (void *), void *arg, void *d);
  extern int __gxx_personality_v0 (int a, int b,
							unsigned c, struct _Unwind_Exception *d, struct _Unwind_Context *e);

  extern int *__errno_location (void);
  //extern const unsigned short int **__ctype_b_loc (void);
  extern int32_t * * ctype_tolower_loc (void);

#include "dce-global-variables.h"
}

extern "C" {

struct Libc
{
#define NATIVE DCE
#define DCE(name) \
	__typeof (libc:: name) * name ## _fn;

#include "libc-ns3.h"
};

}

#endif /* LIBC_H */

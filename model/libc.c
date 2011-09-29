#define _GNU_SOURCE 1
#define _GNU_SOURCE
#undef __OPTIMIZE__
#define _LARGEFILE64_SOURCE 1

namespace libc
{
#include <arpa/inet.h>
#include <cxxabi.h>
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
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>
#include <xlocale.h>

  void __cxa_finalize (void *d);
  int __cxa_atexit (void (*func) (void *), void *arg, void *d);
  int __gxx_personality_v0 (int a, int b,
							unsigned c, struct _Unwind_Exception *d, struct _Unwind_Context *e);

  int *__errno_location (void);
  const unsigned short int **__ctype_b_loc (void);
  
  int __fxstat64 (int ver, int fd, struct stat64 *stat_buf);
  int32_t * * ctype_tolower_loc (void);

  // #include <arpa/inet.h>
#undef htonl
#undef htons
#undef ntohl
#undef ntohs
  uint32_t htonl(uint32_t hostlong);
  uint16_t htons(uint16_t hostshort);
  uint32_t ntohl(uint32_t netlong);
  uint16_t ntohs(uint16_t netshort);

  // #include <semaphore.h>
  int sem_wait(sem_t *sem);
  int sem_trywait(sem_t *sem);
  int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
}

#include "libc.h"

// macros stolen from glibc.
#define weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)))

static struct Libc g_libc;


// Step 1.  Define exported symbols (pointers to functions) and assign 0 pointers to them
#define NATIVE DCE
#define DCE(name) \
  __typeof (libc::name) * name = 0;

// Note: it looks like that the stdio.h header does
// not define putc and getc as macros if you include
// them from C++ so that we do need to define the putc
// and getc functions anyway.
#undef putc
#undef getc

#include "libc-ns3.h" // do the work

// void exit(int status)
// {
//   g_libc.exit_fn (status);
//   int a = 0;
//   while (1)
//     {
//       // loop forever to quiet compiler warning:
//       // warning: ‘noreturn’ function does return
//       a++;
//     }
// }

// void abort(void)
// {
//   g_libc.abort_fn ();
//   int a = 0;
//   while (1)
//     {
//       // loop forever to quiet compiler warning:
//       // warning: ‘noreturn’ function does return
//       a++;
//     }
// }


void LIBSETUP (const struct Libc *fn)
{
  /* The following assignment of fn to g_libc is a bit weird: we perform a copy of the data 
   * structures by hand rather than executing g_libc = fn. This is obviously done on purpose. 
   * The reason is that g_libc = fn would trigger a call to the memcpy function because the 
   * Libc structure is very big. The memcpy function is resolved through the dynamic loader's 
   * symbol lookup mechanism to the local memcpy function and that local memcpy function happens 
   * to be calling g_libc.memcpy_fn which is set to NULL before the data structure is initialized. 
   */
  // const uint8_t *src = (const uint8_t *)fn;
  // uint8_t *dst = (uint8_t *)&g_libc;
  // uint32_t i;
  // for (i = 0; i < sizeof (struct Libc); ++i)
  //   {
  //     *dst = *src;
  //     src++;
  //     dst++;
  //   }
  // struct SimuGlobalVariables globals;
  // globals.pstdin = &stdin;
  // globals.pstdout = &stdout;
  // globals.pstderr = &stderr;
  // globals.penvp = &environ;
  // g_libc.global_variables_setup (&globals);
}




#include "libc.h"

// macros stolen from glibc.
#define weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)))

extern "C" {

// Step 1.  Define exported symbols (pointers to functions) and assign 0 pointers to them
#define NATIVE DCE
#define DCE(name) \
  __typeof (libc::name) * name;

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
#define NATIVE DCE
#define DCE(name) \
  ::name = fn->name ## _fn;

#include "libc-ns3.h"

  struct libc::SimuGlobalVariables globals;
  globals.pstdin = &libc::stdin;
  globals.pstdout = &libc::stdout;
  globals.pstderr = &libc::stderr;
  globals.penvp = &libc::environ;
  dce_global_variables_setup (&globals);

#define NATIVE DCE
#define DCE(name)				\
  printf ("%s = %X\n", #name, name);

#include "libc-ns3.h"
}

} // extern "C"

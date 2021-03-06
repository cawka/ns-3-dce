#undef __OPTIMIZE__
#include <stdio.h>
#include <unistd.h>

#include "libc-globals.h"
#include "dce-global-variables.h"
#include "libc.h"

extern Libc g_libc; // this is initialized in libc.c

extern char *__progname;

extern "C" {

void setup_global_variables ()
{
  struct DceGlobalVariables globals;
  globals.pstdin = &stdin;
  globals.pstdout = &stdout;
  globals.pstderr = &stderr;
  globals.penvp = &environ;
  globals.poptarg = &optarg;
  globals.popterr = &opterr;
  globals.poptind = &optind;
  globals.poptopt = &optopt;
  globals.pprogname = &__progname;

  typedef void (*dce_global_variables_setup_t) (struct DceGlobalVariables *);
  ((dce_global_variables_setup_t)g_libc.dce_global_variables_setup_fn) (&globals);
}

}

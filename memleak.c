#if 0
exe="$(mktemp)"
cc -ggdb3 -O2 -fwrapv -fms-extensions -Wall -Wvla -Wno-parentheses -Wno-microsoft "$0" -o "$exe" && trap "exec rm -f -- $exe" EXIT && "$exe" "$@"
ret="$?"
rm -f -- "$exe"
exit "$ret"
static_assert (0, "unreachable");
#endif
#include "cmacs.h"

int
main (int argc, char *argv[])
{
#define MPAGE 4096
  void *p;
  while ((p = malloc (MPAGE)))
    memset (p, 0xCC, MPAGE);
}

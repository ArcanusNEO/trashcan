#if 0
exe="$(mktemp)" || exit "$?"; trap 'rm -f -- "$exe"' EXIT && \
cc -ggdb3 -O2 -fwrapv -fms-extensions -fno-strict-aliasing -Wall -Wvla -Wno-microsoft "$0" -o "$exe" && \
"$exe" "$@"
exit "$?"
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

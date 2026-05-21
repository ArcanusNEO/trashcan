#if 0
exe=/tmp/"$(head -c48 /dev/urandom | base64 | tr /+ _-)"
g++ -std=gnu++20 -ggdb2 -fwrapv -Wall -Wvla -Wno-parentheses -O2 "$0" -o "$exe" && trap "exec rm -f -- $exe" HUP INT TERM && "$exe" "$@"
ret="$?"
rm -f -- "$exe"
exit "$ret"
static_assert (0, "unreachable");
#endif
#include <bits/stdc++.h>
using namespace std;

int
main (int argc, char *argv[])
{
}

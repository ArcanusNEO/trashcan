// cc -O3 -oxkeymask xkeymask.c -lX11
#include <stdio.h>

#include <X11/Xlib.h>

void
handle_key_event (XKeyEvent event)
{
  if (event.type == KeyPress)
    NULL;
}

int
main (int argc, char *argv[])
{
  Display *display = XOpenDisplay (NULL);
  Window root = DefaultRootWindow (display);

  for (int i = 1; i < argc; ++i)
    {
      KeySym keysym = XStringToKeysym (argv[i]);
      if (keysym != NoSymbol)
        {
          XGrabKey (display, XKeysymToKeycode (display, keysym), AnyModifier,
                    root, True, GrabModeAsync, GrabModeAsync);
        }
    }

  XSelectInput (display, root, KeyPressMask);

  while (1)
    {
      XEvent event;
      XNextEvent (display, &event);

      if (event.type == KeyPress)
        handle_key_event (event.xkey);
    }

  XCloseDisplay (display);
}

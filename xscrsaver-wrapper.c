#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int
intercept_input (Display *display)
{
  KeyCode left_ctrl_code = XKeysymToKeycode (display, XK_Control_L);
  KeyCode right_ctrl_code = XKeysymToKeycode (display, XK_Control_R);
  KeyCode c_code = XKeysymToKeycode (display, XK_c);

  unsigned long ctrl = 0;

  while (1)
    {
      XEvent event;
      XNextEvent (display, &event);
      switch (event.type)
        {
        case KeyPress:
          if (event.xkey.keycode == left_ctrl_code
              || event.xkey.keycode == right_ctrl_code)
            ++ctrl;
          else if (event.xkey.keycode == c_code && ctrl > 0)
            return 0;
          break;
        case KeyRelease:
          if ((event.xkey.keycode == left_ctrl_code
               || event.xkey.keycode == right_ctrl_code)
              && ctrl > 0)
            --ctrl;
          break;
        case ButtonPress:
          break;
        case ButtonRelease:
        case MotionNotify:
          return 0;
        default:
          break;
        }
    }
  return 1;
}

int
main (int argc, char *argv[])
{
  if (argc < 2)
    exit (EXIT_FAILURE);

  Display *display = XOpenDisplay (NULL);

  if (display == NULL)
    {
      fprintf (stderr, "Failed to open display\n");
      exit (EXIT_FAILURE);
    }

  Window root = DefaultRootWindow (display);

  XGrabPointer (display, root, False,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
  XGrabKeyboard (display, root, False, GrabModeAsync, GrabModeAsync,
                 CurrentTime);
  XSelectInput (display, root,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask
                    | KeyPressMask | KeyReleaseMask);

  pid_t pid = fork ();
  if (pid > 0)
    {
      intercept_input (display);
      if (waitpid (pid, NULL, WNOHANG) == 0)
        kill (pid, SIGTERM);
      if (waitpid (pid, NULL, 0) < 0)
        goto ABORT;
    }
  else if (pid == 0)
    {
      execvp (argv[1], argv + 1);
      while (1)
        getchar ();
    }
  else
    {
      fprintf (stderr, "Failed to spawn child process\n");
      goto ABORT;
    }

  XCloseDisplay (display);
  exit (EXIT_SUCCESS);

ABORT:
  XCloseDisplay (display);
  exit (EXIT_FAILURE);
}

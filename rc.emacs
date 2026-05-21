#!/bin/sh
# Start/stop/restart the emacs daemon.

. /etc/profile
if [ -r ~/.bashrc ]; then
  . ~/.bashrc
fi
unset XDG_RUNTIME_DIR

emacs_start() {
  if [ -x /usr/bin/emacs ]; then
    echo "Starting emacs daemon:  /usr/bin/emacs --fg-daemon"
    /usr/bin/daemon -r -P ~/.run -n emacs -- /usr/bin/emacs --fg-daemon
  fi
}

emacs_stop() {
  echo "Stopping emacs daemon..."
  /usr/bin/daemon -r -P ~/.run -n emacs --stop
}

emacs_restart() {
  echo "Restarting emacs daemon..."
  /usr/bin/daemon -r -P ~/.run -n emacs --restart
}

case "$1" in
  'start')
    emacs_start
    ;;
  'stop')
    emacs_stop
    ;;
  'restart')
    emacs_restart
    ;;
  *)
    echo "usage $0 start|stop|restart"
esac

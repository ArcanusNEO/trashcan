#!/bin/sh

adb_start() {
  if [ -x /usr/bin/adb ]; then
    echo "Starting adb server:  /usr/bin/adb start-server &"
    /usr/bin/adb start-server >/dev/null 2>&1 &
  fi
}

adb_stop() {
  echo "Stopping adb server..."
  /usr/bin/adb kill-server
}

adb_restart() {
  adb_stop
  adb_start
}

case "$1" in
  'start')
    adb_start
    ;;
  'stop')
    adb_stop
    ;;
  'restart')
    adb_restart
    ;;
  *)
    echo "usage $0 start|stop|restart"
esac

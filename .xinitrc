#!/bin/bash

waitfor() {
  local i
  for ((i = 0; i < 200; ++i)); do
    "$@" >/dev/null 2>&1 && return 0
    sleep 0.05
  done
  echo "waitfor timeout: $*" >&2
  return 1
}

sysresources=/etc/X11/xinit/.Xresources
sysmodmap=/etc/X11/xinit/.Xmodmap
userresources=~/.Xresources
usermodmap=~/.Xmodmap
userprofile=~/.xprofile
if [ -f "$sysresources" ]; then
  /usr/bin/xrdb -merge "$sysresources"
fi
if [ -f "$sysmodmap" ]; then
  /usr/bin/xmodmap "$sysmodmap"
fi
if [ -f "$userresources" ]; then
  /usr/bin/xrdb -merge "$userresources"
fi
if [ -f "$usermodmap" ]; then
  /usr/bin/xmodmap "$usermodmap"
fi
if [ -f "$userprofile" ]; then
  . "$userprofile"
fi

if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
  eval "$(dbus-launch --sh-syntax --exit-with-session)"
fi

daemon -rB -P ~/.run -n pipewire /usr/bin/pipewire
waitfor pw-cli info 0
daemon -rB -P ~/.run -n wireplumber /usr/bin/wireplumber
daemon -rB -P ~/.run -n pipewire-pulse /usr/bin/pipewire-pulse

/usr/lib64/xfce4/notifyd/xfce4-notifyd &
tint2 &
(
  timeout 10 xdotool search --sync --class tint2
  (
    waitfor pactl info
    volctl &
  ) &
  fcitx5 -dr &
  xfce4-clipman &
  nm-applet &
  # blueman-applet &
) &

picom --backend=xrender --vsync --unredir-if-possible --use-damage &

(
  firefox --new-instance --private-window about:blank &
  timeout 10 xdotool search --sync --onlyvisible --name 'Mozilla Firefox .' windowunmap %@
) &
(
  thunderbird --new-instance &
  timeout 10 xdotool search --sync --onlyvisible --class thunderbird windowminimize %@
) &
alacritty --daemon &

exec cwm

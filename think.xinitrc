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

echo 248 >/sys/class/backlight/intel_backlight/brightness

if xrandr | grep -q 'HDMI-1 connected'; then
  xrandr --output HDMI-1 --primary --mode 2560x1440 --rate 75 --dpi 192 --scale 1.5
  xrandr --output eDP-1 --off
else
  xrandr --output eDP-1 --primary --mode 2880x1800 --rate 120 --dpi 192
fi

if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
  eval "$(dbus-launch --sh-syntax --exit-with-session)"
fi

daemon -rB -P ~/.run -n pipewire /usr/bin/pipewire
waitfor pw-cli info 0
daemon -rB -P ~/.run -n wireplumber /usr/bin/wireplumber
daemon -rB -P ~/.run -n pipewire-pulse /usr/bin/pipewire-pulse

/usr/lib64/xfce4/notifyd/xfce4-notifyd &
~/.local/bin/platform-profile-notifyd &
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
  blueman-applet &
) &

picom --backend=xrender --no-vsync --unredir-if-possible --use-damage &

(
  firefox --new-instance --private-window about:blank &
  timeout 10 xdotool search --sync --onlyvisible --name 'Mozilla Firefox .' windowunmap %@
) &
(
  thunderbird --no-remote --new-instance &
  timeout 10 xdotool search --sync --onlyvisible --class thunderbird windowminimize %@
) &
alacritty --daemon &

exec cwm

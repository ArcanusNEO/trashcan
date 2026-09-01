#!/bin/bash

sysresources=/etc/X11/xinit/.Xresources
sysmodmap=/etc/X11/xinit/.Xmodmap
userresources="$HOME"/.Xresources
usermodmap="$HOME"/.Xmodmap
userprofile="$HOME"/.xprofile

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

xrandr --output eDP-1 --primary --mode 2880x1800 --rate 120 --dpi 192

daemon -rB -P ~/.run -n pipewire /usr/bin/pipewire
sleep 0.3
daemon -rB -P ~/.run -n wireplumber /usr/bin/wireplumber
sleep 0.3
daemon -rB -P ~/.run -n pipewire-pulse /usr/bin/pipewire-pulse
sleep 0.1

if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
  eval "$(dbus-launch --sh-syntax --exit-with-session)"
fi
sleep 0.1

daemon -rB -P ~/.run -n platform-profile-notifyd ~/.local/bin/platform-profile-notifyd

echo 248 >/sys/class/backlight/intel_backlight/brightness

tint2 &
/usr/lib64/xfce4/notifyd/xfce4-notifyd &

sleep 1 && dash -c ' \
volctl & \
fcitx5 -dr & \
xfce4-clipman & \
nm-applet & \
blueman-applet & \
' &

picom --backend=xrender --no-vsync --unredir-if-possible --use-damage &

alacritty --hold --daemon &
(
  thunderbird &
  xdotool search --sync --onlyvisible --class thunderbird windowminimize
) &
(
  firefox --private-window &
  xdotool search --sync --onlyvisible --class firefox windowunmap
) &

exec cwm

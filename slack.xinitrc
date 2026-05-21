#!/bin/sh

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

echo 62015 >/sys/class/backlight/intel_backlight/brightness

tint2 &
/usr/lib64/xfce4/notifyd/xfce4-notifyd &
volumeicon &
fcitx5 -dr &
xfce4-clipman &

# picom --backend=xrender --no-vsync --no-fading-openclose --unredir-if-possible --blur-method=none --use-damage &

alacritty --hold --daemon &
thunderbird &
firefox --private-window &

exec cwm

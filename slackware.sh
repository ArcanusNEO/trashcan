#!/bin/sh

prefix=/data/root/slackware

droidmount() {
  mount --bind --make-private "$*" "$*"
  mount --bind --make-private /data "$*"/data
  mount --bind --make-private /metadata "$*"/metadata
  mount --bind --make-private /debug_ramdisk "$*"/debug_ramdisk
  mount --bind --make-private /debug_ramdisk/.magisk/pts "$*"/debug_ramdisk/.magisk/pts
  mount --bind --make-private /dev "$*"/dev
  mount -t devpts -o mode=0620,nosuid,noexec devpts "$*"/dev/pts
  mount -t binder binder "$*"/dev/binderfs
  mount -t proc -o nosuid,noexec,nodev proc "$*"/proc
  mount -t sysfs -o nosuid,noexec,nodev,ro sys "$*"/sys
  mount -t tmpfs -o mode=1777,nosuid,nodev shm "$*"/dev/shm
  mount -t tmpfs -o mode=0755,nosuid,nodev run "$*"/run
  mount -t tmpfs -o mode=1777,nodev,nosuid tmp "$*"/tmp
}

droidmount "$prefix"
mkdir "$prefix"/tmp/.ICE-unix && chmod 1777 "$prefix"/tmp/.ICE-unix
mkdir "$prefix"/tmp/.X11-unix && chmod 1777 "$prefix"/tmp/.X11-unix

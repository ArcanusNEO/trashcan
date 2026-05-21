#!/bin/sh

grub_file=/boot/grub/grub.cfg
custom_file=/boot/grub/custom.cfg

add_entry() {
  local mode=$(echo "$2" | tr '[:upper:]' '[:lower:]')
  # local mode="$2"
  sed -E -n "/menuentry '$1'/,/}/p" $grub_file | grep -v savedefault | sed -E -e "s/menuentry '$1'/menuentry '$1 ($2)'/g" -e "s/(^\s*linux.*$)/\1 optimus-manager.startup=${mode}/g" >>$custom_file
}

cat /dev/null >$custom_file

add_entry 'Arch Linux, with Linux linux-zen' 'Hybrid'
add_entry 'Arch Linux, with Linux linux-zen' 'Integrated'
add_entry 'Arch Linux, with Linux linux-zen' 'Nvidia'

cat >>$custom_file <<EOF
menuentry 'System Restart' {
	reboot
}
menuentry 'System Shutdown' {
	halt
}
EOF

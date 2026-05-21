#!/bin/sh
acme.sh --issue -d 'mail.transcliff.eu.org' --webroot /var/www/html --keylength ec-256
acme.sh --installcert --ecc -d 'mail.transcliff.eu.org' --key-file /etc/ssl/private/iRedMail.key --fullchain-file /etc/ssl/certs/iRedMail.crt --reloadcmd 'systemctl restart postfix.service; systemctl restart dovecot.service; systemctl restart nginx.service'
acme.sh --upgrade --auto-upgrade

#!/bin/bash

set -ex

apt update && apt upgrade -y
apt install -y coreutils moreutils man-db openssl apt-transport-https ca-certificates gnupg wget curl git ncat socat iproute2 nftables iptables iprange ipset kmod
apt install --no-install-recommends -y wireguard-tools

# hysteria
HYPASS=$(head -c12 /dev/urandom | base64)

iptables -t nat -A PREROUTING -i eth+ -p udp --dport "$2" -j DNAT --to-destination :$1
ip6tables -t nat -A PREROUTING -i eth+ -p udp --dport "$2" -j DNAT --to-destination :$1
apt install -y iptables-persistent

bash <(curl -fsSL https://get.hy2.sh/)

cd /etc/hysteria
rm -rf -- hysteria*
openssl ecparam -name prime256v1 -out hysteria.alg
openssl req -batch -x509 -sha256 -nodes -days 32767 -newkey ec:hysteria.alg -keyout hysteria.key -out hysteria.crt
rm -rf -- hysteria.alg
chown root:hysteria *
chmod 640 *

cat <<EOF >config.yaml
listen: :$1

tls:
  cert: /etc/hysteria/hysteria.crt
  key: /etc/hysteria/hysteria.key

auth:
  type: password
  password: $HYPASS

congestion:
  type: bbr
  bbrProfile: aggressive

masquerade:
  type: proxy
  proxy:
    url: http://info.cern.ch
    rewriteHost: true
  listenHTTPS: :$1
EOF

systemctl restart hysteria-server.service
systemctl enable hysteria-server.service

apt clean all

cd /etc/hysteria
grep -F -- listen: config.yaml | sed -E 's/^\s*//'
grep -F -- password: config.yaml | sed -E 's/^\s*//'
openssl x509 -noout -fingerprint -sha256 -in hysteria.crt
systemctl status hysteria-server.service

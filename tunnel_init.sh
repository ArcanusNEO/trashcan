#!/bin/bash
set -ex

apt update && apt upgrade -y
apt install -y coreutils moreutils man-db openssl apt-transport-https ca-certificates gnupg wget curl git ncat socat iproute2 nftables iptables iprange ipset kmod
apt install --no-install-recommends -y wireguard-tools

# iptables -t nat -A PREROUTING -i eth+ -p udp --dport "$2" -j DNAT --to-destination ":$1"
# ip6tables -t nat -A PREROUTING -i v6net+ -p udp --dport "$2" -j DNAT --to-destination ":$1"
# debconf-set-selections <<< 'iptables-persistent iptables-persistent/autosave_v4 boolean true'
# debconf-set-selections <<< 'iptables-persistent iptables-persistent/autosave_v6 boolean true'
# apt install -y iptables-persistent

bash <(curl -fsSL https://get.hy2.sh/)
cd /etc/hysteria
rm -rf -- hysteria*
openssl ecparam -name prime256v1 -out hysteria.alg
openssl req -batch -x509 -sha256 -nodes -days 32767 -newkey ec:hysteria.alg -keyout hysteria.key -out hysteria.crt
rm -rf -- hysteria.alg
chown root:hysteria *
chmod 640 *
HYPASSWORD=$(head -c12 /dev/urandom | base64 | tr /+ _-)
cat <<EOF >config.yaml
listen: :$1

tls:
  cert: /etc/hysteria/hysteria.crt
  key: /etc/hysteria/hysteria.key

auth:
  type: password
  password: $HYPASSWORD

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

bash -c "$(curl -L https://github.com/XTLS/Xray-install/raw/main/install-release.sh)" @ install -u root
cd /usr/local/etc/xray
VLSID=$(cat /proc/sys/kernel/random/uuid)
cat >config.json <<EOF
{
  "log": {
    "loglevel": "warning"
  },
  "inbounds": [{
    "port": 80,
    "listen": "0.0.0.0",
    "protocol": "vless",
    "settings": {
      "clients": [{
        "id": "$VLSID"
      }],
      "decryption": "none"
    },
    "streamSettings": {
      "network": "ws",
      "security": "none"
    }
  }],
  "outbounds": [{
    "protocol": "freedom"
  }]
}
EOF
systemctl restart xray.service
systemctl enable xray.service

set +ex

apt clean all

cd /etc/hysteria
systemctl -l --no-pager status hysteria-server.service
grep -F -- listen: config.yaml | sed -E 's/^\s*//'
grep -F -- password: config.yaml | sed -E 's/^\s*//'
openssl x509 -noout -fingerprint -sha256 -in hysteria.crt

cd /usr/local/etc/xray
systemctl -l --no-pager status xray.service
grep -F -- '"id":' config.json | sed -E 's/^\s*//'

#!/usr/bin/env bash
set -euo pipefail

# ------------------------
# Config
# ------------------------
CON_NAME="pi-usb"
HOST_IP="192.168.7.1/24"
PI_IP="192.168.7.2"
SSH_HOST="prism-usb"
SSH_USER="prism"

echo "==> Setting up USB Ethernet connection for Raspberry Pi"

# ------------------------
# Create or update NM connection
# ------------------------
if nmcli con show "$CON_NAME" >/dev/null 2>&1; then
  echo "    Connection '$CON_NAME' exists"
else
  echo "    Creating connection '$CON_NAME'"
  nmcli con add type ethernet con-name "$CON_NAME"
fi

nmcli con modify "$CON_NAME" \
  connection.autoconnect yes \
  connection.autoconnect-priority 100 \
  connection.interface-name "" \
  connection.multi-connect 1 \
  ipv4.method manual \
  ipv4.addresses "$HOST_IP" \
  ipv6.method ignore

echo "==> NetworkManager connection configured"

# ------------------------
# Reset stale state (important)
# ------------------------
nmcli con down "$CON_NAME" 2>/dev/null || true

# ------------------------
# Find Pi USB Ethernet device
# ------------------------
echo "==> Waiting for Pi USB Ethernet device..."

DEV=""
for i in {1..10}; do
  for d in $(nmcli -t -f DEVICE,TYPE,STATE device | awk -F: '$2=="ethernet" && $3!="connected"{print $1}'); do
    DEV="$d"
    break
  done
  [[ -n "$DEV" ]] && break
  sleep 0.5
done

if [[ -z "$DEV" ]]; then
  echo "ERROR: No disconnected ethernet device found"
  echo "Is the Pi plugged into the USB *data* port?"
  exit 1
fi

echo "==> Using interface: $DEV"

# ------------------------
# Bring connection up
# ------------------------
nmcli con up "$CON_NAME" ifname "$DEV"

# ------------------------
# Wait for route to exist
# ------------------------
echo "==> Waiting for route..."
for i in {1..10}; do
  ip route | grep -q "192.168.7.0/24" && break
  sleep 0.5
done

# ------------------------
# SSH config
# ------------------------
echo "==> Configuring SSH"

SSH_CONFIG="$HOME/.ssh/config"
mkdir -p "$HOME/.ssh"
touch "$SSH_CONFIG"
chmod 600 "$SSH_CONFIG"

if ! grep -qE "^Host[[:space:]]+$SSH_HOST\$" "$SSH_CONFIG"; then
  cat >> "$SSH_CONFIG" <<EOF

Host $SSH_HOST
  HostName $PI_IP
  User $SSH_USER
  ConnectTimeout 3
  ConnectionAttempts 5
EOF
fi

echo
echo "Setup complete"
echo "You can now run:"
echo "  ssh $SSH_HOST"

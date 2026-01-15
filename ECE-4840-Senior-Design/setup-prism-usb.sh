#!/usr/bin/env bash
set -euo pipefail

# ------------------------
# Configuration
# ------------------------
CON_NAME="pi-usb"
HOST_IP="192.168.7.1/24"
PI_IP="192.168.7.2"
SSH_HOST="prism-usb"
SSH_USER="prism"

echo "==> Setting up NetworkManager connection: $CON_NAME"

# Check if connection already exists
if nmcli con show "$CON_NAME" >/dev/null 2>&1; then
  echo "    Connection already exists, updating it"
else
  echo "    Creating new connection"
  nmcli con add type ethernet con-name "$CON_NAME"
fi

# Configure the connection (idempotent)
nmcli con modify "$CON_NAME" \
  connection.autoconnect yes \
  connection.autoconnect-priority 100 \
  connection.interface-name "" \
  connection.match-device-type ethernet \
  connection.multi-connect 1 \
  ipv4.method manual \
  ipv4.addresses "$HOST_IP" \
  ipv6.method link-local

echo "==> NetworkManager configuration complete"

# Automatically bring up the connection on any disconnected Ethernet device
for dev in $(nmcli -t -f DEVICE,TYPE,STATE device | awk -F: '$2=="ethernet" && $3!="connected" {print $1}'); do
  echo "==> Activating $CON_NAME on $dev"
  nmcli con up "$CON_NAME" ifname "$dev" && break || true
done

# ------------------------
# SSH config
# ------------------------
echo "==> Setting up SSH config"

SSH_CONFIG="$HOME/.ssh/config"
mkdir -p "$HOME/.ssh"
touch "$SSH_CONFIG"
chmod 600 "$SSH_CONFIG"

# Add host entry if it doesn't exist
if grep -qE "^Host[[:space:]]+$SSH_HOST\$" "$SSH_CONFIG"; then
  echo "    SSH host '$SSH_HOST' already exists, skipping"
else
  echo "    Adding SSH host '$SSH_HOST'"
  cat >> "$SSH_CONFIG" <<EOF

Host $SSH_HOST
  HostName $PI_IP
  User $SSH_USER
EOF
fi

echo "==> SSH configuration complete"

echo
echo "Setup finished."
echo "You should be able to run:"
echo "  ssh $SSH_HOST"

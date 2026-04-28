#!/usr/bin/env bash
set -euo pipefail

IFACE="wlan1"
CHANNEL="44"

if [[ "${EUID}" -ne 0 ]]; then
  echo "This script must run as root. Try: sudo ./setup_monitor_wlan1_ch10.sh"
  exit 1
fi

if ! ip link show "${IFACE}" >/dev/null 2>&1; then
  echo "Interface ${IFACE} not found."
  exit 1
fi

echo "Configuring ${IFACE} for monitor mode on channel ${CHANNEL}..."

# Bring interface down before changing mode/channel.
ip link set "${IFACE}" down

# Set monitor mode with iw (no airmon-ng dependency).
iw dev "${IFACE}" set type monitor

# Bring interface up and pin channel.
ip link set "${IFACE}" up
iw dev "${IFACE}" set channel "${CHANNEL}"

echo
echo "Done. Current interface state:"
iw dev "${IFACE}" info

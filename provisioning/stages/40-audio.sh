#!/bin/bash
# Stage 40: the Scarlett 2i2.
#
# PieJam talks to the kernel's ALSA interface directly (it doesn't link
# alsa-lib), so there's no asound.conf to write. The udev rule gives the card
# a stable name and marks the "audio ready" milestone.
set -euo pipefail
# shellcheck source=../lib.sh
source "$(dirname "$0")/../lib.sh"
require_root

install_file config/udev/70-piejam-audio.rules /etc/udev/rules.d/70-piejam-audio.rules 0644
install_file systemd/piejam-audio-ready.service /etc/systemd/system/piejam-audio-ready.service 0644
udevadm control --reload
systemctl daemon-reload

# A sound server would compete with PieJam for the interface. Raspberry Pi OS
# Lite doesn't install one, but check.
for pkg in pipewire pulseaudio jackd2; do
    if dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q 'ok installed'; then
        warn "$pkg is installed and may hold the audio interface open"
    fi
done

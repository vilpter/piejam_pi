#!/bin/bash
# Stage 60: boot straight into PieJam.
set -euo pipefail
# shellcheck source=../lib.sh
source "$(dirname "$0")/../lib.sh"
require_root

install_file bin/piejam-launch /usr/local/lib/piejam/piejam-launch 0755
install_file systemd/piejam.service /etc/systemd/system/piejam.service 0644
systemctl daemon-reload

systemctl set-default multi-user.target

# Nothing is drawn on tty1 before the mixer appears. Other consoles still
# spawn on demand (Alt+F2) if the app is down.
systemctl disable --quiet getty@tty1.service

# The network is never a boot dependency (ADR 0005), but SSH is always there
# for maintenance once a network comes up.
if unit_exists NetworkManager-wait-online.service; then
    systemctl disable --quiet NetworkManager-wait-online.service
fi
systemctl enable --quiet ssh.service

systemctl enable --quiet piejam.service
log "piejam.service enabled"

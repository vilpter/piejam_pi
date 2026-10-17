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

# The NFS server is shared on demand from the UI, never at boot (ADR 0005).
# Installing nfs-kernel-server enables and starts it, so turn it off and stop
# it; the app starts it when the user shares recordings.
if unit_exists nfs-server.service; then
    systemctl disable --now --quiet nfs-server.service
fi

# Phase 2 boot trims (F10). On this hardware both sit on piejam.service's
# critical chain: fsck -> boot-firmware.mount -> cloud-init -> sysinit.

# Skip the fsck of the FAT boot partition. It runs on every boot and cloud-init
# waits for the mount, yet the partition is written only on kernel/firmware
# updates, so a boot-time check earns little. The root filesystem is still
# checked. Set the boot partition's fstab pass number (6th field) to 0.
fstab=/etc/fstab
if awk '$2=="/boot/firmware" && $6!=0 {f=1} END{exit !f}' "$fstab"; then
    backup_once "$fstab"
    tmp=$(mktemp)
    awk 'BEGIN{OFS="\t"} $2=="/boot/firmware"{$6="0"} {print}' "$fstab" > "$tmp"
    cat "$tmp" > "$fstab"
    rm -f "$tmp"
    log "fstab: boot-partition fsck disabled (pass 0)"
fi

# Disable cloud-init. It ran once to apply the image's first-boot settings
# (user, hostname, SSH, network), which now persist on disk; on every later
# boot it still runs before sysinit.target. This file is the documented kill
# switch its systemd generator checks. Delete the file to re-enable.
if [[ -d /etc/cloud && ! -e /etc/cloud/cloud-init.disabled ]]; then
    touch /etc/cloud/cloud-init.disabled
    log "cloud-init disabled (/etc/cloud/cloud-init.disabled)"
fi

systemctl enable --quiet piejam.service
log "piejam.service enabled"

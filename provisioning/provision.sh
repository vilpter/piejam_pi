#!/bin/bash
# Turn a fresh Raspberry Pi OS Lite install into a PieJam appliance.
#
# Usage:
#   sudo ./provisioning/provision.sh            run every stage in order
#   sudo ./provisioning/provision.sh 50-app     run selected stages only
#
# Every stage is idempotent, so re-running is safe. See provisioning/README.md.
set -euo pipefail

cd "$(dirname "$0")"
source ./lib.sh

require_root

# Warn rather than fail on an unexpected platform, so the scripts stay usable
# for testing on other hardware.
check_platform() {
    local codename model
    codename=$(. /etc/os-release && echo "${VERSION_CODENAME:-unknown}")
    [[ $codename == trixie ]] ||
        warn "expected Raspberry Pi OS based on Debian 13 (trixie), found '$codename'"
    [[ $(uname -m) == aarch64 ]] ||
        warn "expected a 64-bit (aarch64) system, found $(uname -m)"
    model=$(tr -d '\0' < /proc/device-tree/model 2>/dev/null || echo unknown)
    [[ $model == "Raspberry Pi 5"* ]] ||
        warn "expected a Raspberry Pi 5, found '$model'"
    [[ -d $BOOT_DIR ]] || die "$BOOT_DIR not found; is this Raspberry Pi OS?"
}

check_platform

if (( $# )); then
    stages=()
    for name in "$@"; do
        stages+=("stages/${name%.sh}.sh")
    done
else
    stages=(stages/[0-9][0-9]-*.sh)
fi

for stage in "${stages[@]}"; do
    [[ -f $stage ]] || die "no such stage: $stage"
    log "=== ${stage#stages/}"
    bash "$stage"
done

log "provisioning complete; reboot to start the appliance"

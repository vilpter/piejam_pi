#!/bin/bash
# Record boot milestones for the current boot. Run on the Pi after it boots:
#
#   sudo ./benchmarks/measure-boot.sh [label]
#
# Writes a full report to benchmarks/results/ and appends one row to
# benchmarks/results/boot-log.csv. All times are seconds since kernel start
# (CLOCK_MONOTONIC). Firmware time before the kernel is not visible to
# systemd; see docs/benchmark-plan.md for measuring it.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
results=$here/results
label=${1:-unlabeled}
mkdir -p "$results"

# prop [UNIT] PROPERTY — a systemd property (the manager's if UNIT is omitted).
prop() {
    if (( $# == 1 )); then systemctl show -P "$1"; else systemctl show -P "$2" "$1"; fi
}

# Microseconds to seconds; "n/a" for zero (milestone not reached).
secs() {
    awk -v us="${1:-0}" 'BEGIN { if (us > 0) printf "%.2f", us / 1e6; else printf "n/a" }'
}

# Last journal line of piejam.service matching a pattern, in the given output
# format (short-monotonic or short-unix); prints its leading timestamp. The
# last match belongs to the app's latest start, like the other milestones.
journal_stamp() {
    journalctl -b -u piejam.service -o "$2" --no-pager 2>/dev/null |
        grep -E "$1" | tail -n 1 |
        sed -nE 's/^\[? *([0-9]+\.[0-9]+)\]? .*/\1/p' || true
}

# The same, as seconds since kernel start.
journal_secs() {
    journal_stamp "$1" short-monotonic | awk '{ printf "%.2f", $1 }'
}

# Wait for boot to finish so systemd has final numbers.
systemctl is-system-running --wait >/dev/null 2>&1 || true

userspace_start=$(secs "$(prop UserspaceTimestampMonotonic)")
boot_finished=$(secs "$(prop FinishTimestampMonotonic)")
piejam_start=$(secs "$(prop piejam.service ExecMainStartTimestampMonotonic)")
restarts=$(prop piejam.service NRestarts)
display_found=$(journal_secs 'piejam-launch: using')
# piejam.service is Type=notify: it goes active exactly when PieJam sends
# READY=1, which it does once its first frame is on screen (F5).
ui_visible=$(secs "$(prop piejam.service ActiveEnterTimestampMonotonic)")
ui_note="sd_notify at first frame"
audio_ready=$(secs "$(prop piejam-audio-ready.service ActiveEnterTimestampMonotonic)")

chain=$(systemd-analyze critical-chain piejam.service --no-pager 2>&1 || true)
# Units that wait for or bring up the network. Match unit names, not the bare
# word: cloud-init-network.service is on the chain but doesn't wait for one.
network_units_re='(network(-online|-pre)?\.target|NetworkManager[^ ]*\.service|systemd-networkd[^ ]*\.service|[^ ]*wait-online[^ ]*\.service)'
if grep -qE "$network_units_re" <<< "$chain"; then
    network_in_chain=yes
else
    network_in_chain=no
fi

stamp=$(date +%Y%m%d-%H%M%S)
report=$results/boot-$stamp-$label.txt
{
    echo "label:            $label"
    echo "date:             $(date -Is)"
    echo "kernel:           $(uname -r)"
    echo "piejam version:   $(cat /usr/local/share/piejam/VERSION 2>/dev/null || echo unknown)"
    echo
    echo "== Milestones (seconds since kernel start)"
    echo "userspace start:  $userspace_start"
    echo "piejam started:   $piejam_start   (restarts: $restarts)"
    echo "display found:    ${display_found:-n/a}"
    echo "UI visible:       ${ui_visible:-n/a}   ($ui_note)"
    echo "audio ready:      $audio_ready"
    echo "boot finished:    $boot_finished"
    echo "network in chain: $network_in_chain"
    echo
    echo "== systemd-analyze"
    systemd-analyze --no-pager 2>&1 || true
    echo
    echo "== critical chain: piejam.service"
    echo "$chain"
    echo
    echo "== blame (top 25)"
    systemd-analyze blame --no-pager 2>&1 | head -25 || true
} > "$report"
systemd-analyze plot > "${report%.txt}.svg" 2>/dev/null || true

csv=$results/boot-log.csv
if [[ ! -f $csv ]]; then
    echo "date,label,userspace_start,piejam_start,restarts,display_found,ui_visible,audio_ready,boot_finished,network_in_chain" > "$csv"
fi
echo "$(date -Is),$label,$userspace_start,$piejam_start,$restarts,${display_found:-n/a},${ui_visible:-n/a},$audio_ready,$boot_finished,$network_in_chain" >> "$csv"

sed -n '1,/^== systemd-analyze/p' "$report" | head -n -1
echo "report: $report"
if [[ $network_in_chain == yes ]]; then
    echo "WARNING: a network unit is on piejam.service's critical chain (see ADR 0005)" >&2
fi
if (( ${restarts:-0} > 0 )); then
    echo "WARNING: piejam.service restarted $restarts time(s); milestones reflect the last start" >&2
fi

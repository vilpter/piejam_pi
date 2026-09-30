#!/bin/bash
# Tests for the config-editing helpers in provisioning/lib.sh.
# Runs on any Linux machine against temporary copies of the boot files.
set -u

here=$(cd "$(dirname "$0")" && pwd)
source "$here/../provisioning/lib.sh"
log() { :; }   # keep test output clean

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
BOOT_DIR=$work

failures=0
check() {   # check NAME EXPECTED ACTUAL
    if [[ $2 == "$3" ]]; then
        echo "ok   - $1"
    else
        echo "FAIL - $1"
        echo "       expected: $2"
        echo "       actual:   $3"
        failures=$((failures + 1))
    fi
}

# --- ensure_cmdline_param -----------------------------------------------------
stock='console=serial0,115200 console=tty1 root=PARTUUID=abcd-02 rootfstype=ext4 fsck.repair=yes rootwait'
echo "$stock" > "$work/cmdline.txt"

ensure_cmdline_param quiet
ensure_cmdline_param loglevel=3
check "appends new parameters" "$stock quiet loglevel=3" "$(cat "$work/cmdline.txt")"

ensure_cmdline_param quiet
ensure_cmdline_param loglevel=3
check "is idempotent" "$stock quiet loglevel=3" "$(cat "$work/cmdline.txt")"

ensure_cmdline_param loglevel=1
check "replaces an existing key's value" "$stock quiet loglevel=1" "$(cat "$work/cmdline.txt")"

check "keeps the stays-one-line invariant" 1 "$(wc -l < "$work/cmdline.txt")"
check "backs up the original once" "$stock" "$(cat "$work/cmdline.txt.piejam-orig")"

# --- ensure_overlay_param -----------------------------------------------------
config=$work/config.txt
cat > "$config" <<'EOF'
dtoverlay=vc4-kms-v3d
dtoverlay=vc4-kms-v3d-pi4
max_framebuffers=2
EOF

ensure_overlay_param "$config" vc4-kms-v3d noaudio
check "adds the parameter to the bare overlay line" \
    "dtoverlay=vc4-kms-v3d,noaudio" "$(sed -n 1p "$config")"
check "leaves an overlay whose name only shares a prefix alone" \
    "dtoverlay=vc4-kms-v3d-pi4" "$(sed -n 2p "$config")"

ensure_overlay_param "$config" vc4-kms-v3d noaudio
check "is idempotent" "dtoverlay=vc4-kms-v3d,noaudio" "$(sed -n 1p "$config")"

printf 'dtoverlay=vc4-kms-v3d,cma-128 # graphics\n' > "$config"
ensure_overlay_param "$config" vc4-kms-v3d noaudio
check "appends after existing parameters and keeps comments" \
    "dtoverlay=vc4-kms-v3d,cma-128,noaudio # graphics" "$(cat "$config")"

printf 'dtoverlay=vc4-kms-v3d,noaudio,cma-128\n' > "$config"
ensure_overlay_param "$config" vc4-kms-v3d noaudio
check "notices the parameter anywhere in the list" \
    "dtoverlay=vc4-kms-v3d,noaudio,cma-128" "$(cat "$config")"

printf 'dtparam=audio=on\n' > "$config"
ensure_overlay_param "$config" vc4-kms-v3d noaudio
check "reports a missing overlay line" 1 "$?"

echo
if (( failures )); then
    echo "$failures test(s) failed"
    exit 1
fi
echo "all tests passed"

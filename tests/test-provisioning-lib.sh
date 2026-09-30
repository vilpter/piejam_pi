#!/bin/bash
# Tests for the config-editing helpers in provisioning/lib.sh.
# Runs on any Linux machine against temporary copies of the boot files.
# pipefail matches the stages; -e is left off so every case gets to run.
set -uo pipefail

here=$(cd "$(dirname "$0")" && pwd)
# shellcheck source=../provisioning/lib.sh
source "$here/../provisioning/lib.sh"
log() { :; }   # keep test output clean

work=$(mktemp -d)
trap 'chmod -R u+w "$work"; rm -rf "$work"' EXIT
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
chmod 644 "$work/cmdline.txt"

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
check "keeps the file's mode" 644 "$(stat -c %a "$work/cmdline.txt")"
check "leaves no temporary files behind" "" \
    "$(find "$work" -maxdepth 1 -name 'cmdline.txt.*' ! -name '*.piejam-orig')"

printf '%s' "$stock" > "$work/cmdline.txt"
ensure_cmdline_param quiet
check "handles a file without a trailing newline" "$stock quiet" "$(cat "$work/cmdline.txt")"

: > "$work/cmdline.txt"
( ensure_cmdline_param quiet ) 2>/dev/null
check "refuses an empty cmdline.txt" 1 "$?"
check "leaves an empty cmdline.txt untouched" "" "$(cat "$work/cmdline.txt")"

echo 'console=tty1 rootwait' > "$work/cmdline.txt"
( ensure_cmdline_param quiet ) 2>/dev/null
check "refuses a cmdline.txt without root=" 1 "$?"
check "leaves it untouched" "console=tty1 rootwait" "$(cat "$work/cmdline.txt")"

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

printf 'dtoverlay=vc4-kms-v3d # noaudio\n' > "$config"
ensure_overlay_param "$config" vc4-kms-v3d noaudio
check "doesn't count the parameter in a comment" \
    "dtoverlay=vc4-kms-v3d,noaudio # noaudio" "$(cat "$config")"

printf 'dtparam=audio=on\n' > "$config"
ensure_overlay_param "$config" vc4-kms-v3d noaudio
check "reports a missing overlay line" 1 "$?"

# --- failures must be loud, even where set -e is off --------------------------
# Stage 20 calls ensure_overlay_param inside "||", where bash disables set -e.
# A failed edit there must stop provisioning, not look like "no overlay line".
if (( EUID == 0 )); then
    echo "skip - write-failure cases (root ignores directory permissions)"
else
    ro=$work/readonly
    mkdir "$ro"
    printf 'dtoverlay=vc4-kms-v3d\n' > "$ro/config.txt"
    cp "$ro/config.txt" "$ro/config.txt.piejam-orig"
    chmod 555 "$ro"
    output=$( (set -e; ensure_overlay_param "$ro/config.txt" vc4-kms-v3d noaudio || echo "treated as missing line") 2>&1 )
    check "a failed overlay edit exits with an error" 1 "$?"
    check "a failed overlay edit says so" "yes" \
        "$(grep -q 'failed to edit' <<< "$output" && ! grep -q 'treated as missing' <<< "$output" && echo yes)"
    check "a failed overlay edit leaves the file alone" "dtoverlay=vc4-kms-v3d" "$(cat "$ro/config.txt")"

    chmod 755 "$ro"
    echo "$stock" > "$ro/cmdline.txt"
    cp "$ro/cmdline.txt" "$ro/cmdline.txt.piejam-orig"
    chmod 555 "$ro"
    ( BOOT_DIR=$ro; ensure_cmdline_param quiet ) 2>/dev/null
    check "a failed cmdline.txt write exits with an error" 1 "$?"
    check "a failed cmdline.txt write leaves the file alone" "$stock" "$(cat "$ro/cmdline.txt")"
    chmod 755 "$ro"
fi

echo
if (( failures )); then
    echo "$failures test(s) failed"
    exit 1
fi
echo "all tests passed"

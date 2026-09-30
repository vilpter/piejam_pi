#!/bin/bash
# Tests for bin/piejam-launch against a fake sysfs tree and a stub app.
# Runs on any Linux machine; no Raspberry Pi needed.
set -u

here=$(cd "$(dirname "$0")" && pwd)
launcher=$here/../bin/piejam-launch
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

failures=0
pass() { echo "ok   - $1"; }
fail() { echo "FAIL - $1"; failures=$((failures + 1)); }

# Fresh fake environment for each case.
setup() {
    rm -rf "$work"/*
    mkdir -p "$work/sys/class/drm" "$work/sys/class/input" "$work/run" "$work/bin"
    # Stub systemctl records what it was asked to do.
    cat > "$work/bin/systemctl" <<EOF
#!/bin/sh
echo "\$*" >> "$work/systemctl.log"
EOF
    chmod +x "$work/bin/systemctl"
}

add_connector() {   # add_connector card1-DSI-1 [status]
    mkdir -p "$work/sys/class/drm/$1"
    echo "${2:-connected}" > "$work/sys/class/drm/$1/status"
}

add_input() {       # add_input event0 "generic ft5x06 (79)"
    mkdir -p "$work/sys/class/input/$1/device"
    echo "$2" > "$work/sys/class/input/$1/device/name"
}

stub_app() {        # stub_app 'exit 0'
    printf '#!/bin/bash\ntouch "%s/app.ran"\n%s\n' "$work" "$1" > "$work/app"
    chmod +x "$work/app"
}

run_launcher() {
    PATH="$work/bin:$PATH" PIEJAM_APP="$work/app" PIEJAM_SYSFS="$work/sys" \
        RUNTIME_DIRECTORY="$work/run" PIEJAM_DISPLAY_TIMEOUT_TENTHS=3 \
        "$launcher" 2>"$work/stderr"
}

kms() { cat "$work/run/eglfs-kms.json" 2>/dev/null; }

# --- DSI panel with touch is chosen, and named the way Qt names it ----------
setup
add_connector card1-DSI-1
add_connector card1-HDMI-A-1 connected
add_input event0 "generic ft5x06 (79)"
stub_app 'exit 1'
run_launcher
kms | grep -q '"device": "/dev/dri/card1"' && kms | grep -q '"name": "DSI1"' &&
    pass "prefers DSI panel and converts DSI-1 to DSI1" ||
    fail "prefers DSI panel and converts DSI-1 to DSI1 (got: $(kms))"

# --- DSI connector without a touch controller is ignored ---------------------
setup
add_connector card1-DSI-1
add_connector card2-HDMI-A-2 connected
add_input event0 "Logitech USB Keyboard"
stub_app 'exit 1'
run_launcher
kms | grep -q '"device": "/dev/dri/card2"' && kms | grep -q '"name": "HDMI2"' &&
    pass "falls back to connected HDMI and converts HDMI-A-2 to HDMI2" ||
    fail "falls back to connected HDMI (got: $(kms))"

# --- "disconnected" HDMI is not treated as connected (PieJam OS bug) --------
setup
add_connector card1-HDMI-A-1 disconnected
stub_app 'exit 0'
run_launcher
status=$?
[[ $status -eq 1 && ! -e $work/app.ran ]] && grep -q 'no display found' "$work/stderr" &&
    pass "disconnected HDMI is ignored; no display fails after timeout" ||
    fail "disconnected HDMI handling (status $status, stderr: $(cat "$work/stderr"))"

# --- quitting the app (status 0) powers off ----------------------------------
setup
add_connector card1-DSI-1
add_input event0 "generic ft5x06 (79)"
stub_app 'exit 0'
run_launcher
grep -qx poweroff "$work/systemctl.log" 2>/dev/null &&
    pass "app exit 0 powers off" ||
    fail "app exit 0 powers off"

# --- a crash propagates its status and does not power off --------------------
setup
add_connector card1-DSI-1
add_input event0 "generic ft5x06 (79)"
stub_app 'exit 3'
run_launcher
status=$?
[[ $status -eq 3 && ! -e $work/systemctl.log ]] &&
    pass "app crash returns its status without powering off" ||
    fail "app crash (status $status)"

# --- SIGTERM from systemd never powers off, even if the app exits 0 ---------
setup
add_connector card1-DSI-1
add_input event0 "generic ft5x06 (79)"
# The app exits cleanly on SIGTERM, which is exactly the risky case.
stub_app 'trap "exit 0" TERM; while :; do sleep 0.05; done'
run_launcher &
pid=$!
sleep 0.5
# systemd signals every process in the service's cgroup.
pkill -TERM -P "$pid"
kill -TERM "$pid"
wait "$pid"
status=$?
[[ $status -eq 143 && ! -e $work/systemctl.log ]] &&
    pass "SIGTERM (systemctl stop) exits 143 without powering off" ||
    fail "SIGTERM handling (status $status, systemctl: $(cat "$work/systemctl.log" 2>/dev/null))"

echo
if (( failures )); then
    echo "$failures test(s) failed"
    exit 1
fi
echo "all tests passed"

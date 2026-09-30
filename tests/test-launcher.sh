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
    rm -rf "${work:?}"/*
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

run_launcher() {     # DISPLAY_TIMEOUT=20 run_launcher (tenths of a second)
    PATH="$work/bin:$PATH" PIEJAM_APP="$work/app" PIEJAM_SYSFS="$work/sys" \
        RUNTIME_DIRECTORY="$work/run" PIEJAM_DISPLAY_TIMEOUT_TENTHS="${DISPLAY_TIMEOUT:-3}" \
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
if kms | grep -q '"device": "/dev/dri/card1"' && kms | grep -q '"name": "DSI1"'; then
    pass "prefers DSI panel and converts DSI-1 to DSI1"
else
    fail "prefers DSI panel and converts DSI-1 to DSI1 (got: $(kms))"
fi

# --- DSI connector without a touch controller is ignored ---------------------
setup
add_connector card1-DSI-1
add_connector card2-HDMI-A-2 connected
add_input event0 "Logitech USB Keyboard"
stub_app 'exit 1'
run_launcher
if kms | grep -q '"device": "/dev/dri/card2"' && kms | grep -q '"name": "HDMI2"'; then
    pass "falls back to connected HDMI and converts HDMI-A-2 to HDMI2"
else
    fail "falls back to connected HDMI (got: $(kms))"
fi

# --- a display that probes after the launcher starts is still found --------
# systemd can start the service before the panel driver has probed.
setup
stub_app 'exit 1'
( sleep 0.3; add_connector card1-DSI-1; add_input event0 "generic ft5x06 (79)" ) &
DISPLAY_TIMEOUT=20 run_launcher
wait
if kms | grep -q '"name": "DSI1"'; then
    pass "waits for a display that appears after startup"
else
    fail "waits for a display that appears after startup (stderr: $(cat "$work/stderr"))"
fi

# --- "disconnected" HDMI is not treated as connected (PieJam OS bug) --------
setup
add_connector card1-HDMI-A-1 disconnected
stub_app 'exit 0'
run_launcher
status=$?
if [[ $status -eq 1 && ! -e $work/app.ran ]] && grep -q 'no display found' "$work/stderr"; then
    pass "disconnected HDMI is ignored; no display fails after timeout"
else
    fail "disconnected HDMI handling (status $status, stderr: $(cat "$work/stderr"))"
fi

# --- quitting the app (status 0) powers off ----------------------------------
setup
add_connector card1-DSI-1
add_input event0 "generic ft5x06 (79)"
stub_app 'exit 0'
run_launcher
if grep -qx poweroff "$work/systemctl.log" 2>/dev/null; then
    pass "app exit 0 powers off"
else
    fail "app exit 0 powers off"
fi

# --- a crash propagates its status and does not power off --------------------
setup
add_connector card1-DSI-1
add_input event0 "generic ft5x06 (79)"
stub_app 'exit 3'
run_launcher
status=$?
if [[ $status -eq 3 && ! -e $work/systemctl.log ]]; then
    pass "app crash returns its status without powering off"
else
    fail "app crash (status $status)"
fi

# --- SIGTERM from systemd never powers off, even if the app exits 0 ---------
setup
add_connector card1-DSI-1
add_input event0 "generic ft5x06 (79)"
# The app exits cleanly on SIGTERM, which is exactly the risky case. Its loop
# is bounded so a broken launcher can't leave it running forever.
# shellcheck disable=SC2016  # script text for the stub; the stub expands it
stub_app 'trap "exit 0" TERM; for _ in $(seq 100); do sleep 0.05; done; exit 9'
# Start the launcher itself in the background (not via run_launcher, whose
# subshell would make $! the wrong process).
PATH="$work/bin:$PATH" PIEJAM_APP="$work/app" PIEJAM_SYSFS="$work/sys" \
    RUNTIME_DIRECTORY="$work/run" PIEJAM_DISPLAY_TIMEOUT_TENTHS=3 \
    "$launcher" 2>"$work/stderr" &
pid=$!
for _ in $(seq 50); do [[ -e $work/app.ran ]] && break; sleep 0.05; done
# systemd signals every process in the service's cgroup: launcher and app.
pkill -TERM -P "$pid"
kill -TERM "$pid"
# Watchdog: if the launcher doesn't exit, kill it so the test fails, not hangs.
( sleep 5; pkill -KILL -P "$pid"; kill -KILL "$pid" ) >/dev/null 2>&1 &
watchdog=$!
wait "$pid"
status=$?
kill "$watchdog" 2>/dev/null
wait "$watchdog" 2>/dev/null
if [[ $status -eq 143 && ! -e $work/systemctl.log ]]; then
    pass "SIGTERM (systemctl stop) exits 143 without powering off"
else
    fail "SIGTERM handling (status $status, systemctl: $(cat "$work/systemctl.log" 2>/dev/null))"
fi

echo
if (( failures )); then
    echo "$failures test(s) failed"
    exit 1
fi
echo "all tests passed"

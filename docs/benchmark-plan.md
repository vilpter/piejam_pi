# Test and benchmark plan

How we check that the appliance works and how we measure boot time. The rule
from the project brief applies throughout: **get a stable functional baseline
first, then optimize**, one measured change at a time.

## Milestones

| Milestone | Meaning | How it's measured |
|---|---|---|
| **M0 Power on** | Power applied | External video (see below) |
| **M1 Kernel start** | Firmware hands over to Linux | `t = 0` of `CLOCK_MONOTONIC` |
| **M2 UI visible** | First PieJam frame on the panel | Interim: first Qt scene-graph log line (`QSG_INFO=1`). Planned: `sd_notify` from the app |
| **M3 Audio ready** | Scarlett 2i2's ALSA card exists | Activation of `piejam-audio-ready.service` (started by udev) |

M2 and M3 are **separate** on purpose. They depend on different subsystems
(DRM/KMS vs USB enumeration), can finish in either order, and the UI must come up
even with no audio interface connected.

**Headline number:** power-on to usable UI = firmware time (M0→M1) + M2.

### What systemd can't see

The Pi has no UEFI, so `systemd-analyze` only reports from kernel start onward.
The EEPROM bootloader, PCIe/NVMe bring-up and firmware time before M1 need an
external measurement:

1. Film the Pi's power LED and the panel together at 60 fps or more (a phone's
   slow-motion mode is fine).
2. Count frames from power applied to the first PieJam frame. That's the total.
3. Subtract M2 from `measure-boot.sh` to get firmware time.

Firmware time changes only with EEPROM settings, `config.txt` or hardware, so it
only needs re-measuring after those change.

### Making M2 exact

`QSG_INFO` output comes from the scene-graph setup just before the first frame,
so it slightly under-reports M2. The exact fix belongs in the app fork: after the
window's first `frameSwapped()`, call `sd_notify(0, "READY=1")` and switch
`piejam.service` to `Type=notify` with `NotifyAccess=all` (the notification
comes from the app, not the launcher). Then `systemd-analyze critical-chain`
reports the UI milestone directly. Tracked as a follow-up in the
[decision log](decision-log.md).

## Running a measurement

```sh
sudo ./benchmarks/measure-boot.sh <label>
```

This waits for boot to finish and then records:
- the milestones above, plus when the launcher found the display
- `systemd-analyze`, `critical-chain piejam.service`, and the top of `blame`
- a boot chart (`systemd-analyze plot`, SVG)
- one CSV row in `benchmarks/results/boot-log.csv` for comparing runs

It warns if any network unit sits on `piejam.service`'s critical chain
([ADR 0005](adr/0005-network-not-boot-dependency.md)) or if the app restarted
during boot.

## Method

- **Cold boots only.** Power-cycle the Pi; don't use `reboot`, which skips part of
  firmware initialization.
- **Five boots per configuration.** Report median and worst case.
- **Skip the first boot after installing the app.** Qt compiles and caches the QML
  on first launch, so that boot is slower than every later one.
- **Two network conditions:** no network at all (cable out, no WiFi configured),
  and network present. The UI time should be the same for both.
- **Scarlett connected** unless the test says otherwise.
- **One change at a time.** Every optimization gets its own labeled measurement
  run and its own commit, so any regression can be traced and reverted.

## Phase 1: functional baseline

The baseline is accepted when all of these hold on five cold boots in a row:

- [ ] The PieJam UI appears on the 7″ panel with no login prompt, console text or cursor.
- [ ] Touch input lands where tapped.
- [ ] The Scarlett 2i2 shows up in PieJam, and audio passes input → mixer → output.
- [ ] The UI comes up with the Scarlett unplugged, and PieJam picks the Scarlett up
      when it's plugged in afterwards (assumption A7 in the [decision log](decision-log.md)).
- [ ] The UI comes up with no network, and SSH works when a network is present.
- [ ] `measure-boot.sh` shows no network unit on the critical chain.
- [ ] Shutdown from the PieJam UI powers the Pi off cleanly.
- [ ] `sudo systemctl stop piejam` frees the display and does **not** power off.
- [ ] Killing the app (`sudo pkill piejam_app`) brings it back within a few seconds.

Record the baseline numbers in the results table below. No time target is enforced
in Phase 1.

## Phase 2: optimization candidates

Only after the baseline is accepted. Each is measured on its own; keep it only if
it helps and doesn't cost reliability or maintainability.

| Candidate | Expected effect | Risk |
|---|---|---|
| Drop services `blame` shows as slow and unneeded (candidates: `bluetooth`/`hciuart` if unused, `triggerhappy`, `keyboard-setup`, `e2scrub_reap`) | Shorter userspace | Low; check each is truly unused |
| Move `apt-daily`, `apt-daily-upgrade` and `man-db` timers away from boot | No background I/O during the first minutes, which can cause audio dropouts | Low |
| Skip the initramfs if one is loaded (root on NVMe ext4 doesn't need it) | Shorter kernel phase | Medium; affects boot recovery |
| `dtparam=pciex1_gen=3` | Faster NVMe | Medium; Gen 3 isn't officially certified on the Pi 5 |
| Trim EEPROM boot order to try NVMe only | Shorter firmware phase | Medium; removes SD/USB fallback for recovery |
| Early splash (PieJam OS used psplash) | Better *perceived* speed; doesn't change M2 | Low |
| `performance` CPU governor | Fewer audio dropouts; slightly faster startup | Low; more heat and power |

## Results

| Date | Label | Firmware (M0→M1) | UI visible (M2) | Audio ready (M3) | Total to UI | Notes |
|---|---|---|---|---|---|---|
| — | — | — | — | — | — | No hardware runs yet |

## Automated tests

These run on any Linux machine, no Pi needed:

```sh
tests/test-provisioning-lib.sh   # cmdline.txt / config.txt editing helpers
tests/test-launcher.sh           # display selection, KMS config, power-off rules
```

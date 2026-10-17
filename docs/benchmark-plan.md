# Test and benchmark plan

How we check that the appliance works and how we measure boot time. The rule
from the project brief applies throughout: **get a stable functional baseline
first, then optimize**, one measured change at a time.

## Milestones

| Milestone | Meaning | How it's measured |
|---|---|---|
| **M0 Power on** | Power applied | External video (see below) |
| **M1 Kernel start** | Firmware hands over to Linux | `t = 0` of `CLOCK_MONOTONIC` |
| **M2 UI visible** | First PieJam frame on the panel | `sd_notify(READY=1)` after the first `frameSwapped()`; the `Type=notify` unit goes active then |
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

### M2 is exact (sd_notify)

PieJam calls `sd_notify(0, "READY=1")` after its window's first `frameSwapped()`,
and `piejam.service` is `Type=notify` with `NotifyAccess=all` (the notification
comes from PieJam, which `piejam-launch` runs as a child, not the main process).
The unit therefore goes active exactly at the first visible frame, so
`measure-boot.sh` reads M2 from the service's `ActiveEnterTimestampMonotonic` and
`systemd-analyze critical-chain` reports the UI milestone directly — no log
scraping, and no need to run the script as root. (Done, F5.)

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
      _UI and no getty confirmed 2026-10-15; console text and cursor not yet checked by eye._
- [ ] Touch input lands where tapped.
- [ ] The Scarlett 2i2 shows up in PieJam, and audio passes input → mixer → output.
- [ ] The UI comes up with the Scarlett unplugged, and PieJam picks the Scarlett up
      when it's plugged in afterwards (assumption A7 in the [decision log](decision-log.md)).
- [ ] The UI comes up with no network, and SSH works when a network is present.
- [x] `measure-boot.sh` shows no network unit on the critical chain.
- [ ] Shutdown from the PieJam UI powers the Pi off cleanly.
- [ ] `sudo systemctl stop piejam` frees the display and does **not** power off.
- [x] Killing the app (`sudo pkill piejam_app`) brings it back within a few seconds.

Record the baseline numbers in the results table below. No time target is enforced
in Phase 1.

## Phase 2: optimization candidates

Only after the baseline is accepted. Each is measured on its own; keep it only if
it helps and doesn't cost reliability or maintainability.

| Candidate | Expected effect | Risk |
|---|---|---|
| Drop services `blame` shows as slow and unneeded (candidates: `bluetooth`/`hciuart` if unused, `triggerhappy`, `keyboard-setup`, `e2scrub_reap`) | Shorter userspace | Low; check each is truly unused |
| Move `apt-daily`, `apt-daily-upgrade` and `man-db` timers away from boot | No background I/O during the first minutes, which can cause audio dropouts | Low |
| ~~Disable cloud-init after the first boot (`/etc/cloud/cloud-init.disabled`); it runs before `sysinit.target` on every boot~~ Done (F10, stage 60). The WiFi profile survives (NetworkManager renders the persisted netplan profile; verified it reconnects) | ~0.5 s off the boot; cloud-init no longer on the critical chain | Low |
| ~~Skip or speed up the `fsck` of the FAT boot partition, which `cloud-init` waits for~~ Done (F10, stage 60): fstab pass number set to 0; the root filesystem is still checked | part of the ~0.5–0.7 s saved; no longer on the chain | Medium; a damaged boot partition goes unnoticed |
| Skip the initramfs if one is loaded (root on NVMe ext4 doesn't need it) | Shorter kernel phase | Medium; affects boot recovery |
| `dtparam=pciex1_gen=3` | Faster NVMe | Medium; Gen 3 isn't officially certified on the Pi 5 |
| Trim EEPROM boot order to try NVMe only | Shorter firmware phase | Medium; removes SD/USB fallback for recovery |
| Early splash (PieJam OS used psplash) | Better *perceived* speed; doesn't change M2 | Low |
| `performance` CPU governor | Fewer audio dropouts; slightly faster startup | Low; more heat and power |

## Results

| Date | Label | Firmware (M0→M1) | UI visible (M2) | Audio ready (M3) | Total to UI | Notes |
|---|---|---|---|---|---|---|
| 2026-10-15 | baseline-reboot | not measured | 5.31 s | n/a (no interface) | — | `reboot`, not a cold boot. Interim `QSG_INFO` marker (under-reports M2). Kernel 1.89 s, userspace 5.73 s, `piejam.service` started at 4.58 s |
| 2026-10-17 | f5-sd-notify | not measured | 6.01 s | n/a (no interface) | — | `reboot`. First exact M2 (`sd_notify`); the higher number vs. baseline is the honest first frame, not a regression. `piejam.service` started at 4.50 s, boot finished 7.58 s |
| 2026-10-17 | f10-boot-trims | not measured | 5.49 s | n/a (no interface) | — | `reboot`. cloud-init disabled + boot-partition `fsck` skipped (F10); both off the critical chain. `piejam.service` started at 3.82 s, boot finished 6.90 s |

## Automated tests

These run on any Linux machine, no Pi needed:

```sh
tests/test-provisioning-lib.sh   # cmdline.txt / config.txt editing helpers, incl. failure handling
tests/test-launcher.sh           # display selection and wait, KMS config, power-off rules
tests/lint.sh                    # shellcheck on every shell script
```

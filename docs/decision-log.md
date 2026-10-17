# Decision log

A running record of decisions and the assumptions behind them. Consequential,
hard-to-reverse choices get a full [ADR](adr/); everything else is logged here.

Status key: **Decided** (confirmed by the project owner) · **Default** (reasonable
default chosen to avoid blocking; revisit freely) · **Open** (needs an answer).

## Decisions

| # | Date | Decision | Status | Reference |
|---|---|---|---|---|
| D1 | 2026-09-30 | Provision stock Raspberry Pi OS Lite 64-bit; don't build a custom OS image | Decided | [ADR 0001](adr/0001-pi-os-lite-not-buildroot.md) |
| D2 | 2026-09-30 | Render directly to DRM/KMS with Qt `eglfs`; no compositor | Decided | [ADR 0002](adr/0002-drm-kms-qt-eglfs.md) |
| D3 | 2026-09-30 | Fork and modify `vilpter/piejam` rather than run upstream as-is or write a new UI | Decided | [ADR 0003](adr/0003-fork-reuse-strategy.md) |
| D4 | 2026-09-30 | Move the fork's network backend onto NetworkManager (`nmcli`) and systemd | Decided | [ADR 0004](adr/0004-network-backend-nmcli.md) |
| D5 | 2026-09-30 | When they conflict: maintainability first, then boot speed | Decided | — |
| D6 | 2026-09-30 | Tooling repository is `vilpter/piejam_pi` (created by the owner) | Decided | — |
| D7 | 2026-09-30 | Target hardware: Pi 5, NVMe HAT, legacy Gen 1 7″ DSI touchscreen, Scarlett 2i2. The unit is a Pi 5 **4 GB** (revision `c04170`), confirmed by the owner as the intended board; the app uses about 150 MB | Decided | — |
| D8 | 2026-09-30 | The network (including SSH) is never a boot dependency | Decided | [ADR 0005](adr/0005-network-not-boot-dependency.md) |
| D9 | 2026-09-30 | Tooling repo is MIT-licensed; the app stays GPL-3.0 | Default | — |
| D10 | 2026-09-30 | Target Raspberry Pi OS based on Debian 13 "trixie". PieJam sets `CMAKE_CXX_STANDARD 26`, which needs GCC 14; bookworm has GCC 12 | Default | [ADR 0001](adr/0001-pi-os-lite-not-buildroot.md) |
| D11 | 2026-09-30 | The app runs as a dedicated `piejam` user, not root. Polkit allows power-off and NetworkManager | Default | [architecture](architecture.md#runtime-user) |
| D12 | 2026-09-30 | Recordings and settings stay on the root filesystem under `/home/piejam`. PieJam OS's separate data partition isn't replicated | Default | [architecture](architecture.md#runtime-user) |
| D13 | 2026-09-30 | Pick the display at each boot with a launcher ported from PieJam OS, not a static KMS config | Default | [ADR 0002](adr/0002-drm-kms-qt-eglfs.md) |
| D14 | 2026-09-30 | Quitting from the UI powers the Pi off (PieJam OS behavior); `systemctl stop piejam` doesn't | Default | [architecture](architecture.md#power-and-recovery) |
| D15 | 2026-09-30 | Rely on Pi OS's `display_auto_detect=1` for the Gen 1 panel (as PieJam OS's Pi 5 image did) rather than an explicit overlay | Default | — |
| D16 | 2026-10-15 | `piejam.service` allows real-time priority up to 99 but keeps memory locking limited (8 MB): PieJam's `mlockall(MCL_FUTURE)` crashes Debian's Qt 5 QML engine if it succeeds | Default | [architecture](architecture.md#runtime-user) |
| D17 | 2026-10-15 | The app's source lives in this repository under `app/` (imported from `vilpter/piejam` `e1291b94`, which is archived); app changes go through pull requests here | Decided | [ADR 0003](adr/0003-fork-reuse-strategy.md#amendment-2026-10-15-the-app-moves-into-this-repository) |
| D18 | 2026-10-16 | NFS is **server-only**: the Pi shares its recordings; the client (mounting remote shares) was removed. The share is read-write to the Pi's current subnet, applied with `exportfs` on demand and never written to `/etc/exports`, so it never costs boot time; the UI warns before enabling it | Decided | [ADR 0004](adr/0004-network-backend-nmcli.md) |

## Assumptions

Working assumptions, checked on the hardware as it becomes possible.

| # | Assumption | Result (2026-10-15), or how we'll check |
|---|---|---|
| A1 | The fork builds on trixie with GCC 14 and Qt 5.15 from `apt`, including with `-Werror` | **Holds** (2026-10-15): clean build in 8 min on the Pi 5, no warnings |
| A2 | `display_auto_detect=1` brings up the legacy Gen 1 panel on the Pi 5 | **Holds**: the panel comes up on RP1's DSI controller (`drm-rp1-dsi`) |
| A3 | Qt `eglfs_kms` works on the Pi 5 without a compositor, and names connectors `DSI1`/`HDMI1` in its KMS config | **Holds**: PieJam renders through Mesa (V3D, OpenGL ES 3.1) to the panel. The DSI card was `card1` on one boot and `card0` on the next, so the per-boot lookup (D13) is needed |
| A4 | Touch lines up without a coordinate transform (rotation is an app setting applied in QML) | Looks right: the owner navigated the UI by touch. A formal tap test is still open |
| A5 | The Scarlett 2i2 needs no quirk flags on the trixie kernel | Not tested yet (no interface connected) |
| A6 | 8–15 s is reachable without a custom kernel or initramfs | Promising: UI visible 5.3 s after kernel start (firmware time not yet measured) |
| A7 | PieJam picks up the Scarlett when it's plugged in after the app has started | Not tested yet |
| A8 | ~~The `QSG_INFO` log line lands within a frame or two of the first visible frame~~ No longer relied on: M2 now comes from `sd_notify` at the first `frameSwapped()` (F5) | Superseded |
| A9 | The package names in stage `10-packages` are right for trixie (notably `qtvirtualkeyboard5-dev`, `qtquickcontrols2-5-dev`, `caps`, `tap-plugins`) | **Failed, fixed**: the dev package is `libqt5virtualkeyboard5-dev`; three runtime packages were missing |
| A10 | udev can set the Scarlett's ALSA card id (`ATTR{id}="Scarlett"`) | Not tested yet |
| A11 | The KMS panel driver turns the backlight on by itself (PieJam OS's init script set `bl_power` explicitly) | **Holds**: `bl_power=0`, brightness 255 at boot |

## Open questions

- **O3** — Which generation is the Scarlett 2i2? The udev rule matches any generation,
  so this doesn't block anything. It matters because some generations ship in "MSD
  mode", where they appear as a USB drive until MSD mode is turned off. If the Scarlett
  isn't listed as a sound card, check this first.

- **O4** — Should the recordings file browser come back? An earlier snapshot of the
  fork had a `file_manager` module with a FileBrowser UI (cards, tags, ratings, notes),
  but the current fork doesn't. It could be ported from the archived snapshot, or left
  out.

Resolved:
- ~~O1 — Display orientation.~~ Rotation is PieJam's own `display_rotation` setting
  (PieJam OS read it from `piejam.config` for its splash screen). No KMS rotation needed.
- ~~O2 — Run as root or a dedicated user.~~ Dedicated user (D11).

## Follow-ups

| # | Task | Where |
|---|---|---|
| F1 | ~~Merge `nooploop/master` into the fork~~ Already done: the fork contains upstream's current `master` (`005feb30`) | — |
| F2 | ~~Remove the Buildroot-specific WiFi paths (busybox `udhcpc`, `modprobe brcmfmac`)~~ Done in #2; the NFS init-script paths (`/etc/init.d/S60nfs`) were removed with F4 | `app/` |
| F3 | ~~Review the Buildroot cross-compile fixes~~ Not applicable to the current fork history | — |
| F4 | ~~Rework the network backend onto `nmcli`/systemd with `QProcess` argument lists~~ **Done.** WiFi in #2 (2026-10-15); NFS in #4 (2026-10-16): server-only (the client was removed, D18), shared on demand and never at boot, with the export applied by a validated root helper (`piejam-nfs-export`) and the service started through polkit | `app/` + provisioning, [ADR 0004](adr/0004-network-backend-nmcli.md) |
| F5 | ~~Emit `sd_notify READY=1` after the first frame; switch the unit to `Type=notify`~~ Done 2026-10-17: PieJam notifies after its first `frameSwapped()`, the unit is `Type=notify`/`NotifyAccess=all`, and `measure-boot.sh` reads M2 from the service's active-enter time | `app/` + `systemd/`, [benchmark plan](benchmark-plan.md#m2-is-exact-sd_notify) |
| F6 | ~~Lint the scripts with shellcheck~~ Done: `tests/lint.sh` is clean. Still open: CI running `tests/lint.sh` and the test scripts on every push | This repo |
| F7 | Restore memory locking without the Qt crash: lock with `MCL_CURRENT` once startup is done, or lock only the audio engine's buffers; then raise `LimitMEMLOCK` again (D16) | App |
| F8 | ~~Reduce the Info page's diagnostics to the actionable ones~~ Done in #3: an info threshold on the Info page's sink, and user-facing levels for network messages. The badge after start went from 16 to 0, and from 27 to 5 after a full Network-page run | `app/` |
| F9 | PieJam drives one sound card. Four inputs need one interface with four or more inputs (Scarlett 4i4/8i6/18i8); two 2i2s can't be combined (separate clocks) without a substantial app change. Owner to decide | — |
| F10 | Phase 2 candidates seen on hardware: cloud-init sits before `sysinit.target` (about 0.6 s per boot, 3.5 s on the first), and the boot partition's `fsck` adds about 0.6 s | [Benchmark plan](benchmark-plan.md) |

## Findings: first hardware run

Recorded 2026-10-15 on a Pi 5 4 GB booting Raspberry Pi OS Lite (trixie, image
2026-10-06, kernel 6.18) from NVMe, following the README from a fresh image.

- **Raspberry Pi Imager configures the image through cloud-init.** `cmdline.txt` carries
  `ds=nocloud;i=…`, WiFi arrives as a netplan-generated NetworkManager profile, and the
  user gets ordinary password-protected `sudo`.
- **The EEPROM already tried NVMe first** (`BOOT_ORDER=0xf146`); any order with NVMe
  (`6`) last is fine.
- **Three stage 10 gaps:** a renamed dev package and three runtime packages that a
  `--no-install-recommends` install leaves out (A9). Without
  `qml-module-qt-labs-folderlistmodel` the app's main view fails to load, because the
  virtual keyboard imports it.
- **Two startup crashes**, both from the service's limits: the audio threads want
  priority 96, and a successful `mlockall(MCL_FUTURE)` makes Debian's Qt 5 abort
  (D16, F7).
- **`measure-boot.sh` needed two fixes:** Qt's messages go to PieJam's own log, not the
  journal, and `cloud-init-network.service` tripped the network check.
- **Boot:** kernel 1.9 s, userspace 5.7 s. PieJam starts 4.6 s after kernel start and
  shows its UI at 5.3 s.
- **Recovery behaviour:** killing the app restarts it in 2 s (it exits 1 on SIGTERM),
  and `getty@tty1` stays off.
- **Not on this branch's account:** upstream's `intx_io.float_to_unsigned` test fails on
  aarch64 (also on unmodified master), and upstream code doesn't compile with GCC 15
  (`pitch_yin.cpp`). Neither affects trixie's GCC 14 build.

## Findings: the existing fork

Recorded 2026-09-30 against `vilpter/piejam` at `e1291b94`, the current `master` on
GitHub (the same as the archived `piejam-dev.bak2`). These drove D3 and D4.

- **Up to date with upstream.** The fork contains upstream's current `master`
  (`005feb30`, including `system: add memory locking`), plus a few later commits by the
  upstream author.
- **One feature module, `network_manager`:** WiFi and NFS client/server, with Redux
  state, a `NetworkSettings` GUI model that follows upstream's patterns
  (`SubscribableItem`, `CompositeSubscribableModel`), QML views, and tests. The state,
  GUI model and QML are reused.
- **WiFi drives `wpa_supplicant` directly** over its control socket, starts it if it
  isn't running, loads `brcmfmac` with `modprobe`, and runs busybox `udhcpc` for DHCP.
  On Pi OS, NetworkManager owns `wlan0`, its `wpa_supplicant` and DHCP, and `udhcpc`
  isn't installed. → D4, F2.
- **NFS server control** tries `/etc/init.d/S60nfs` and `/etc/init.d/nfs`, then falls
  back to `systemctl … nfs-kernel-server` (argument list, no shell). The fallback also
  enables the server at boot, which conflicts with ADR 0005. → F4.
- **Shell use:** the remaining `std::system`/`popen` calls run fixed commands, not user
  input. They're all Buildroot-specific and go away with F2.
- **Recording no longer turns WiFi off.** `b4ed39af` removed that middleware.
- **Relevant to our setup:** `ae7de8cd` removed spdlog's stdout sink because console
  output deadlocked with DRM/fbcon under eglfs. Under systemd the app's output goes to
  the journal instead.

An earlier snapshot (`piejam-dev.bak`, February 2026, with a different history) also had
a `file_manager` module with a FileBrowser UI, and passed WiFi credentials to `wpa_cli`
inside a shell string. Neither is in the current fork. → O4.

## Findings: PieJam OS

Recorded 2026-09-30 from `vilpter/piejam_os` (`br2_external`). This is what we reuse
and what we leave behind.

- **Pi 5 boot config:** `display_auto_detect=1`, `dtoverlay=vc4-kms-v3d,cma-128,noaudio`,
  `disable_splash=1`, and `quiet vt.global_cursor_default=0` on the command line. We keep
  `noaudio`, `disable_splash`, `quiet` and the hidden cursor. Pi OS already sets
  `display_auto_detect`. CMA stays at the Pi OS default; raise it to 128 MB if Qt reports
  allocation failures.
- **Launcher (`/usr/bin/piejam`):** detects the DSI panel through its touch controller,
  falls back to HDMI, writes Qt's KMS config, runs the app, then halts. Ported as
  `bin/piejam-launch` with fixes, each covered by `tests/test-launcher.sh`:
  - it used `[[ ]]` under `#!/bin/sh`, which fails on Debian's dash;
  - `grep -q connected` also matched `disconnected`;
  - it used sysfs connector names (`DSI-1`), while Qt's KMS config expects `DSI1`;
  - if no display was found it halted immediately. Ours waits up to 10 s, then fails
    so systemd can retry.
- **Init script (`S00piejam.sh`):** turned on the backlight, showed a `psplash` splash
  rotated to the app's setting, and created and mounted a data partition on first
  boot. Not replicated in the baseline: the backlight is assumed to be handled by KMS
  (A11), a splash is a Phase 2 candidate, and data stays on the root filesystem (D12).
- **LADSPA plugins:** PieJam OS shipped the LADSPA SDK examples plus CAPS and TAP. We
  install the Debian packages (`ladspa-sdk`, `caps`, `tap-plugins`) into
  `/usr/lib/ladspa`, where PieJam looks.
- **The app itself:** the Shutdown button calls `Qt.quit()`. The app sets `QT_IM_MODULE`
  for the virtual keyboard itself. It keeps its data under `$HOME`, and it doesn't link
  alsa-lib, because it talks to the kernel's ALSA interface directly.

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
| D7 | 2026-09-30 | Target hardware: Pi 5 8 GB, NVMe HAT, legacy Gen 1 7″ DSI touchscreen, Scarlett 2i2 | Decided | — |
| D8 | 2026-09-30 | The network (including SSH) is never a boot dependency | Decided | [ADR 0005](adr/0005-network-not-boot-dependency.md) |
| D9 | 2026-09-30 | Tooling repo is MIT-licensed; the app stays GPL-3.0 | Default | — |
| D10 | 2026-09-30 | Target Raspberry Pi OS based on Debian 13 "trixie". PieJam sets `CMAKE_CXX_STANDARD 26`, which needs GCC 14; bookworm has GCC 12 | Default | [ADR 0001](adr/0001-pi-os-lite-not-buildroot.md) |
| D11 | 2026-09-30 | The app runs as a dedicated `piejam` user, not root. Polkit allows power-off and NetworkManager | Default | [architecture](architecture.md#runtime-user) |
| D12 | 2026-09-30 | Recordings and settings stay on the root filesystem under `/home/piejam`. PieJam OS's separate data partition isn't replicated | Default | [architecture](architecture.md#runtime-user) |
| D13 | 2026-09-30 | Pick the display at each boot with a launcher ported from PieJam OS, not a static KMS config | Default | [ADR 0002](adr/0002-drm-kms-qt-eglfs.md) |
| D14 | 2026-09-30 | Quitting from the UI powers the Pi off (PieJam OS behavior); `systemctl stop piejam` doesn't | Default | [architecture](architecture.md#power-and-recovery) |
| D15 | 2026-09-30 | Rely on Pi OS's `display_auto_detect=1` for the Gen 1 panel (as PieJam OS's Pi 5 image did) rather than an explicit overlay | Default | — |

## Assumptions

Working assumptions, not facts. Each needs checking on the hardware.

| # | Assumption | How we'll check |
|---|---|---|
| A1 | The fork builds on trixie with GCC 14 and Qt 5.15 from `apt`, including with `-Werror` | Run stage `50-app`; `PIEJAM_CMAKE_ARGS` offers an escape hatch |
| A2 | `display_auto_detect=1` brings up the legacy Gen 1 panel on the Pi 5 | Boot and see output on the panel |
| A3 | Qt `eglfs_kms` works on the Pi 5 without a compositor, and names connectors `DSI1`/`HDMI1` in its KMS config | Launch the app; if needed, list Qt's names with `QT_LOGGING_RULES=qt.qpa.*=true` |
| A4 | Touch lines up without a coordinate transform (rotation is an app setting applied in QML) | Tap test |
| A5 | The Scarlett 2i2 needs no quirk flags on the trixie kernel | `cat /proc/asound/cards` and a playback test |
| A6 | 8–15 s is reachable without a custom kernel or initramfs | [Benchmark plan](benchmark-plan.md) |
| A7 | PieJam picks up the Scarlett when it's plugged in after the app has started | Phase 1 checklist. If not, add a bounded wait in the launcher |
| A8 | The `QSG_INFO` log line lands within a frame or two of the first visible frame | Compare with the external video |
| A9 | The package names in stage `10-packages` are right for trixie (notably `qtvirtualkeyboard5-dev`, `qtquickcontrols2-5-dev`, `caps`, `tap-plugins`) | First provisioning run |
| A10 | udev can set the Scarlett's ALSA card id (`ATTR{id}="Scarlett"`) | `cat /proc/asound/cards` |
| A11 | The KMS panel driver turns the backlight on by itself (PieJam OS's init script set `bl_power` explicitly) | Boot and look at the panel |

## Open questions

- **O3** — Which generation is the Scarlett 2i2? The udev rule matches any generation,
  so this doesn't block anything. It matters because some generations ship in "MSD
  mode", where they appear as a USB drive until MSD mode is turned off. If the Scarlett
  isn't listed as a sound card, check this first.

Resolved:
- ~~O1 — Display orientation.~~ Rotation is PieJam's own `display_rotation` setting
  (PieJam OS read it from `piejam.config` for its splash screen). No KMS rotation needed.
- ~~O2 — Run as root or a dedicated user.~~ Dedicated user (D11).

## Follow-ups

| # | Task | Where |
|---|---|---|
| F1 | Merge `nooploop/master` into the fork (10+ commits behind, including memory locking) | `vilpter/piejam`, [ADR 0003](adr/0003-fork-reuse-strategy.md) |
| F2 | Revert the Buildroot workarounds from `ac8f4c79`; keep its QML binding-loop fixes | `vilpter/piejam` |
| F3 | Review `19394146` (Qt 5 / Buildroot cross-compile fixes) for what still applies to native builds | `vilpter/piejam` |
| F4 | Rework the network backend onto `nmcli`/systemd with `QProcess` argument lists. Add NFS packages and narrow sudo rules at the same time | `vilpter/piejam` + this repo, [ADR 0004](adr/0004-network-backend-nmcli.md) |
| F5 | Emit `sd_notify READY=1` after the first frame; switch the unit to `Type=notify` | `vilpter/piejam` + this repo, [benchmark plan](benchmark-plan.md#making-m2-exact) |
| F6 | Lint the scripts with shellcheck (it wasn't available when they were written); consider CI running shellcheck and `tests/` | This repo |

## Findings: the existing fork

Recorded 2026-09-30 from `piejam-dev.bak` (fork HEAD `9fccfad0`, branched from upstream
at `490e4c8f`). These drove D3 and D4.

- The fork adds two well-structured modules, `network_manager` and `file_manager`, each
  with tests. The `file_manager` module and all QML and GUI models are reused unchanged.
- Commit `ac8f4c79` adapted the network code **for Buildroot**. It removed `systemctl`
  and `sudo` and hard-coded `wpa_supplicant` and busybox `/etc/init.d/S60nfs`. None of
  that is idiomatic on Pi OS, and the busybox init script doesn't exist there. → D4, F2.
- WiFi credentials are passed through a shell string
  (`wpa_cli ... ssid '"<ssid>"'`, `psk '"<password>"'`). An SSID or passphrase containing
  `'`, `` ` ``, `$` or `\` breaks the quoting. D4 fixes this by passing arguments as a
  list (`QProcess::start(program, args)`) with no shell.
- The fork is 10+ commits behind upstream, including `system: add memory locking`,
  which matters for audio-thread reliability. → F1.

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

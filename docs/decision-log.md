# Decision log

A running record of decisions and the assumptions behind them. Consequential,
hard-to-reverse choices get a full [ADR](adr/); everything else is logged here.

Status key: **Decided** (confirmed by the project owner) · **Default** (reasonable
default chosen to avoid blocking; revisit freely) · **Open** (needs an answer).

## Decisions

| # | Date | Decision | Status | Reference |
|---|---|---|---|---|
| D1 | 2026-09-30 | Provision stock Raspberry Pi OS Lite 64-bit; do not build a custom OS image | Decided | [ADR 0001](adr/0001-pi-os-lite-not-buildroot.md) |
| D2 | 2026-09-30 | Render directly to DRM/KMS via Qt `eglfs`; no compositor | Decided | [ADR 0002](adr/0002-drm-kms-qt-eglfs.md) |
| D3 | 2026-09-30 | Fork and modify `vilpter/piejam` rather than run upstream as-is or write a new UI | Decided | [ADR 0003](adr/0003-fork-reuse-strategy.md) |
| D4 | 2026-09-30 | Rework the fork's network backend onto NetworkManager (`nmcli`) + systemd | Decided | [ADR 0004](adr/0004-network-backend-nmcli.md) |
| D5 | 2026-09-30 | When they conflict: maintainability first, then boot speed | Decided | — |
| D6 | 2026-09-30 | Tooling repository is `vilpter/piejam-pi` | Decided | — |
| D7 | 2026-09-30 | Target hardware: Pi 5 8 GB, NVMe HAT, legacy Gen 1 7″ DSI touchscreen, Scarlett 2i2 | Decided | — |
| D8 | 2026-09-30 | Network (incl. SSH) is never a boot dependency | Decided | [ADR 0005](adr/0005-network-not-boot-dependency.md) |
| D9 | 2026-09-30 | Tooling repo licensed MIT; the app stays GPL-3.0 | Default | — |

## Assumptions

These are working assumptions, not facts. Each needs hardware validation.

| # | Assumption | Status | How we'll check |
|---|---|---|---|
| A1 | The fork builds against Qt 5 from Bookworm's `apt` repos | Default | Build on the Pi ([ADR 0003](adr/0003-fork-reuse-strategy.md)) |
| A2 | The legacy Gen 1 display works on Pi 5 with `dtoverlay=vc4-kms-dsi-7inch` | Default | Boot and confirm panel output |
| A3 | Qt `eglfs_kms` works on the Pi 5 V3D Mesa driver without a compositor | Default | Launch the app on the panel |
| A4 | FT5406 touch works through libinput with no coordinate transform at 0° | Default | Tap test; revisit if rotated |
| A5 | The Scarlett 2i2 needs no quirk flags on the Bookworm kernel | Default | Check `aplay -l` and a playback test |
| A6 | The 8–15 s target is reachable without a custom kernel or initramfs | Default | Measure with `systemd-analyze` ([benchmark plan](benchmark-plan.md)) |

## Open questions

- **O1** — Display orientation. Assumed landscape at 0°. If the panel is mounted
  rotated, rotation must be set consistently for both KMS output and touch input.
- **O2** — Whether the app runs as `root` or as a dedicated `piejam` user. Default is
  a dedicated user with real-time audio limits; see [architecture](architecture.md#runtime-user).

## Findings from reviewing the existing fork

Recorded 2026-09-30 from `piejam-dev.bak` (fork HEAD `9fccfad0`, branched from upstream
at `490e4c8f`). These drove D3 and D4.

- The fork adds two well-structured modules (`network_manager`, `file_manager`) with
  tests. The `file_manager` module and all QML/GUI models are reused unchanged.
- Commit `ac8f4c79` adapted the network code **for Buildroot**: it removed `systemctl`
  and `sudo` and hard-coded `wpa_supplicant` plus busybox `/etc/init.d/S60nfs`. None of
  that is idiomatic on Pi OS, and the busybox init script path doesn't exist there. → D4.
- WiFi credentials are passed through a shell string
  (`wpa_cli ... ssid '"<ssid>"'`, `psk '"<password>"'`). An SSID or passphrase containing
  `'`, `` ` ``, `$` or `\` breaks the quoting. D4 fixes this by passing arguments as an
  array (`QProcess::start(program, args)`) with no shell.
- The fork is 10+ commits behind upstream, including `system: add memory locking`,
  which matters for audio-thread reliability. We merge upstream before building on the fork.

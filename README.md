# piejam_pi

Turn a **Raspberry Pi 5** into a single-purpose, touchscreen **audio-mixer appliance**
running [PieJam](https://github.com/vilpter/piejam) — booting from NVMe straight into a
full-screen mixer, with no desktop, login screen, panel, or window decorations.

This is the spiritual successor to [PieJam OS](https://github.com/nooploop/piejam_os),
but it deliberately does **not** build a custom OS image. It provisions a stock
**Raspberry Pi OS Lite 64-bit** install (Debian 13 "trixie") instead, so the system
keeps normal `apt` package management and stays maintainable.
See [ADR 0001](docs/adr/0001-pi-os-lite-not-buildroot.md).

> **Status:** scaffolding, not yet run on hardware. Everything that depends on the
> real Pi is tracked as an assumption in the [decision log](docs/decision-log.md).

## Target hardware

| Component | Model |
|---|---|
| Board | Raspberry Pi 5, 8 GB |
| Storage | NVMe SSD on an M.2 HAT (boot device) |
| Display | Official Raspberry Pi 7″ touchscreen, **legacy Gen 1** (800×480 DSI, FT5406 touch) |
| Audio | Focusrite Scarlett 2i2 (USB Audio Class) |

## How it works

```
power on → firmware → kernel → systemd ─┬─► piejam.service ─► piejam-launch ─► PieJam (Qt eglfs) ─► DRM/KMS ─► 7″ panel
                                         │                                        [milestone: UI visible]
                                         └─► USB ─► Scarlett 2i2 ─► udev ─► piejam-audio-ready.service
                                                                            [milestone: audio ready]
```

- PieJam starts from a **systemd service** and renders straight to **DRM/KMS** with
  Qt's `eglfs` backend. There's no X11 and no Wayland compositor ([ADR 0002](docs/adr/0002-drm-kms-qt-eglfs.md)).
- `piejam-launch` finds the display and writes Qt's KMS config at each boot, ported
  from PieJam OS's launcher. Quitting from the UI powers the Pi off, as on PieJam OS.
- **SSH** is available for maintenance, but the network is **never a boot dependency**
  ([ADR 0005](docs/adr/0005-network-not-boot-dependency.md)).
- "UI visible" and "audio ready" are measured as **separate** boot milestones.

Full design: [docs/architecture.md](docs/architecture.md).

## Goal

About **8–15 s** from power-on to a usable UI, _subject to hardware validation_.
The functional baseline comes first; optimization comes after, one measured change at
a time. When they conflict, maintainability wins over raw boot speed.

## Getting started

1. Flash **Raspberry Pi OS Lite (64-bit)** to the NVMe drive, set the bootloader to
   boot from NVMe, and enable SSH. Details: [provisioning/README.md](provisioning/README.md#prerequisites).
2. Boot the Pi, SSH in, and clone this repository.
3. Provision it:

   ```sh
   sudo ./provisioning/provision.sh
   ```

4. Reboot. The mixer should appear on the touchscreen.
5. Measure the boot: `sudo ./benchmarks/measure-boot.sh baseline`
   (see the [benchmark plan](docs/benchmark-plan.md)).

## Repository layout

```
bin/piejam-launch      Display detection and app launcher (installed to /usr/local/lib/piejam)
systemd/               piejam.service and the audio-ready milestone unit
config/                Boot, udev, polkit and limits files the provisioning installs
provisioning/          Idempotent setup stages for a fresh Pi OS Lite install
benchmarks/            Boot measurement script (results go to benchmarks/results/, not committed)
tests/                 Tests for the launcher and provisioning helpers; no Pi needed
docs/                  Architecture, decision log, benchmark plan, ADRs
```

## Tests

```sh
tests/test-provisioning-lib.sh
tests/test-launcher.sh
```

## Related repositories

- [`vilpter/piejam`](https://github.com/vilpter/piejam) — the mixer application (fork of `nooploop/piejam`).
- [`nooploop/piejam`](https://github.com/nooploop/piejam) — upstream application.
- [`nooploop/piejam_os`](https://github.com/nooploop/piejam_os) — upstream Buildroot image this project replaces.

## License

The tooling in this repository is MIT-licensed (see [LICENSE](LICENSE)). The PieJam
application it installs is licensed separately under GPL-3.0 by its authors.

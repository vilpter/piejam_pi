# piejam-pi

Turn a **Raspberry Pi 5** into a single-purpose, touchscreen **audio-mixer appliance**
running [PieJam](https://github.com/vilpter/piejam) — booting from NVMe straight into a
full-screen mixer, with no desktop, login screen, panel, or window decorations.

This is the spiritual successor to [PieJam OS](https://github.com/nooploop/piejam_os),
but it deliberately does **not** build a custom OS image. It provisions a stock
**Raspberry Pi OS Lite 64-bit** install instead, so the system keeps normal `apt`
package management and stays maintainable. See [ADR 0001](docs/adr/0001-pi-os-lite-not-buildroot.md).

> **Status:** early scaffolding. Nothing here has been validated on hardware yet.
> Items marked _unvalidated_ are reasoned defaults awaiting a real boot.

## Target hardware

| Component | Model |
|---|---|
| Board | Raspberry Pi 5, 8 GB |
| Storage | NVMe SSD on an M.2 HAT (boot device) |
| Display | Official Raspberry Pi 7″ touchscreen, **legacy Gen 1** (800×480 DSI, FT5406 touch) |
| Audio | Focusrite Scarlett 2i2 (USB Audio Class, `snd-usb-audio`) |

## How it works

```
power on → firmware → kernel → systemd ─┬─► piejam.service ─► Qt eglfs ─► DRM/KMS ─► 7″ panel
                                         │     (UI visible milestone)
                                         └─► Scarlett 2i2 enumerated
                                               (audio ready milestone)
```

- The PieJam app launches from a dedicated **systemd service** directly onto **DRM/KMS**
  using Qt's `eglfs` backend — no X11, no Wayland compositor. See [ADR 0002](docs/adr/0002-drm-kms-qt-eglfs.md).
- **SSH** is available for maintenance and recovery, but the network is **never a boot
  dependency** — the mixer comes up whether or not a network is present.
- "UI visible" and "audio interface ready" are tracked as **separate** boot milestones.

Full detail: [docs/architecture.md](docs/architecture.md).

## Goal

Approximately **8–15 s** from power-on to a usable UI, _subject to hardware validation_.
We establish a stable functional baseline first and optimize only after that
(maintainability wins over raw boot speed when they conflict).

## Repository layout

```
docs/
  architecture.md       System design and boot critical path
  decision-log.md        Running log of decisions and open assumptions
  benchmark-plan.md      How boot time is measured and what "done" means
  adr/                   Architecture Decision Records
provisioning/            Idempotent setup scripts for a fresh Pi OS Lite install
systemd/                 Unit files for the appliance
config/                  Boot configuration fragments (config.txt, cmdline.txt)
benchmarks/              Boot-time measurement scripts
```

## Quick start

> _Unvalidated — the provisioning flow has not been run on hardware yet._

1. Flash **Raspberry Pi OS Lite (64-bit)** to the NVMe drive and enable SSH.
2. Boot the Pi, SSH in, and clone this repo.
3. Run the provisioner as root:

   ```sh
   sudo ./provisioning/provision.sh
   ```

4. Reboot. The mixer should appear on the touchscreen.

See [provisioning/](provisioning/) for what each stage does.

## Related repositories

- [`vilpter/piejam`](https://github.com/vilpter/piejam) — the mixer application (fork of `nooploop/piejam`).
- [`nooploop/piejam`](https://github.com/nooploop/piejam) — upstream application.
- [`nooploop/piejam_os`](https://github.com/nooploop/piejam_os) — upstream Buildroot image this project replaces.

## License

The provisioning tooling in this repository is MIT-licensed (see [LICENSE](LICENSE)).
The PieJam application it installs is separately licensed under GPL-3.0 by its authors.

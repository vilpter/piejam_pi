# Architecture

## Goals and constraints

- Boot from NVMe into a responsive, full-screen mixer UI as fast as practical
  (target ~8–15 s, _unvalidated_).
- Feel like an appliance: no login screen, desktop, panel, or window decorations.
- Stay on stock Raspberry Pi OS Lite so the system keeps normal `apt` maintenance.
- Keep a working maintenance path (SSH) without making the network a boot dependency.
- Get a stable functional baseline first; optimize after that.

## What we reuse from PieJam / PieJam OS

| Piece | Source | Treatment |
|---|---|---|
| Mixer app (audio engine + Qt/QML UI) | `vilpter/piejam` (fork of `nooploop/piejam`) | **Reuse** — built from source on the Pi |
| `file_manager` module + FileBrowser UI | fork | **Reuse** unchanged |
| `network_manager` QML + GUI models | fork | **Reuse** |
| `network_manager` command backend | fork | **Rework** onto `nmcli`/systemd ([ADR 0004](adr/0004-network-backend-nmcli.md)) |
| Buildroot image, kernel config, busybox init | `piejam_os` | **Replace** with Pi OS Lite provisioning ([ADR 0001](adr/0001-pi-os-lite-not-buildroot.md)) |
| Appliance idea: single-app autostart, no desktop | `piejam_os` | **Replicate** with a systemd service |

PieJam OS gets its fast boot by owning the whole image. We replicate the *behavior*
(one app, straight to the screen) with standard Pi OS parts instead of a custom build.

## Graphics stack

```
PieJam (Qt 5 / QML)
   └─ QPA platform: eglfs  (QT_QPA_PLATFORM=eglfs)
        └─ integration: eglfs_kms  (QT_QPA_EGLFS_INTEGRATION=eglfs_kms)
             └─ GBM + EGL (Mesa, V3D driver)
                  └─ DRM/KMS (vc4-kms-v3d)
                       └─ DSI → legacy Gen 1 7″ panel (800×480)
```

The app owns the display directly through DRM/KMS; there's no X server or Wayland
compositor ([ADR 0002](adr/0002-drm-kms-qt-eglfs.md)). Touch input comes from the
FT5406 controller as an evdev device, read by Qt through libinput.

Because nothing else holds DRM master, only one graphical program can run at a time.
That's intended for an appliance. For debugging, stop `piejam.service` before starting
anything else that needs the display.

## Audio stack

The Scarlett 2i2 is a USB Audio Class device handled by the in-kernel `snd-usb-audio`
driver. PieJam talks to ALSA directly. No PulseAudio, PipeWire, or JACK server is
needed, and none should run, because they would compete for the device.

A udev rule gives the interface a stable ALSA card name (`Scarlett`) so the config
doesn't depend on USB enumeration order.

## Boot sequence and milestones

Boot is measured as two **separate** milestones:

1. **UI visible** — the first frame of PieJam is on the panel.
2. **Audio ready** — the Scarlett 2i2 is enumerated and its ALSA card is usable.

They're separate because they depend on different things (DRM/KMS vs USB
enumeration) and can finish in either order. The UI must come up even if no audio
interface is connected, so a missing Scarlett is never a boot blocker.

```
firmware (EEPROM bootloader, NVMe)
  └─ kernel + DT (vc4-kms-v3d, vc4-kms-dsi-7inch)
       └─ systemd
            ├─ local-fs.target ─► piejam.service ─► [milestone: UI visible]
            │
            └─ USB enumeration ─► udev ─► sound card ─► [milestone: audio ready]

   (no network units and no systemd-udev-settle on either path)
```

### Critical path

The critical path to **UI visible** is:

firmware → kernel → `local-fs.target` → `piejam.service` (Qt startup + first frame).

Things we keep **off** the critical path:

- **Network.** `piejam.service` doesn't order after `network-online.target`, and
  `NetworkManager-wait-online.service` is disabled ([ADR 0005](adr/0005-network-not-boot-dependency.md)).
- **Audio device.** The service doesn't wait for the Scarlett; PieJam picks it up
  when it appears.
- **`systemd-udev-settle.service`.** This is a known slow unit and nothing should pull it in.

Measurement method: [benchmark-plan.md](benchmark-plan.md).

## Runtime user

The app runs as a dedicated `piejam` user (not root) with membership in `audio`,
`video`, `render`, and `input`, and with real-time scheduling and memlock limits
granted through `/etc/security/limits.d/`. Upstream added memory locking
(`system: add memory locking`), so the memlock limit is needed for the app to lock its
memory and avoid page faults on the audio thread.

The network backend rework ([ADR 0004](adr/0004-network-backend-nmcli.md)) needs
narrowly scoped privileges (NetworkManager via polkit, NFS mount/export via a
specific sudoers entry) rather than running the whole UI as root.

## Recovery and maintenance

- **SSH** is enabled and starts in the background. It's never a boot dependency.
- **Stop the appliance**: `sudo systemctl stop piejam` frees the display for debugging.
- **Updates**: normal `sudo apt update && sudo apt full-upgrade`, plus rebuilding the
  app from the fork.
- **Fallback console**: if the app fails, systemd restarts it with a back-off; after
  repeated failures it stops, and a login getty on `tty1` becomes reachable over a
  keyboard. _Unvalidated._

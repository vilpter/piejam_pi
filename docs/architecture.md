# Architecture

## Goals and constraints

- Boot from NVMe into a responsive, full-screen mixer UI as fast as practical
  (target ~8–15 s, _unvalidated_).
- Feel like an appliance: no login screen, desktop, panel, or window decorations.
- Stay on stock Raspberry Pi OS Lite (Debian 13 "trixie") so the system keeps normal
  `apt` maintenance.
- Keep a working maintenance path (SSH) without making the network a boot dependency.
- Get a stable functional baseline first; optimize after that.

## What we reuse from PieJam and PieJam OS

| Piece | Source | Treatment |
|---|---|---|
| Mixer app (audio engine + Qt/QML UI) | `vilpter/piejam` (fork of `nooploop/piejam`, up to date with upstream) | **Reuse**, built from source on the Pi |
| `network_manager` Redux state, GUI model, QML | fork | **Reuse** |
| `network_manager` backend | fork | **Rework** onto `nmcli`/systemd ([ADR 0004](adr/0004-network-backend-nmcli.md)) |
| `file_manager` + FileBrowser UI | earlier fork snapshot only | **Open** (O4 in the [decision log](decision-log.md)) |
| Display-detecting launcher | PieJam OS `usr/bin/piejam` (Pi 5) | **Port** with fixes → `bin/piejam-launch` |
| Boot config (`noaudio`, no splash, quiet, no cursor) | PieJam OS `config.txt` / `cmdline.txt` | **Replicate** as `config/boot/piejam.txt` and cmdline edits |
| LADSPA plugin sets (SDK examples, CAPS, TAP) | PieJam OS packages | **Replicate** with Debian packages |
| "Quit = power off" | PieJam OS launcher | **Replicate** in `piejam-launch` |
| Buildroot image, kernel config, busybox init | PieJam OS | **Replace** with Pi OS Lite ([ADR 0001](adr/0001-pi-os-lite-not-buildroot.md)) |
| psplash boot splash | PieJam OS `S00piejam.sh` | **Deferred** to Phase 2 ([benchmark plan](benchmark-plan.md)) |
| Separate data partition | PieJam OS `S00piejam.sh` | **Not replicated** (decision D12) |

PieJam OS gets its fast boot by owning the whole image. We replicate its *behavior*
(one app, straight to the screen) with standard Pi OS parts instead of a custom build.

## Graphics stack

```
piejam-launch  (finds the display, writes /run/piejam/eglfs-kms.json)
 └─ PieJam (Qt 5 / QML)
     └─ QPA platform: eglfs, integration: eglfs_kms
         └─ GBM + EGL (Mesa, V3D)
             └─ DRM/KMS (vc4-kms-v3d)
                 └─ DSI → legacy Gen 1 7″ panel (800×480)
```

The app owns the display directly through DRM/KMS. There's no X server and no Wayland
compositor ([ADR 0002](adr/0002-drm-kms-qt-eglfs.md)). Touch input comes from the
FT5406 controller as an evdev device, which Qt reads through libinput.

The DRM card number isn't stable on the Pi 5, which has separate display and render
devices. So the launcher looks the panel up at each boot, the way PieJam OS did: a DSI
connector counts only if the panel's touch controller is present, otherwise the
first connected HDMI port is used. It waits up to 10 s for the display to probe.

Only one program can hold the display at a time. That's intended for an appliance. To
run anything else on the display, `sudo systemctl stop piejam` first.

## Audio stack

The Scarlett 2i2 is a USB Audio Class device handled by the in-kernel `snd-usb-audio`
driver. PieJam talks to the kernel's ALSA interface directly (it doesn't link
alsa-lib), so there's no `asound.conf` to configure. No PulseAudio, PipeWire or JACK
should run, because they would compete for the device.

A udev rule gives the interface the stable ALSA card id `Scarlett`, whatever the USB
enumeration order. HDMI audio is disabled (`noaudio`), so the Scarlett is the only
card PieJam lists.

## Boot sequence and milestones

Boot is measured as two **separate** milestones:

1. **UI visible** — the first PieJam frame is on the panel.
2. **Audio ready** — the Scarlett 2i2's ALSA card exists (`piejam-audio-ready.service`
   is started by udev at that moment).

They're separate because they depend on different things (DRM/KMS vs USB enumeration)
and can finish in either order. The UI must come up even with no audio interface
connected, so a missing Scarlett never blocks boot.

```
firmware (EEPROM bootloader, NVMe)
  └─ kernel + device tree (vc4-kms-v3d, auto-detected DSI panel)
       └─ systemd
            ├─ basic.target ─► piejam.service ─► piejam-launch ─► PieJam ─► [UI visible]
            │
            └─ USB enumeration ─► udev ─► sound card ─► piejam-audio-ready ─► [audio ready]

   (no network units and no systemd-udev-settle on either path)
```

### Critical path

The critical path to **UI visible** is:

firmware → kernel → `basic.target` → `piejam.service` → display probe → Qt startup → first frame.

Kept **off** the critical path:

- **Network.** `piejam.service` doesn't order after `network-online.target`, and
  `NetworkManager-wait-online.service` is disabled ([ADR 0005](adr/0005-network-not-boot-dependency.md)).
- **Audio device.** The service doesn't wait for the Scarlett (assumption A7: PieJam
  picks it up when it appears).
- **`systemd-udev-settle.service`.** A known slow unit that nothing should pull in.

Measurement method: [benchmark-plan.md](benchmark-plan.md).

## Runtime user

The app runs as a dedicated `piejam` system user (not root), in the groups `audio`,
`video`, `render` and `input`. `piejam.service` grants real-time scheduling
(`LimitRTPRIO=95`) and unlimited memory locking (`LimitMEMLOCK=infinity`), which the
app needs to lock its memory (upstream's `system: add memory locking`) and avoid page
faults on the audio thread. The same limits are set in `/etc/security/limits.d/` for
running the app by hand in a login session. That file doesn't apply to systemd services.

The app keeps its data under `/home/piejam`: settings in `~/.config/`, sessions in
`~/sessions/`, and recordings in `~/recordings/`. It's all on the NVMe root
filesystem (decision D12).

Privileges beyond that are narrow:
- a polkit rule lets `piejam` power off and reboot, and manage NetworkManager
  (for the network backend rework, [ADR 0004](adr/0004-network-backend-nmcli.md));
- NFS mount and export will get specific `sudo` rules as part of that rework.

## Power and recovery

| Event | What happens |
|---|---|
| **Shutdown** pressed in PieJam | The app quits with status 0; `piejam-launch` runs `systemctl poweroff` |
| `sudo systemctl stop piejam` | The app stops; the display is freed; the Pi stays on |
| The app crashes | systemd restarts it after 2 s |
| Five crashes within 2 minutes | systemd gives up and leaves the service failed |

Recovery paths:

- **SSH** is enabled and starts in parallel. It's never a boot dependency.
- **Local console:** `getty@tty1` is disabled, so nothing is drawn before the mixer.
  If the app is down, Alt+F2 on a USB keyboard gives a login prompt (logind starts
  consoles on demand). _Unvalidated._
- **Updates:** `sudo apt update && sudo apt full-upgrade` for the OS; re-run
  `provisioning/provision.sh 50-app` to rebuild the app.

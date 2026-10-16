# Provisioning

Scripts that turn a fresh **Raspberry Pi OS Lite 64-bit** (Debian 13 "trixie")
install into the PieJam appliance. Run as root from a clone of this repository:

```sh
sudo ./provisioning/provision.sh            # all stages, in order
sudo ./provisioning/provision.sh 50-app     # one stage
```

Every stage is idempotent, so it's safe to re-run after pulling changes. A first run
takes about 15 minutes on a Pi 5, most of it the app build.
Files the scripts modify in `/boot/firmware` are backed up once as `*.piejam-orig`.
`cmdline.txt` is only edited if it contains a `root=` parameter, and it's replaced
through a temporary file, so a failed or interrupted run can't leave it unbootable.
Any failed edit stops provisioning with an error.

## Prerequisites

1. Flash Raspberry Pi OS Lite (64-bit) to the NVMe drive with Raspberry Pi Imager,
   and enable SSH and a user account in its settings. Imager applies these through
   cloud-init on first boot, and the account's `sudo` asks for its password.
2. Make sure the Pi 5 bootloader tries NVMe first: check `sudo rpi-eeprom-config`.
   `BOOT_ORDER` is read from the right, and `6` is NVMe, so `0xf416` (NVMe, SD, USB)
   and `0xf146` (NVMe, USB, SD) both work. Change it with `sudo rpi-eeprom-config --edit`.
   Non-HAT+ NVMe adapters may also need `PCIE_PROBE=1`.
3. Connect the touchscreen and the Scarlett 2i2.

## Stages

| Stage | What it does |
|---|---|
| `10-packages` | Installs the QML runtime modules, LADSPA plugins (CAPS, TAP), polkit, and the build toolchain. |
| `20-boot-config` | Installs `/boot/firmware/piejam.txt` and includes it from `config.txt`; adds `noaudio` to the KMS overlay; quiets the kernel command line. |
| `30-user` | Creates the `piejam` user and installs its polkit rule and real-time limits. |
| `40-audio` | Installs the Scarlett udev rule and the "audio ready" milestone unit. |
| `50-app` | Builds and installs PieJam from `app/`; fetches the app's submodules if the clone lacks them. |
| `60-appliance` | Installs the launcher and `piejam.service`; disables `getty@tty1` and `NetworkManager-wait-online`; enables SSH. |

## Building PieJam (`50-app`)

Stage 50 builds whatever `app/` holds in this checkout, so pulling and re-running it
updates the app. The build directory is kept and builds are incremental.

| Variable | Default | Purpose |
|---|---|---|
| `PIEJAM_BUILD_DIR` | `/usr/local/src/piejam-build` | Build directory |
| `PIEJAM_CMAKE_ARGS` | _(empty)_ | Extra CMake arguments |
| `PIEJAM_FORCE_BUILD` | `0` | `1` starts from an empty build directory |

PieJam builds with `-Werror`. If a newer compiler adds warnings upstream hasn't
seen yet, get unblocked with `PIEJAM_CMAKE_ARGS="-DCMAKE_CXX_FLAGS=-Wno-error"`, then
fix the warnings in the fork.

## Day-to-day

```sh
sudo systemctl stop piejam        # free the display for debugging (doesn't power off)
sudo systemctl start piejam
journalctl -u piejam -b           # app and launcher logs for this boot
```

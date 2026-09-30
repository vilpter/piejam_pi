# Provisioning

Scripts that turn a fresh **Raspberry Pi OS Lite 64-bit** (Debian 13 "trixie")
install into the PieJam appliance. Run as root from a clone of this repository:

```sh
sudo ./provisioning/provision.sh            # all stages, in order
sudo ./provisioning/provision.sh 50-app     # one stage
```

Every stage is idempotent, so it's safe to re-run after pulling changes.
Files the scripts modify in `/boot/firmware` are backed up once as `*.piejam-orig`.
`cmdline.txt` is only edited if it contains a `root=` parameter, and it's replaced
through a temporary file, so a failed or interrupted run can't leave it unbootable.
Any failed edit stops provisioning with an error.

## Prerequisites

1. Flash Raspberry Pi OS Lite (64-bit) to the NVMe drive with Raspberry Pi Imager,
   and enable SSH and a user account in its settings.
2. Make sure the Pi 5 bootloader tries NVMe first. With `sudo rpi-eeprom-config --edit`,
   set `BOOT_ORDER=0xf416` (NVMe, then SD, then USB). Non-HAT+ NVMe adapters may also
   need `PCIE_PROBE=1`.
3. Connect the touchscreen and the Scarlett 2i2.

## Stages

| Stage | What it does |
|---|---|
| `10-packages` | Installs the QML runtime modules, LADSPA plugins (CAPS, TAP), polkit, and the build toolchain. |
| `20-boot-config` | Installs `/boot/firmware/piejam.txt` and includes it from `config.txt`; adds `noaudio` to the KMS overlay; quiets the kernel command line. |
| `30-user` | Creates the `piejam` user and installs its polkit rule and real-time limits. |
| `40-audio` | Installs the Scarlett udev rule and the "audio ready" milestone unit. |
| `50-app` | Clones, builds and installs PieJam from `vilpter/piejam`. |
| `60-appliance` | Installs the launcher and `piejam.service`; disables `getty@tty1` and `NetworkManager-wait-online`; enables SSH. |

## Building PieJam (`50-app`)

| Variable | Default | Purpose |
|---|---|---|
| `PIEJAM_REPO` | `https://github.com/vilpter/piejam.git` | Repository to build |
| `PIEJAM_REF` | `master` | Branch, tag or commit |
| `PIEJAM_SRC_DIR` | `/usr/local/src/piejam` | Checkout location |
| `PIEJAM_CMAKE_ARGS` | _(empty)_ | Extra CMake arguments |
| `PIEJAM_FORCE_BUILD` | `0` | `1` rebuilds even if that commit is installed |

PieJam builds with `-Werror`. If a newer compiler adds warnings upstream hasn't
seen yet, get unblocked with `PIEJAM_CMAKE_ARGS="-DCMAKE_CXX_FLAGS=-Wno-error"`, then
fix the warnings in the fork.

## Day-to-day

```sh
sudo systemctl stop piejam        # free the display for debugging (doesn't power off)
sudo systemctl start piejam
journalctl -u piejam -b           # app and launcher logs for this boot
```

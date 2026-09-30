# ADR 0001: Provision Raspberry Pi OS Lite instead of building a custom image

- **Status:** Accepted
- **Date:** 2026-09-30

## Context

PieJam OS (and the `vilpter/piejam_os` fork) builds a complete Linux image with
Buildroot. That gives a small, fast-booting system, but:

- Every package, kernel option and init script is hand-maintained. The fork's recent
  history shows this cost: several commits just to get WiFi drivers, firmware loading
  and NFS working (`Enable WiFi and NFS kernel/firmware support`, `Switch WiFi drivers to
  kernel modules for firmware loading`, `Pin boot-critical options as built-in`).
- Security updates mean rebuilding and reflashing the whole image.
- Application code has to be bent around the image's gaps. The app fork's network
  code runs busybox `udhcpc` for DHCP and controls NFS through busybox init scripts,
  because the image had nothing more standard.

## Decision

Start from stock **Raspberry Pi OS Lite 64-bit** and turn it into an appliance with
idempotent provisioning scripts, systemd units and boot config fragments. No custom OS
image is built.

The release is the one based on **Debian 13 "trixie"**. PieJam sets
`CMAKE_CXX_STANDARD 26`, which needs GCC 14; the older bookworm-based release ships
GCC 12 and can't build it.

## Consequences

- **Good:** normal `apt` updates, the Raspberry Pi kernel and firmware stay current,
  standard tools (`systemd`, `NetworkManager`, `sudo`) are available, and hardware
  support for the Pi 5 comes from upstream.
- **Good:** the provisioning is readable shell that can be re-run on a fresh NVMe install.
- **Cost:** a bigger base system than Buildroot, with more services to boot. We handle
  that by measuring and disabling only what's unneeded ([benchmark plan](../benchmark-plan.md)),
  in line with D5 (maintainability before speed).
- **Risk:** the 8–15 s target may be harder to reach than with Buildroot. We'll know
  after the first measured baseline. If it's missed by a wide margin, revisit the
  optimization stages before revisiting this ADR.

## Alternatives considered

- **Keep Buildroot (`piejam_os`).** Fastest boot, highest maintenance. Rejected by the
  project brief.
- **Raspberry Pi OS with desktop, autostart the app.** Least effort, but brings a
  desktop session, compositor and login manager that the appliance doesn't want.
- **Yocto or another image builder.** Same maintenance problem as Buildroot.

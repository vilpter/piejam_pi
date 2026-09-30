# ADR 0004: Move the fork's network backend onto NetworkManager and systemd

- **Status:** Accepted
- **Date:** 2026-09-30

## Context

The fork's `network_manager` backend (reviewed at `e1291b94`) was written for Buildroot:

- **WiFi:** it configures `wpa_supplicant` directly over its control socket, starts
  `wpa_supplicant` itself if it isn't running, loads the WiFi driver with
  `modprobe brcmfmac`, and runs busybox `udhcpc` for DHCP.
- **NFS server:** it's controlled through busybox init scripts (`/etc/init.d/S60nfs`),
  falling back to `systemctl`. The fallback also *enables* the server at boot.

On Raspberry Pi OS Lite, **NetworkManager** owns the WiFi interface: it runs
`wpa_supplicant` and DHCP itself, and `udhcpc` isn't installed. Configuring
`wpa_supplicant` behind NetworkManager's back leaves two managers fighting over one
interface. The NFS server is a systemd service (`nfs-server.service`), and enabling it
at boot would put it on every boot, against [ADR 0005](0005-network-not-boot-dependency.md).

## Decision

- **WiFi:** use `nmcli` (NetworkManager) for scan, connect, save, forget, radio on/off
  and status.
- **NFS server:** `systemctl start|stop|is-active nfs-server` and `exportfs`. Started on
  demand from the UI, never enabled at boot.
- **NFS client:** `mount -t nfs` / `umount` with explicit arguments.
- **Process execution:** run every command through `QProcess::start(program, arguments)`
  with an argument list and **no shell**. User input is only ever a separate argument.
- **Privileges:** the app runs as the `piejam` user, not root.
  - NetworkManager actions are authorized by a polkit rule for the `piejam` user.
  - Starting and stopping `nfs-server` is authorized by polkit, limited to that unit.
  - NFS mount and export operations go through `sudo` rules restricted to exact commands.

The Redux state, GUI model and QML layer (`NetworkSettings`, dialogs, delegates) are
kept. The backend class interfaces stay the same where practical, so the UI needs
little or no change.

## Consequences

- **Good:** stops fighting the OS. NetworkManager keeps ownership of WiFi and DHCP, and
  remembers saved networks across reboots on its own.
- **Good:** removes the Buildroot-specific paths from the app.
- **Good:** NFS only runs when asked for, so it never costs boot time.
- **Cost:** a backend rewrite of `network_controller.cpp`, `wifi_manager.cpp`,
  `nfs_server.cpp` and `nfs_client.cpp`, plus their tests.

## Alternatives considered

- **Keep direct `wpa_supplicant` control, disable NetworkManager.** Works, but we'd
  maintain our own WiFi stack, including DHCP.
- **Talk to NetworkManager over D-Bus (QtDBus).** Cleaner long term, but more code than
  `nmcli` for the same features. Worth revisiting later.

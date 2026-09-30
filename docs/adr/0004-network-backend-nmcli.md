# ADR 0004: Move the fork's network backend onto NetworkManager and systemd

- **Status:** Accepted
- **Date:** 2026-09-30

## Context

The fork's `network_manager` backend was written for Buildroot:

- WiFi is managed by starting `wpa_supplicant` directly and driving it with `wpa_cli`.
- The NFS server is controlled through busybox init (`/etc/init.d/S60nfs`) and `pidof`.
- Commands are built as strings and run through `std::system` / `popen`, so they go
  through `/bin/sh`.

On Raspberry Pi OS Lite, **NetworkManager** already owns the WiFi interface.
Starting a second `wpa_supplicant` fights it. `/etc/init.d/S60nfs` doesn't exist, and NFS
is a systemd service (`nfs-server.service`).

The shell-string approach is also a correctness bug. For example, in `wifi_manager.cpp`:

```cpp
wpa_cli(interface, "set_network " + id_str + " psk '\"" + password + "\"'");
```

A passphrase containing `'`, `` ` ``, `$` or `\` breaks the quoting. At best the
connection fails; at worst characters in an SSID or passphrase run as shell commands.

## Decision

- **WiFi:** use `nmcli` (NetworkManager) for scan, connect, forget, and status.
- **NFS server:** use `systemctl start|stop|is-active nfs-server` and `exportfs`.
- **NFS client:** use `mount -t nfs` / `umount` with explicit arguments.
- **Process execution:** run every command through `QProcess::start(program, arguments)`
  with an argument list and **no shell**. User input is only ever a separate argument.
- **Privileges:** the app runs as the `piejam` user, not root.
  - NetworkManager actions are authorized by a polkit rule for the `piejam` user.
  - NFS mount/export operations go through a small set of `sudo` rules restricted to
    exact commands.

The QML and GUI model layer (`NetworkSettings`, dialogs, delegates) is kept. The
backend class interfaces stay the same where practical, so the UI needs little or no change.

## Consequences

- **Good:** fixes the injection and quoting bug by design.
- **Good:** stops fighting the OS: NetworkManager keeps ownership of WiFi and remembers
  saved networks across reboots.
- **Good:** the auto-disable-WiFi-while-recording middleware still works; it just calls
  `nmcli radio wifi off|on`.
- **Cost:** backend rewrite of four files (`network_controller.cpp`, `wifi_manager.cpp`,
  `nfs_server.cpp`, `nfs_client.cpp`) plus their tests.

## Alternatives considered

- **Keep `wpa_supplicant`, disable NetworkManager.** Works, but we'd maintain our own
  WiFi stack, and the quoting bug still needs fixing.
- **Talk to NetworkManager over D-Bus (QtDBus).** Cleaner long term, but more code than
  `nmcli` for the same features. Worth revisiting later.

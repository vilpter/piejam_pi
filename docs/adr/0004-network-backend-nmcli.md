# ADR 0004: Move the fork's network backend onto NetworkManager and systemd

- **Status:** Accepted. WiFi implemented 2026-10-15 (#2); NFS implemented 2026-10-16 (#4)
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
  demand from the UI, never enabled at boot. The recordings directory is shared
  read-write to the Pi's current subnet; the export is applied with `exportfs` and
  never written to `/etc/exports`, so it is gone after a reboot.
- **NFS client: dropped.** The fork could also mount remote shares under `/mnt`. The
  appliance only ever needs to *offer* its recordings, so the client backend, its GUI
  model and its QML were removed (D18). Recordings can also be pulled off over SSH.
- **Process execution:** run every command through `QProcess::start(program, arguments)`
  with an argument list and **no shell**. User input is only ever a separate argument.
- **Privileges:** the app runs as the `piejam` user, not root.
  - NetworkManager actions are authorized by a polkit rule for the `piejam` user.
  - Starting and stopping `nfs-server` is authorized by polkit, limited to that unit.
  - The export can't be an "exact command" (the subnet is computed at runtime), so it
    goes through a validated root helper, `piejam-nfs-export`, allowed by a `sudoers`
    rule. The helper fixes the export path and accepts only a strict IPv4 CIDR and
    `rw`/`ro`, so the app can never ask it to export anything else.

The Redux state, GUI model and QML layer (`NetworkSettings`, dialogs, delegates) are
kept. The backend class interfaces stay the same where practical, so the UI needs
little or no change.

## Consequences

- **Good:** stops fighting the OS. NetworkManager keeps ownership of WiFi and DHCP, and
  remembers saved networks across reboots on its own.
- **Good:** removes the Buildroot-specific paths from the app.
- **Good:** NFS only runs when asked for, so it never costs boot time.
- **Cost:** a backend rewrite of `network_controller.cpp`, `wifi_manager.cpp` and
  `nfs_server.cpp`, plus their tests. `nfs_client.cpp` and the mount UI were removed.

## Implementation notes (WiFi, 2026-10-15)

- The Network switch is NetworkManager's WiFi radio. NetworkManager remembers it
  across reboots, so turning Network off in the app keeps WiFi off until it's
  turned on again.
- Passwords reach `nmcli` on standard input through its connection editor, never on a
  command line. The editor trims values, so passwords with leading or trailing spaces
  are refused rather than saved altered.
- Status is polled with `nmcli device show` every 3 s while the Network page is
  visible, and not otherwise, so the audio threads don't share the CPU with process
  launches during normal use.

## Implementation notes (NFS, 2026-10-16)

- **Server only.** The Pi exports its recordings (`/home/piejam/recordings`); mounting
  remote shares was removed. The NFS toggle on the Network page starts `nfs-server`,
  computes the Pi's current subnet, and exports read-write to it.
- **Short mount path.** The recordings are exposed under `/srv/rec` — a bind mount of
  `/home/piejam/recordings`, set up only while sharing is on — so the command clients
  type stays short (`<pi>:/srv/rec`). A bind mount needs an explicit `fsid` to be
  exportable, which the helper sets. The recordings themselves stay in `/home/piejam`.
- **Read-write to the whole subnet, with a warning.** NFS has no per-user login, so the
  allowed-hosts rule is the only control. Before the share is turned on, the UI warns
  that everyone on the local network can read, change or delete recordings while it is
  on. The subnet is recomputed on every enable, so moving the Pi to another network
  just works.
- **Ephemeral.** The export is applied with `exportfs` and never written to
  `/etc/exports`, and the `/srv/rec` bind mount is torn down when sharing stops. Both
  are gone after a reboot, and the service is disabled at boot, so NFS never costs boot
  time ([ADR 0005](0005-network-not-boot-dependency.md)).
- **Privilege boundary.** The service is started and stopped through polkit (limited to
  `nfs-server.service`). The export runs through the root helper `piejam-nfs-export`
  (installed by provisioning, allowed by `/etc/sudoers.d/piejam-nfs`): it hard-codes the
  export path and validates the subnet against a strict IPv4 CIDR pattern.
- **Access maps to the recordings' owner.** The export uses `all_squash` with
  `anonuid`/`anongid` set to the owner of the recordings directory (the `piejam` user),
  so anyone on the subnet reads and writes the recordings *as that owner* — which is the
  read-write access the UI warns about. This is deliberately not `no_root_squash`: a
  remote client never gets root on the system, only access as the recordings' owner.
  (Plain `root_squash` was tried first but left the share effectively read-only, since
  the directory isn't world-writable.)
- **Turned off with the network.** Turning WiFi off, or dropping the connection, stops
  the share as well.

## Alternatives considered

- **Keep direct `wpa_supplicant` control, disable NetworkManager.** Works, but we'd
  maintain our own WiFi stack, including DHCP.
- **Talk to NetworkManager over D-Bus (QtDBus).** Cleaner long term, but more code than
  `nmcli` for the same features. Worth revisiting later.

# ADR 0005: The network is never a boot dependency

- **Status:** Accepted
- **Date:** 2026-09-30

## Context

The appliance needs a maintenance and recovery path (SSH), but it's used on stages and
in rooms where there may be no network at all. Anything that waits for the network can
add tens of seconds to boot or hang it.

## Decision

- `piejam.service` has no ordering or dependency on `network.target` or
  `network-online.target`.
- `NetworkManager-wait-online.service` is disabled, so nothing waits for a connection.
- SSH (`ssh.service`) is enabled and starts in parallel. It's reachable whenever a
  network happens to be up, and it never blocks the UI.
- The UI must come up whether the network is up, down, or absent.

## Consequences

- **Good:** boot time is independent of the network.
- **Cost:** anything that genuinely needs the network (time sync, sharing recordings)
  must tolerate it arriving late. The app's NFS share is turned on from the UI on
  demand, never at boot, and the `nfs-server` service is disabled at boot.
- **Check:** `systemd-analyze critical-chain piejam.service` must show no network units
  ([benchmark plan](../benchmark-plan.md)).

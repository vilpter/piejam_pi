# ADR 0003: Build on the `vilpter/piejam` fork

- **Status:** Accepted
- **Date:** 2026-09-30

## Context

There's an existing fork, `vilpter/piejam` (reviewed at `e1291b94`, the current
`master`). It's based on the latest upstream `nooploop/piejam` with nothing unmerged,
and adds one feature module:

- `network_manager` — WiFi and NFS client/server management, with Redux state, a
  `NetworkSettings` GUI model that follows upstream's patterns, QML views, and tests.

An earlier snapshot of the fork also had a `file_manager` module with a FileBrowser
UI for recordings. It isn't in the current fork (open question O4 in the
[decision log](../decision-log.md)).

The options were: run upstream unchanged, continue the fork, or write a new UI.

## Decision

Continue the fork. Specifically:

1. **Keep following upstream.** The fork is currently up to date with `nooploop/master`.
   Merge upstream regularly so the fork's delta stays small.
2. **Reuse unchanged:** the `network_manager` Redux state, GUI model and QML.
3. **Rework:** the `network_manager` backend ([ADR 0004](0004-network-backend-nmcli.md)).
   That rework also removes the Buildroot-specific paths (busybox `udhcpc`,
   `modprobe brcmfmac`, `/etc/init.d/S60nfs`).

The app is built from source **on the Pi** against trixie's GCC 14 and Qt 5.15
packages, instead of cross-compiled as Buildroot did. PieJam builds with `-Werror`, so
a compiler newer than upstream's may surface new warnings that fail the build;
`PIEJAM_CMAKE_ARGS` in `provisioning/stages/50-app.sh` provides a temporary escape hatch.

## Consequences

- **Good:** keeps working, tested features and the look of PieJam.
- **Cost:** we own merging upstream into the fork from now on.
- **Risk:** building on the Pi is slower than cross-compiling. Fine for a baseline;
  if it gets painful, package the build as a `.deb` built in CI (arm64).

## Alternatives considered

- **Upstream unchanged.** Loses the network features.
- **New UI.** Most effort, slowest path to a working appliance.

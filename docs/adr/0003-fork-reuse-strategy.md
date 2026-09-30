# ADR 0003: Build on the `vilpter/piejam` fork

- **Status:** Accepted
- **Date:** 2026-09-30

## Context

There's an existing fork, `vilpter/piejam`, that adds two modules on top of upstream
`nooploop/piejam` (branch point `490e4c8f`, fork HEAD `9fccfad0`, ~11,600 lines):

- `file_manager` — recording scanner, metadata database, metadata extraction, and a
  FileBrowser UI with cards, tags, ratings and notes. Has tests.
- `network_manager` — WiFi and NFS client/server management with a Settings UI. Has tests.

The options were: run upstream unchanged, continue the fork, or write a new UI.

## Decision

Continue the fork. Specifically:

1. **Merge upstream first.** The fork is 10+ commits behind `nooploop/master`, including
   `system: add memory locking`, which matters for audio reliability. Merge before adding
   anything new so the delta stays small.
2. **Reuse unchanged:** the `file_manager` module, the FileBrowser UI, and all
   `network_manager` QML and GUI models.
3. **Rework:** the `network_manager` command backend ([ADR 0004](0004-network-backend-nmcli.md)).
4. **Revert** the Buildroot-specific workarounds from commit `ac8f4c79` (removed `sudo`
   and `systemctl`, busybox init scripts, `modprobe brcmfmac`). Keep that commit's QML
   binding-loop fixes, which aren't Buildroot-specific.

The app is built from source **on the Pi** against trixie's GCC 14 and Qt 5.15
packages, instead of cross-compiled as Buildroot did. PieJam builds with `-Werror`, so
a compiler newer than upstream's may surface new warnings that fail the build;
`PIEJAM_CMAKE_ARGS` in `provisioning/stages/50-app.sh` provides a temporary escape hatch. Commit `19394146` (`Fix Qt5 and buildroot
cross-compilation compatibility issues`) needs a review to see which parts still apply.

## Consequences

- **Good:** keeps working, tested features and the look of PieJam.
- **Cost:** we own merging upstream into the fork from now on.
- **Risk:** building on the Pi is slower than cross-compiling. Fine for a baseline;
  if it gets painful, package the build as a `.deb` built in CI (arm64).

## Alternatives considered

- **Upstream unchanged.** Loses the file browser and network features.
- **New UI.** Most effort, slowest path to a working appliance.

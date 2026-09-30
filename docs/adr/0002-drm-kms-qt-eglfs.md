# ADR 0002: Render directly to DRM/KMS with Qt eglfs

- **Status:** Accepted
- **Date:** 2026-09-30

## Context

PieJam's UI is Qt/QML. It needs to fill the 7″ panel with no desktop chrome. The
options are a compositor (Weston, Cage) or letting Qt drive the display itself.

## Decision

Run PieJam with Qt's **`eglfs`** platform plugin using the **`eglfs_kms`** integration,
so the app renders straight to DRM/KMS through GBM/EGL. No X server, no Wayland
compositor.

Environment (set in `piejam.service`):

```
QT_QPA_PLATFORM=eglfs
QT_QPA_EGLFS_INTEGRATION=eglfs_kms
QT_QPA_EGLFS_HIDECURSOR=1
```

`QT_QPA_EGLFS_KMS_CONFIG` is set by `bin/piejam-launch`, which picks the display
at each boot and writes the config to `/run/piejam/eglfs-kms.json`. The launcher is
ported from PieJam OS's Pi 5 launcher (decision D13).

## Consequences

- **Good:** shortest path to the first frame — nothing to start before the app.
- **Good:** no window decorations or compositor, so it looks like an appliance by default.
- **Cost:** only one DRM master at a time. Debugging other graphical tools means
  stopping `piejam.service` first.
- **Cost:** display selection is configured through Qt's KMS config rather than a
  compositor. Rotation isn't affected: PieJam rotates its own UI in QML.
- **Risk:** the Pi 5 has separate display (`vc4`) and render (`v3d`) DRM devices, and
  their card numbers aren't stable. The launcher handles this by finding the card that
  has the panel's connector at each boot. Qt's connector naming (`DSI1` rather than
  sysfs's `DSI-1`) still needs confirming on hardware (assumption A3).

## Fallback

If `eglfs` misbehaves (for example on input handling or rotation), switch to **Cage**,
a single-app Wayland kiosk compositor, and run PieJam with `QT_QPA_PLATFORM=wayland`.
That's a change to one unit file and adds one package, so it's cheap to reverse.

## Alternatives considered

- **Cage.** Small cost in boot time; the fallback above.
- **Weston kiosk-shell.** Heaviest option; only if Cage also fails.
- **X11 + a kiosk window manager.** More moving parts and slower than either.

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
QT_QPA_EGLFS_KMS_CONFIG=/etc/piejam/eglfs-kms.json
QT_QPA_EGLFS_ALWAYSSET_MODE=1
```

## Consequences

- **Good:** shortest path to the first frame — nothing to start before the app.
- **Good:** no window decorations or compositor, so it looks like an appliance by default.
- **Cost:** only one DRM master at a time. Debugging other graphical tools means
  stopping `piejam.service` first.
- **Cost:** rotation and multi-display are configured through Qt's KMS config
  (`eglfs-kms.json`) and, for touch, libinput — not through a compositor.
- **Risk:** the Pi 5 has separate display (`vc4`) and render (`v3d`) DRM devices. Qt must
  open the KMS-capable card. We pin the device in `eglfs-kms.json` once it's confirmed on
  hardware (assumption A3).

## Fallback

If `eglfs` misbehaves (for example on input handling or rotation), switch to **Cage**,
a single-app Wayland kiosk compositor, and run PieJam with `QT_QPA_PLATFORM=wayland`.
That's a change to one unit file and adds one package, so it's cheap to reverse.

## Alternatives considered

- **Cage.** Small cost in boot time; the fallback above.
- **Weston kiosk-shell.** Heaviest option; only if Cage also fails.
- **X11 + a kiosk window manager.** More moving parts and slower than either.

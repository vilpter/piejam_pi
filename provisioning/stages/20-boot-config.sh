#!/bin/bash
# Stage 20: firmware and kernel command-line settings.
#
# Our config.txt settings live in their own file, pulled in with a single
# "include" line, so the distribution's config.txt stays recognizable.
set -euo pipefail
# shellcheck source=../lib.sh
source "$(dirname "$0")/../lib.sh"
require_root

config=$BOOT_DIR/config.txt

install_file config/boot/piejam.txt "$BOOT_DIR/piejam.txt" 0644

# The include goes under [all]: config.txt may end inside a model-specific
# section such as [cm5], which would otherwise hide our settings.
if ! grep -qxF 'include piejam.txt' "$config"; then
    backup_once "$config"
    printf '\n# Added by piejam_pi provisioning\n[all]\ninclude piejam.txt\n' >> "$config"
    log "config.txt: added include piejam.txt"
fi

# Disable HDMI audio so the Scarlett is the only sound card PieJam lists
# (as PieJam OS did). This modifies the distribution's own overlay line,
# because loading vc4-kms-v3d a second time isn't supported.
ensure_overlay_param "$config" vc4-kms-v3d noaudio ||
    warn "no dtoverlay=vc4-kms-v3d line in $config; KMS may not be enabled"

# Quiet, cursor-free boot. Printing to the framebuffer console also slows boot.
ensure_cmdline_param quiet
ensure_cmdline_param loglevel=3
ensure_cmdline_param logo.nologo
ensure_cmdline_param vt.global_cursor_default=0

#!/bin/bash
# Stage 30: the unprivileged user PieJam runs as.
#
# The app keeps its config, session and recordings under $HOME
# (~/.config/piejam.config, ~/last.pjs, ~/recordings).
set -euo pipefail
source "$(dirname "$0")/../lib.sh"
require_root

if ! id piejam &>/dev/null; then
    useradd --system --create-home --home-dir /home/piejam \
        --shell /usr/sbin/nologin --user-group piejam
    log "created user piejam"
fi

# audio: ALSA devices. video/render: DRM display and GPU. input: touchscreen.
usermod --append --groups audio,video,render,input piejam

# Let piejam power off the device (and, later, manage NetworkManager) without
# an interactive login session.
install_file config/polkit/50-piejam.rules /etc/polkit-1/rules.d/50-piejam.rules 0644

# Real-time and memlock limits for interactive sessions (e.g. running the app
# by hand over SSH). The service sets its own limits in piejam.service.
install_file config/limits/piejam.conf /etc/security/limits.d/piejam.conf 0644

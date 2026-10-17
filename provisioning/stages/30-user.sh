#!/bin/bash
# Stage 30: the unprivileged user PieJam runs as.
#
# The app keeps its settings, sessions and recordings under $HOME
# (~/.config/, ~/sessions/, ~/recordings/).
set -euo pipefail
# shellcheck source=../lib.sh
source "$(dirname "$0")/../lib.sh"
require_root

if ! id piejam &>/dev/null; then
    useradd --system --create-home --home-dir /home/piejam \
        --shell /usr/sbin/nologin --user-group piejam
    log "created user piejam"
fi

# audio: ALSA devices. video/render: DRM display and GPU. input: touchscreen.
usermod --append --groups audio,video,render,input piejam

# Let piejam power off the device, manage NetworkManager, and start/stop the
# NFS server (limited to nfs-server.service) without an interactive login.
install_file config/polkit/50-piejam.rules /etc/polkit-1/rules.d/50-piejam.rules 0644

# The NFS export is applied by a validated root helper the app calls through a
# restricted sudoers rule (ADR 0004). Check the sudoers syntax before putting
# it in place: a bad file there can lock everyone out of sudo.
install_file bin/piejam-nfs-export /usr/local/sbin/piejam-nfs-export 0755
visudo -cf "$REPO_ROOT/config/sudoers/piejam-nfs" >/dev/null ||
    die "config/sudoers/piejam-nfs is not valid sudoers syntax"
install_file config/sudoers/piejam-nfs /etc/sudoers.d/piejam-nfs 0440

# Real-time and memlock limits for interactive sessions (e.g. running the app
# by hand over SSH). The service sets its own limits in piejam.service.
install_file config/limits/piejam.conf /etc/security/limits.d/piejam.conf 0644

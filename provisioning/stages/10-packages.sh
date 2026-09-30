#!/bin/bash
# Stage 10: install what PieJam needs to build and run.
set -euo pipefail
source "$(dirname "$0")/../lib.sh"
require_root

# Runtime: the QML modules PieJam imports, the LADSPA plugin sets PieJam OS
# shipped (PieJam scans /usr/lib/ladspa at startup), and polkit so the
# unprivileged app user can power off the device.
runtime_packages=(
    qml-module-qtquick2
    qml-module-qtquick-controls2
    qml-module-qtquick-templates2
    qml-module-qtquick-layouts
    qml-module-qtquick-window2
    qml-module-qtqml-models2
    qml-module-qtquick-virtualkeyboard
    qml-module-qtgraphicaleffects
    caps
    tap-plugins
    polkitd
)

# Build: PieJam is built from source on the Pi (ADR 0003). It needs C++26,
# so GCC 14 or newer (Debian 13 / trixie).
build_packages=(
    build-essential
    cmake
    git
    pkg-config
    qtbase5-dev
    qtdeclarative5-dev
    qtquickcontrols2-5-dev
    qtvirtualkeyboard5-dev
    libqt5svg5-dev
    libboost-dev
    libspdlog-dev
    libfftw3-dev
    libsndfile1-dev
    nlohmann-json3-dev
    ladspa-sdk
)

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends "${runtime_packages[@]}" "${build_packages[@]}"

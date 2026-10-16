#!/bin/bash
# Stage 50: build and install PieJam from app/ in this repository.
#
# Environment:
#   PIEJAM_BUILD_DIR   build directory  (default: /usr/local/src/piejam-build)
#   PIEJAM_CMAKE_ARGS  extra CMake arguments, e.g. "-DCMAKE_CXX_FLAGS=-Wno-error"
#   PIEJAM_FORCE_BUILD set to 1 to start from an empty build directory
#
# The build is incremental: re-running after a pull rebuilds only what changed.
set -euo pipefail
# shellcheck source=../lib.sh
source "$(dirname "$0")/../lib.sh"
require_root

app=$REPO_ROOT/app
build=${PIEJAM_BUILD_DIR:-/usr/local/src/piejam-build}
version_file=/usr/local/share/piejam/VERSION
read -r -a extra_cmake_args <<< "${PIEJAM_CMAKE_ARGS:-}"

[[ -f $app/CMakeLists.txt ]] || die "no app sources in $app"

# Run git as the checkout's owner. As root it would trip git's ownership
# check, and fetching submodules would leave root-owned files in their .git.
owner=$(stat -c %U "$REPO_ROOT")
as_owner() { runuser -u "$owner" -- "$@"; }

# The app's libraries are submodules, which a plain clone doesn't fetch.
if [[ ! -f $app/libs/MIPP/include/mipp.h || ! -f $app/libs/nod/include/nod/nod.hpp ]]; then
    [[ -e $REPO_ROOT/.git ]] || die "app/libs is empty and $REPO_ROOT is not a git checkout"
    log "fetching the app's submodules"
    as_owner git -C "$REPO_ROOT" submodule update --init --recursive ||
        die "couldn't fetch the submodules (try: git submodule update --init)"
fi

# Before the app lived in this repository, this stage cloned it to
# /usr/local/src/piejam. Remove that clone and its build.
old_clone=/usr/local/src/piejam
if [[ -d $old_clone/.git ]] &&
   [[ $(git -c safe.directory="$old_clone" -C "$old_clone" remote get-url origin 2>/dev/null) == */vilpter/piejam.git ]]; then
    rm -rf -- "$old_clone"
    log "removed the old app checkout $old_clone"
fi

if [[ ${PIEJAM_FORCE_BUILD:-0} == 1 && -d $build ]]; then
    rm -rf -- "${build:?}"
fi

log "building PieJam from $app (the first build takes a while)"
cmake -S "$app" -B "$build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    "${extra_cmake_args[@]}"
cmake --build "$build" --parallel "$(nproc)"
cmake --install "$build"

# Record what was built: this repository's commit, marked if app/ had local
# changes. benchmarks/measure-boot.sh reports it.
version=$(as_owner git -C "$REPO_ROOT" rev-parse --short=12 HEAD 2>/dev/null || echo unknown)
if [[ -n $(as_owner git -C "$REPO_ROOT" status --porcelain -- app 2>/dev/null) ]]; then
    version+="-modified"
fi
install -d "$(dirname "$version_file")"
echo "$version" > "$version_file"
log "installed PieJam $version"

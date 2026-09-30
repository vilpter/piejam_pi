#!/bin/bash
# Stage 50: build and install PieJam from the fork.
#
# Environment:
#   PIEJAM_REPO        git URL          (default: https://github.com/vilpter/piejam.git)
#   PIEJAM_REF         branch/tag/SHA   (default: master)
#   PIEJAM_SRC_DIR     checkout path    (default: /usr/local/src/piejam)
#   PIEJAM_CMAKE_ARGS  extra CMake arguments, e.g. "-DCMAKE_CXX_FLAGS=-Wno-error"
#   PIEJAM_FORCE_BUILD set to 1 to rebuild even if this commit is installed
set -euo pipefail
source "$(dirname "$0")/../lib.sh"
require_root

repo=${PIEJAM_REPO:-https://github.com/vilpter/piejam.git}
ref=${PIEJAM_REF:-master}
src=${PIEJAM_SRC_DIR:-/usr/local/src/piejam}
version_file=/usr/local/share/piejam/VERSION
read -r -a extra_cmake_args <<< "${PIEJAM_CMAKE_ARGS:-}"

if [[ ! -d $src/.git ]]; then
    git clone "$repo" "$src"
fi
git -C "$src" remote set-url origin "$repo"
git -C "$src" fetch --tags --prune origin

# A branch name means the remote branch; anything else is a tag or commit.
if git -C "$src" rev-parse --verify --quiet "origin/$ref^{commit}" >/dev/null; then
    target=origin/$ref
else
    target=$ref
fi
commit=$(git -C "$src" rev-parse "$target^{commit}")

if [[ ${PIEJAM_FORCE_BUILD:-0} != 1 && -x /usr/local/bin/piejam_app &&
      $(cat "$version_file" 2>/dev/null) == "$commit" ]]; then
    log "PieJam $commit already installed"
    exit 0
fi

git -C "$src" checkout --quiet --detach "$commit"
git -C "$src" submodule update --init --recursive

log "building PieJam $commit (this takes a while)"
cmake -S "$src" -B "$src/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    "${extra_cmake_args[@]}"
cmake --build "$src/build" --parallel "$(nproc)"
cmake --install "$src/build"

install -d "$(dirname "$version_file")"
echo "$commit" > "$version_file"
log "installed PieJam $commit"

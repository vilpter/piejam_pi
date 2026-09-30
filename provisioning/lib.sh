# Shared helpers for the provisioning stages. Source this file; don't run it.
# shellcheck shell=bash

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BOOT_DIR=/boot/firmware

log()  { printf '\033[1m[piejam]\033[0m %s\n' "$*"; }
warn() { printf '\033[33m[piejam] warning:\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[31m[piejam] error:\033[0m %s\n' "$*" >&2; exit 1; }

require_root() {
    [[ $EUID -eq 0 ]] || die "must run as root (try: sudo $0)"
}

# install_file SRC DEST MODE
# Copy SRC (relative to the repo root) to DEST, logging only when it changes.
install_file() {
    local src="$REPO_ROOT/$1" dest=$2 mode=$3
    [[ -f $src ]] || die "missing $src"
    if [[ -f $dest ]] && cmp -s "$src" "$dest"; then
        chmod "$mode" "$dest"
        return 0
    fi
    install -D -m "$mode" "$src" "$dest"
    log "installed $dest"
}

# backup_once FILE
# Keep the distribution's original copy the first time we modify FILE.
backup_once() {
    [[ -e $1.piejam-orig ]] || cp -a "$1" "$1.piejam-orig"
}

# unit_exists UNIT
unit_exists() {
    [[ -n $(systemctl list-unit-files --no-legend "$1" 2>/dev/null) ]]
}

# ensure_overlay_param FILE OVERLAY PARAM
# Append PARAM to an existing "dtoverlay=OVERLAY..." line in FILE, unless it's
# already there. Returns 1 if FILE has no such line.
ensure_overlay_param() {
    local file=$1 overlay=$2 param=$3
    local line_re="^dtoverlay=${overlay}([,[:space:]#]|\$)"

    grep -qE "$line_re" "$file" || return 1
    grep -E "$line_re" "$file" | grep -qE "[=,]${param}([,[:space:]#]|\$)" && return 0

    backup_once "$file"
    sed -i -E "s/^(dtoverlay=${overlay}(,[^[:space:]#]*)?)([[:space:]#]|\$)/\\1,${param}\\3/" "$file"
    log "$(basename "$file"): $overlay now has $param"
}

# ensure_cmdline_param PARAM
# Add PARAM to the kernel command line. For key=value parameters, an existing
# value for the same key is replaced rather than duplicated.
ensure_cmdline_param() {
    local param=$1 file=$BOOT_DIR/cmdline.txt
    local key=${param%%=*} tok found=0
    local -a tokens out=()

    read -r -a tokens < "$file"
    for tok in "${tokens[@]}"; do
        if [[ $tok == "$param" ]]; then
            found=1
            out+=("$tok")
        elif [[ $param == *=* && ${tok%%=*} == "$key" ]]; then
            found=1
            out+=("$param")
        else
            out+=("$tok")
        fi
    done
    (( found )) || out+=("$param")

    if [[ "${out[*]}" != "${tokens[*]}" ]]; then
        backup_once "$file"
        printf '%s\n' "${out[*]}" > "$file"
        log "cmdline.txt: set $param"
    fi
}

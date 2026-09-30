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

# The helpers below exit via die() on any failure instead of relying on
# set -e, which bash disables inside functions called from if/||/&&.

# backup_once FILE
# Keep the distribution's original copy the first time we modify FILE.
backup_once() {
    [[ -e $1.piejam-orig ]] || cp -a "$1" "$1.piejam-orig" || die "cannot back up $1"
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
    local has_param_re="^dtoverlay=${overlay}(,[^[:space:]#]*)?,${param}([,[:space:]#]|\$)"

    [[ -r $file ]] || die "cannot read $file"
    grep -qE "$line_re" "$file" || return 1
    grep -qE "$has_param_re" "$file" && return 0

    backup_once "$file"
    sed -i -E "s/^(dtoverlay=${overlay}(,[^[:space:]#]*)?)([[:space:]#]|\$)/\\1,${param}\\3/" "$file" ||
        die "failed to edit $file"
    log "$(basename "$file"): $overlay now has $param"
}

# ensure_cmdline_param PARAM
# Add PARAM to the kernel command line. For key=value parameters, an existing
# value for the same key is replaced rather than duplicated.
ensure_cmdline_param() {
    local param=$1 file=$BOOT_DIR/cmdline.txt
    local key=${param%%=*} tok found=0 tmp
    local -a tokens out=()

    # A command line without root= won't boot. Never build on one: that would
    # mean the file was empty or unreadable, not that it needs our parameters.
    [[ -r $file ]] || die "cannot read $file"
    read -r -a tokens < "$file" || [[ ${#tokens[@]} -gt 0 ]] ||
        die "$file is empty; refusing to edit it"
    [[ " ${tokens[*]} " == *" root="* ]] ||
        die "$file has no root= parameter; refusing to edit it"
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
        # Write a temporary file and rename it over the original, so an
        # interruption can't leave cmdline.txt truncated.
        tmp=$(mktemp "$file.XXXXXX") || die "cannot create a temporary file next to $file"
        # Keep the original mode. On the FAT boot partition modes come from the
        # mount options and chmod may be refused, which is harmless there.
        chmod --reference="$file" "$tmp" 2>/dev/null || true
        if ! printf '%s\n' "${out[*]}" > "$tmp" || ! mv -f "$tmp" "$file"; then
            rm -f "$tmp"
            die "failed to write $file"
        fi
        log "cmdline.txt: set $param"
    fi
}

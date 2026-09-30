#!/bin/bash
# Lint every shell script in the repository with shellcheck.
#
# Problems are only reported for the files passed in, not for files they
# source, so scripts are discovered rather than listed: anything with a sh/bash
# shebang, plus files that declare a "shellcheck shell=" directive (such as
# provisioning/lib.sh, which is sourced and has no shebang).
set -euo pipefail
cd "$(dirname "$0")/.."

if ! command -v shellcheck >/dev/null; then
    echo "shellcheck is not installed (sudo apt install shellcheck)" >&2
    exit 1
fi

files=()
while IFS= read -r f; do
    [[ -f $f ]] || continue
    if head -n1 "$f" | grep -qE '^#!.*[/ ](ba)?sh([[:space:]]|$)' ||
       grep -q '^# shellcheck shell=' "$f"; then
        files+=("$f")
    fi
done < <(git ls-files --cached --others --exclude-standard)

(( ${#files[@]} )) || { echo "no shell scripts found" >&2; exit 1; }
shellcheck "${files[@]}"
echo "shellcheck: ${#files[@]} files clean"

#!/usr/bin/env bash
set -u

if [[ $# -ne 1 ]]; then
    printf 'Usage: %s <patchlist.txt>\n' "$0" >&2
    exit 2
fi

PATCHLIST=$1
SCRIPT_DIR=$(dirname "${BASH_SOURCE[0]}")
TEST_BIN="$SCRIPT_DIR/test_patchlist"

IB_WIDTH_MAX=${IB_WIDTH_MAX:-960}
IB_HEIGHT_MAX=${IB_HEIGHT_MAX:-544}
IB_STEP=${IB_STEP:-4}

if [[ ! -r "$PATCHLIST" ]]; then
    printf 'Cannot read patch list: %s\n' "$PATCHLIST" >&2
    exit 2
fi

if (( IB_WIDTH_MAX < 4 || IB_HEIGHT_MAX < 4 || IB_STEP <= 0 )); then
    printf 'IB_WIDTH_MAX and IB_HEIGHT_MAX must be at least 4; IB_STEP must be positive.\n' >&2
    exit 2
fi

make -C "$SCRIPT_DIR" test_patchlist >&2

declare -A first_error_for_line

run_case() {
    local fb=$1
    local ib=$2
    local fps=$3
    local msaa=$4
    local result

    while IFS= read -r result; do
        if [[ $result == *' ERR '* ]]; then
            local line=${result%% *}
            if [[ -z ${first_error_for_line[$line]+set} ]]; then
                first_error_for_line[$line]=1
                printf 'FB=%s IB=%s FPS=%s MSAA=%s %s\n' "$fb" "$ib" "$fps" "$msaa" "$result"
            fi
        fi
    done < <("$TEST_BIN" "$PATCHLIST" --fb "$fb" --ib "$ib" --fps "$fps" --msaa "$msaa" 2>/dev/null)
}

for fb in 480x272 640x368 720x408 960x544; do
    run_case "$fb" 960x544 60 2
done

for ((width = 4; width <= IB_WIDTH_MAX; width += IB_STEP)); do
    run_case 960x544 "${width}x544" 60 2
done

for ((height = 4; height <= IB_HEIGHT_MAX; height += IB_STEP)); do
    run_case 960x544 "960x${height}" 60 2
done

for fps in 20 30 60; do
    run_case 960x544 960x544 "$fps" 2
done

for msaa in 0 1 2; do
    run_case 960x544 960x544 60 "$msaa"
done

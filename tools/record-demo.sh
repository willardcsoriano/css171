#!/usr/bin/env bash
# Record a build-and-run demo of one activity as a 1920x1080 MP4, with no
# real screen needed.
#
# A real terminal (xterm) runs `make clean` and `make` from source, then
# launches the program. The virtual screen is sized to fit exactly
# [program window | gap | terminal], with the terminal as tall as the
# program window, and the recording is scaled up and centered in the frame.
#
# Usage (run from the activity's folder; the Makefiles do this):
#   ../tools/record-demo.sh BINARY APP_WIDTH APP_HEIGHT SECONDS OUTPUT.mp4
# With RECORD_PREVIEW=1 it also writes preview.png and preview.gif, cropped
# pixel-exact from the unscaled recording to the program window.
#
# Requires: Xvfb, xterm, ffmpeg, and the DejaVu Sans Mono font.
#   Debian/Ubuntu: sudo apt install xvfb xterm ffmpeg fonts-dejavu-core

set -euo pipefail

readonly OUTPUT_WIDTH=1920
readonly OUTPUT_HEIGHT=1080
readonly OUTPUT_MARGIN=40   # minimum border around the scaled content
readonly FRAMERATE=30
readonly BUILD_TIMEOUT_SECONDS=300

# Terminal appearance. Cell and chrome sizes were measured for this font at
# the fixed 96 DPI below: a text cell is 9x18 px and xterm adds 21 px of
# border/padding in each direction. Fixing the DPI keeps them identical on
# every machine.
readonly SCREEN_DPI=96
readonly TERMINAL_FONT="DejaVu Sans Mono"
readonly TERMINAL_FONT_SIZE=11
readonly TERMINAL_PADDING=10
readonly TERMINAL_COLUMNS=72
readonly CELL_WIDTH=9
readonly CELL_HEIGHT=18
readonly TERMINAL_CHROME=21
readonly WINDOW_GAP=24      # space between program window and terminal

die() {
    echo "record-demo: $*" >&2
    exit 1
}

if [[ $# -ne 5 ]]; then
    die "usage: $0 BINARY APP_WIDTH APP_HEIGHT SECONDS OUTPUT.mp4"
fi
readonly BINARY=$1
readonly APP_WIDTH=$2
readonly APP_HEIGHT=$3
readonly RUN_SECONDS=$4
readonly OUTPUT=$5

[[ -f Makefile ]] || die "run this from an activity folder (no Makefile in $(pwd))"
for number in "$APP_WIDTH" "$APP_HEIGHT" "$RUN_SECONDS"; do
    [[ $number =~ ^[0-9]+$ ]] || die "APP_WIDTH, APP_HEIGHT and SECONDS must be whole numbers, got '$number'"
done

missing=()
for tool in Xvfb xterm ffmpeg; do
    command -v "$tool" >/dev/null || missing+=("$tool")
done
if [[ ${#missing[@]} -gt 0 ]]; then
    die "missing ${missing[*]}; install with: sudo apt install xvfb xterm ffmpeg fonts-dejavu-core"
fi

# Layout. The program window always opens at the screen's top-left corner
# (there is no window manager to place it), so everything is arranged
# around that: the terminal takes as many rows as fit in the window's
# height and is centered vertically beside it.
readonly TERMINAL_ROWS=$(((APP_HEIGHT - TERMINAL_CHROME) / CELL_HEIGHT))
readonly TERMINAL_WIDTH=$((TERMINAL_COLUMNS * CELL_WIDTH + TERMINAL_CHROME))
readonly TERMINAL_HEIGHT=$((TERMINAL_ROWS * CELL_HEIGHT + TERMINAL_CHROME))
readonly TERMINAL_X=$((APP_WIDTH + WINDOW_GAP))
readonly TERMINAL_Y=$(((APP_HEIGHT - TERMINAL_HEIGHT) / 2))
# x264 with yuv420p needs even frame sizes
readonly SCREEN_WIDTH=$(((TERMINAL_X + TERMINAL_WIDTH + 1) / 2 * 2))
readonly SCREEN_HEIGHT=$(((APP_HEIGHT + 1) / 2 * 2))
((TERMINAL_ROWS >= 16)) || die "program window is too short (${APP_HEIGHT}px) to fit the terminal output"

work_dir=$(mktemp -d)
xvfb_pid=""
ffmpeg_pid=""
terminal_pid=""

cleanup() {
    for pid in "$terminal_pid" "$ffmpeg_pid" "$xvfb_pid"; do
        if [[ -n $pid ]]; then
            kill "$pid" 2>/dev/null || true
        fi
    done
    rm -rf "$work_dir"
}
trap cleanup EXIT

# 1. Virtual screen. -displayfd makes Xvfb pick a free display number and
#    report it once it is ready, so parallel recordings never collide.
Xvfb -displayfd 3 -dpi "$SCREEN_DPI" -screen 0 "${SCREEN_WIDTH}x${SCREEN_HEIGHT}x24" -nolisten tcp \
    3>"$work_dir/display" 2>"$work_dir/xvfb.log" &
xvfb_pid=$!
for _ in $(seq 50); do
    [[ -s $work_dir/display ]] && break
    kill -0 "$xvfb_pid" 2>/dev/null || die "Xvfb failed to start: $(cat "$work_dir/xvfb.log")"
    sleep 0.1
done
[[ -s $work_dir/display ]] || die "Xvfb did not report a display within 5 seconds"
display=":$(head -n1 "$work_dir/display")"

# 2. The script the terminal runs on camera. A plain "$" prompt and relative
#    paths keep the username, hostname and home directory out of the video.
#    MAKEFLAGS etc. are cleared so the outer `make video` does not leak its
#    options into the on-camera build.
cat >"$work_dir/demo.sh" <<EOF
unset MAKEFLAGS MFLAGS MAKELEVEL
prompt() { printf '\033[1;32m\$\033[0m %s\n' "\$*"; sleep 0.8; }
printf '\033[1m%s\033[0m\n' "Build and run from source: \$(basename "\$PWD")"
date
echo
prompt ls; ls; echo
prompt make clean; make clean; echo
prompt make
if ! make; then
    echo 1 >"$work_dir/status"
    sleep 3
    exit 1
fi
echo
prompt ./$BINARY
./$BINARY &
app_pid=\$!
sleep $RUN_SECONDS
kill \$app_pid 2>/dev/null
wait \$app_pid 2>/dev/null
echo 0 >"$work_dir/status"
sleep 1.5
EOF

# 3. Record the raw screen (high quality; it is re-encoded once when scaled).
#    Recording starts before the terminal opens so the whole run is captured.
ffmpeg -loglevel error -y -draw_mouse 0 -f x11grab -video_size "${SCREEN_WIDTH}x${SCREEN_HEIGHT}" \
    -framerate "$FRAMERATE" -i "$display" -c:v libx264 -preset veryfast -crf 12 -pix_fmt yuv420p \
    "$work_dir/raw.mp4" </dev/null &
ffmpeg_pid=$!
sleep 1

# 4. The terminal, which builds and runs the program.
DISPLAY=$display xterm \
    -geometry "${TERMINAL_COLUMNS}x${TERMINAL_ROWS}+${TERMINAL_X}+${TERMINAL_Y}" \
    -fa "$TERMINAL_FONT" -fs "$TERMINAL_FONT_SIZE" -b "$TERMINAL_PADDING" \
    -bg "#1e1e1e" -fg "#d4d4d4" -title "Terminal" -e bash "$work_dir/demo.sh" &
terminal_pid=$!

# 5. Wait for the demo to finish, then stop ffmpeg with SIGINT so it
#    finalizes the recording properly.
deadline=$((SECONDS + BUILD_TIMEOUT_SECONDS + RUN_SECONDS))
while [[ ! -s $work_dir/status ]]; do
    if ! kill -0 "$terminal_pid" 2>/dev/null; then
        die "the terminal exited before the demo finished (is DejaVu Sans Mono installed?)"
    fi
    ((SECONDS < deadline)) || die "demo did not finish within $((BUILD_TIMEOUT_SECONDS + RUN_SECONDS)) seconds"
    sleep 0.2
done
kill -INT "$ffmpeg_pid"
wait "$ffmpeg_pid" || true
ffmpeg_pid=""
build_status=$(cat "$work_dir/status")

# 6. Scale the recording as large as fits inside the margins, then center
#    it on a 1920x1080 black frame.
readonly FIT_WIDTH=$((OUTPUT_WIDTH - 2 * OUTPUT_MARGIN))
readonly FIT_HEIGHT=$((OUTPUT_HEIGHT - 2 * OUTPUT_MARGIN))
ffmpeg -loglevel error -y -i "$work_dir/raw.mp4" \
    -vf "scale=${FIT_WIDTH}:${FIT_HEIGHT}:force_original_aspect_ratio=decrease:force_divisible_by=2:flags=lanczos,pad=${OUTPUT_WIDTH}:${OUTPUT_HEIGHT}:(ow-iw)/2:(oh-ih)/2:black" \
    -c:v libx264 -crf 18 -pix_fmt yuv420p -movflags +faststart "$OUTPUT"

[[ $build_status == 0 ]] || die "the on-camera build failed; see $OUTPUT for the compiler output"
echo "Generated $OUTPUT"

# 7. Optional previews: pixel-exact crops of the program window, taken from
#    the end of the raw recording while the program is running.
if [[ ${RECORD_PREVIEW:-0} == 1 ]]; then
    crop="crop=${APP_WIDTH}:${APP_HEIGHT}:0:0"
    ffmpeg -loglevel error -y -sseof -3 -i "$work_dir/raw.mp4" -vf "$crop" -frames:v 1 -update 1 preview.png
    ffmpeg -loglevel error -y -sseof -6 -t 4 -i "$work_dir/raw.mp4" \
        -vf "$crop,fps=10,scale=500:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse" preview.gif
    echo "Generated preview.png and preview.gif"
fi

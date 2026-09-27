#!/usr/bin/env bash
# Record a build-and-run demo of one activity as an MP4, with no real screen.
#
# The video shows a real terminal (xterm) on the right running `make clean`
# and `make` from source, then launching the program, whose window appears
# on the left and runs for the requested number of seconds.
#
# Usage (run from the activity's folder; the Makefiles' `video` target does this):
#   ../tools/record-demo.sh BINARY SECONDS OUTPUT.mp4
#
# Requires: Xvfb, xterm, ffmpeg, and the DejaVu Sans Mono font.
#   Debian/Ubuntu: sudo apt install xvfb xterm ffmpeg fonts-dejavu-core

set -euo pipefail

readonly SCREEN_WIDTH=1920
readonly SCREEN_HEIGHT=1080
readonly FRAMERATE=30
readonly BUILD_TIMEOUT_SECONDS=300 # upper bound for the on-camera build

# The program window opens at the screen's top-left corner (there is no
# window manager to place it), and every activity window is at most 1000 px
# wide, so the terminal starts just to the right of that area.
readonly TERMINAL_X=1040
readonly TERMINAL_Y=40
readonly TERMINAL_COLUMNS=96
readonly TERMINAL_ROWS=52
readonly TERMINAL_FONT="DejaVu Sans Mono"
readonly TERMINAL_FONT_SIZE=11

die() {
    echo "record-demo: $*" >&2
    exit 1
}

if [[ $# -ne 3 ]]; then
    die "usage: $0 BINARY SECONDS OUTPUT.mp4"
fi
readonly BINARY=$1
readonly RUN_SECONDS=$2
readonly OUTPUT=$3

[[ -f Makefile ]] || die "run this from an activity folder (no Makefile in $(pwd))"
[[ $RUN_SECONDS =~ ^[0-9]+$ ]] || die "SECONDS must be a whole number, got '$RUN_SECONDS'"

missing=()
for tool in Xvfb xterm ffmpeg; do
    command -v "$tool" >/dev/null || missing+=("$tool")
done
if [[ ${#missing[@]} -gt 0 ]]; then
    die "missing ${missing[*]}; install with: sudo apt install xvfb xterm ffmpeg fonts-dejavu-core"
fi

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
Xvfb -displayfd 3 -screen 0 "${SCREEN_WIDTH}x${SCREEN_HEIGHT}x24" -nolisten tcp \
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

# 3. Start recording before the terminal opens so the whole run is captured.
ffmpeg -loglevel error -y -draw_mouse 0 -f x11grab -video_size "${SCREEN_WIDTH}x${SCREEN_HEIGHT}" \
    -framerate "$FRAMERATE" -i "$display" -c:v libx264 -pix_fmt yuv420p "$OUTPUT" \
    </dev/null &
ffmpeg_pid=$!
sleep 1

# 4. The terminal, which builds and runs the program.
DISPLAY=$display xterm \
    -geometry "${TERMINAL_COLUMNS}x${TERMINAL_ROWS}+${TERMINAL_X}+${TERMINAL_Y}" \
    -fa "$TERMINAL_FONT" -fs "$TERMINAL_FONT_SIZE" -bg "#1e1e1e" -fg "#d4d4d4" \
    -title "Terminal" -e bash "$work_dir/demo.sh" &
terminal_pid=$!

# 5. Wait for the demo to finish, then stop ffmpeg with SIGINT so it
#    finalizes the MP4 properly.
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

if [[ $(cat "$work_dir/status") != 0 ]]; then
    die "the on-camera build failed; see $OUTPUT for the compiler output"
fi
echo "Generated $OUTPUT"

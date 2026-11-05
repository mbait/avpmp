#!/bin/sh
# Drive a running AvP instance on a virtual X display (for headless testing).
#
#   avpctl.sh start              start Xvfb (if needed) and the game
#   avpctl.sh keys K [K ...]     send xdotool key names, 0.6s apart
#                                ("sleep:N" waits N seconds, "type:TEXT" types)
#   avpctl.sh shot FILE.png      screenshot the display
#   avpctl.sh marine1            from the profile screen (an existing profile)
#                                start Marine episode 1 (Derelict), Realistic
#   avpctl.sh console CMD        type a console command (debug commands such
#                                as ALIENBOT need AVP_ARGS=-d)
#   avpctl.sh grab               toggle input grab (Ctrl+G); the windowed game
#                                only reads mouse buttons while grabbed
#   avpctl.sh fire SECONDS [BTN] hold a mouse button (1 primary, 3 secondary)
#   avpctl.sh turn left|right S  turn for S seconds (keypad 4/6; arrows strafe)
#   avpctl.sh stop               kill the game (Xvfb is left running)
#
# Environment: AVP_DISPLAY (default :98), AVP_DATA (default ~/avp-data),
#              AVP_BIN (default <repo>/avp), AVP_GL (libGL to load),
#              AVP_LOG (game stdout/stderr, default /tmp/avp-game.log),
#              AVP_ARGS (extra game arguments, e.g. -d for debug commands)
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
: "${AVP_DISPLAY:=:98}"
: "${AVP_DATA:=$HOME/avp-data}"
: "${AVP_BIN:=$HERE/../../avp}"
: "${AVP_GL:=libGL.so.1}"
: "${AVP_LOG:=/tmp/avp-game.log}"
export DISPLAY="$AVP_DISPLAY"

case "$1" in
start)
	if ! xdpyinfo >/dev/null 2>&1; then
		setsid nohup Xvfb "$AVP_DISPLAY" -screen 0 1024x768x24 +extension GLX \
			>/dev/null 2>&1 &
		sleep 2
	fi
	pkill -9 -x avp 2>/dev/null || true
	cd "$AVP_DATA"
	# shellcheck disable=SC2086
	AVP_DATA="$AVP_DATA" setsid "$AVP_BIN" -w -c -g "$AVP_GL" $AVP_ARGS >"$AVP_LOG" 2>&1 &
	sleep "${2:-8}"
	;;
keys)
	shift
	for k in "$@"; do
		case "$k" in
		sleep:*) sleep "${k#sleep:}" ;;
		type:*) xdotool type --delay 80 "${k#type:}"; sleep 0.4 ;;
		*) xdotool key "$k"; sleep 0.6 ;;
		esac
	done
	;;
shot)
	import -window root "$2"
	;;
marine1)
	"$0" keys Return sleep:1.5 Return sleep:1 Down sleep:1 Return sleep:1.5 \
		Return sleep:1.5 Return sleep:"${2:-25}" space sleep:4
	;;
console)
	shift
	"$0" keys grave sleep:1 "type:$*" Return sleep:0.5 grave sleep:1
	;;
grab)
	xdotool key ctrl+g
	sleep 2
	;;
fire)
	W=$(xdotool search --name 'Aliens vs Predator' | head -1)
	eval "$(xdotool getwindowgeometry --shell "$W")"
	xdotool mousemove $((X + WIDTH / 2)) $((Y + HEIGHT / 2))
	xdotool mousedown "${3:-1}"
	sleep "${2:-1}"
	xdotool mouseup "${3:-1}"
	;;
turn)
	k=KP_4
	[ "${2:-left}" = right ] && k=KP_6
	xdotool keydown "$k"
	sleep "${3:-0.5}"
	xdotool keyup "$k"
	;;
stop)
	pkill -x avp 2>/dev/null || true
	;;
*)
	sed -n '2,23p' "$0"; exit 1 ;;
esac

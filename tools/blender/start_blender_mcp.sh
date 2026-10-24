#!/bin/sh
# Start a Blender UI session on a virtual X display with the blender-mcp
# bridge (https://github.com/ahujasid/blender-mcp) listening on localhost:9876
# and the AvP RIF add-on enabled. An MCP client (e.g. Claude Code with
# `claude mcp add blender -- uvx blender-mcp`) can then drive Blender.
#
# Environment: BLENDER (blender binary), BLENDER_DISPLAY (default :97)
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
: "${BLENDER:=blender}"
: "${BLENDER_DISPLAY:=:97}"
export DISPLAY="$BLENDER_DISPLAY"
export BLENDERMCP_NO_UPDATE_CHECK=1

if ! xdpyinfo >/dev/null 2>&1; then
	setsid nohup Xvfb "$BLENDER_DISPLAY" -screen 0 1600x1000x24 +extension GLX \
		>/dev/null 2>&1 &
	sleep 2
fi

exec setsid "$BLENDER" --python "$HERE/mcp_session_setup.py" "$@"

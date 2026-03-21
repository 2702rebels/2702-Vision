#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CALIB_SELECT="$SCRIPT_DIR/calib-select"
SENSOR_CALIB="$SCRIPT_DIR/sensor-calib"

usage() {
    echo "Usage: $0 [options] <image-directory>"
    echo ""
    echo "Run the full calibration pipeline: select optimal images, then calibrate."
    echo ""
    echo "Options:"
    echo "  -s <name>   Sensor/camera name (required, e.g. s1, s2)"
    echo "  -r          Scan directory recursively"
    echo "  -n <N>      Number of images to select (default: 15)"
    echo "  -b <WxH>    Board size as WxH inner corners (default: 9x6)"
    echo "  -v          Verbose - show all image metrics"
    echo "  -h          Show this help"
    echo ""
    echo "Output file is named <sensor>-calib.cam (e.g. s1-calib.cam)"
    exit 1
}

RECURSIVE=""
COUNT=""
BOARD=""
VERBOSE=""
SENSOR=""

while getopts "s:rn:b:vh" opt; do
    case $opt in
        s) SENSOR="$OPTARG" ;;
        r) RECURSIVE="-r" ;;
        n) COUNT="-n $OPTARG" ;;
        b) BOARD="-b $OPTARG" ;;
        v) VERBOSE="-v" ;;
        h) usage ;;
        *) usage ;;
    esac
done
shift $((OPTIND - 1))

if [ $# -lt 1 ]; then
    echo "Error: no image directory specified"
    usage
fi

DIR="$1"

if [ ! -d "$DIR" ]; then
    echo "Error: '$DIR' is not a directory"
    exit 1
fi

if [ -z "$SENSOR" ]; then
    echo "Error: sensor name is required (-s s1, -s s2, etc.)"
    usage
fi

OUTPUT="${SENSOR}-calib.cam"

echo "=== Selecting optimal calibration images ==="
echo ""
$CALIB_SELECT $RECURSIVE $COUNT $BOARD $VERBOSE "$DIR"

echo ""
echo "=== Running calibration ==="
echo ""
SELECTED=$($CALIB_SELECT $RECURSIVE $COUNT $BOARD -p "$DIR" 2>/dev/null)
echo "$SELECTED" | xargs "$SENSOR_CALIB" > "$OUTPUT"

echo ""
echo "Results saved to $OUTPUT"

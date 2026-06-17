#!/bin/bash
set -e
SPECTRUM="../../spectrum/src/spectrum2"
CONFIG="$1"
TEST_BIN="$2"
PORTFILE="${3:-localhost.port}"
TEST_MODE="${4:-irc}"
if [ -z "$CONFIG" ] || [ -z "$TEST_BIN" ]; then
    echo "Usage: $0 <config_file> <integration_test_bin> [portfile] [mode]"
    exit 1
fi
echo "Starting spectrum2 with config: $CONFIG"
$SPECTRUM -n "./$CONFIG" > spectrum2.log 2>&1 &
SPECTRUM_PID=$!
echo "Waiting for spectrum2 frontend..."
for i in $(seq 1 15); do
    if timeout 1 bash -c "echo >/dev/tcp/127.0.0.1/5223" 2>/dev/null; then
        echo "Spectrum2 frontend is listening"; break
    fi
    if [ $i -eq 15 ]; then
        echo "Timeout"; cat spectrum2.log; kill $SPECTRUM_PID 2>/dev/null || true; exit 1
    fi
    sleep 1
done
# Wait for backend portfile (spectrum2 auto-spawns backend)
sleep 3
if [ -f "$PORTFILE" ]; then
    BACKEND_PORT=$(cat "$PORTFILE")
    echo "Backend port: $BACKEND_PORT"
else
    echo "Portfile $PORTFILE not found"
    kill $SPECTRUM_PID 2>/dev/null || true; exit 1
fi
echo "Running integration tests..."
EXIT_CODE=0
"$TEST_BIN" "127.0.0.1" "$BACKEND_PORT" "$TEST_MODE" || EXIT_CODE=$?
kill $SPECTRUM_PID 2>/dev/null || true
wait $SPECTRUM_PID 2>/dev/null || true
if [ $EXIT_CODE -ne 0 ]; then
    echo "--- spectrum2 log ---"; cat spectrum2.log
fi
echo "Integration tests completed with exit code $EXIT_CODE"
exit $EXIT_CODE

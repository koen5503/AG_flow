"""
Phase 5 - Autonomous Testing & Log Analysis
============================================
This script:
1. Runs the native unit tests (pulse-to-volume math & 30ms timer logic)
2. Compiles the ESP32-C3 firmware
3. Optionally uploads firmware and captures serial boot log

Usage:
    python run_tests.py                  # Run tests + compile only
    python run_tests.py --upload         # Also upload and capture serial log
    python run_tests.py --monitor        # Only capture serial log (no build)
"""

import subprocess
import sys
import os
import time
import argparse

LOG_FILE = "debug.log"
SERIAL_PORT = "COM3"
SERIAL_BAUD = 115200
SERIAL_LISTEN_SECONDS = 30


def log(msg):
    """Print and log a message."""
    print(msg, flush=True)
    with open(LOG_FILE, "a") as f:
        f.write(msg + "\n")


def run_command(command, description):
    """Run a shell command, log output, return success boolean."""
    log(f"\n{'='*60}")
    log(f"STEP: {description}")
    log(f"CMD:  {command}")
    log(f"{'='*60}")

    result = subprocess.run(command, shell=True, capture_output=True, text=True)

    with open(LOG_FILE, "a") as f:
        f.write("STDOUT:\n")
        f.write(result.stdout)
        f.write("\nSTDERR:\n")
        f.write(result.stderr)
        f.write(f"\nExit Code: {result.returncode}\n\n")

    # Print condensed output
    for line in result.stdout.splitlines():
        if any(kw in line for kw in ["PASSED", "FAILED", "ERRORED", "Error", "error",
                                      "SUCCESS", "WARNING", "Tests", "succeeded"]):
            log(f"  >> {line.strip()}")

    if result.returncode != 0:
        log(f"  !! FAILED (exit code {result.returncode})")
        # Show last 10 lines of stderr for debugging
        for line in result.stderr.splitlines()[-10:]:
            log(f"  !! {line.strip()}")
        return False

    log(f"  >> OK")
    return True


def capture_serial_log():
    """
    Open the serial port, trigger a reset via DTR, and capture output.
    Saves to debug.log. Works with ESP32-C3 USB-Serial/JTAG.
    """
    try:
        import serial
    except ImportError:
        log("ERROR: pyserial not installed. Run: pip install pyserial")
        return False

    log(f"\n{'='*60}")
    log(f"STEP: Capturing serial output from {SERIAL_PORT} ({SERIAL_LISTEN_SECONDS}s)")
    log(f"{'='*60}")

    try:
        s = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=1)

        # Reset ESP32 via DTR toggle (works with USB-Serial/JTAG on C3)
        s.dtr = False
        s.rts = True
        time.sleep(0.1)
        s.rts = False
        time.sleep(0.1)
        s.dtr = True
        time.sleep(0.5)
        s.reset_input_buffer()

        log("  >> Reset triggered. Listening for serial output...")

        start = time.time()
        lines_captured = 0
        while time.time() - start < SERIAL_LISTEN_SECONDS:
            data = s.readline()
            if data:
                line = data.decode("utf-8", errors="replace").strip()
                if line:
                    elapsed = time.time() - start
                    log(f"  [{elapsed:5.1f}s] {line}")
                    lines_captured += 1

        s.close()
        log(f"  >> Captured {lines_captured} lines in {SERIAL_LISTEN_SECONDS}s")
        return lines_captured > 0

    except Exception as e:
        log(f"  !! Serial error: {e}")
        log(f"  !! Make sure the ESP32-C3 is connected to {SERIAL_PORT}")
        return False


def main():
    global SERIAL_PORT, SERIAL_LISTEN_SECONDS

    parser = argparse.ArgumentParser(description="ESP32-C3 Autonomous Test Runner")
    parser.add_argument("--upload", action="store_true", help="Upload firmware after testing")
    parser.add_argument("--monitor", action="store_true", help="Only capture serial log")
    parser.add_argument("--port", default=SERIAL_PORT, help=f"Serial port (default: {SERIAL_PORT})")
    parser.add_argument("--duration", type=int, default=SERIAL_LISTEN_SECONDS,
                        help=f"Serial listen duration in seconds (default: {SERIAL_LISTEN_SECONDS})")
    args = parser.parse_args()

    SERIAL_PORT = args.port
    SERIAL_LISTEN_SECONDS = args.duration

    # Clear log file
    open(LOG_FILE, "w").close()

    log(f"ESP32-C3 Water Flow Controller - Test Runner")
    log(f"Time: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    log(f"Log file: {os.path.abspath(LOG_FILE)}")

    if args.monitor:
        # Monitor-only mode
        capture_serial_log()
        return

    success = True

    # Step 1: Run native unit tests
    if not run_command(
        "python -m platformio test -e native",
        "Running Unit Tests (native: test_math + test_timer)"
    ):
        success = False

    # Step 2: Compile firmware for ESP32-C3
    if not run_command(
        "python -m platformio run -e seeed_xiao_esp32c3",
        "Compiling firmware for ESP32-C3"
    ):
        success = False

    # Step 3: Upload (optional)
    if args.upload and success:
        if not run_command(
            f"python -m platformio run -e seeed_xiao_esp32c3 --target upload --upload-port {SERIAL_PORT}",
            "Uploading firmware to ESP32-C3"
        ):
            success = False
        else:
            # Wait a moment for the board to reboot after upload
            time.sleep(2)
            capture_serial_log()

    # Summary
    log(f"\n{'='*60}")
    if success:
        log("RESULT: ALL TESTS AND BUILDS PASSED [OK]")
    else:
        log("RESULT: THERE WERE ERRORS [FAIL] - Check debug.log for details")
    log(f"{'='*60}")

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()

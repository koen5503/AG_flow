import subprocess
import sys

def run_command(command, log_file):
    print(f"Running: {command}")
    with open(log_file, "a") as f:
        f.write(f"--- RUNNING: {command} ---\n")
    
    result = subprocess.run(command, shell=True, capture_output=True, text=True)
    
    with open(log_file, "a") as f:
        f.write("STDOUT:\n")
        f.write(result.stdout)
        f.write("\nSTDERR:\n")
        f.write(result.stderr)
        f.write(f"\nExit Code: {result.returncode}\n\n")
        
    print(result.stdout)
    if result.returncode != 0:
        print(f"Command failed with exit code {result.returncode}")
        print(result.stderr)
        return False
    return True

if __name__ == "__main__":
    log_file = "debug.log"
    # Clear log file
    open(log_file, "w").close()

    success = True
    
    # Run tests on native environment
    if not run_command("pio test -e native", log_file):
        success = False
        
    # Test if the ESP32-C3 firmware compiles successfully
    if not run_command("pio run -e seeed_xiao_esp32c3", log_file):
        success = False
        
    if success:
        print("All tests and builds passed successfully!")
        sys.exit(0)
    else:
        print("There were errors. Check debug.log for details.")
        sys.exit(1)

import subprocess
import serial
import time

def main():
    print('Running pio test -e native...')
    subprocess.run(['python', '-m', 'platformio', 'test', '-e', 'native'], check=True)

    print('Running pio run -e seeed_xiao_esp32c3 -t upload...')
    subprocess.run(['python', '-m', 'platformio', 'run', '-e', 'seeed_xiao_esp32c3', '-t', 'upload'], check=True)

    print('Capturing 15 seconds of Serial Monitor output to debug.log...')
    try:
        s = serial.Serial('COM3', 115200, timeout=1)
        s.dtr = False
        s.rts = True
        time.sleep(0.1)
        s.rts = False
        time.sleep(0.1)
        s.dtr = True
        time.sleep(0.5)
        s.reset_input_buffer()

        start = time.time()
        with open('debug.log', 'w') as f:
            while time.time() - start < 40:
                data = s.readline()
                if data:
                    line = data.decode('utf-8', errors='replace').strip()
                    if line:
                        f.write(f'[{time.time()-start:.1f}s] {line}\n')
                        print(line)
        s.close()
    except Exception as e:
        print(f'Error reading serial: {e}')

if __name__ == '__main__':
    main()

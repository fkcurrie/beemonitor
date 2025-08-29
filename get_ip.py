import serial
import time

PORT = '/dev/ttyACM0'
BAUD_RATE = 115200
TIMEOUT = 1.0  # Read timeout in seconds
DURATION = 60  # Total time to listen for in seconds

print(f"--- STARTING SERIAL MONITOR ---")
print(f"Listening on {PORT} at {BAUD_RATE} for {DURATION} seconds...")

try:
    ser = serial.Serial(PORT, BAUD_RATE, timeout=TIMEOUT)
    
    start_time = time.time()
    while time.time() - start_time < DURATION:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(line)
        except serial.SerialException as e:
            print(f"ERROR: Serial read error: {e}")
            break
    
    print(f"--- SERIAL MONITOR FINISHED ---")

except serial.SerialException as e:
    print(f"ERROR: Could not open serial port {PORT}: {e}")
    exit(1)
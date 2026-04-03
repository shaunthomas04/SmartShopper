import serial
import sys
import time
import os

PORT = "COM3"
BAUD = 115200
FILE = sys.argv[1]

if not os.path.exists(FILE):
    print(f"File not found: {FILE}")
    sys.exit(1)

filesize = os.path.getsize(FILE)
print(f"Sending {FILE} ({filesize} bytes) to {PORT}...")

ser = serial.Serial(PORT, BAUD, timeout=5)
time.sleep(3)

ser.reset_input_buffer()
time.sleep(0.5)

ser.write(f"START:{filesize}\n".encode())

resp = ""
timeout = time.time() + 10
while time.time() < timeout:
    line = ser.readline().decode(errors="ignore").strip()
    print(f"  M5: {line}")
    if line == "READY":
        resp = "READY"
        break

if resp != "READY":
    print("M5 not ready, aborting.")
    ser.close()
    sys.exit(1)

print("M5 ready, sending file...")

# Smaller chunks with ACK handshake
CHUNK = 256
sent = 0
with open(FILE, "rb") as f:
    while True:
        chunk = f.read(CHUNK)
        if not chunk:
            break
        ser.write(chunk)
        sent += len(chunk)
        print(f"  {sent}/{filesize} bytes sent", end="\r")

        # Wait for ACK every chunk so M5 can keep up
        ack = ser.readline().decode(errors="ignore").strip()
        if ack != "ACK":
            print(f"\nExpected ACK, got: '{ack}', aborting.")
            ser.close()
            sys.exit(1)

print(f"\nAll sent. Waiting for confirmation...")
timeout = time.time() + 15
while time.time() < timeout:
    line = ser.readline().decode(errors="ignore").strip()
    if line:
        print(f"  M5: {line}")
    if line == "DONE":
        print("Transfer complete!")
        break

ser.close()
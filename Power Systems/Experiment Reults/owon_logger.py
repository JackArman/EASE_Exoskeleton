import serial
import time
import csv
from datetime import datetime
from pathlib import Path

# ---------------- CONFIGURATION ----------------
PORT = "COM5"            # change to your actual port
BAUD = 115200            # default from the manual
CMD  = "MEAS:CURR?"      # or MEAS:VOLT? / MEAS?
SAMPLE_PERIOD = 0.05     # 50 ms between polls (~20 Hz)
TIMEOUT = 0.1            # serial read timeout (s)   
# ------------------------------------------------

# Create a dated log file
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
log_path = Path(f"owon_log_{timestamp}.csv")

# Open serial port
ser = serial.Serial(
    port=PORT,
    baudrate=BAUD,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=TIMEOUT,
    xonxoff=False,
    rtscts=False,
    dsrdtr=False,
)
ser.setDTR(True)
ser.setRTS(True)
ser.reset_input_buffer()
ser.reset_output_buffer()

print(f"Connected to {ser.name}")
print(f"Logging to {log_path}")

# Open CSV file for logging
with open(log_path, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["epoch_s", "iso_time", "value", "raw"])

    print("Starting capture (Ctrl+C to stop)...")
    try:
        next_t = time.perf_counter()
        while True:
            next_t += SAMPLE_PERIOD

            # Send command
            ser.write((CMD + "\r\n").encode("ascii"))
            ser.flush()

            # Read response line
            raw = ser.readline()
            if raw:
                txt = raw.decode("ascii", errors="replace").strip()
                try:
                    val = float(txt)
                except ValueError:
                    val = None
                now = time.time()
                writer.writerow([now, datetime.fromtimestamp(now).isoformat(), val, txt])
                f.flush()

                print(f"{datetime.now().strftime('%H:%M:%S.%f')[:-3]}  {val}")

            # Sleep until next sample
            delay = next_t - time.perf_counter()
            if delay > 0:
                time.sleep(delay)

    except KeyboardInterrupt:
        print("\nStopped by user.")
    finally:
        ser.close()
        print("Serial port closed.")

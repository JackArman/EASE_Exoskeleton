import serial
import time
import csv
import os

# --- Configuration ---
COM_PORT = 'COM3'                     # Change to your ESP32 port
BAUD_RATE = 921600                    # Must match Serial.begin() baud
OUTPUT_FILENAME = 'gait_data_log_' + time.strftime("%Y%m%d_%H%M%S") + '.csv'

def log_serial_data():
    print(f"Starting serial logger...\nSaving to: {OUTPUT_FILENAME}")
    
    try:
        # Open serial connection
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)  # Allow ESP32 boot
        print(f"Connected to {COM_PORT} at {BAUD_RATE} baud.\nPress Ctrl+C to stop.\n")

        # --- Fix: Create directory only if one is specified ---
        dir_name = os.path.dirname(OUTPUT_FILENAME)
        if dir_name:
            os.makedirs(dir_name, exist_ok=True)

        # Open CSV file for writing
        with open(OUTPUT_FILENAME, 'w', newline='', encoding='utf-8') as csvfile:
            writer = csv.writer(csvfile)
            header_written = False

            while True:
                line = ser.readline().decode(errors='ignore').strip()

                if line:
                    # Detect and write header
                    if not header_written and any(c.isalpha() for c in line):
                        writer.writerow(line.split(','))
                        header_written = True
                        print(f"[HEADER] {line}")
                    else:
                        writer.writerow(line.split(','))
                        csvfile.flush()
                        print(f"[LOG] {line}")

    except serial.SerialException as e:
        print(f"\n[ERROR] Could not open port {COM_PORT}. "
              f"Check port number and close Arduino Serial Monitor.\nDetails: {e}")

    except KeyboardInterrupt:
        print("\nLogging stopped by user (Ctrl+C).")

    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Serial connection closed.")
        print(f"Data saved to {OUTPUT_FILENAME}")

if __name__ == '__main__':
    log_serial_data()

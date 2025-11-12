import serial
import time
import csv
import os

# --- Configuration ---
COM_PORT = 'COM3'                     # Change to your ESP32 port
BAUD_RATE = 921600                    # Must match Serial.begin() baud
OUTPUT_FILENAME = 'gait_data_log_' + time.strftime("%Y%m%d_%H%M%S") + '.csv'

def looks_like_data(fields):
    if len(fields) < 4:
        return False
    try:
        int(fields[0]); int(fields[1]); int(fields[2])
        return True
    except ValueError:
        return False

def make_byte_labels(prefix, n=8):
    return [f"{prefix}_{i}" for i in range(n)]

def expand_compact_header(fields):
    """
    Expand RH[8], RK[8], LK[8], LH[8] into 8 columns each.
    Returns expanded list and expected column count.
    """
    expanded = []
    for f in fields:
        f = f.strip()
        if f == "RH[8]":
            expanded += make_byte_labels("RH", 8)
        elif f == "RK[8]":
            expanded += make_byte_labels("RK", 8)
        elif f == "LK[8]":
            expanded += make_byte_labels("LK", 8)
        elif f == "LH[8]":
            expanded += make_byte_labels("LH", 8)
        else:
            expanded.append(f)
    return expanded

def infer_header_for_width(ncols):
    # 36 -> TimeStep, Elapsed_us, L_Gait_Index, R_Gait_Index + 32 bytes
    # 35 -> TimeStep, L_Gait_Index, R_Gait_Index + 32 bytes (legacy)
    if ncols == 36:
        return ["TimeStep", "Elapsed_us", "L_Gait_Index", "R_Gait_Index"] + \
               make_byte_labels("RH", 8) + make_byte_labels("RK", 8) + \
               make_byte_labels("LK", 8) + make_byte_labels("LH", 8)
    if ncols == 35:
        return ["TimeStep", "L_Gait_Index", "R_Gait_Index"] + \
               make_byte_labels("RH", 8) + make_byte_labels("RK", 8) + \
               make_byte_labels("LK", 8) + make_byte_labels("LH", 8)
    return None

def log_serial_data():
    print(f"Starting serial logger...\nSaving to: {OUTPUT_FILENAME}")
    
    try:
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)  # Allow ESP32 boot
        print(f"Connected to {COM_PORT} at {BAUD_RATE} baud.\nPress Ctrl+C to stop.\n")

        dir_name = os.path.dirname(OUTPUT_FILENAME)
        if dir_name:
            os.makedirs(dir_name, exist_ok=True)

        with open(OUTPUT_FILENAME, 'w', newline='', encoding='utf-8') as csvfile:
            writer = csv.writer(csvfile)
            header_written = False
            expected_cols = None
            warned_once = False

            while True:
                raw = ser.readline()
                if not raw:
                    continue

                line = raw.decode(errors='ignore').strip()
                if not line:
                    continue

                # Skip startup chatter
                if "Multi-joint gait tracking started" in line:
                    continue

                fields = [f.strip() for f in line.split(',') if f != ""]

                # Accept explicit header (compact or expanded) only if it starts with TimeStep
                if not header_written and fields and fields[0].lower() == "timestep":
                    # If compact tokens like RH[8] are present, expand them to 32 columns
                    if any(tok.endswith("[8]") for tok in fields):
                        expanded = expand_compact_header(fields)
                        writer.writerow(expanded)
                        expected_cols = len(expanded)
                        print(f"[HEADER] (expanded) {','.join(expanded)}")
                    else:
                        writer.writerow(fields)
                        expected_cols = len(fields)
                        print(f"[HEADER] {line}")
                    csvfile.flush()
                    header_written = True
                    continue

                # If no header yet, infer one from the first numeric row
                if not header_written:
                    if looks_like_data(fields):
                        inferred = infer_header_for_width(len(fields))
                        if inferred:
                            writer.writerow(inferred)
                            csvfile.flush()
                            header_written = True
                            expected_cols = len(inferred)
                            print(f"[HEADER] (inferred) {','.join(inferred)}")
                            # fall through to write this row below
                        else:
                            # Unexpected width; wait for a proper row/header
                            continue
                    else:
                        # Not a header, not data → skip
                        continue

                # Now we have a header; write rows and only warn once if width mismatches
                if expected_cols is not None and len(fields) != expected_cols:
                    if not warned_once:
                        print(f"[WARN] Column count {len(fields)} != expected {expected_cols}. "
                              f"Suppressing further warnings.")
                        warned_once = True
                    # Still write the row to avoid data loss
                    writer.writerow(fields)
                else:
                    writer.writerow(fields)

                csvfile.flush()
                # Optional: comment out to reduce console spam
                # print(f"[LOG] {line}")

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

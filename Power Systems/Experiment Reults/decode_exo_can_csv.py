#!/usr/bin/env python3
"""
Decode CubeMars CAN feedback bytes logged by the Arduino gait script.

Usage (CLI as before):
  python decode_exo_can_csv.py input.csv -o decoded.csv --pole-pairs 7

Or just set DEFAULT_INPUT_PATH below and run without CLI args.
"""

import argparse
import csv
from pathlib import Path

# >>> EDIT THIS LINE: put your CSV path here (leave "" to use CLI argument)
DEFAULT_INPUT_PATH = r"C:\Users\india\OneDrive\Desktop\Thesis\Experiment Reults\Experiment1\gait_data_log_20251111_173516.csv"  # e.g., r"C:\Users\you\Desktop\input.csv" or r"/home/you/input.csv"

MOTOR_ORDER = ["RightHip", "RightKnee", "LeftKnee", "LeftHip"]

ERROR_MAP = {
    0: "OK",
    1: "Motor Over-Temp",
    2: "Over-Current",
    3: "Over-Voltage",
    4: "Under-Voltage",
    5: "Encoder Fault",
    6: "MOSFET Over-Temp",
    7: "Motor Lock",
}

def to_int16(hi, lo):
    v = ((hi & 0xFF) << 8) | (lo & 0xFF)
    if v >= 0x8000:
        v -= 0x10000
    return v

def to_int8(b):
    return b - 256 if b > 127 else b

def decode_block(block8):
    if len(block8) != 8:
        raise ValueError("Each motor block must have 8 bytes")
    pos_raw = to_int16(block8[0], block8[1])
    spd_raw = to_int16(block8[2], block8[3])
    cur_raw = to_int16(block8[4], block8[5])
    temp    = to_int8(block8[6])
    err     = block8[7] & 0xFF
    return {
        "pos_deg":   pos_raw * 0.1,     # degrees
        "spd_erpm":  spd_raw * 10.0,    # electrical RPM
        "cur_A":     cur_raw * 0.01,    # amps
        "temp_C":    temp,
        "err_code":  err,
        "err_text":  ERROR_MAP.get(err, f"Unknown({err})"),
    }

def maybe_mech_rpm(spd_erpm, pole_pairs):
    return (spd_erpm / float(pole_pairs)) if pole_pairs else None

def main():
    ap = argparse.ArgumentParser()
    # If DEFAULT_INPUT_PATH is set, make the positional arg optional and default to it
    if DEFAULT_INPUT_PATH:
        ap.add_argument("input_csv", nargs="?", type=Path, default=Path(DEFAULT_INPUT_PATH))
    else:
        ap.add_argument("input_csv", type=Path)

    ap.add_argument("-o", "--output", type=Path, default=None, help="Output CSV (default: <input>_decoded.csv)")
    ap.add_argument("--pole-pairs", type=int, default=21,
                    help="Pole pairs for mechanical RPM conversion (e.g., 7)")

    args = ap.parse_args()

    out_path = args.output or args.input_csv.with_name(args.input_csv.stem + "_decoded.csv")

    with args.input_csv.open("r", newline="") as f_in, out_path.open("w", newline="") as f_out:
        reader = csv.reader(f_in)
        writer = csv.writer(f_out)

        base_cols = ["TimeStep", "L_Gait_Index", "R_Gait_Index"]
        motor_cols = []
        for m in MOTOR_ORDER:
            motor_cols += [
                f"{m}_pos_deg",
                f"{m}_spd_eRPM",
            ]
            if args.pole_pairs:
                motor_cols += [f"{m}_spd_mech_RPM"]
            motor_cols += [
                f"{m}_current_A",
                f"{m}_temp_C",
                f"{m}_err_code",
                f"{m}_err_text",
            ]
        writer.writerow(base_cols + motor_cols)

        first = True
        for row in reader:
            if not row:
                continue
            if first and row[0].strip().lower() == "timestep":
                first = False
                continue
            first = False

            if len(row) < 3 + 32:
                # Skip legacy/short rows gracefully
                continue

            try:
                timestep = int(row[0]); L_idx = int(row[1]); R_idx = int(row[2])
            except ValueError:
                continue

            try:
                bytes_all = list(map(int, row[3:3+32]))
            except ValueError:
                continue

            decoded = []
            for mi, _motor in enumerate(MOTOR_ORDER):
                blk = bytes_all[mi*8:(mi+1)*8]
                dd = decode_block(blk)
                mech_rpm = maybe_mech_rpm(dd["spd_erpm"], args.pole_pairs)
                decoded.extend([
                    round(dd["pos_deg"], 3),
                    round(dd["spd_erpm"], 3),
                ])
                if args.pole_pairs:
                    decoded.append(round(mech_rpm, 3))
                decoded.extend([
                    round(dd["cur_A"], 3),
                    int(dd["temp_C"]),
                    int(dd["err_code"]),
                    dd["err_text"],
                ])

            writer.writerow([timestep, L_idx, R_idx] + decoded)

    print(f"Wrote: {out_path}")

if __name__ == "__main__":
    main()

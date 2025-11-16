#!/usr/bin/env python3
"""
Decode CubeMars CAN feedback bytes logged by the Arduino gait script.

STRICT: expects rows with Elapsed_us:
  [TimeStep, Elapsed_us, L_idx, R_idx, 32 bytes...]

Usage:
  python decode_exo_can_csv.py input.csv -o decoded.csv --pole-pairs 7
"""

import argparse
import csv
from pathlib import Path

# >>> EDIT THIS LINE: put your CSV path here (leave "" to use CLI argument)
DEFAULT_INPUT_PATH = r"C:\Users\india\Downloads\thesis\EASE_Exoskeleton\Power Systems\Experiment Reults\Experiment2\gait_data_log_20251114_161409.csv"

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

def _parse_row_strict_elapsed(row):
    """
    STRICT parser: require
      [TimeStep, Elapsed_us, L_idx, R_idx, 32 bytes...]

    Returns (timestep:int, elapsed_us:int, L_idx:int, R_idx:int, bytes32:list[int]) or None.
    """
    row = [x.strip() for x in row if x.strip() != ""]
    if not row:
        return None

    # Skip header row if present (must start with 'TimeStep')
    if row[0].lower() == "timestep":
        return None

    # Need at least 36 fields: 4 scalars + 32 bytes
    if len(row) < 36:
        return None

    try:
        timestep    = int(row[0])
        elapsed_us  = int(row[1])
        L_idx       = int(row[2])
        R_idx       = int(row[3])

        bytes_str   = row[4:4+32]
        if len(bytes_str) != 32:
            return None

        bytes_all = [int(b) for b in bytes_str]
        if any((b < 0 or b > 255) for b in bytes_all):
            return None

        return (timestep, elapsed_us, L_idx, R_idx, bytes_all)

    except (ValueError, IndexError):
        return None

def main():
    ap = argparse.ArgumentParser()
    if DEFAULT_INPUT_PATH:
        ap.add_argument("input_csv", nargs="?", type=Path, default=Path(DEFAULT_INPUT_PATH))
    else:
        ap.add_argument("input_csv", type=Path)

    ap.add_argument("-o", "--output", type=Path, default=None,
                    help="Output CSV (default: <input>_decoded.csv)")
    ap.add_argument("--pole-pairs", type=int, default=21,
                    help="Pole pairs for mechanical RPM conversion (e.g., 7)")

    args = ap.parse_args()
    out_path = args.output or args.input_csv.with_name(args.input_csv.stem + "_decoded.csv")

    with args.input_csv.open("r", newline="") as f_in, out_path.open("w", newline="") as f_out:
        reader = csv.reader(f_in)
        writer = csv.writer(f_out)

        # Output header NOW includes Elapsed_us
        base_cols = ["TimeStep", "Elapsed_us", "L_Gait_Index", "R_Gait_Index"]
        motor_cols = []
        for m in MOTOR_ORDER:
            motor_cols += [f"{m}_pos_deg", f"{m}_spd_eRPM"]
            if args.pole_pairs:
                motor_cols += [f"{m}_spd_mech_RPM"]
            motor_cols += [f"{m}_current_A", f"{m}_temp_C", f"{m}_err_code", f"{m}_err_text"]
        writer.writerow(base_cols + motor_cols)

        for row in reader:
            parsed = _parse_row_strict_elapsed(row)
            if parsed is None:
                continue  # skip anything that isn't strict-elapsed layout

            timestep, elapsed_us, L_idx, R_idx, bytes_all = parsed

            decoded = []
            for mi, _motor in enumerate(MOTOR_ORDER):
                blk = bytes_all[mi*8:(mi+1)*8]
                if len(blk) != 8:
                    decoded = None
                    break
                dd = decode_block(blk)
                mech_rpm = maybe_mech_rpm(dd["spd_erpm"], args.pole_pairs)
                decoded.extend([round(dd["pos_deg"], 3), round(dd["spd_erpm"], 3)])
                if args.pole_pairs:
                    decoded.append(round(mech_rpm, 3))
                decoded.extend([round(dd["cur_A"], 3), int(dd["temp_C"]), int(dd["err_code"]), dd["err_text"]])

            if decoded is None:
                continue

            writer.writerow([timestep, elapsed_us, L_idx, R_idx] + decoded)

    print(f"Wrote: {out_path}")

if __name__ == "__main__":
    main()

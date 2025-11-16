#!/usr/bin/env python3
"""
Plot paired graphs for Right/Left Hip and Right/Left Knee from the decoded exoskeleton CSV.

Generates separate PNGs (per metric) for hips and knees:
- Mechanical RPM (preferred) or Electrical RPM (optionally converted)
- Current (A)
- Temperature (°C)
- Position (deg)

Usage examples:
  python plot_joint_pairs.py
  python plot_joint_pairs.py -i Experiment1/input_decoded.csv
  python plot_joint_pairs.py -i Experiment1/input_decoded.csv --pole-pairs 7
  python plot_joint_pairs.py --invert-left --phase-hip 5 --phase-knee -3
"""

from pathlib import Path
import argparse
import pandas as pd
import matplotlib.pyplot as plt

DEFAULT_INPUT  = Path(__file__).parent / "Experiment2" / "gait_data_log_20251114_161409_decoded.csv"
DEFAULT_OUTDIR = Path(__file__).parent / "Experiment2"

MOTOR_NAMES = {
    "hip":  ("RightHip", "LeftHip"),
    "knee": ("RightKnee", "LeftKnee"),
}

# Column bases produced by your decoder
COLS = {
    "pos":      "pos_deg",
    "spd_erpm": "spd_eRPM",
    "spd_mrpm": "spd_mech_RPM",
    "cur":      "current_A",
    "temp":     "temp_C",
}

def pick_speed_columns(df, pole_pairs):
    """
    Decide which speed columns to use and return (label, right_col, left_col, converter or None).
    Priority:
      1) *_spd_mech_RPM if present
      2) *_spd_eRPM with conversion if pole_pairs given
      3) *_spd_eRPM raw (electrical RPM)
    """
    have_mech = all(f"{m}_{COLS['spd_mrpm']}" in df.columns
                    for pair in MOTOR_NAMES.values() for m in pair)
    have_elec = all(f"{m}_{COLS['spd_erpm']}" in df.columns
                    for pair in MOTOR_NAMES.values() for m in pair)

    if have_mech:
        label = "Mechanical RPM"
        suffix = COLS["spd_mrpm"]
        conv = None
    elif have_elec and pole_pairs:
        label = f"Mechanical RPM (converted from eRPM, pole pairs = {pole_pairs})"
        suffix = COLS["spd_erpm"]
        conv = (lambda s: s / float(pole_pairs))
    elif have_elec:
        label = "Electrical RPM"
        suffix = COLS["spd_erpm"]
        conv = None
    else:
        label = None
        suffix = None
        conv = None
    return label, suffix, conv

def shift_series(s, n):
    return s.shift(n) if n else s

def plot_pair(df, right_name, left_name, y_right, y_left, x, title, ylabel, out_path, x_label):
    plt.figure(figsize=(12, 6))
    plt.plot(x, y_right, label=right_name)
    plt.plot(x, y_left,  label=left_name)
    plt.xlabel(x_label)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.legend()
    plt.tight_layout()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(out_path, dpi=150)
    plt.close()
    print(f"Saved: {out_path}")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-i", "--input", type=Path, default=DEFAULT_INPUT,
                    help="Path to decoded CSV (default: ./Experiment1/input_decoded.csv)")
    ap.add_argument("-o", "--outdir", type=Path, default=DEFAULT_OUTDIR,
                    help="Directory to save PNGs (default: ./Experiment1)")
    ap.add_argument("--pole-pairs", type=int, default=None,
                    help="Pole pairs for converting eRPM to mechanical RPM (if needed)")
    ap.add_argument("--downsample", type=int, default=1,
                    help="Plot every Nth sample (default: 1 = no downsample)")
    ap.add_argument("--invert-left", action="store_true",
                    help="Invert Left side SPEED sign (multiply by -1) to account for reversed motor orientation")
    ap.add_argument("--phase-hip", type=int, default=0,
                    help="Shift LeftHip by N samples (+N forward, -N backward)")
    ap.add_argument("--phase-knee", type=int, default=0,
                    help="Shift LeftKnee by N samples (+N forward, -N backward)")
    args = ap.parse_args()

    df = pd.read_csv(args.input)

    # ==== X-axis: prefer Elapsed_us (→ seconds), else TimeStep, else index ====
    if "Elapsed_us" in df.columns:
        x = df["Elapsed_us"] * 1e-6
        x_label = "Time (s)"
    elif "TimeStep" in df.columns:
        x = df["TimeStep"]
        x_label = "TimeStep"
    else:
        x = df.index
        x_label = "Sample"

    # Downsample index for speed
    idx = slice(None, None, args.downsample)
    x = x[idx]

    # Decide speed columns and optional converter
    speed_label, speed_suffix, speed_conv = pick_speed_columns(df, args.pole_pairs)

    # ============ SPEED (RPM) ============
    if speed_suffix:
        for group_key, (right, left) in MOTOR_NAMES.items():
            rcol = f"{right}_{speed_suffix}"
            lcol = f"{left}_{speed_suffix}"

            if rcol in df.columns and lcol in df.columns:
                r = df[rcol].copy()
                l = df[lcol].copy()

                # Optional conversion from eRPM -> mech
                if speed_conv:
                    r = speed_conv(r)
                    l = speed_conv(l)

                # Optional inversion of LEFT (speed only)
                if args.invert_left:
                    l = -l

                # Optional phase shift for LEFT per group
                phase = args.phase_hip if group_key == "hip" else args.phase_knee
                if phase:
                    l = shift_series(l, phase)
                    shift_note = f" (Left shifted by {phase})"
                else:
                    shift_note = ""

                title = f"{group_key.capitalize()} {speed_label}{shift_note}"
                out = args.outdir / f"{group_key.capitalize()}_MechRPM.png" if "Mechanical" in speed_label \
                      else args.outdir / f"{group_key.capitalize()}_ElecRPM.png"

                plot_pair(df, right, left, r[idx], l[idx], x, title, speed_label, out, x_label)
    else:
        print("Speed columns not found; skipping RPM plots.")

    # ============ CURRENT (A) ============
    for group_key, (right, left) in MOTOR_NAMES.items():
        rcol = f"{right}_{COLS['cur']}"
        lcol = f"{left}_{COLS['cur']}"
        if rcol in df.columns and lcol in df.columns:
            r = df[rcol]
            l = df[lcol]
            title = f"{group_key.capitalize()} Current"
            out = args.outdir / f"{group_key.capitalize()}_CurrentA.png"
            plot_pair(df, right, left, r[idx], l[idx], x, title, "Current (A)", out, x_label)
        else:
            print(f"Current columns missing for {group_key}; skipping.")

    # ============ TEMPERATURE (°C) ============
    for group_key, (right, left) in MOTOR_NAMES.items():
        rcol = f"{right}_{COLS['temp']}"
        lcol = f"{left}_{COLS['temp']}"
        if rcol in df.columns and lcol in df.columns:
            r = df[rcol]
            l = df[lcol]
            title = f"{group_key.capitalize()} Temperature"
            out = args.outdir / f"{group_key.capitalize()}_Temperature.png"
            plot_pair(df, right, left, r[idx], l[idx], x, title, "Temperature (°C)", out, x_label)
        else:
            print(f"Temperature columns missing for {group_key}; skipping.")

    # ============ POSITION (deg) ============
    for group_key, (right, left) in MOTOR_NAMES.items():
        rcol = f"{right}_{COLS['pos']}"
        lcol = f"{left}_{COLS['pos']}"
        if rcol in df.columns and lcol in df.columns:
            r = df[rcol]
            l = df[lcol]
            title = f"{group_key.capitalize()} Position"
            out = args.outdir / f"{group_key.capitalize()}_PositionDeg.png"
            plot_pair(df, right, left, r[idx], l[idx], x, title, "Position (deg)", out, x_label)
        else:
            print(f"Position columns missing for {group_key}; skipping.")

    # ============ TORQUE (N·m) from Current (ADDED) ============
    # Build a seconds x-axis regardless of earlier choice, without altering earlier plots
    if "Elapsed_us" in df.columns:
        x_sec_full = df["Elapsed_us"] * 1e-6
    elif "epoch_s" in df.columns:
        x_sec_full = df["epoch_s"] - float(df["epoch_s"].iloc[0])
    elif "iso_time" in df.columns:
        t = pd.to_datetime(df["iso_time"], errors="coerce")
        x_sec_full = (t - t.iloc[0]).dt.total_seconds()
    else:
        x_sec_full = pd.Series(range(len(df)), index=df.index, dtype=float)  # fallback: sample index as seconds

    x_sec = x_sec_full[idx]

    # Torque constant (N·m/A). Adjust if using a different motor model.
    Kt = 0.159

    for group_key, (right, left) in MOTOR_NAMES.items():
        rcur = f"{right}_{COLS['cur']}"
        lcur = f"{left}_{COLS['cur']}"
        if rcur in df.columns and lcur in df.columns:
            r_tau = df[rcur] * Kt
            l_tau = df[lcur] * Kt
            title = f"{group_key.capitalize()} Torque"
            out   = args.outdir / f"{group_key.capitalize()}_TorqueNm.png"
            # Force x-axis label to seconds for these torque plots
            plot_pair(df, right, left, r_tau[idx], l_tau[idx], x_sec, title, "Torque (N·m)", out, "Time (s)")
        else:
            print(f"Current columns missing for {group_key}; skipping torque plots.")

if __name__ == "__main__":
    main()

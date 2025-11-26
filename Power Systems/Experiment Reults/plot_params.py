#!/usr/bin/env python3
"""
Plot paired graphs for Right/Left Hip and Right/Left Knee from the decoded exoskeleton CSV.

Now generates ONE combined PNG with 10 subplots arranged as:
4 rows x 3 columns (12 axes, last 2 unused):

  [0] Hip Speed       [1] Knee Speed      [2] Hip Current
  [3] Knee Current    [4] Hip Temp        [5] Knee Temp
  [6] Hip Position    [7] Knee Position   [8] Hip Torque
  [9] Knee Torque     [10] (unused)       [11] (unused)

Usage examples:
  python plot_joint_pairs_grid.py
  python plot_joint_pairs_grid.py -i Experiment1/input_decoded.csv
  python plot_joint_pairs_grid.py -i Experiment1/input_decoded.csv --pole-pairs 7
  python plot_joint_pairs_grid.py --invert-left --phase-hip 5 --phase-knee -3
"""

from pathlib import Path
import argparse
import pandas as pd
import matplotlib.pyplot as plt

DEFAULT_INPUT  = Path(__file__).parent / "Experiment1" / "gait_data_log_20251119_152238_decoded.csv"
DEFAULT_OUTDIR = Path(__file__).parent

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
    Decide which speed columns to use and return (label, suffix, converter or None).
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

    # === LIMIT TO A TIME WINDOW (seconds) ===
    START_T = 50.0   # change as needed
    END_T   = 150.0  # change as needed

    # Build a seconds timeline from available columns
    if "Elapsed_us" in df.columns:
        t_sec = df["Elapsed_us"] * 1e-6
    elif "epoch_s" in df.columns:
        t0 = df["epoch_s"].iloc[0]
        t_sec = df["epoch_s"] - t0
    elif "iso_time" in df.columns:
        t = pd.to_datetime(df["iso_time"], errors="coerce")
        t_sec = (t - t.iloc[0]).dt.total_seconds()
    else:
        print("WARNING: No time column found; cannot trim data.")
        t_sec = None

    if t_sec is not None:
        mask = (t_sec >= START_T) & (t_sec <= END_T)
        df = df.loc[mask].reset_index(drop=True)

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

    # Build a seconds x-axis for torque specifically
    if "Elapsed_us" in df.columns:
        x_sec_full = df["Elapsed_us"] * 1e-6
    elif "epoch_s" in df.columns:
        x_sec_full = df["epoch_s"] - float(df["epoch_s"].iloc[0])
    elif "iso_time" in df.columns:
        t = pd.to_datetime(df["iso_time"], errors="coerce")
        x_sec_full = (t - t.iloc[0]).dt.total_seconds()
    else:
        x_sec_full = pd.Series(range(len(df)), index=df.index, dtype=float)
    x_sec = x_sec_full[idx]

    # Torque constant & transmission
    Kt = 0.159
    gearRatio = 9.0
    motorEfficiency = 0.8

    # ================================================================
    #               BUILD ALL PLOTS INTO ONE GRID
    #            (4 rows × 3 columns = 12 subplots)
    # ================================================================
    args.outdir.mkdir(parents=True, exist_ok=True)
    fig, axes = plt.subplots(4, 3, figsize=(24, 16))
    axes = axes.flatten()

    # Layout map:
    # 0: hip speed,    1: knee speed,   2: hip current
    # 3: knee current, 4: hip temp,     5: knee temp
    # 6: hip pos,      7: knee pos,     8: hip torque
    # 9: knee torque,  10: (unused),    11: (unused)

    # ---------------- SPEED PLOTS ----------------
    if speed_suffix:
        for i, group_key in enumerate(["hip", "knee"]):
            right, left = MOTOR_NAMES[group_key]
            rcol = f"{right}_{speed_suffix}"
            lcol = f"{left}_{speed_suffix}"
            ax = axes[i]  # 0 for hip, 1 for knee

            if rcol in df.columns and lcol in df.columns:
                r = df[rcol].copy()
                l = df[lcol].copy()

                if speed_conv:
                    r = speed_conv(r)
                    l = speed_conv(l)

                if args.invert_left:
                    l = -l

                phase = args.phase_hip if group_key == "hip" else args.phase_knee
                if phase:
                    l = shift_series(l, phase)

                ax.plot(x, r[idx], label=right)
                ax.plot(x, l[idx], label=left)
                ax.set_title(f"{group_key.capitalize()} {speed_label}")
                ax.set_ylabel("RPM")
                ax.set_xlabel(x_label)
                ax.legend()
            else:
                ax.set_title(f"{group_key.capitalize()} {speed_label} (missing cols)")
                ax.set_xlabel(x_label)
    else:
        print("Speed columns not found; leaving speed plots blank.")

    # ---------------- CURRENT PLOTS ----------------
    for i, group_key in enumerate(["hip", "knee"]):
        right, left = MOTOR_NAMES[group_key]
        rcol = f"{right}_{COLS['cur']}"
        lcol = f"{left}_{COLS['cur']}"
        ax = axes[2 + i]  # 2: hip current, 3: knee current

        if rcol in df.columns and lcol in df.columns:
            ax.plot(x, df[rcol][idx], label=right)
            ax.plot(x, df[lcol][idx], label=left)
            ax.set_title(f"{group_key.capitalize()} Current")
            ax.set_ylabel("Current (A)")
            ax.set_xlabel(x_label)
            ax.legend()
        else:
            ax.set_title(f"{group_key.capitalize()} Current (missing cols)")
            ax.set_xlabel(x_label)

    # ---------------- TEMPERATURE PLOTS ----------------
    for i, group_key in enumerate(["hip", "knee"]):
        right, left = MOTOR_NAMES[group_key]
        rcol = f"{right}_{COLS['temp']}"
        lcol = f"{left}_{COLS['temp']}"
        ax = axes[4 + i]  # 4: hip temp, 5: knee temp

        if rcol in df.columns and lcol in df.columns:
            ax.plot(x, df[rcol][idx], label=right)
            ax.plot(x, df[lcol][idx], label=left)
            ax.set_title(f"{group_key.capitalize()} Temperature")
            ax.set_ylabel("Temperature (°C)")
            ax.set_xlabel(x_label)
            ax.legend()
        else:
            ax.set_title(f"{group_key.capitalize()} Temperature (missing cols)")
            ax.set_xlabel(x_label)

    # ---------------- POSITION PLOTS ----------------
    for i, group_key in enumerate(["hip", "knee"]):
        right, left = MOTOR_NAMES[group_key]
        rcol = f"{right}_{COLS['pos']}"
        lcol = f"{left}_{COLS['pos']}"
        ax = axes[6 + i]  # 6: hip pos, 7: knee pos

        if rcol in df.columns and lcol in df.columns:
            ax.plot(x, df[rcol][idx], label=right)
            ax.plot(x, df[lcol][idx], label=left)
            ax.set_title(f"{group_key.capitalize()} Position")
            ax.set_ylabel("Position (deg)")
            ax.set_xlabel(x_label)
            ax.legend()
        else:
            ax.set_title(f"{group_key.capitalize()} Position (missing cols)")
            ax.set_xlabel(x_label)

    # ---------------- TORQUE PLOTS ----------------
    for i, group_key in enumerate(["hip", "knee"]):
        right, left = MOTOR_NAMES[group_key]
        rcur = f"{right}_{COLS['cur']}"
        lcur = f"{left}_{COLS['cur']}"
        ax = axes[8 + i]  # 8: hip torque, 9: knee torque

        if rcur in df.columns and lcur in df.columns:
            r_tau = df[rcur] * Kt * gearRatio * motorEfficiency
            l_tau = df[lcur] * Kt * gearRatio * motorEfficiency
            ax.plot(x_sec[idx], r_tau[idx], label=right)
            ax.plot(x_sec[idx], l_tau[idx], label=left)
            ax.set_title(f"{group_key.capitalize()} Torque")
            ax.set_ylabel("Torque (N·m)")
            ax.set_xlabel("Time (s)")
            ax.legend()
        else:
            ax.set_title(f"{group_key.capitalize()} Torque (missing cols)")
            ax.set_xlabel("Time (s)")

    # You can optionally hide the two unused axes:
    for j in [10, 11]:
        axes[j].axis("off")

    plt.tight_layout()
    out_path = args.outdir / "Experiment8Results.png"
    plt.savefig(out_path, dpi=75)
    plt.close()
    print(f"Saved combined figure: {out_path}")


if __name__ == "__main__":
    main()

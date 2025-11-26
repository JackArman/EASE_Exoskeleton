#!/usr/bin/env python3
"""
Sync BMS log with exoskeleton gait CSV + DMM current CSV, compute
power + energy + efficiency, and plot annotated graphs.

Adds:
 - Total electrical energy (Wh)
 - Mechanical energy from gait (Wh)
 - Average system efficiency
 - Battery drain rate (W, A)
 - Horizontal lines for averages
 - Linear regression fits
"""

import argparse
from pathlib import Path
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt


def parse_hhmmss(hhmmss_str: str):
    s = hhmmss_str.strip()
    if len(s) != 6 or not s.isdigit():
        raise ValueError("Time must be 6 digits in HHMMSS format, e.g. 162032")
    hh = int(s[0:2])
    mm = int(s[2:4])
    ss = int(s[4:6])
    if not (0 <= hh < 24 and 0 <= mm < 60 and 0 <= ss < 60):
        raise ValueError("Invalid HHMMSS time")
    return hh, mm, ss


def linear_fit(x, y):
    """Return slope m and intercept b for y = mx + b."""
    m, b = np.polyfit(x, y, 1)
    return m, b


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("bms_csv", type=Path)
    ap.add_argument("gait_csv", type=Path)
    ap.add_argument("current_csv", type=Path)
    ap.add_argument("--save", action="store_true")
    ap.add_argument("--output", type=Path, default=None)
    args = ap.parse_args()

    # ----------------------- LOAD BMS -----------------------
    bms = pd.read_csv(args.bms_csv)
    bms.columns = [c.strip() for c in bms.columns]

    if "Date & Time" not in bms.columns:
        raise SystemExit("BMS CSV missing 'Date & Time' column")

    # find voltage column
    v_col = None
    for name in ["Battery Voltage", "BatteryVoltage", "Pack Voltage", "Voltage"]:
        if name in bms.columns:
            v_col = name
            break
    if v_col is None:
        raise SystemExit("No voltage column found in BMS CSV")

    bms["DateTime"] = pd.to_datetime(
        bms["Date & Time"].astype(str).str.strip(), errors="coerce"
    )
    if bms["DateTime"].isna().all():
        raise SystemExit("Cannot parse BMS timestamps")

    # ensure naive datetimes (no timezone)
    bms["DateTime"] = bms["DateTime"].astype("datetime64[ns]")

    # ----------------------- LOAD GAIT -----------------------
    gait = pd.read_csv(args.gait_csv)
    gait.columns = [c.strip() for c in gait.columns]

    if "Elapsed_us" not in gait.columns:
        raise SystemExit("Gait CSV missing Elapsed_us")

    elapsed_us = gait["Elapsed_us"].astype(float)
    duration_s = (elapsed_us.max() - elapsed_us.min()) * 1e-6
    print(f"Gait duration: {duration_s:.3f} s")

    # ----------------------- LOAD CURRENT CSV -----------------------
    cur = pd.read_csv(args.current_csv)
    cur.columns = [c.strip() for c in cur.columns]

    if "iso_time" in cur.columns:
        cur["DateTime"] = pd.to_datetime(cur["iso_time"], errors="coerce")
    elif "epoch_s" in cur.columns:
        cur["DateTime"] = pd.to_datetime(cur["epoch_s"], unit="s", errors="coerce")
    else:
        raise SystemExit("Current CSV must contain iso_time or epoch_s")

    if cur["DateTime"].isna().all():
        raise SystemExit("Cannot parse current timestamps")

    cur["DateTime"] = cur["DateTime"].astype("datetime64[ns]")

    if "value" not in cur.columns:
        raise SystemExit("Current CSV missing 'value' (amps)")

    # ----------------------- USER INPUT TIME -----------------------
    hhmmss_str = input("Enter start time HHMMSS: ")
    hh, mm, ss = parse_hhmmss(hhmmss_str)
    target_sod = hh * 3600 + mm * 60 + ss  # seconds of day

    # compute seconds-of-day for BMS and current
    for df in (bms, cur):
        df["sec_of_day"] = (
            df["DateTime"].dt.hour * 3600
            + df["DateTime"].dt.minute * 60
            + df["DateTime"].dt.second
        )

    # first BMS sample at/after requested time
    cand_bms = bms.loc[bms["sec_of_day"] >= target_sod].sort_values("DateTime")
    # first CURRENT sample at/after requested time
    cand_cur = cur.loc[cur["sec_of_day"] >= target_sod].sort_values("DateTime")

    if cand_bms.empty:
        print(f"BMS log range: {bms['DateTime'].min()} → {bms['DateTime'].max()}")
        raise SystemExit(
            f"No BMS samples at or after {hh:02d}:{mm:02d}:{ss:02d}. "
            f"Check the time you entered against the BMS log."
        )
    if cand_cur.empty:
        print(f"Current log range: {cur['DateTime'].min()} → {cur['DateTime'].max()}")
        raise SystemExit(
            f"No current samples at or after {hh:02d}:{mm:02d}:{ss:02d}. "
            f"Check the time you entered against the current log."
        )

    # choose T0 such that both logs have data from that point:
    T0_bms = cand_bms["DateTime"].iloc[0]
    T0_cur = cand_cur["DateTime"].iloc[0]
    T0 = max(T0_bms, T0_cur)  # start at the later of the two
    T1 = T0 + pd.to_timedelta(duration_s, unit="s")
    print(f"Sync window: {T0}  →  {T1}")

    # ----------------------- SLICE BMS -----------------------
    mask_bms = (bms["DateTime"] >= T0) & (bms["DateTime"] <= T1)
    bms_slice = bms.loc[mask_bms].copy()
    if bms_slice.empty:
        print(f"BMS range: {bms['DateTime'].min()} → {bms['DateTime'].max()}")
        raise SystemExit("No BMS samples in sync window.")

    bms_slice["t_rel_s"] = (bms_slice["DateTime"] - T0).dt.total_seconds()

    # ----------------------- SLICE CURRENT -----------------------
    mask_cur = (cur["DateTime"] >= T0) & (cur["DateTime"] <= T1)
    cur_slice = cur.loc[mask_cur].copy()
    if cur_slice.empty:
        print(f"Current range: {cur['DateTime'].min()} → {cur['DateTime'].max()}")
        raise SystemExit("No current samples in sync window.")

    cur_slice["t_rel_s"] = (cur_slice["DateTime"] - T0).dt.total_seconds()

    # ----------------------- MERGE + POWER -----------------------
    merged = pd.merge_asof(
        cur_slice.sort_values("DateTime")[["DateTime", "t_rel_s", "value"]],
        bms_slice.sort_values("DateTime")[["DateTime", v_col]],
        on="DateTime",
        direction="nearest",
    )
    merged["Power_W"] = merged[v_col] * merged["value"]

    # ----------------------- MECHANICAL ENERGY -----------------------
    # Use per-motor mechanical speed and current, sum power across joints,
    # and clip negative power (regeneration / resisting phases) to zero.
    mech_cols = [c for c in gait.columns if c.endswith("_spd_mech_RPM")]
    cur_cols  = [c for c in gait.columns if c.endswith("_current_A")]

    if mech_cols and cur_cols:
        # Extract motor names, e.g. "RightHip" from "RightHip_spd_mech_RPM"
        motors = sorted({c.replace("_spd_mech_RPM", "") for c in mech_cols})

        Kt  = 0.159   # Nm/A  (adjust if needed for your motor)
        gear = 9.0    # gear ratio
        eta  = 0.8    # mechanical efficiency guess

        # Total mechanical power across all motors
        P_mech_total = np.zeros(len(gait), dtype=float)

        for m in motors:
            spd_col = f"{m}_spd_mech_RPM"
            cur_col = f"{m}_current_A"
            if spd_col not in gait.columns or cur_col not in gait.columns:
                continue

            omega  = gait[spd_col].to_numpy(dtype=float) * (2.0 * np.pi / 60.0)  # rad/s
            torque = gait[cur_col].to_numpy(dtype=float) * Kt * gear * eta       # Nm

            P_mech_total += omega * torque  # W for this motor, summed across motors

        # Only count *positive* mechanical power (assistive work)
        P_mech_total = np.maximum(P_mech_total, 0.0)

        # Integrate over time using Elapsed_us from gait
        dt_gait = np.diff(elapsed_us * 1e-6, prepend=float(elapsed_us.iloc[0]) * 1e-6)  # seconds
        E_mech_Wh = np.sum(P_mech_total * dt_gait) / 3600.0
    else:
        E_mech_Wh = np.nan


    # ----------------------- ELECTRICAL ENERGY -----------------------
    dt_power = np.diff(merged["t_rel_s"], prepend=0)
    E_elec_Wh = np.sum(merged["Power_W"] * dt_power) / 3600.0

    # ----------------------- BATTERY DRAIN RATE (correct units) ------
    avg_current = cur_slice["value"].mean()     # A
    avg_voltage = bms_slice[v_col].mean()       # V
    avg_power   = merged["Power_W"].mean()      # W

    drain_power_W = avg_power                  # W
    drain_current_A = avg_current              # A

    # ----------------------- EFFICIENCY -----------------------
    if not np.isnan(E_mech_Wh) and E_elec_Wh > 0:
        efficiency = E_mech_Wh / E_elec_Wh
        # Clamp to [0, 1.0] just in case of small numerical issues
        efficiency = max(0.0, min(efficiency, 1.0))
    else:
        efficiency = np.nan

    # ----------------------- PRINT TO TERMINAL -----------------------
    print("\n========== ENERGY + EFFICIENCY RESULTS ==========")
    print(f"Total Electrical Energy:  {E_elec_Wh:.3f} Wh")
    print(f"Total Mechanical Energy:  {E_mech_Wh:.3f} Wh")
    if not np.isnan(efficiency):
        print(f"System Efficiency:        {efficiency*100:.1f}%")
    else:
        print("System Efficiency:        N/A (no mech columns)")
    print(f"Battery Drain Rate (P):   {drain_power_W:.2f} W")
    print(f"Battery Drain Rate (I):   {drain_current_A:.2f} A")
    print("=================================================\n")

    # ----------------------- LINEAR FITS -----------------------
    m_v, b_v = linear_fit(bms_slice["t_rel_s"], bms_slice[v_col])
    m_i, b_i = linear_fit(cur_slice["t_rel_s"], cur_slice["value"])
    m_p, b_p = linear_fit(merged["t_rel_s"], merged["Power_W"])

    # ----------------------- PLOTTING -----------------------
    fig, axes = plt.subplots(3, 1, figsize=(12, 14), sharex=True)

    # Voltage
    axes[0].plot(bms_slice["t_rel_s"], bms_slice[v_col], label="Voltage")
    axes[0].axhline(avg_voltage, color="red", linestyle="--",
                    label=f"Avg {avg_voltage:.2f} V")
    axes[0].plot(
        bms_slice["t_rel_s"],
        m_v * bms_slice["t_rel_s"] + b_v,
        color="orange",
        linestyle="--",
        label=f"Fit y={m_v:.4f}t+{b_v:.2f}",
    )
    axes[0].set_ylabel("Voltage (V)")
    axes[0].set_title("Battery Voltage vs Time")
    axes[0].legend()
    axes[0].grid(True)

    # Current
    axes[1].plot(cur_slice["t_rel_s"], cur_slice["value"], label="Current")
    axes[1].axhline(avg_current, color="red", linestyle="--",
                    label=f"Avg {avg_current:.2f} A")
    axes[1].plot(
        cur_slice["t_rel_s"],
        m_i * cur_slice["t_rel_s"] + b_i,
        color="orange",
        linestyle="--",
        label=f"Fit y={m_i:.4f}t+{b_i:.2f}",
    )
    axes[1].set_ylabel("Current (A)")
    axes[1].set_title("Measured Current vs Time")
    axes[1].legend()
    axes[1].grid(True)

    # Power
    axes[2].plot(merged["t_rel_s"], merged["Power_W"], label="Power")
    axes[2].axhline(avg_power, color="red", linestyle="--",
                    label=f"Avg {avg_power:.2f} W")
    axes[2].plot(
        merged["t_rel_s"],
        m_p * merged["t_rel_s"] + b_p,
        color="orange",
        linestyle="--",
        label=f"Fit y={m_p:.4f}t+{b_p:.2f}",
    )
    axes[2].set_ylabel("Power (W)")
    axes[2].set_xlabel("Time (s)")
    axes[2].legend()
    axes[2].grid(True)

    # annotate summary on power plot
    text = (
        f"E_elec = {E_elec_Wh:.2f} Wh\n"
        f"E_mech = {E_mech_Wh:.2f} Wh\n"
        f"Efficiency = {efficiency*100:.1f}%\n"
        f"Drain P = {drain_power_W:.1f} W\n"
        f"Drain I = {drain_current_A:.2f} A"
    )
    axes[2].text(
        0.02,
        0.98,
        text,
        transform=axes[2].transAxes,
        verticalalignment="top",
        fontsize=10,
        bbox=dict(facecolor="white", alpha=0.6),
    )

    plt.tight_layout()

    if args.save:
        if args.output is not None:
            out = args.output
            if out.suffix == "":
                out = out.with_suffix(".png")
        else:
            out = args.bms_csv.with_name(args.bms_csv.stem + "_sync_results.png")
        plt.savefig(out, dpi=100, bbox_inches="tight")
        print(f"Saved plot → {out}")

    plt.show()


if __name__ == "__main__":
    main()

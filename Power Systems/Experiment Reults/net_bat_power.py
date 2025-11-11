#!/usr/bin/env python3
"""
Display net power/current/energy from decoded exo CSV, converting motor rail currents
to estimated battery current. Optionally overlay DMM battery current.

Uses torque × motor-speed:
  tau = Kt * Iq (A)
  omega_m = motor mech RPM * 2π/60
  P_mech_sum = sum_i (tau_i * omega_m_i)

Speeds are taken from decoded CSV:
  - Prefer *_spd_mech_RPM (already motor mechanical RPM)
  - Else use *_spd_eRPM / pole_pairs

Motor efficiency curve (from your image):
  - Rises quickly to ~0.80 at ~11 N·m
  - Then declines roughly linearly toward ~0.60 by ~55 N·m

Shows:
  1) Sum mechanical power τ·ω (W)
  2) Battery power (W)
  3) Battery current (A) [estimated]  + average line
  4) Cumulative battery energy (Wh)
"""

from pathlib import Path
import argparse
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

DEFAULT_CSV = Path(__file__).parent / "Experiment1" / "gait_data_log_20251111_173516_decoded.csv"
MOTORS = ["RightHip", "LeftHip", "RightKnee", "LeftKnee"]

# ---------- helpers ----------
def cumulative_trapezoid_np(y, x):
    """Pure NumPy cumulative trapezoid (Joules if y=Watts, x=seconds)."""
    y = np.asarray(y, dtype=float)
    x = np.asarray(x, dtype=float)
    if y.size != x.size:
        raise ValueError("y and x must have the same length")
    if y.size == 0:
        return np.array([], dtype=float)
    dt = np.diff(x)
    avg = 0.5 * (y[1:] + y[:-1])
    incr = avg * dt
    return np.concatenate(([0.0], np.cumsum(incr)))

def build_timebase(df, dt):
    if "time_s" in df.columns:
        return df["time_s"].to_numpy(dtype=float)
    if "TimeStep" in df.columns:
        return df["TimeStep"].to_numpy(dtype=float) * float(dt)
    return np.arange(len(df), dtype=float) * float(dt)

def get_motor_omega_rad_s(df, motor, pole_pairs):
    """Return motor mechanical angular speed ω [rad/s] for a given motor name."""
    mech_col = f"{motor}_spd_mech_RPM"
    erpm_col = f"{motor}_spd_eRPM"
    if mech_col in df.columns:
        mech_rpm = df[mech_col].astype(float).to_numpy()
    elif erpm_col in df.columns and pole_pairs:
        mech_rpm = (df[erpm_col].astype(float) / float(pole_pairs)).to_numpy()
    else:
        return None
    return mech_rpm * (2.0 * np.pi / 60.0)

def motor_eta_from_tau(tau_abs, eta_min=0.60, eta_peak=0.80, tau_peak=11.0, tau_max=55.0):
    """
    Approximate motor efficiency curve based on your provided plot:
      - Rises ~linearly from eta_min at 0 N·m to eta_peak at tau_peak (~11 N·m)
      - Then declines ~linearly from eta_peak at tau_peak to ~0.60 at tau_max (~55 N·m)
    """
    tau_abs = np.asarray(tau_abs, dtype=float)
    eta = np.zeros_like(tau_abs)

    # Rising section (0 → tau_peak)
    rise_mask = tau_abs <= tau_peak
    if np.any(rise_mask):
        eta[rise_mask] = eta_min + (eta_peak - eta_min) * (tau_abs[rise_mask] / max(tau_peak, 1e-6))

    # Falling section (tau_peak → tau_max)
    fall_mask = tau_abs > tau_peak
    if np.any(fall_mask):
        span = max(tau_max - tau_peak, 1e-6)
        eta_fall = eta_peak - (eta_peak - 0.60) * ((tau_abs[fall_mask] - tau_peak) / span)
        eta[fall_mask] = eta_fall

    # Clamp to [eta_min, eta_peak] for safety
    eta = np.clip(eta, eta_min, eta_peak)
    return eta

# ---------- main ----------
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-i", "--input", type=Path, default=DEFAULT_CSV, help="Path to decoded CSV")
    ap.add_argument("--v_batt", type=float, default=48.0, help="Battery voltage (V)")
    ap.add_argument("--eta_fwd", type=float, default=0.90, help="Converter efficiency forward")
    ap.add_argument("--eta_regen", type=float, default=0.90, help="Converter efficiency on regen")
    ap.add_argument("--unidirectional", action="store_true", help="No backflow to battery (clamp regen)")
    ap.add_argument("--dt", type=float, default=0.04, help="Sample period (s)")
    ap.add_argument("--downsample", type=int, default=1, help="Plot every Nth sample")
    # Motor parameters
    ap.add_argument("--kt", type=float, default=0.16, help="Torque constant Kt (N·m/A)")
    ap.add_argument("--pole-pairs", type=int, default=7, help="Motor pole pairs for eRPM→mech RPM")
    # Motor efficiency curve tuning
    ap.add_argument("--eta-motor-peak", type=float, default=0.80, help="Motor efficiency at τ_peak")
    ap.add_argument("--eta-motor-min", type=float, default=0.2, help="Motor efficiency near zero torque")
    ap.add_argument("--tau-peak", type=float, default=11.0, help="Torque where efficiency peaks (N·m)")
    ap.add_argument("--tau-max", type=float, default=55.0, help="Torque where efficiency trends to ~0.60 (N·m)")
    # DMM overlay
    ap.add_argument("--dmm-constant", type=float, default=None, help="Overlay constant DMM current (A)")
    ap.add_argument("--dmm-col", type=str, default=None, help="CSV column for measured battery current (A)")
    args = ap.parse_args()

    df = pd.read_csv(args.input)
    t = build_timebase(df, args.dt)

    # --- ensure currents exist ---
    cur_cols = [f"{m}_current_A" for m in MOTORS]
    missing = [c for c in cur_cols if c not in df.columns]
    if missing:
        raise ValueError(f"Missing columns: {missing}")

    # --- gather speeds (ω) ---
    omegas = {}
    have_speed = True
    for m in MOTORS:
        om = get_motor_omega_rad_s(df, m, args.pole_pairs)
        if om is None:
            have_speed = False
            break
        omegas[m] = om
    if not have_speed:
        raise RuntimeError("Speed columns missing; τ·ω method unavailable. Need *_spd_mech_RPM or *_spd_eRPM.")

    # --- torques, mechanical power per motor ---
    torques = {m: args.kt * df[f"{m}_current_A"].astype(float).to_numpy() for m in MOTORS}
    p_mech = {m: torques[m] * omegas[m] for m in MOTORS}
    df["P_sum_mech_W"] = np.sum(list(p_mech.values()), axis=0)

    # --- motor efficiency vs |τ| (from your curve) ---
    eta_mot = {
        m: motor_eta_from_tau(
            np.abs(torques[m]),
            eta_min=args.eta_motor_min,
            eta_peak=args.eta_motor_peak,
            tau_peak=args.tau_peak,
            tau_max=args.tau_max,
        )
        for m in MOTORS
    }

    # --- map to battery power (per motor), then sum ---
    eta_conv_fwd, eta_conv_regen = float(args.eta_fwd), float(args.eta_regen)
    P_batt_total = np.zeros_like(df["P_sum_mech_W"], dtype=float)

    for m in MOTORS:
        Pm = p_mech[m]           # mechanical power (W), signed
        em = eta_mot[m]          # motor efficiency array (0..1)

        if args.unidirectional:
            # Forward only: negative mech power doesn't reduce battery power
            P_elec_in = np.where(Pm > 0.0, Pm / np.maximum(em, 1e-6), 0.0)
            P_batt = P_elec_in / max(eta_conv_fwd, 1e-6)
        else:
            # Bidirectional path
            P_elec_in  = np.where(Pm > 0.0, Pm / np.maximum(em, 1e-6), 0.0)
            P_elec_out = np.where(Pm < 0.0, Pm * np.maximum(em, 1e-6), 0.0)  # negative
            P_batt = (P_elec_in / max(eta_conv_fwd, 1e-6)) + (P_elec_out * max(eta_conv_regen, 1e-6))

        P_batt_total += P_batt

    df["P_batt_W"] = P_batt_total

    # --- battery current & energy ---
    df["I_batt_est_A"] = df["P_batt_W"] / float(args.v_batt)
    energy_J = cumulative_trapezoid_np(df["P_batt_W"].to_numpy(), t)
    energy_Wh = energy_J / 3600.0

    # --- downsample for plotting ---
    ds = max(1, args.downsample)
    t_p = t[::ds]
    Pmech_p = df["P_sum_mech_W"].to_numpy()[::ds]
    Pbat_p  = df["P_batt_W"].to_numpy()[::ds]
    Ibat_p  = df["I_batt_est_A"].to_numpy()[::ds]
    EWh_p   = energy_Wh[::ds]

    # --- optional DMM overlay ---
    dmm_series = None
    dmm_label = None
    if args.dmm_col and args.dmm_col in df.columns:
        dmm_series = df[args.dmm_col].to_numpy()[::ds]
        dmm_label = f"DMM ({args.dmm_col})"
    elif args.dmm_constant is not None:
        dmm_series = np.full_like(Ibat_p, float(args.dmm_constant), dtype=float)
        dmm_label = f"DMM constant = {args.dmm_constant} A"

    # ---------- plots ----------
    plt.figure(figsize=(12, 6))
    plt.plot(t_p, Pmech_p, label="Σ Mechanical Power τ·ω (W)")
    plt.xlabel("Time (s)"); plt.ylabel("Power (W)")
    plt.title("Σ Mechanical Power τ·ω (W)"); plt.legend(); plt.tight_layout()

    plt.figure(figsize=(12, 6))
    plt.plot(t_p, Pbat_p, label="P_batt (W)")
    plt.xlabel("Time (s)"); plt.ylabel("Power (W)")
    topo = "Unidirectional" if args.unidirectional else "Bidirectional"
    plt.title(f"Battery Power vs Time ({topo})"); plt.legend(); plt.tight_layout()

    plt.figure(figsize=(12, 6))
    plt.plot(t_p, Ibat_p, label="I_batt_est (A)")
    Iavg = np.mean(Ibat_p)
    plt.axhline(Iavg, color="red", linestyle="--", label=f"Average = {Iavg:.3f} A")
    if dmm_series is not None:
        plt.plot(t_p, dmm_series, linestyle="--", label=dmm_label)
    plt.xlabel("Time (s)"); plt.ylabel("Current (A)")
    plt.title(
        f"Battery Current vs Time  (η_peak={args.eta_motor_peak}, τ_peak={args.tau_peak} N·m → η≈0.60@{args.tau_max} N·m)"
    )
    plt.legend(); plt.tight_layout()

    plt.figure(figsize=(12, 6))
    plt.plot(t_p, EWh_p, label="Energy (Wh)")
    plt.xlabel("Time (s)"); plt.ylabel("Energy (Wh)")
    plt.title("Cumulative Battery Energy vs Time"); plt.legend(); plt.tight_layout()

    plt.show()

if __name__ == "__main__":
    main()

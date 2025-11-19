#!/usr/bin/env python3
"""
Plot joint positions from decoded exoskeleton CSV and overlay target gait points.

Creates HIP and KNEE plots:

- Measured:
    RightHip_pos_deg, LeftHip_pos_deg
    RightKnee_pos_deg, LeftKnee_pos_deg

- Targets:
    Right side from R_hip / R_knee
    Left side = flipped (-1 * right) with optional phase shift

Period is fixed at 4.112 s (from fitted data).
X-axis uses elapsed time (converted from microseconds → seconds).
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# === CHANGE THIS TO YOUR FILE ===
CSV_PATH = r"Experiment2\gait_data_log_20251114_161409_decoded.csv"

# === GAIT / PHASE CONFIG ===
T_PERIOD = 4.120  # seconds, from your fit

# Phase shifts (in seconds) for targets
#   0        -> no shift
#   T_PERIOD/4  (~1.028 s) -> 90° phase shift
#   T_PERIOD/2  (~2.056 s) -> 180° phase shift
PHASE_SHIFT_RHIP  = 0.25 + T_PERIOD / 2.0
PHASE_SHIFT_RKNEE = 0.5 + T_PERIOD / 2.0
PHASE_SHIFT_LHIP  = 0.15   # e.g. 180° out of phase with right hip
PHASE_SHIFT_LKNEE = 0.7  # same idea for knees

GAIT_LENGTH = 100

# ---- BASE TARGET ARRAYS (RIGHT SIDE, radians) ----
# (same as in your firmware)
R_knee = [
    0.037000, 0.052771, 0.084629, 0.130832, 0.188328, 0.253307, 0.322092, 0.391888, 0.461043, 0.528814,
    0.594912, 0.659118, 0.721086, 0.780312, 0.836206, 0.888165, 0.935595, 0.977879, 1.014357, 1.044349,
    1.067232, 1.082515, 1.089876, 1.089130, 1.080177, 1.062958, 1.037468, 1.003836, 0.962450, 0.914084,
    0.859921, 0.801478, 0.740430, 0.678434, 0.616989, 0.557384, 0.500702, 0.447811, 0.399288, 0.355357,
    0.315940, 0.280827, 0.249841, 0.222902, 0.199944, 0.180784, 0.165032, 0.152124, 0.141447, 0.132526,
    0.125131, 0.119250, 0.114934, 0.112159, 0.110768, 0.110518, 0.111165, 0.112556, 0.114696, 0.117765,
    0.122043, 0.127776, 0.135032, 0.143645, 0.153282, 0.163575, 0.174242, 0.185119, 0.196120, 0.207169,
    0.218164, 0.228979, 0.239484, 0.249582, 0.259220, 0.268387, 0.277085, 0.285280, 0.292881, 0.299740,
    0.305674, 0.310455, 0.313749, 0.315066, 0.313789, 0.309336, 0.301404, 0.290163, 0.276288, 0.260765,
    0.244510, 0.227946, 0.210720, 0.191788, 0.169891, 0.144314, 0.115610, 0.085967, 0.059021, 0.039173
]

R_hip = [
    0.330000, 0.331301, 0.335672, 0.343296, 0.353111, 0.363612, 0.373177, 0.380432, 0.384554, 0.385413,
    0.383465, 0.379469, 0.374125, 0.367799, 0.360427, 0.351604, 0.340792, 0.327532, 0.311585, 0.292979,
    0.271981, 0.249010, 0.224518, 0.198860, 0.172211, 0.144572, 0.115859, 0.086032, 0.055214, 0.023762,
    -0.007752, -0.038644, -0.068275, -0.096169, -0.122032, -0.145656, -0.166822, -0.185285, -0.200851, -0.213446,
    -0.223136, -0.230068, -0.234403, -0.236298, -0.235952, -0.233664, -0.229848, -0.224949, -0.219312, -0.213080,
    -0.206199, -0.198526, -0.189985, -0.180683, -0.170927, -0.161121, -0.151609, -0.142530, -0.133761, -0.124958,
    -0.115683, -0.105567, -0.094431, -0.082317, -0.069412, -0.055917, -0.041935, -0.027455, -0.012408, 0.003225,
    0.019357, 0.035805, 0.052363, 0.068890, 0.085359, 0.101854, 0.118501, 0.135401, 0.152584, 0.170029,
    0.187704, 0.205571, 0.223518, 0.241274, 0.258375, 0.274250, 0.288403, 0.300598, 0.310954, 0.319869,
    0.327803, 0.335008, 0.341347, 0.346295, 0.349147, 0.349355, 0.346828, 0.342093, 0.336243, 0.330710
]


def build_repeated_gait(target_array, t_min, t_max, gait_period, phase_shift=0.0):
    """
    Repeat a gait trajectory every `gait_period` seconds so it spans t_min→t_max.

    phase_shift: time shift (seconds) of the pattern.
    Returns:
        T_full   – time stamps (s) for each target sample
        Y_full   – target values (deg)
    """
    gait_len = len(target_array)
    target_deg = np.rad2deg(target_array)

    # base time within one cycle (exclude endpoint to avoid duplicate at wrap)
    t_one = np.linspace(0.0, gait_period, gait_len, endpoint=False)

    # how many cycles across the log
    cycles = int(np.ceil((t_max - t_min) / gait_period)) + 1

    T_full = []
    Y_full = []

    for i in range(cycles):
        T_full.append(t_one + phase_shift + i * gait_period + t_min)
        Y_full.append(target_deg)

    T_full = np.concatenate(T_full)
    Y_full = np.concatenate(Y_full)

    mask = (T_full >= t_min) & (T_full <= t_max)
    return T_full[mask], Y_full[mask]


def main():
    # Load CSV
    df = pd.read_csv(CSV_PATH)

    # Convert elapsed µs → seconds
    df["Elapsed_s"] = df["Elapsed_us"] / 1_000_000.0
    t = df["Elapsed_s"].values
    t_min, t_max = t[0], t[-1]

    # === Build RIGHT targets ===
    T_Rhip,  Y_Rhip_base  = build_repeated_gait(R_hip,  t_min, t_max, T_PERIOD, phase_shift=PHASE_SHIFT_RHIP)
    T_Rknee, Y_Rknee_base = build_repeated_gait(R_knee, t_min, t_max, T_PERIOD, phase_shift=PHASE_SHIFT_RKNEE)

    # === Build LEFT targets from RIGHT (flip + phase shift) ===
    # Left hip/knee are NOT using L_hip/L_knee; they are generated from R_hip/R_knee.
    T_Lhip,  Y_Lhip_raw  = build_repeated_gait(R_hip,  t_min, t_max, T_PERIOD, phase_shift=PHASE_SHIFT_LHIP)
    T_Lknee, Y_Lknee_raw = build_repeated_gait(R_knee, t_min, t_max, T_PERIOD, phase_shift=PHASE_SHIFT_LKNEE)

    Y_Lhip  = -Y_Lhip_raw   # flip sign
    Y_Lknee = -Y_Lknee_raw  # flip sign

    # --------------------- HIP PLOT ---------------------
    # plt.figure(figsize=(10, 6))

    # # Measured hips
    # plt.plot(t, df["RightHip_pos_deg"], label="RightHip_meas")
    # plt.plot(t, df["LeftHip_pos_deg"],  label="LeftHip_meas")

    # # Target hips
    # plt.plot(T_Rhip, Y_Rhip_base, "o", markersize=2, label="RightHip_target")
    # plt.plot(T_Lhip, Y_Lhip,      "o", markersize=2, label="LeftHip_target (from right)")

    # plt.xlabel("Time (s)")
    # plt.ylabel("Hip Position (deg)")
    # plt.title("Hip Positions vs Time (Measured + Targets from Right Gait)")
    # plt.grid(True, alpha=0.3)
    # plt.legend()
    # plt.tight_layout()
    # plt.show()

    # --------------------- KNEE PLOT ---------------------
    plt.figure(figsize=(10, 6))

    # Measured knees
    plt.plot(t, df["RightKnee_pos_deg"], label="RightKnee_meas")
    plt.plot(t, df["LeftKnee_pos_deg"],  label="LeftKnee_meas")

    # Target knees
    plt.plot(T_Rknee, Y_Rknee_base, "o", markersize=2, label="RightKnee_target")
    plt.plot(T_Lknee, Y_Lknee,      "o", markersize=2, label="LeftKnee_target (from right)")

    plt.xlabel("Time (s)")
    plt.ylabel("Knee Position (deg)")
    plt.title("Knee Positions vs Time (Measured + Targets from Right Gait)")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()

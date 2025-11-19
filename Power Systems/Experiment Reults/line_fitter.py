import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit

# ------------------ CONFIG ------------------
CSV_PATH = r"Experiment2\gait_data_log_20251114_161409_decoded.csv"
# Choose which joint to fit:
JOINT_COL = "LeftKnee_pos_deg"   # change to "RightHip_pos_deg", "RightKnee_pos_deg", etc.

# ------------------ LOAD CSV ------------------
df = pd.read_csv(CSV_PATH)

# Time in seconds from start (using Elapsed_us)
t = (df["Elapsed_us"] - df["Elapsed_us"].iloc[0]) / 1_000_000.0
t = t.to_numpy().astype(float)

# Signal to fit
y = df[JOINT_COL].to_numpy().astype(float)

# Optionally remove any NaNs / infs
mask = np.isfinite(t) & np.isfinite(y)
t = t[mask]
y = y[mask]

# ------------------ MODEL ------------------
def model(t, a, omega, phi, b, c):
    # y(t) = a*sin(omega*t + phi) + b*t + c
    return a * np.sin(omega * t + phi) + b * t + c

# ------------------ INITIAL GUESSES (from RAW) ------------------
if len(t) > 1:
    dt_med = np.median(np.diff(t))
else:
    dt_med = 1.0

# FFT-based frequency guess
fft_freqs = np.fft.rfftfreq(len(t), d=dt_med)
fft_mag = np.abs(np.fft.rfft(y - y.mean()))

if len(fft_mag) > 1:
    # skip DC (index 0)
    dom_idx = np.argmax(fft_mag[1:]) + 1
    freq_guess = fft_freqs[dom_idx]
    if freq_guess <= 0:
        freq_guess = 0.25  # fallback
else:
    freq_guess = 0.25  # fallback (0.25 Hz => period 4 s)

omega_guess = 2 * np.pi * freq_guess

a_guess = 0.5 * (np.max(y) - np.min(y))
b_guess = (y[-1] - y[0]) / (t[-1] - t[0]) if t[-1] > t[0] else 0.0
c_guess = np.mean(y)
phi_guess = 0.0

p0 = [a_guess, omega_guess, phi_guess, b_guess, c_guess]

print("Initial guesses:")
print(f"a ≈ {a_guess:.4f}, ω ≈ {omega_guess:.4f} rad/s "
      f"(period ≈ {2*np.pi/omega_guess if omega_guess>0 else np.nan:.3f} s), "
      f"φ ≈ {phi_guess:.3f}, b ≈ {b_guess:.3e}, c ≈ {c_guess:.4f}")

# ------------------ FIT TO RAW ------------------
try:
    # No bounds: easier for optimizer
    params, cov = curve_fit(
        model, t, y,
        p0=p0,
        maxfev=100000  # more evaluations allowed
    )
    a, omega, phi, b, c = params

    print(f"\nFitted parameters for {JOINT_COL}:")
    print(f"Amplitude a = {a:.5f}")
    print(f"Angular frequency ω = {omega:.5f} rad/s  ->  period = {2*np.pi/omega:.3f} s")
    print(f"Phase φ = {phi:.3f} rad")
    print(f"Slope b = {b:.5e}")
    print(f"Offset c = {c:.5f}")

    # ------------------ PREDICT & PLOT ------------------
    # 1) Predictions at original timestamps
    y_pred = model(t, *params)

    # 2) Smooth curve
    t_fit = np.linspace(t.min(), t.max(), 1000)
    y_fit = model(t_fit, *params)

    plt.figure(figsize=(14, 6))
    plt.plot(t, y, label=f"{JOINT_COL} (raw)", alpha=0.5)
    plt.plot(t, y_pred, label="Fit (at samples)", linewidth=2)
    plt.plot(t_fit, y_fit, label="Fit (smooth)", linewidth=2, alpha=0.8)
    plt.xlabel("Time (s)")
    plt.ylabel("Position (deg)")
    plt.title(f"Sinusoidal + Linear Trend Fit: {JOINT_COL}")
    plt.legend()
    plt.tight_layout()
    plt.show()

except RuntimeError as e:
    print("Curve fitting failed:", e)
    print("Try a different JOINT_COL, trimming the dataset, or tweaking the initial guesses.")

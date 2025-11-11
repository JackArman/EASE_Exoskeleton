import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit

# ------------------ LOAD CSV ------------------
df = pd.read_csv("Experiment 1\owon_log_20251107_161836.csv")

# Time in seconds from start
df["iso_time"] = pd.to_datetime(df["iso_time"])
t = (df["iso_time"] - df["iso_time"].iloc[0]).dt.total_seconds().to_numpy()
y = df["value"].to_numpy()

# ------------------ MODEL ------------------
def model(t, a, omega, phi, b, c):
    # y(t) = a*sin(omega*t + phi) + b*t + c
    return a * np.sin(omega * t + phi) + b * t + c

# ------------------ INITIAL GUESSES (from RAW) ------------------
dt_med = np.median(np.diff(t)) if len(t) > 1 else 1.0
fft_freqs = np.fft.rfftfreq(len(t), d=dt_med)
fft_mag = np.abs(np.fft.rfft(y - y.mean()))
if len(fft_mag) > 1:
    omega_guess = 2 * np.pi * fft_freqs[np.argmax(fft_mag[1:]) + 1]  # skip DC
else:
    omega_guess = 2 * np.pi * 0.5  # fallback

a_guess  = 0.5 * (np.max(y) - np.min(y))
b_guess  = (y[-1] - y[0]) / (t[-1] - t[0]) if t[-1] > t[0] else 0.0
c_guess  = np.mean(y)
phi_guess = 0.0

p0 = [a_guess, omega_guess, phi_guess, b_guess, c_guess]

# Optional: bounds to stabilize (omega>0)
bounds = ([-np.inf, 0.0, -2*np.pi, -np.inf, -np.inf],
          [ np.inf, np.inf,  2*np.pi,  np.inf,  np.inf])

# ------------------ FIT TO RAW ------------------
params, cov = curve_fit(model, t, y, p0=p0, bounds=bounds, maxfev=20000)
a, omega, phi, b, c = params

print("Fitted parameters (RAW):")
print(f"Amplitude a = {a:.5f}")
print(f"Angular frequency ω = {omega:.5f} rad/s  ->  period = {2*np.pi/omega:.3f} s")
print(f"Phase φ = {phi:.3f} rad")
print(f"Slope b = {b:.5e}")
print(f"Offset c = {c:.5f}")

# ------------------ PREDICT & PLOT ------------------
# 1) Plot predictions at the original timestamps (no interpolation errors)
y_pred = model(t, *params)

# 2) Also create a smooth curve on a dense time grid
t_fit = np.linspace(t.min(), t.max(), 1000)
y_fit = model(t_fit, *params)
ts_fit = df["iso_time"].iloc[0] + pd.to_timedelta(t_fit, unit="s")

plt.figure(figsize=(14,6))
plt.plot(df["iso_time"], y, label="Raw", alpha=0.5)
plt.plot(df["iso_time"], y_pred, label="Fit (at samples)", linewidth=2)
plt.plot(ts_fit, y_fit, label="Fit (smooth)", linewidth=2, alpha=0.8)
plt.xlabel("Time"); plt.ylabel("Measurement")
plt.title("Sinusoidal + Linear Trend Fit (RAW data)")
plt.legend(); plt.tight_layout(); plt.show()

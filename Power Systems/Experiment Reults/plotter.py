# save as plot_owon_csv.py
import argparse
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt

def main():
    ap = argparse.ArgumentParser(description="Plot OWON CSV logs + torque estimation")
    ap.add_argument("csv", help="Path to owon_log_YYYYMMDD_HHMMSS.csv")
    ap.add_argument("--time", choices=["iso_time", "epoch_s"], default="iso_time",
                    help="Which time column to use on x-axis (default: iso_time)")
    ap.add_argument("--rolling", type=int, default=0,
                    help="Optional rolling-window size (in samples) for smoothing (default: 0 = off)")
    ap.add_argument("--save", action="store_true",
                    help="Save the plot as a PNG next to the CSV")
    args = ap.parse_args()

    csv_path = Path(args.csv)
    if not csv_path.exists():
        raise SystemExit(f"CSV not found: {csv_path}")

    # Load CSV
    df = pd.read_csv(csv_path)

    # Ensure required columns exist
    for col in ["epoch_s", "iso_time", "value"]:
        if col not in df.columns:
            raise SystemExit(f"Missing column {col!r} in {csv_path.name}")

    # Build x-axis
    if args.time == "iso_time":
        x = pd.to_datetime(df["iso_time"], errors="coerce")
        x_label = "Time"
    else:
        x = pd.to_datetime(df["epoch_s"], unit="s", errors="coerce")
        x_label = "Time (from epoch)"

    current = df["value"]

    # Optional smoothing
    if args.rolling and args.rolling > 1:
        current = current.rolling(args.rolling, min_periods=1, center=False).mean()

    # --- Torque calculation ---
    Kt = 0.16  # N·m per amp (AK10-9 KV60)
    torque = current * Kt

    # --- Plot: two stacked subplots ---
    fig, axs = plt.subplots(2, 1, sharex=True, figsize=(10, 6))

    axs[0].plot(x, current, label="Current (A)")
    axs[0].set_ylabel("Amperage [A]")
    axs[0].grid(True)
    axs[0].legend(loc="upper right")

    axs[1].plot(x, torque, color="orange", label=f"Torque (N·m) = {Kt:.3f} × Current(A)")
    axs[1].set_xlabel(x_label)
    axs[1].set_ylabel("Torque [N·m]")
    axs[1].grid(True)
    axs[1].legend(loc="upper right")

    plt.suptitle(f"OWON Log: {csv_path.name}")
    plt.tight_layout()

    if args.save:
        out_png = csv_path.with_suffix(".png")
        plt.savefig(out_png, dpi=150, bbox_inches="tight")
        print(f"Saved plot: {out_png}")
    else:
        plt.show()

if __name__ == "__main__":
    main()

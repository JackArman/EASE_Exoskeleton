# save as plot_owon_csv.py
import argparse
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt

def main():
    ap = argparse.ArgumentParser(description="Plot OWON CSV logs")
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

    y = df["value"]

    # Optional smoothing
    if args.rolling and args.rolling > 1:
        y = y.rolling(args.rolling, min_periods=1, center=False).mean()

    # Plot
    plt.figure()  # single plot, no specific colors/styles per your environment
    plt.plot(x, y)
    plt.xlabel(x_label)
    plt.ylabel("Measurement")
    plt.title(f"OWON Log: {csv_path.name}")
    plt.grid(True)

    if args.save:
        out_png = csv_path.with_suffix(".png")
        plt.savefig(out_png, dpi=150, bbox_inches="tight")
        print(f"Saved plot: {out_png}")
    else:
        plt.show()

if __name__ == "__main__":
    main()

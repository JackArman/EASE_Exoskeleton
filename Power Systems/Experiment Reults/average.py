import pandas as pd
from pathlib import Path
import argparse

def main():
    ap = argparse.ArgumentParser(description="Compute average value from OWON CSV log")
    ap.add_argument("csv", help="Path to owon_log_YYYYMMDD_HHMMSS.csv")
    args = ap.parse_args()

    csv_path = Path(args.csv)
    if not csv_path.exists():
        raise SystemExit(f"File not found: {csv_path}")

    # Load CSV
    df = pd.read_csv(csv_path)

    # Check that 'value' column exists
    if "value" not in df.columns:
        raise SystemExit("CSV missing 'value' column. Check logger output format.")

    # Drop missing / invalid values
    valid = df["value"].dropna()

    # Compute average
    mean_val = valid.mean()
    std_val = valid.std()
    count = len(valid)

    print(f"File: {csv_path.name}")
    print(f"Samples: {count}")
    print(f"Average value: {mean_val:.6f}")
    print(f"Standard deviation: {std_val:.6f}")

if __name__ == "__main__":
    main()

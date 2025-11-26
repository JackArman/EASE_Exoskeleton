#!/usr/bin/env python3
"""
Merge two OWON logger CSVs (epoch_s, iso_time, value, raw).

- Drops any rows where epoch_s is not numeric (e.g. junk/debug lines)
- Appends second CSV after the first
- Preserves epoch_s and iso_time (no offset applied)

Usage:
  python merge_owon_logs.py first.csv second.csv -o merged.csv
"""

import argparse
import pandas as pd

def load_and_clean_owon(path):
    """
    Load an OWON CSV and drop any rows where epoch_s is non-numeric.
    """
    df = pd.read_csv(path)

    if "epoch_s" not in df.columns:
        raise SystemExit(f"{path} does not contain 'epoch_s' column.")

    # Force epoch_s to numeric; bad values become NaN
    df["epoch_s"] = pd.to_numeric(df["epoch_s"], errors="coerce")

    # Drop rows with NaN epoch_s (junk/debug lines)
    df = df[df["epoch_s"].notna()].reset_index(drop=True)

    return df

def main():
    ap = argparse.ArgumentParser(description="Merge two OWON CSV logs.")
    ap.add_argument("first", help="First OWON CSV file")
    ap.add_argument("second", help="Second OWON CSV file")
    ap.add_argument("-o", "--output", default="merged_owon.csv",
                    help="Output filename (default: merged_owon.csv)")
    args = ap.parse_args()

    # Load & clean both
    df1 = load_and_clean_owon(args.first)
    df2 = load_and_clean_owon(args.second)

    # Concatenate (optionally sort by time)
    df_out = pd.concat([df1, df2], ignore_index=True)

    # If you want to ensure strict time ordering, uncomment:
    # df_out = df_out.sort_values("epoch_s").reset_index(drop=True)

    # Save result
    df_out.to_csv(args.output, index=False)
    print(f"Saved merged OWON CSV to {args.output}")

if __name__ == "__main__":
    main()

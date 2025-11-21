#!/usr/bin/env python3
"""
Append two gait CSVs with cleaned debug lines and continuous Elapsed_us.

Usage:
  python temp.py first.csv second.csv -o merged.csv
"""

import argparse
import pandas as pd

def load_and_clean(path):
    """
    Load a CSV and drop any rows where Elapsed_us is not numeric.
    This removes lines like:
      'Moving legs to start,,,,,,,,,,,,,,,,,,,,,,,,,,,'
      'finished control loop,restarting'
    because their Elapsed_us column will be empty/non-numeric.
    """
    df = pd.read_csv(path)

    if "Elapsed_us" not in df.columns:
        raise SystemExit(f"{path} does not contain 'Elapsed_us' column.")

    # Force Elapsed_us to numeric, turning bad values into NaN
    df["Elapsed_us"] = pd.to_numeric(df["Elapsed_us"], errors="coerce")

    # Drop any rows where Elapsed_us is NaN (debug / junk lines)
    df = df[df["Elapsed_us"].notna()].reset_index(drop=True)

    return df

def main():
    ap = argparse.ArgumentParser(description="Append two gait CSVs with Elapsed_us continuity.")
    ap.add_argument("first", help="First CSV file")
    ap.add_argument("second", help="Second CSV file")
    ap.add_argument("-o", "--output", default="merged.csv", help="Output filename")
    args = ap.parse_args()

    # Load & clean both CSVs
    df1 = load_and_clean(args.first)
    df2 = load_and_clean(args.second)

    # Last timestamp of file 1
    last_us = df1["Elapsed_us"].iloc[-1]

    # Offset file 2's elapsed_us so it continues from file 1
    df2["Elapsed_us"] = df2["Elapsed_us"] + last_us

    # Append
    df_out = pd.concat([df1, df2], ignore_index=True)

    # Save
    df_out.to_csv(args.output, index=False)
    print(f"Saved merged CSV to {args.output}")

if __name__ == "__main__":
    main()

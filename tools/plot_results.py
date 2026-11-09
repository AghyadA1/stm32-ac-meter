"""Plot ACmeter readings against a reference multimeter (AVO).

Usage:  python tools/plot_results.py
Reads   results/voltage_measurements.csv and results/current_measurements.csv
Writes  docs/images/results/accuracy.png
"""
import csv
from pathlib import Path

import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
MEASURED = "#2a78d6"   # measured points
IDEAL = "#8a8a85"      # ideal y = x reference line
INK = "#2b2b29"
MUTED = "#6b6b66"


def load(name):
    with open(ROOT / "results" / name, newline="") as f:
        rows = list(csv.reader(f))[1:]
    ref = [float(r[0]) for r in rows]
    meas = [float(r[1]) for r in rows]
    return ref, meas


def panel(ax, ref, meas, quantity, unit):
    top = max(ref + meas) * 1.08
    ax.plot([0, top], [0, top], color=IDEAL, lw=1.5, ls="--", zorder=1)
    ax.scatter(ref, meas, s=46, color=MEASURED, edgecolor="white",
               linewidth=1.5, zorder=3)
    errs = [abs(m - r) / r * 100 for r, m in zip(ref, meas) if r > 0]
    ax.set_title(f"{quantity} (RMS, 50 Hz)", loc="left", color=INK,
                 fontsize=12, fontweight="bold")
    ax.text(0.03, 0.93,
            f"mean error {sum(errs) / len(errs):.1f} %   max {max(errs):.1f} %",
            transform=ax.transAxes, color=MUTED, fontsize=9.5, va="top")
    ax.set_xlabel(f"Reference multimeter ({unit})", color=MUTED)
    ax.set_ylabel(f"ACmeter reading ({unit})", color=MUTED)
    ax.set_xlim(0, top)
    ax.set_ylim(0, top)
    ax.set_aspect("equal")
    ax.grid(color="#e6e5e0", lw=0.8)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color("#c9c8c2")
    ax.tick_params(colors=MUTED, labelsize=9)


def main():
    fig, axes = plt.subplots(1, 2, figsize=(10, 4.8), dpi=150)
    panel(axes[0], *load("voltage_measurements.csv"), "Voltage", "V")
    panel(axes[1], *load("current_measurements.csv"), "Current", "A")
    handles = [
        plt.Line2D([], [], marker="o", ls="", color=MEASURED, markersize=7,
                   label="ACmeter reading"),
        plt.Line2D([], [], color=IDEAL, ls="--", label="Ideal (reading = reference)"),
    ]
    fig.legend(handles=handles, loc="lower center", ncol=2, frameon=False,
               fontsize=9.5, labelcolor=INK)
    fig.tight_layout(rect=(0, 0.06, 1, 1))
    out = ROOT / "docs" / "images" / "results" / "accuracy.png"
    fig.savefig(out, facecolor="white")
    print(f"wrote {out}")


if __name__ == "__main__":
    main()

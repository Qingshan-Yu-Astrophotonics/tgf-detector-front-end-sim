from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import pandas as pd

from config_loader import load_config, require_section


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Generate a synthetic edep_events.csv fallback dataset.")
    parser.add_argument("--config", default="config.yaml", help="Path to the repository config YAML file.")
    parser.add_argument(
        "--output",
        default="examples/generated/edep_events.csv",
        help="Output CSV path.",
    )
    return parser


def main() -> None:
    args = build_parser().parse_args()
    config = load_config(args.config)
    synthetic_cfg = require_section(config, "synthetic_edep")
    physics_cfg = require_section(config, "physics")

    rng = np.random.default_rng(int(require_section(config, "python_engine")["rng_seed"]))
    num_events = int(synthetic_cfg["num_events"])
    photopeak_fraction = float(synthetic_cfg["photopeak_fraction"])
    escape_fraction = float(synthetic_cfg["escape_fraction"])
    continuum_fraction = max(0.0, 1.0 - photopeak_fraction - escape_fraction)

    counts = rng.multinomial(
        num_events,
        [photopeak_fraction, continuum_fraction, escape_fraction],
    )

    photopeak = rng.normal(
        loc=float(synthetic_cfg["photopeak_mean_keV"]),
        scale=float(synthetic_cfg["photopeak_sigma_keV"]),
        size=counts[0],
    )
    continuum = rng.uniform(
        low=float(synthetic_cfg["continuum_min_keV"]),
        high=float(synthetic_cfg["continuum_max_keV"]),
        size=counts[1],
    )
    escape = rng.normal(
        loc=float(synthetic_cfg["escape_mean_keV"]),
        scale=float(synthetic_cfg["escape_sigma_keV"]),
        size=counts[2],
    )

    source_energy_keV = float(physics_cfg["source_energy_keV"])
    energies = np.clip(np.concatenate([photopeak, continuum, escape]), 0.0, source_energy_keV)
    rng.shuffle(energies)

    dataframe = pd.DataFrame(
        {
            "EventID": np.arange(energies.size, dtype=int),
            "Edep_keV": energies,
        }
    )

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    dataframe.to_csv(output_path, index=False)

    print(f"Wrote {energies.size} synthetic events to {output_path}")


if __name__ == "__main__":
    main()

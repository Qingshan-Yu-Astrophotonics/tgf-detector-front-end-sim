from __future__ import annotations

import argparse
from pathlib import Path
from typing import Any, Dict, Iterable

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from config_loader import load_config, require_section
from signal_models import (
    apply_probe_transfer,
    build_unit_charge_kernel,
    generate_arrival_times,
    synthesize_current,
    write_pwl,
    write_summary,
)


INPUT_TIMESTAMPS_DIR = "input_timestamps"


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Synthesize RF1105-like anode waveforms from edep_events.csv.")
    parser.add_argument("--config", default="config.yaml", help="Path to config.yaml")
    parser.add_argument("--input", required=True, help="Path to edep_events.csv")
    parser.add_argument(
        "--output-root",
        default="python_engine/output",
        help="Directory where rate-specific output folders will be created.",
    )
    parser.add_argument("--rate", type=float, help="Single rate in counts per second.")
    parser.add_argument(
        "--all-rates",
        action="store_true",
        help="Run the configured rate sweep from config.yaml.",
    )
    return parser


def _format_rate_dir(rate_cps: float) -> str:
    return f"rate_{rate_cps:.0f}_cps"


def _build_rate_list(engine_cfg: Dict[str, Any], args: argparse.Namespace) -> Iterable[float]:
    if args.all_rates:
        return [float(rate) for rate in engine_cfg["rate_sweep_cps"]]
    if args.rate is not None:
        return [float(args.rate)]
    return [float(engine_cfg["default_rate_cps"])]


def _load_input_events(input_path: str | Path) -> pd.DataFrame:
    dataframe = pd.read_csv(input_path)
    required_columns = {"EventID", "Edep_keV"}
    if not required_columns.issubset(dataframe.columns):
        missing = ", ".join(sorted(required_columns - set(dataframe.columns)))
        raise KeyError(f"Input CSV is missing columns: {missing}")
    if dataframe.empty:
        raise ValueError("Input CSV contains no energy deposition samples")
    dataframe = dataframe.copy()
    dataframe["Edep_keV"] = np.clip(dataframe["Edep_keV"].to_numpy(dtype=float), 0.0, None)
    return dataframe


def _resolve_arrival_mode(engine_cfg: Dict[str, Any], input_events: pd.DataFrame) -> str:
    configured_mode = str(engine_cfg.get("arrival_mode", "auto"))
    has_timestamps = "t_ns" in input_events.columns
    if configured_mode == "auto":
        return "input_timestamps" if has_timestamps else "poisson"
    if configured_mode == "poisson":
        return "poisson"
    if configured_mode == "input_timestamps":
        if not has_timestamps:
            raise KeyError("arrival_mode=input_timestamps requires the input CSV to contain a t_ns column")
        return "input_timestamps"
    raise ValueError("python_engine.arrival_mode must be one of auto, poisson, or input_timestamps")


def _sample_energies(edep_library_keV: np.ndarray, num_events: int, rng: np.random.Generator) -> np.ndarray:
    if num_events == 0:
        return np.empty(0, dtype=float)
    indices = rng.integers(0, edep_library_keV.size, size=num_events)
    return edep_library_keV[indices]


def _load_external_timestamps(input_events: pd.DataFrame) -> tuple[np.ndarray, np.ndarray]:
    timestamps_ns = input_events["t_ns"].to_numpy(dtype=float)
    if not np.all(np.isfinite(timestamps_ns)):
        raise ValueError("Input CSV contains non-finite t_ns values")
    if np.any(timestamps_ns < 0.0):
        raise ValueError("Input CSV contains negative t_ns values")

    order = np.argsort(timestamps_ns, kind="stable")
    arrival_times_s = timestamps_ns[order] * 1.0e-9
    energies_keV = np.clip(input_events["Edep_keV"].to_numpy(dtype=float)[order], 0.0, None)
    return arrival_times_s, energies_keV


def _plot_preview(
    output_path: Path,
    time_axis_s: np.ndarray,
    current_a: np.ndarray,
    voltage_v: np.ndarray,
    short_window_us: float,
    long_window_us: float,
    rate_label: str,
) -> None:
    time_us = time_axis_s * 1.0e6
    short_mask = time_us <= short_window_us
    long_mask = time_us <= long_window_us

    fig, axes = plt.subplots(2, 2, figsize=(12, 7), sharex="col")
    fig.suptitle(f"RF1105 MVP waveform preview at {rate_label}")

    axes[0, 0].plot(time_us[short_mask], current_a[short_mask] * 1.0e3, linewidth=1.0)
    axes[0, 0].set_title(f"Anode current, first {short_window_us:g} us")
    axes[0, 0].set_ylabel("Current [mA]")
    axes[0, 0].grid(True, alpha=0.3)

    axes[1, 0].plot(time_us[short_mask], voltage_v[short_mask], linewidth=1.0)
    axes[1, 0].set_title("Probe-output voltage")
    axes[1, 0].set_xlabel("Time [us]")
    axes[1, 0].set_ylabel("Voltage [V]")
    axes[1, 0].grid(True, alpha=0.3)

    axes[0, 1].plot(time_us[long_mask], current_a[long_mask] * 1.0e3, linewidth=0.8)
    axes[0, 1].set_title(f"Anode current, first {long_window_us:g} us")
    axes[0, 1].grid(True, alpha=0.3)

    axes[1, 1].plot(time_us[long_mask], voltage_v[long_mask], linewidth=0.8)
    axes[1, 1].set_title("Probe-output voltage and baseline behavior")
    axes[1, 1].set_xlabel("Time [us]")
    axes[1, 1].grid(True, alpha=0.3)

    fig.tight_layout()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output_path, dpi=150)
    plt.close(fig)


def _infer_burst_duration_us(arrival_times_s: np.ndarray, external_timestamps_used: bool) -> float | None:
    if not external_timestamps_used:
        return None
    if arrival_times_s.size == 0:
        return 0.0
    return float((arrival_times_s.max() - arrival_times_s.min()) * 1.0e6)


def _build_summary(
    config: Dict[str, Any],
    summary_rate_cps: float,
    arrival_times_s: np.ndarray,
    sampled_edep_keV: np.ndarray,
    photoelectrons: np.ndarray,
    charges_c: np.ndarray,
    time_axis_s: np.ndarray,
    output_dir: Path,
    dynode_generated: bool,
    arrival_mode_used: str,
    external_timestamps_used: bool,
) -> Dict[str, Any]:
    duration_s = float(time_axis_s[-1]) if time_axis_s.size else 0.0
    occupancy = float(arrival_times_s.size / duration_s) if duration_s > 0.0 else 0.0
    return {
        "detector_model": config["detector_model"],
        "rate_cps": summary_rate_cps,
        "simulation_window_us": duration_s * 1.0e6,
        "num_arrivals": int(arrival_times_s.size),
        "mean_arrival_rate_cps_realized": occupancy,
        "mean_edep_keV": float(sampled_edep_keV.mean()) if sampled_edep_keV.size else 0.0,
        "max_edep_keV": float(sampled_edep_keV.max()) if sampled_edep_keV.size else 0.0,
        "mean_photoelectrons": float(photoelectrons.mean()) if photoelectrons.size else 0.0,
        "mean_charge_pC": float(charges_c.mean() * 1.0e12) if charges_c.size else 0.0,
        "total_charge_nC": float(charges_c.sum() * 1.0e9),
        "arrival_mode_used": arrival_mode_used,
        "external_timestamps_used": external_timestamps_used,
        "burst_duration_us_inferred": _infer_burst_duration_us(arrival_times_s, external_timestamps_used),
        "output_directory": str(output_dir),
        "files": {
            "anode_current_pwl": str(output_dir / "anode_current.pwl"),
            "probe_anode_output_voltage_pwl": str(output_dir / "probe_anode_output_voltage.pwl"),
            "waveform_preview_png": str(output_dir / "waveform_preview.png"),
            "summary_json": str(output_dir / "summary.json"),
            "probe_dynode_output_voltage_pwl": str(output_dir / "probe_dynode_output_voltage.pwl")
            if dynode_generated
            else None,
        },
    }


def _synthesize_waveform(
    config: Dict[str, Any],
    arrival_times_s: np.ndarray,
    sampled_edep_keV: np.ndarray,
    duration_s: float,
    output_dir: Path,
    rng: np.random.Generator,
    summary_rate_cps: float,
    preview_label: str,
    arrival_mode_used: str,
    external_timestamps_used: bool,
) -> Path:
    engine_cfg = require_section(config, "python_engine")
    probe_cfg = require_section(config, "probe_output_model")
    dynode_cfg = require_section(config, "dynode_output_model")

    dt_s = float(engine_cfg["sample_period_ns"]) * 1.0e-9
    tau_rise_s = float(config["pmt_rise_time_ns"]) * 1.0e-9
    tau_decay_s = float(config["scintillation_decay_ns"]) * 1.0e-9
    kernel_length_s = float(engine_cfg["pulse_kernel_length_ns"]) * 1.0e-9

    mean_photoelectrons = sampled_edep_keV * float(config["scintillation_light_yield_ph_per_keV"]) * float(
        engine_cfg["quantum_efficiency"]
    )
    photoelectrons = rng.poisson(np.clip(mean_photoelectrons, 0.0, None))
    charges_c = (
        photoelectrons.astype(float)
        * float(config["pmt_gain_default"])
        * float(engine_cfg["elementary_charge_c"])
    )

    _, kernel = build_unit_charge_kernel(tau_rise_s, tau_decay_s, dt_s, kernel_length_s)
    time_axis_s, anode_current_a = synthesize_current(arrival_times_s, charges_c, duration_s, dt_s, kernel)
    probe_voltage_v = apply_probe_transfer(anode_current_a, dt_s, probe_cfg)

    output_dir.mkdir(parents=True, exist_ok=True)
    write_pwl(output_dir / "anode_current.pwl", time_axis_s, anode_current_a)
    write_pwl(output_dir / "probe_anode_output_voltage.pwl", time_axis_s, probe_voltage_v)

    dynode_generated = bool(dynode_cfg.get("enabled", False))
    if dynode_generated:
        dynode_voltage_v = apply_probe_transfer(anode_current_a, dt_s, dynode_cfg)
        write_pwl(output_dir / "probe_dynode_output_voltage.pwl", time_axis_s, dynode_voltage_v)

    _plot_preview(
        output_dir / "waveform_preview.png",
        time_axis_s,
        anode_current_a,
        probe_voltage_v,
        short_window_us=float(engine_cfg["waveform_preview_short_us"]),
        long_window_us=min(float(engine_cfg["waveform_preview_long_us"]), duration_s * 1.0e6),
        rate_label=preview_label,
    )

    summary = _build_summary(
        config,
        summary_rate_cps,
        arrival_times_s,
        sampled_edep_keV,
        photoelectrons,
        charges_c,
        time_axis_s,
        output_dir,
        dynode_generated,
        arrival_mode_used,
        external_timestamps_used,
    )
    write_summary(output_dir / "summary.json", summary)
    return output_dir


def synthesize_for_rate(
    config: Dict[str, Any],
    edep_library_keV: np.ndarray,
    rate_cps: float,
    output_root: Path,
    rng: np.random.Generator,
) -> Path:
    engine_cfg = require_section(config, "python_engine")
    duration_s = float(engine_cfg["simulation_window_us"]) * 1.0e-6
    arrival_times_s = generate_arrival_times(rate_cps, duration_s, rng)
    sampled_edep_keV = _sample_energies(edep_library_keV, arrival_times_s.size, rng)
    return _synthesize_waveform(
        config=config,
        arrival_times_s=arrival_times_s,
        sampled_edep_keV=sampled_edep_keV,
        duration_s=duration_s,
        output_dir=output_root / _format_rate_dir(rate_cps),
        rng=rng,
        summary_rate_cps=rate_cps,
        preview_label=f"{rate_cps:.2e} cps",
        arrival_mode_used="poisson",
        external_timestamps_used=False,
    )


def synthesize_from_input_timestamps(
    config: Dict[str, Any],
    input_events: pd.DataFrame,
    output_root: Path,
    rng: np.random.Generator,
) -> Path:
    engine_cfg = require_section(config, "python_engine")
    arrival_times_s, sampled_edep_keV = _load_external_timestamps(input_events)
    configured_duration_s = float(engine_cfg["simulation_window_us"]) * 1.0e-6
    kernel_length_s = float(engine_cfg["pulse_kernel_length_ns"]) * 1.0e-9
    final_arrival_s = float(arrival_times_s.max()) if arrival_times_s.size else 0.0
    duration_s = max(configured_duration_s, final_arrival_s + kernel_length_s)
    realized_rate_cps = float(arrival_times_s.size / duration_s) if duration_s > 0.0 else 0.0
    return _synthesize_waveform(
        config=config,
        arrival_times_s=arrival_times_s,
        sampled_edep_keV=sampled_edep_keV,
        duration_s=duration_s,
        output_dir=output_root / INPUT_TIMESTAMPS_DIR,
        rng=rng,
        summary_rate_cps=realized_rate_cps,
        preview_label="input timestamps",
        arrival_mode_used="input_timestamps",
        external_timestamps_used=True,
    )


def main() -> None:
    args = build_parser().parse_args()
    config = load_config(args.config)
    engine_cfg = require_section(config, "python_engine")
    input_events = _load_input_events(args.input)
    arrival_mode = _resolve_arrival_mode(engine_cfg, input_events)

    output_root = Path(args.output_root)
    if arrival_mode == "input_timestamps":
        rng = np.random.default_rng(int(engine_cfg["rng_seed"]))
        output_dir = synthesize_from_input_timestamps(
            config=config,
            input_events=input_events,
            output_root=output_root,
            rng=rng,
        )
        print(f"Synthesized input-timestamp waveform set in {output_dir}")
        return

    edep_library_keV = input_events["Edep_keV"].to_numpy(dtype=float)
    rates = list(_build_rate_list(engine_cfg, args))
    seed_sequence = np.random.SeedSequence(int(engine_cfg["rng_seed"]))
    child_seeds = seed_sequence.spawn(len(rates))

    for rate_cps, child_seed in zip(rates, child_seeds):
        rate_rng = np.random.default_rng(child_seed)
        output_dir = synthesize_for_rate(
            config=config,
            edep_library_keV=edep_library_keV,
            rate_cps=rate_cps,
            output_root=output_root,
            rng=rate_rng,
        )
        print(f"Synthesized {rate_cps:.2e} cps waveform set in {output_dir}")


if __name__ == "__main__":
    main()

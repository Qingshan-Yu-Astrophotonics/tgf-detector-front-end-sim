from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Dict

import numpy as np
from scipy import signal


def generate_arrival_times(rate_cps: float, duration_s: float, rng: np.random.Generator) -> np.ndarray:
    if rate_cps <= 0.0:
        raise ValueError("rate_cps must be positive")
    if duration_s <= 0.0:
        raise ValueError("duration_s must be positive")

    times = []
    current_time = 0.0
    while True:
        current_time += float(rng.exponential(1.0 / rate_cps))
        if current_time > duration_s:
            break
        times.append(current_time)
    return np.asarray(times, dtype=float)


def build_time_axis(duration_s: float, dt_s: float) -> np.ndarray:
    return np.arange(0.0, duration_s + dt_s, dt_s, dtype=float)


def build_unit_charge_kernel(
    tau_rise_s: float,
    tau_decay_s: float,
    dt_s: float,
    kernel_length_s: float,
) -> tuple[np.ndarray, np.ndarray]:
    if tau_decay_s <= tau_rise_s:
        raise ValueError("tau_decay must be larger than tau_rise")

    kernel_time = np.arange(0.0, kernel_length_s + dt_s, dt_s, dtype=float)
    kernel = (
        np.exp(-kernel_time / tau_decay_s) - np.exp(-kernel_time / tau_rise_s)
    ) / (tau_decay_s - tau_rise_s)
    return kernel_time, kernel


def synthesize_current(
    arrival_times_s: np.ndarray,
    charges_c: np.ndarray,
    duration_s: float,
    dt_s: float,
    kernel: np.ndarray,
) -> tuple[np.ndarray, np.ndarray]:
    time_axis = build_time_axis(duration_s, dt_s)
    charge_train = np.zeros_like(time_axis)

    if arrival_times_s.size:
        indices = np.clip(np.rint(arrival_times_s / dt_s).astype(int), 0, time_axis.size - 1)
        np.add.at(charge_train, indices, charges_c)

    current = np.convolve(charge_train, kernel, mode="full")[: time_axis.size]
    return time_axis, current


def _apply_load_divider(voltage: np.ndarray, source_resistance_ohm: float, external_load_ohm: float | None) -> np.ndarray:
    if external_load_ohm is None or external_load_ohm <= 0.0:
        return voltage
    return voltage * (external_load_ohm / (source_resistance_ohm + external_load_ohm))


def _apply_first_order_filter(
    waveform: np.ndarray,
    dt_s: float,
    filter_type: str,
    tau_s: float,
) -> np.ndarray:
    if tau_s <= 0.0:
        return waveform

    sample_rate_hz = 1.0 / dt_s
    if filter_type == "highpass":
        b, a = signal.bilinear([tau_s, 0.0], [tau_s, 1.0], fs=sample_rate_hz)
    elif filter_type == "lowpass":
        b, a = signal.bilinear([1.0], [tau_s, 1.0], fs=sample_rate_hz)
    else:
        raise ValueError(f"Unsupported filter type: {filter_type}")
    return signal.lfilter(b, a, waveform)


def apply_probe_transfer(
    current_a: np.ndarray,
    dt_s: float,
    model_config: Dict[str, Any],
) -> np.ndarray:
    voltage = current_a * float(model_config["transimpedance_ohm"]) * float(model_config.get("dc_gain", 1.0))
    voltage = _apply_load_divider(
        voltage,
        float(model_config.get("source_resistance_ohm", 50.0)),
        model_config.get("external_load_ohm"),
    )
    voltage = _apply_first_order_filter(
        voltage,
        dt_s,
        "highpass",
        float(model_config.get("high_pass_tau_us", 0.0)) * 1.0e-6,
    )
    voltage = _apply_first_order_filter(
        voltage,
        dt_s,
        "lowpass",
        float(model_config.get("low_pass_tau_ns", 0.0)) * 1.0e-9,
    )
    return voltage


def write_pwl(path: str | Path, time_axis_s: np.ndarray, values: np.ndarray) -> None:
    output_path = Path(path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as stream:
        for time_s, value in zip(time_axis_s, values):
            stream.write(f"{time_s:.9e} {value:.9e}\n")


def write_summary(path: str | Path, payload: Dict[str, Any]) -> None:
    output_path = Path(path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as stream:
        json.dump(payload, stream, indent=2, sort_keys=True)

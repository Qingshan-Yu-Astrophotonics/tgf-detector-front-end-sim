# RF1105 3-Step Cascade Simulation MVP

## Scope
This repository provides a minimal, loose-coupled MVP for the cascade

1. Geant4 event energy deposition
2. Python waveform synthesis
3. SPICE post-probe receiver simulation

The target detector module is the RF1105 NaI(Tl)-PMT assembly. The RF1105 is treated as a detector module, not a bare PMT. Its internal voltage divider and built-in preamplifier are not reconstructed in the default flow.

The default modeling boundary is:

- Geant4 outputs event-level `Edep_keV` in the NaI(Tl) crystal.
- Python converts deposited energy into anode-current pulses and a black-box approximation of the detector module external output voltage.
- SPICE models only the external post-probe receiver chain.

An optional exploratory netlist, `spice_sim/toy_internal_stage.cir`, is included for learning purposes only. It is explicitly non-faithful and should not be treated as a vendor-circuit reconstruction.

## Repository Layout
```text
.
©À©¤©¤ config.yaml
©À©¤©¤ requirements.txt
©À©¤©¤ README.md
©À©¤©¤ geant4_sim/
©À©¤©¤ python_engine/
©À©¤©¤ spice_sim/
©¸©¤©¤ examples/
```

## Stage 1: Geant4
`geant4_sim/` contains a minimal serial Geant4 application with:

- a 75 mm diameter, 75 mm height NaI(Tl) cylindrical crystal
- NaI(Tl) density set to 3.67 g/cm^3
- optional simplified MgO reflector shell and aluminum housing shell
- `G4EmStandardPhysics`
- config-gated source modes with legacy `lab_mono` kept as the default
- event-level output CSV `edep_events.csv`

Source modes:

- `lab_mono`: the legacy demo source, one 662 keV gamma per event from `z = -80 mm` toward `+z`
- `downward_tgf_mvp`: gamma-only downward TGF MVP with an exponentially cut power-law spectrum, source altitude, downward cone angle, and enlarged air world from `config.yaml`

In Phase A the downward TGF mode still uses a uniform `G4_AIR` world and gamma-only primaries. The TGF timing field `tgf_source.timing_mode` is currently a placeholder; the MVP writes a simple uniform burst emission time within `burst_duration_us`.

Output columns:

- legacy lab mode: `EventID`, `Edep_keV`
- downward TGF mode: `EventID`, `Edep_keV`, `t_ns`

The TGF world size remains config-driven, but the world is automatically expanded when needed so the configured source altitude and cone fit inside the world volume.

### Build
From the repository root:

```bash
cmake -S geant4_sim -B build/geant4
cmake --build build/geant4 --config Release
```

### Run
The executable still accepts the same two positional arguments as before:

```bash
./build/geant4/Release/rf1105_geant4 20000 examples/generated/edep_events.csv
```

The first argument is the number of Geant4 events. The second argument is the CSV output path. An optional third argument can be used to point at a non-default config file.

Legacy lab example:

```bash
./build/geant4/Release/rf1105_geant4 20000 examples/generated/edep_events.csv
```

Downward TGF example:

1. Set `physics.source_mode: downward_tgf_mvp` in `config.yaml`
2. Run the same command:

```bash
./build/geant4/Release/rf1105_geant4 20000 examples/generated/edep_events.csv
```

For quick local TGF tests, use a smaller `source_altitude_m`, a narrower `cone_half_angle_deg`, or a larger event count. A kilometer-altitude source with a wide cone is intentionally sparse at a single small detector.

## Stage 2: Python Signal Synthesis
`python_engine/pulse_synthesizer.py` loads `edep_events.csv`, applies photoelectron and PMT gain statistics, synthesizes double-exponential anode-current pulses, and writes PWL files for SPICE.

Arrival timing is now config-gated:

- `python_engine.arrival_mode: auto` uses input `t_ns` when present and otherwise falls back to the legacy Poisson generator
- `python_engine.arrival_mode: poisson` always uses the legacy steady Poisson timing path
- `python_engine.arrival_mode: input_timestamps` requires `t_ns` in the CSV and uses those timestamps directly

The default pulse model is:

```text
I(t) = Q / (tau_decay - tau_rise) * (exp(-t/tau_decay) - exp(-t/tau_rise))
```

Default values are driven by `config.yaml`:

- `light_yield_ph_per_keV = 38.0`
- `quantum_efficiency = 0.25`
- `pmt_gain_default = 2.73e5`
- `tau_rise_ns = 8`
- `tau_decay_ns = 250`
- rate sweep `1e5`, `5e5`, `1e6` cps

The default probe-output model is a configurable black-box approximation:

- transimpedance gain
- optional output-load division
- first-order high-pass behavior
- first-order low-pass behavior

Default output polarity is positive for both anode and dynode paths.

### Install
```bash
pip install -r requirements.txt
```

### Run With Geant4 Output
Legacy Poisson timing example:

```bash
python python_engine/pulse_synthesizer.py \
  --config config.yaml \
  --input examples/generated/edep_events.csv \
  --output-root python_engine/output \
  --all-rates
```

Externally timed burst example:

1. Generate a TGF CSV with `physics.source_mode: downward_tgf_mvp`
2. Set `python_engine.arrival_mode: input_timestamps` or leave it at `auto`
3. Run:

```bash
python python_engine/pulse_synthesizer.py \
  --config config.yaml \
  --input examples/generated/edep_events.csv \
  --output-root python_engine/output
```

When input timestamps are used, the Python stage writes a single burst directory at `python_engine/output/input_timestamps/` and records the inferred burst duration in `summary.json`.

### Fallback Without Geant4
`python_engine/generate_synthetic_edep.py` generates a synthetic legacy `edep_events.csv` so the whole chain can be exercised even when Geant4 is not installed.

```bash
python python_engine/generate_synthetic_edep.py --config config.yaml --output examples/generated/edep_events.csv
python python_engine/pulse_synthesizer.py --config config.yaml --input examples/generated/edep_events.csv --output-root python_engine/output --all-rates
```

### Python Outputs
For Poisson timing, the synthesizer writes per-rate directories such as `python_engine/output/rate_10000_cps/` containing:

- `anode_current.pwl`
- `probe_anode_output_voltage.pwl`
- `waveform_preview.png`
- `summary.json`
- `probe_dynode_output_voltage.pwl` if the dynode path is enabled in `config.yaml`

For external timestamps, it writes the same file set under `python_engine/output/input_timestamps/`.

The preview plot includes:

- one 5 us window
- one longer window for pile-up and baseline behavior

## Stage 3: SPICE
`spice_sim/post_probe_anode_chain.cir` is the default receiver netlist. It assumes the Python-generated probe output is the RF1105 module external output, then models:

- configurable source resistance and output capacitance
- selectable loading with high-Z default and optional 50 ohm
- AC coupling
- a simple voltage gain stage
- an optional RC shaper
- a simple ADC input model

`spice_sim/post_probe_dynode_chain.cir` is an optional faster dynode-output path with 50 ohm loading by default.

`spice_sim/toy_internal_stage.cir` is exploratory only.

### Run in LTspice or ngspice
Adjust the `PWL FILE=...` path inside the selected netlist so it points at the rate directory or `input_timestamps/` directory you want to simulate, then run transient analysis.

Examples:

```bash
ngspice spice_sim/post_probe_anode_chain.cir
ngspice spice_sim/post_probe_dynode_chain.cir
```

## Demo Workflow
`examples/run_demo.py` runs the Python-only end-to-end fallback flow:

```bash
python examples/run_demo.py
```

This creates:

- `examples/generated/edep_events.csv`
- `examples/demo_outputs/rate_100000_cps/`
- `examples/demo_outputs/rate_500000_cps/`
- `examples/demo_outputs/rate_1000000_cps/`

## Configuration Notes
All physical and waveform parameters live in `config.yaml`. The main additions for the downward-TGF path are:

- `physics.source_mode` to select `lab_mono` or `downward_tgf_mvp`
- `tgf_source.*` for TGF altitude, cone angle, spectrum, burst timing placeholder, and minimum world size
- `python_engine.arrival_mode` to select Poisson timing or input timestamps

This structure preserves the current lab flow while adding an optional downward-TGF path without changing the 3-stage architecture.

## Validation Note
The referenced detector report indicates Cs-137 energy resolution on the order of roughly 6.2% to 6.9% at operating voltages around 490 V to 530 V. In this MVP, that number is a sanity-check target only. It is not hard-coded as an enforced smearing model.

## Current Simplifications
- Optical photon transport is intentionally omitted in Geant4.
- The downward TGF MVP still uses a single uniform air material instead of a layered atmosphere.
- The downward TGF MVP still uses gamma-only primaries.
- The Python stage uses a black-box transfer from anode current to external module voltage.
- The default SPICE stage starts at the detector-module output connector, not inside the module.
- The optional toy internal-stage netlist is exploratory only.

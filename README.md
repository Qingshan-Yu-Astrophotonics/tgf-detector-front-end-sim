# Cascade Simulation Framework for NaI(Tl) Gamma-Ray Detectors

## Overview
This repository contains a full-chain simulation framework designed to model the response of a NaI(Tl) scintillation detector coupled to a PMT and a discrete preamplifier circuit. It bridges Monte Carlo particle transport physics, optoelectronic statistical fluctuations, and analog circuit transient analysis.

This tool is specifically optimized for evaluating detector performance in high-flux transient radiation environments, such as Terrestrial Gamma-ray Flashes (TGFs), where severe pulse pile-up and baseline shifts occur. The output from this simulation can be directly used as test vectors for digital pulse processing (DPP) algorithms (e.g., Trapezoidal Shaping, PZ Cancellation) implemented on Zynq FPGA platforms.

## Architecture & Workflow

The simulation is divided into three decoupled stages, allowing for rapid iteration of electronic designs without rerunning computationally expensive physics simulations.

### 1. Physics Layer: Geant4 (`/geant4_sim`)
* **Function:** Models the interaction of incident gamma rays with the NaI crystal.
* **Methodology:** To maintain high computational efficiency under extreme flux rates, optical photon tracking is disabled. Instead, the simulation records macroscopic energy deposition ($E_{dep}$) and absolute global interaction times ($t_0$) for each event.
* **Output:** `hits.csv` containing `EventID`, `Edep_keV`, and `GlobalTime_ns`.

### 2. Synthesis Layer: Python (`/python_engine`)
* **Function:** Translates physical energy deposition into macroscopic electrical currents, accounting for the statistical nature of the PMT.
* **Methodology:**
    1. Calculates photoelectrons incorporating Poisson statistics: $N_{pe} \sim \text{Poisson}(E_{dep} \times Y \times QE)$
    2. Calculates total charge: $Q = N_{pe} \times G \times e$
    3. Synthesizes a continuous analog waveform on a global time axis using a double-exponential pulse model:
       $$I(t) = \frac{Q}{\tau_d - \tau_r} \left( e^{-t/\tau_d} - e^{-t/\tau_r} \right)$$
       where $\tau_d \approx 250\text{ ns}$ (NaI decay) and $\tau_r \approx 5\text{ ns}$ (PMT/System rise time).
    4. Superimposes pulses in the time domain to accurately recreate stochastic pile-up.
* **Output:** `pmt_current.pwl` (Piece-Wise Linear time-current array).

### 3. Electronics Layer: SPICE (`/spice_sim`)
* **Function:** Models the analog front-end (AFE) circuit's response to the continuous current stream.
* **Methodology:** The PMT anode is modeled as an ideal current source driven by the PWL file, shunted by parasitic capacitance. The simulation evaluates the specific bipolar transistor (NPN) preamplifier layout, assessing AC coupling capacitor recovery times and dynamic range limitations during high-rate bursts.
* **Output:** Voltage waveforms (e.g., `.txt` or `.csv` from LTspice) representing the physical input to an ADC.

## Setup and Usage

### Prerequisites
1.  Install Python dependencies:
    ```bash
    pip install -r requirements.txt
    ```
2.  Install **Geant4** (C++17 compliant compiler required).
3.  Install **LTspice** (or a compatible SPICE engine like Ngspice).

### Execution Sequence
1.  **Run Geant4:** Compile and execute the Geant4 application to generate `hits.csv`.
2.  **Run Python Engine:** Execute `pulse_synthesizer.py`. It will read the CSV, generate the mathematical pulse train, plot a visualization (formatted in manuscript style), and output the PWL file.
3.  **Run SPICE:** Open `preamp.net` in LTspice and run the transient simulation to observe the final voltage waveform output.
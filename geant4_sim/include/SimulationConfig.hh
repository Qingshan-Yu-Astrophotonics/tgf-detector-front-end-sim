#pragma once

#include <string>

struct RF1105SimulationConfig {
  std::string source_mode = "lab_mono";
  double source_energy_keV = 662.0;
  std::string primary_particle = "gamma";
  int events_for_demo = 20000;

  bool tgf_enabled = false;
  double source_altitude_m = 1000.0;
  double cone_half_angle_deg = 15.0;
  std::string spectrum_model = "expcut_powerlaw";
  double alpha = 1.0;
  double ec_mev = 7.3;
  double emin_mev = 0.2;
  double emax_mev = 40.0;
  double burst_duration_us = 100.0;
  std::string timing_mode = "none";
  double world_half_height_m = 2.0;
  double world_half_width_m = 0.5;

  bool IsDownwardTgfMode() const;
  bool WritesTimingColumn() const;
};

RF1105SimulationConfig LoadSimulationConfig(const std::string& config_path, bool require_existing = false);

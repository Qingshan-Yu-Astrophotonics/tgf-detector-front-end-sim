#include "SimulationConfig.hh"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

std::string Trim(std::string value) {
  const auto begin = value.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return {};
  }
  const auto end = value.find_last_not_of(" \t\r\n");
  return value.substr(begin, end - begin + 1);
}

std::string StripComment(const std::string& value) {
  return Trim(value.substr(0, value.find('#')));
}

std::string Unquote(std::string value) {
  value = Trim(std::move(value));
  if (
      value.size() >= 2 &&
      ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) {
    return value.substr(1, value.size() - 2);
  }
  return value;
}

bool ParseBool(const std::string& value) {
  const auto normalized = Unquote(value);
  if (normalized == "true") {
    return true;
  }
  if (normalized == "false") {
    return false;
  }
  throw std::invalid_argument("Invalid boolean value in config.yaml: " + value);
}

double ParseDouble(const std::string& value) {
  return std::stod(Unquote(value));
}

int ParseInt(const std::string& value) {
  return std::stoi(Unquote(value));
}

void ApplyKeyValue(
    RF1105SimulationConfig& config,
    const std::string& section,
    const std::string& key,
    const std::string& value) {
  if (section == "physics") {
    if (key == "source_mode") {
      config.source_mode = Unquote(value);
    } else if (key == "source_energy_keV") {
      config.source_energy_keV = ParseDouble(value);
    } else if (key == "primary_particle") {
      config.primary_particle = Unquote(value);
    } else if (key == "events_for_demo") {
      config.events_for_demo = ParseInt(value);
    }
    return;
  }

  if (section == "tgf_source") {
    if (key == "enabled") {
      config.tgf_enabled = ParseBool(value);
    } else if (key == "source_altitude_m") {
      config.source_altitude_m = ParseDouble(value);
    } else if (key == "cone_half_angle_deg") {
      config.cone_half_angle_deg = ParseDouble(value);
    } else if (key == "spectrum_model") {
      config.spectrum_model = Unquote(value);
    } else if (key == "alpha") {
      config.alpha = ParseDouble(value);
    } else if (key == "Ec_MeV") {
      config.ec_mev = ParseDouble(value);
    } else if (key == "Emin_MeV") {
      config.emin_mev = ParseDouble(value);
    } else if (key == "Emax_MeV") {
      config.emax_mev = ParseDouble(value);
    } else if (key == "burst_duration_us") {
      config.burst_duration_us = ParseDouble(value);
    } else if (key == "timing_mode") {
      config.timing_mode = Unquote(value);
    } else if (key == "world_half_height_m") {
      config.world_half_height_m = ParseDouble(value);
    } else if (key == "world_half_width_m") {
      config.world_half_width_m = ParseDouble(value);
    }
  }
}

void ValidateConfig(const RF1105SimulationConfig& config) {
  if (config.primary_particle != "gamma") {
    throw std::invalid_argument("Only primary_particle=gamma is supported in this MVP");
  }
  if (!config.IsDownwardTgfMode()) {
    return;
  }
  if (config.spectrum_model != "expcut_powerlaw") {
    throw std::invalid_argument("Only tgf_source.spectrum_model=expcut_powerlaw is supported");
  }
  if (config.alpha < 0.0) {
    throw std::invalid_argument("tgf_source.alpha must be non-negative");
  }
  if (config.ec_mev <= 0.0) {
    throw std::invalid_argument("tgf_source.Ec_MeV must be positive");
  }
  if (config.emin_mev <= 0.0 || config.emax_mev <= config.emin_mev) {
    throw std::invalid_argument("tgf_source energy range must satisfy 0 < Emin_MeV < Emax_MeV");
  }
  if (config.cone_half_angle_deg < 0.0 || config.cone_half_angle_deg >= 90.0) {
    throw std::invalid_argument("tgf_source.cone_half_angle_deg must be in [0, 90)");
  }
  if (config.burst_duration_us < 0.0) {
    throw std::invalid_argument("tgf_source.burst_duration_us must be non-negative");
  }
}

}  // namespace

bool RF1105SimulationConfig::IsDownwardTgfMode() const {
  return source_mode == "downward_tgf_mvp";
}

bool RF1105SimulationConfig::WritesTimingColumn() const {
  return IsDownwardTgfMode();
}

RF1105SimulationConfig LoadSimulationConfig(const std::string& config_path, bool require_existing) {
  RF1105SimulationConfig config;
  const auto path = std::filesystem::path(config_path);
  if (!std::filesystem::exists(path)) {
    if (require_existing) {
      throw std::runtime_error("Config file not found: " + config_path);
    }
    return config;
  }

  std::ifstream stream(path);
  if (!stream) {
    throw std::runtime_error("Unable to open config file: " + config_path);
  }

  std::string line;
  std::string current_section;
  while (std::getline(stream, line)) {
    const auto content = StripComment(line);
    if (content.empty()) {
      continue;
    }

    const auto indent = line.find_first_not_of(' ');
    const auto separator = content.find(':');
    if (separator == std::string::npos) {
      continue;
    }

    const auto key = Trim(content.substr(0, separator));
    const auto value = Trim(content.substr(separator + 1));
    if ((indent == std::string::npos || indent == 0) && value.empty()) {
      current_section = key;
      continue;
    }

    if (!current_section.empty()) {
      ApplyKeyValue(config, current_section, key, value);
    }
  }

  ValidateConfig(config);
  return config;
}

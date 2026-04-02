#pragma once

#include <string>

#include "G4VUserActionInitialization.hh"
#include "SimulationConfig.hh"

class RF1105ActionInitialization : public G4VUserActionInitialization {
 public:
  RF1105ActionInitialization(std::string output_path, const RF1105SimulationConfig& config);

  void Build() const override;

 private:
  std::string output_path_;
  RF1105SimulationConfig config_;
};

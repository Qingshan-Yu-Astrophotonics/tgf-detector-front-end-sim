#pragma once

#include <string>

#include "G4VUserActionInitialization.hh"

class RF1105ActionInitialization : public G4VUserActionInitialization {
 public:
  explicit RF1105ActionInitialization(std::string output_path);

  void Build() const override;

 private:
  std::string output_path_;
};

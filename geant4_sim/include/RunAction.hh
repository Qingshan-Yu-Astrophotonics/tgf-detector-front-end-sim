#pragma once

#include <fstream>
#include <string>

#include "G4UserRunAction.hh"

class G4Run;

class RF1105RunAction : public G4UserRunAction {
 public:
  explicit RF1105RunAction(std::string output_path);
  ~RF1105RunAction() override;

  void BeginOfRunAction(const G4Run* run) override;
  void EndOfRunAction(const G4Run* run) override;
  void WriteEvent(int event_id, double edep_keV);

 private:
  std::string output_path_;
  std::ofstream output_stream_;
};

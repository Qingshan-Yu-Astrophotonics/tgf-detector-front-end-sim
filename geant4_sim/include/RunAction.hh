#pragma once

#include <fstream>
#include <string>

#include "G4UserRunAction.hh"

class G4Run;

class RF1105RunAction : public G4UserRunAction {
 public:
  RF1105RunAction(std::string output_path, bool write_timestamps);
  ~RF1105RunAction() override;

  void BeginOfRunAction(const G4Run* run) override;
  void EndOfRunAction(const G4Run* run) override;
  bool WritesTimestamps() const;
  void WriteEvent(int event_id, double edep_keV, double t_ns = 0.0);

 private:
  std::string output_path_;
  bool write_timestamps_;
  std::ofstream output_stream_;
};

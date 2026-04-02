#pragma once

#include "G4UserEventAction.hh"

class G4Event;
class RF1105RunAction;

class RF1105EventAction : public G4UserEventAction {
 public:
  explicit RF1105EventAction(RF1105RunAction* run_action);

  void BeginOfEventAction(const G4Event* event) override;
  void EndOfEventAction(const G4Event* event) override;

  void AddEdep(double edep_mev);

 private:
  double event_edep_mev_;
  RF1105RunAction* run_action_;
};

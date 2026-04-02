#pragma once

#include "G4UserSteppingAction.hh"

class G4Step;
class RF1105DetectorConstruction;
class RF1105EventAction;

class RF1105SteppingAction : public G4UserSteppingAction {
 public:
  RF1105SteppingAction(
      const RF1105DetectorConstruction* detector_construction,
      RF1105EventAction* event_action);

  void UserSteppingAction(const G4Step* step) override;

 private:
  const RF1105DetectorConstruction* detector_construction_;
  RF1105EventAction* event_action_;
};

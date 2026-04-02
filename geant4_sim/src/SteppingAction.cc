#include "SteppingAction.hh"

#include "DetectorConstruction.hh"
#include "EventAction.hh"

#include "G4LogicalVolume.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4TouchableHandle.hh"
#include "G4VPhysicalVolume.hh"

RF1105SteppingAction::RF1105SteppingAction(
    const RF1105DetectorConstruction* detector_construction,
    RF1105EventAction* event_action)
    : detector_construction_(detector_construction), event_action_(event_action) {}

void RF1105SteppingAction::UserSteppingAction(const G4Step* step) {
  const auto edep_mev = step->GetTotalEnergyDeposit();
  if (edep_mev <= 0.0) {
    return;
  }

  const auto* volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
  if (!volume) {
    return;
  }

  if (volume->GetLogicalVolume() == detector_construction_->GetCrystalLogical()) {
    event_action_->AddEdep(edep_mev);
  }
}

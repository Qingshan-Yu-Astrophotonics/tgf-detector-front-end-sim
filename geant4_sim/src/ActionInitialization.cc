#include "ActionInitialization.hh"

#include <utility>

#include "DetectorConstruction.hh"
#include "EventAction.hh"
#include "G4RunManager.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"

RF1105ActionInitialization::RF1105ActionInitialization(std::string output_path)
    : output_path_(std::move(output_path)) {}

void RF1105ActionInitialization::Build() const {
  auto* run_action = new RF1105RunAction(output_path_);
  auto* event_action = new RF1105EventAction(run_action);
  auto* detector = static_cast<const RF1105DetectorConstruction*>(
      G4RunManager::GetRunManager()->GetUserDetectorConstruction());

  SetUserAction(new RF1105PrimaryGeneratorAction());
  SetUserAction(run_action);
  SetUserAction(event_action);
  SetUserAction(new RF1105SteppingAction(detector, event_action));
}

#include "EventAction.hh"

#include "G4Event.hh"

#include "RunAction.hh"

RF1105EventAction::RF1105EventAction(RF1105RunAction* run_action)
    : event_edep_mev_(0.0), run_action_(run_action) {}

void RF1105EventAction::BeginOfEventAction(const G4Event*) {
  event_edep_mev_ = 0.0;
}

void RF1105EventAction::EndOfEventAction(const G4Event* event) {
  const auto edep_keV = event_edep_mev_ * 1000.0;
  run_action_->WriteEvent(event->GetEventID(), edep_keV);
}

void RF1105EventAction::AddEdep(double edep_mev) {
  event_edep_mev_ += edep_mev;
}

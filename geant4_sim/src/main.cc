#include <string>

#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "PhysicsList.hh"

#include "G4RunManagerFactory.hh"

int main(int argc, char** argv) {
  int n_events = 10000;
  std::string output_path = "edep_events.csv";

  if (argc > 1) {
    n_events = std::stoi(argv[1]);
  }
  if (argc > 2) {
    output_path = argv[2];
  }

  auto* run_manager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);
  run_manager->SetUserInitialization(new RF1105DetectorConstruction());
  run_manager->SetUserInitialization(new RF1105PhysicsList());
  run_manager->SetUserInitialization(new RF1105ActionInitialization(output_path));
  run_manager->Initialize();
  run_manager->BeamOn(n_events);

  delete run_manager;
  return 0;
}

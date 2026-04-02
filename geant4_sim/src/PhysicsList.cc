#include "PhysicsList.hh"

#include "G4EmStandardPhysics.hh"
#include "G4SystemOfUnits.hh"

RF1105PhysicsList::RF1105PhysicsList() {
  defaultCutValue = 0.1 * mm;
  SetVerboseLevel(0);
  RegisterPhysics(new G4EmStandardPhysics());
}

void RF1105PhysicsList::SetCuts() {
  SetCutsWithDefault();
}

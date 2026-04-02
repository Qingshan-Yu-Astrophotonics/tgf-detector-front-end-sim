#pragma once

#include "G4VModularPhysicsList.hh"

class RF1105PhysicsList : public G4VModularPhysicsList {
 public:
  RF1105PhysicsList();
  ~RF1105PhysicsList() override = default;

  void SetCuts() override;
};

#pragma once

#include "G4VUserPrimaryGeneratorAction.hh"

class G4Event;
class G4ParticleGun;

class RF1105PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
 public:
  RF1105PrimaryGeneratorAction();
  ~RF1105PrimaryGeneratorAction() override;

  void GeneratePrimaries(G4Event* event) override;

 private:
  G4ParticleGun* particle_gun_;
};

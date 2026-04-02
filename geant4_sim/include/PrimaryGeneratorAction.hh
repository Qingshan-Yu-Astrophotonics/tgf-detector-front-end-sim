#pragma once

#include "G4ThreeVector.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "SimulationConfig.hh"

class G4Event;
class G4ParticleGun;

class RF1105PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
 public:
  explicit RF1105PrimaryGeneratorAction(const RF1105SimulationConfig& config);
  ~RF1105PrimaryGeneratorAction() override;

  void GeneratePrimaries(G4Event* event) override;

 private:
  double SampleBurstTimeNs() const;
  double SampleTgfEnergyMeV() const;
  G4ThreeVector SampleTgfDirection() const;
  G4ThreeVector SampleTgfPosition() const;

  RF1105SimulationConfig config_;
  G4ParticleGun* particle_gun_;
  double tgf_spectrum_max_weight_;
};

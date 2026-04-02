#include "PrimaryGeneratorAction.hh"

#include "G4Event.hh"
#include "G4Gamma.hh"
#include "G4ParticleGun.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"

RF1105PrimaryGeneratorAction::RF1105PrimaryGeneratorAction() : particle_gun_(new G4ParticleGun(1)) {
  particle_gun_->SetParticleDefinition(G4Gamma::GammaDefinition());
  particle_gun_->SetParticleEnergy(662.0 * keV);
  particle_gun_->SetParticlePosition(G4ThreeVector(0.0, 0.0, -80.0 * mm));
  particle_gun_->SetParticleMomentumDirection(G4ThreeVector(0.0, 0.0, 1.0));
}

RF1105PrimaryGeneratorAction::~RF1105PrimaryGeneratorAction() {
  delete particle_gun_;
}

void RF1105PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
  particle_gun_->GeneratePrimaryVertex(event);
}

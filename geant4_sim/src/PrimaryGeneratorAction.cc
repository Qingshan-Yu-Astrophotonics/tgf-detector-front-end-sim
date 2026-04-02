#include "PrimaryGeneratorAction.hh"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "G4Event.hh"
#include "G4Gamma.hh"
#include "G4ParticleGun.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "Randomize.hh"

namespace {

double SpectralEnvelopeWeight(double energy_mev, double alpha, double ec_mev) {
  return std::pow(energy_mev, 1.0 - alpha) * std::exp(-energy_mev / ec_mev);
}

double ComputeSpectrumEnvelopeMaximum(const RF1105SimulationConfig& config) {
  auto max_weight = std::max(
      SpectralEnvelopeWeight(config.emin_mev, config.alpha, config.ec_mev),
      SpectralEnvelopeWeight(config.emax_mev, config.alpha, config.ec_mev));
  if (config.alpha < 1.0) {
    const auto peak_energy_mev = (1.0 - config.alpha) * config.ec_mev;
    if (peak_energy_mev > config.emin_mev && peak_energy_mev < config.emax_mev) {
      max_weight = std::max(
          max_weight,
          SpectralEnvelopeWeight(peak_energy_mev, config.alpha, config.ec_mev));
    }
  }
  return max_weight;
}

}  // namespace

RF1105PrimaryGeneratorAction::RF1105PrimaryGeneratorAction(const RF1105SimulationConfig& config)
    : config_(config),
      particle_gun_(new G4ParticleGun(1)),
      tgf_spectrum_max_weight_(config.IsDownwardTgfMode() ? ComputeSpectrumEnvelopeMaximum(config) : 1.0) {
  particle_gun_->SetParticleDefinition(G4Gamma::GammaDefinition());
  particle_gun_->SetParticleTime(0.0 * ns);
  particle_gun_->SetParticleEnergy(config_.source_energy_keV * keV);
  particle_gun_->SetParticlePosition(G4ThreeVector(0.0, 0.0, -80.0 * mm));
  particle_gun_->SetParticleMomentumDirection(G4ThreeVector(0.0, 0.0, 1.0));
}

RF1105PrimaryGeneratorAction::~RF1105PrimaryGeneratorAction() {
  delete particle_gun_;
}

void RF1105PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
  if (!config_.IsDownwardTgfMode()) {
    particle_gun_->SetParticleTime(0.0 * ns);
    particle_gun_->SetParticleEnergy(config_.source_energy_keV * keV);
    particle_gun_->SetParticlePosition(G4ThreeVector(0.0, 0.0, -80.0 * mm));
    particle_gun_->SetParticleMomentumDirection(G4ThreeVector(0.0, 0.0, 1.0));
    particle_gun_->GeneratePrimaryVertex(event);
    return;
  }

  particle_gun_->SetParticleTime(SampleBurstTimeNs() * ns);
  particle_gun_->SetParticleEnergy(SampleTgfEnergyMeV() * MeV);
  particle_gun_->SetParticlePosition(SampleTgfPosition());
  particle_gun_->SetParticleMomentumDirection(SampleTgfDirection());
  particle_gun_->GeneratePrimaryVertex(event);
}

double RF1105PrimaryGeneratorAction::SampleBurstTimeNs() const {
  return G4UniformRand() * config_.burst_duration_us * 1000.0;
}

double RF1105PrimaryGeneratorAction::SampleTgfEnergyMeV() const {
  const auto log_energy_span = std::log(config_.emax_mev / config_.emin_mev);
  while (true) {
    const auto energy_mev = config_.emin_mev * std::exp(G4UniformRand() * log_energy_span);
    const auto weight = SpectralEnvelopeWeight(energy_mev, config_.alpha, config_.ec_mev);
    if (G4UniformRand() * tgf_spectrum_max_weight_ <= weight) {
      return energy_mev;
    }
  }
}

G4ThreeVector RF1105PrimaryGeneratorAction::SampleTgfDirection() const {
  const auto cos_theta_min = std::cos(config_.cone_half_angle_deg * deg);
  const auto cos_theta = 1.0 - G4UniformRand() * (1.0 - cos_theta_min);
  const auto sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));
  const auto phi = twopi * G4UniformRand();
  return G4ThreeVector(sin_theta * std::cos(phi), sin_theta * std::sin(phi), -cos_theta);
}

G4ThreeVector RF1105PrimaryGeneratorAction::SampleTgfPosition() const {
  return G4ThreeVector(0.0, 0.0, config_.source_altitude_m * m);
}

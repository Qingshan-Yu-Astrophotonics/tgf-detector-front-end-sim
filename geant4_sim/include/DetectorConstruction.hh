#pragma once

#include "G4VUserDetectorConstruction.hh"
#include "SimulationConfig.hh"

class G4LogicalVolume;
class G4VPhysicalVolume;

class RF1105DetectorConstruction : public G4VUserDetectorConstruction {
 public:
  explicit RF1105DetectorConstruction(const RF1105SimulationConfig& config);

  G4VPhysicalVolume* Construct() override;
  G4LogicalVolume* GetCrystalLogical() const;

 private:
  RF1105SimulationConfig config_;
  G4LogicalVolume* crystal_logical_;
};

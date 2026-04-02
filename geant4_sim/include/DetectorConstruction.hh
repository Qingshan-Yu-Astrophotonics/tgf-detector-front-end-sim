#pragma once

#include "G4VUserDetectorConstruction.hh"

class G4LogicalVolume;
class G4VPhysicalVolume;

class RF1105DetectorConstruction : public G4VUserDetectorConstruction {
 public:
  RF1105DetectorConstruction();

  G4VPhysicalVolume* Construct() override;
  G4LogicalVolume* GetCrystalLogical() const;

 private:
  G4LogicalVolume* crystal_logical_;
};

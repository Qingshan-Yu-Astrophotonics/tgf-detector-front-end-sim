#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4Element.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4Tubs.hh"

RF1105DetectorConstruction::RF1105DetectorConstruction() : crystal_logical_(nullptr) {}

G4VPhysicalVolume* RF1105DetectorConstruction::Construct() {
  auto* nist = G4NistManager::Instance();
  auto* air = nist->FindOrBuildMaterial("G4_AIR");
  auto* aluminum = nist->FindOrBuildMaterial("G4_Al");

  auto* sodium = nist->FindOrBuildElement("Na");
  auto* iodine = nist->FindOrBuildElement("I");
  auto* magnesium = nist->FindOrBuildElement("Mg");
  auto* oxygen = nist->FindOrBuildElement("O");

  auto* nai = new G4Material("NaI_Tl_MVP", 3.67 * g / cm3, 2);
  nai->AddElement(sodium, 1);
  nai->AddElement(iodine, 1);

  auto* mgo = new G4Material("MgO_MVP", 3.58 * g / cm3, 2);
  mgo->AddElement(magnesium, 1);
  mgo->AddElement(oxygen, 1);

  constexpr auto world_half_length = 150.0 * mm;
  auto* world_solid = new G4Box("WorldSolid", world_half_length, world_half_length, world_half_length);
  auto* world_logical = new G4LogicalVolume(world_solid, air, "WorldLogical");
  auto* world_physical =
      new G4PVPlacement(nullptr, {}, world_logical, "WorldPhysical", nullptr, false, 0, true);

  constexpr auto crystal_radius = 37.5 * mm;
  constexpr auto crystal_half_height = 37.5 * mm;
  auto* crystal_solid =
      new G4Tubs("CrystalSolid", 0.0, crystal_radius, crystal_half_height, 0.0, 360.0 * deg);
  crystal_logical_ = new G4LogicalVolume(crystal_solid, nai, "CrystalLogical");
  new G4PVPlacement(nullptr, {}, crystal_logical_, "CrystalPhysical", world_logical, false, 0, true);

  constexpr auto reflector_outer_radius = 40.0 * mm;
  auto* reflector_shell = new G4Tubs(
      "ReflectorShell",
      crystal_radius,
      reflector_outer_radius,
      crystal_half_height,
      0.0,
      360.0 * deg);
  auto* reflector_logical = new G4LogicalVolume(reflector_shell, mgo, "ReflectorLogical");
  new G4PVPlacement(nullptr, {}, reflector_logical, "ReflectorPhysical", world_logical, false, 0, true);

  constexpr auto housing_inner_radius = 40.0 * mm;
  constexpr auto housing_outer_radius = 43.0 * mm;
  constexpr auto housing_half_height = 39.5 * mm;
  auto* housing_shell = new G4Tubs(
      "HousingShell",
      housing_inner_radius,
      housing_outer_radius,
      housing_half_height,
      0.0,
      360.0 * deg);
  auto* housing_logical = new G4LogicalVolume(housing_shell, aluminum, "HousingLogical");
  new G4PVPlacement(nullptr, {}, housing_logical, "HousingPhysical", world_logical, false, 0, true);

  return world_physical;
}

G4LogicalVolume* RF1105DetectorConstruction::GetCrystalLogical() const {
  return crystal_logical_;
}

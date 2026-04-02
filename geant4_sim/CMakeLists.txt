cmake_minimum_required(VERSION 3.16)
project(rf1105_geant4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Geant4 REQUIRED)

include(${Geant4_USE_FILE})
include_directories(include)

add_executable(
  rf1105_geant4
  src/main.cc
  src/ActionInitialization.cc
  src/DetectorConstruction.cc
  src/EventAction.cc
  src/PrimaryGeneratorAction.cc
  src/RunAction.cc
  src/SteppingAction.cc
)

target_link_libraries(rf1105_geant4 PRIVATE ${Geant4_LIBRARIES})

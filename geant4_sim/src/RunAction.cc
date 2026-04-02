#include "RunAction.hh"

#include <filesystem>
#include <iomanip>
#include <stdexcept>
#include <utility>

#include "G4Run.hh"

RF1105RunAction::RF1105RunAction(std::string output_path) : output_path_(std::move(output_path)) {}

RF1105RunAction::~RF1105RunAction() {
  if (output_stream_.is_open()) {
    output_stream_.close();
  }
}

void RF1105RunAction::BeginOfRunAction(const G4Run*) {
  const auto output_file = std::filesystem::path(output_path_);
  if (!output_file.parent_path().empty()) {
    std::filesystem::create_directories(output_file.parent_path());
  }

  output_stream_.open(output_path_, std::ios::out | std::ios::trunc);
  if (!output_stream_) {
    throw std::runtime_error("Unable to open output CSV file: " + output_path_);
  }

  output_stream_ << "EventID,Edep_keV\n";
  output_stream_ << std::fixed << std::setprecision(6);
}

void RF1105RunAction::EndOfRunAction(const G4Run*) {
  if (output_stream_.is_open()) {
    output_stream_.flush();
    output_stream_.close();
  }
}

void RF1105RunAction::WriteEvent(int event_id, double edep_keV) {
  output_stream_ << event_id << "," << edep_keV << "\n";
}

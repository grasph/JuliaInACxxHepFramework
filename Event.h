#ifndef CALL_JULIA_H
#define CALL_JULIA_H

#include <vector>
#include <inttypes.h>

class Event {
public:
  Event(size_t nMuonMax): diMuonMass_(-1.){
    for(auto col: {&Muon_pt_, &Muon_eta_, &Muon_phi_, &Muon_mass_}){
      col->resize(nMuonMax);
    }
    Muon_charge_.resize(nMuonMax);
  }

  void diMuonMass(float m){ diMuonMass_ = m; }
  float diMuonMass() const { return diMuonMass_;}
  
  uint32_t nMuons() { return nMuons_; }
  std::vector<float>& Muon_pt() { return Muon_pt_; }
  std::vector<float>& Muon_eta() { return Muon_eta_; }
  std::vector<float>& Muon_phi() { return Muon_phi_; }
  std::vector<float>& Muon_mass() { return Muon_mass_; }
  std::vector<int32_t>& Muon_charge() { return Muon_charge_; }

  uint32_t nMuons_;
  std::vector<float> Muon_pt_;
  std::vector<float> Muon_eta_;
  std::vector<float> Muon_phi_;
  std::vector<float> Muon_mass_;
  std::vector<int32_t> Muon_charge_;

  float diMuonMass_;
};



#endif //CALL_JULIA_H not defined

#include "Event.h"


#include <type_traits>

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"
#include "jlcxx/stl.hpp"

JLCXX_MODULE define_julia_module(jlcxx::Module& types){
  types.add_type<Event>("Event")
    .constructor<size_t>()
    .method("nMuons", &Event::nMuons)
    .method("Muon_pt", &Event::Muon_pt)
    .method("Muon_eta", &Event::Muon_eta)
    .method("Muon_phi", &Event::Muon_phi)
    .method("Muon_mass", &Event::Muon_mass)
    .method("Muon_charge", &Event::Muon_charge)
    .method("diMuonMass", &Event::diMuonMass);
}

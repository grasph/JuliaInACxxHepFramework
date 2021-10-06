module AnalysisModule1

using EventLoopGlue
import EventLoopGlue.analyze

struct Analysis <: JlAnalysis
end

using EventLoopGlue: diMuonMass, nMuons, Muon_pt, Muon_eta, Muon_phi, Muon_charge

function analyze(ana::AnalysisModule1.Analysis, event::Ptr{Event})::Cint

    event = EventLoopGlue.EventDereferenced(event)

    #Event selection: exactly two muons with opposite charges:
    nMuons(event) == 2 || return rejected
    Muon_charge(event)[1] !=  Muon_charge(event)[2] || return rejected

    dη = Muon_eta(event)[2] - Muon_eta(event)[1]
    dφ = Muon_phi(event)[2] - Muon_phi(event)[1]
    m2 = 2Muon_pt(event)[1]*Muon_pt(event)[2]*(cosh(dη) - cos(dφ))
    
    diMuonMass(event, sqrt(m2));
    
    return Int32(1)
end

beginJob(::Analysis) = println(stdout, "Method AnalysisModule1.beginJob() callaed.")
endJob(::Analysis) = println(stdout, "Method AnalysisModule1.endJob() called.")

canalyze() = @cfunction(analyze, Cint, (Ref{Analysis}, Ptr{EventLoopGlue.Event}))
cbeginJob() = @cfunction(beginJob, Cvoid, (Ref{Analysis},))
cendJob() = @cfunction(endJob, Cvoid, (Ref{Analysis},))

end

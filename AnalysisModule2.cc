#ifndef CXXANALYSISMODULE2_CC
#define CXXANALYSISMODULE2_CC

#include <iostream>
#include <memory>

#include "TH1.h"
#include "TCanvas.h"

#include "AnalysisModule.h"
#include "Event.h"


class AnalysisModule2: public AnalysisModule {
  std::unique_ptr<TH1> h_;
  
public:
  void beginJob(){
    std::cout << "Method AnalysisModule2.beginJob() called\n";
    auto bins = 30000; //Number of bins in the histogram
    auto low = 0.25; //Lower edge of the histogram
    auto up = 300.0; //Upper edge of the histogram
    h_ = std::unique_ptr<TH1>(new TH1D("hmass", "Dimuon mass;Dimuon mass [GeV/c^{2}];Events/GeV;", bins, low, up));
  }
  
  bool analyze(Event* event){
    float mass = event->diMuonMass();
    if(mass < 0){
      return false;
    } else{
      h_->Fill(mass);
      return true;
    }
  }
  
  void endJob(){
    std::cout << "Method AnalysisModule2.endJob() called\n";
    std::unique_ptr<TCanvas> c(new TCanvas());
    c->SetLogx();
    c->SetLogy();
    h_->Draw();
    c->Draw();
    c->SaveAs("dimu.png");
    std::cout << "Dimuon spectrum saved in dimu.png";
  }
};


extern "C" AnalysisModule* createModule(){ return new AnalysisModule2();}


#endif //CXXANALYSISMODULE2_CC not defined

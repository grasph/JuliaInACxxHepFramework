#include "julia.h"
#include "TFile.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TTreeReaderArray.h"
#include <iostream>
#include <numeric>
#include <sys/time.h>
#include <exception>
#include <typeinfo>
#include <stdexcept>
#include "TFile.h"
#include "Event.h"
#include <libgen.h> //basename, dirname
#include <dlfcn.h> // dlopen

#include "AnalysisModule.h"

typedef int (*jl_analyze_func_t)(void*, Event*);
typedef int (*jl_begin_job_func_t)(void*);
typedef int (*jl_end_job_func_t)(void*);


template<typename T>
class ToFree{
  T* ptr;
public:
  ToFree(): ptr(0) {}
  ToFree(T* ptr): ptr(ptr) {}
  ~ToFree() { if(ptr) free(ptr); };
  operator T*() const { return ptr;}
};



std::tuple<TTree*, Event*> init_root(const char* fname){
  std::cout << "Reading file: " << fname << "\n";

  TFile *f = TFile::Open(fname);
  if(!f || f->IsZombie()){
    std::cerr << "Failed to open file " << fname << ".\n";
    exit(1);
  }
  auto tree = (TTree*)f->Get("Events");
  if(!tree){
    std::cerr << "Tree Events was not found.\n";
    exit(1);
  }

  auto leaf = tree->GetLeaf("nMuon");
  if(!leaf){
    std::cerr << "Leaf nMuon was not found.\n";
    exit(1);
  }

  struct timeval t0, t1;
  gettimeofday(&t0, 0);

  size_t max_nmuons = leaf->GetMaximum();

  auto event = new Event(max_nmuons);

  tree->SetBranchAddress("nMuon", &event->nMuons_);
  tree->SetBranchAddress("Muon_pt", &(event->Muon_pt_[0]));
  tree->SetBranchAddress("Muon_eta", &(event->Muon_eta_[0]));
  tree->SetBranchAddress("Muon_phi", &(event->Muon_phi_[0]));
  tree->SetBranchAddress("Muon_mass", &(event->Muon_mass_[0]));
  tree->SetBranchAddress("Muon_charge", &(event->Muon_charge_[0]));

  return std::make_tuple(tree, event);
}

jl_value_t* verb_jl_eval_string(const char* cmd){
  jl_value_t* ret = jl_eval_string(cmd);
  jl_value_t* excep = jl_exception_occurred();
  if (excep){
    jl_value_t* jl_stderr = jl_eval_string("stderr");
    jl_function_t *func = jl_get_function(jl_base_module, "showerror");
    if(jl_stderr && func){
      jl_call2(func, jl_stderr, excep);
    } else{//fallback message if failed to use Julia showerror method
      fprintf(stderr, "%s \n", jl_typeof_str(jl_exception_occurred()));
    }
  } 
  return ret;
}

template<typename T>
T get_jl_callback(const char* module_name, const char* func_name,
		  const char* extra_args = ""){
  
  TString cmd = TString::Format("%s.%s()", module_name, func_name);
  
  auto func_boxed = (jl_value_t*) verb_jl_eval_string(cmd.Data());

  if (!func_boxed || !jl_typeis(func_boxed, jl_voidpointer_type)) {
    std::cerr << "Failed to retrieve the " << func_name <<  " function.\n"
	      << "Please include in the module " << module_name
	      << "the statement "
	      << "canalyze() = @cfunction(analyze, Cuchar, (Ref{Analysis}, " << extra_args << "))} ("
	      << cmd.Data() << ")\n";
    exit(1);
  }
  

  return (T) jl_unbox_voidpointer(func_boxed);
}

int main(int argc, char *argv[])
{
  /* required: setup the Julia context */
  jl_init();

  if(argc !=4) {
    std::cerr << "Usage: EventLoop data_filename julia_module cxx_module\n";
    return 1;
  }

  const char* data_fname = argv[1];
  const char* module_path = argv[2];
  const char* cxx_module = argv[3];

  auto cxx_module_path = ToFree<char>(realpath(TString::Format("%s.so", cxx_module).Data(), 0));

  if(cxx_module_path == nullptr){
    std::cerr << cxx_module << ".so was not found!\n";
    return 1;
  }
  

  auto dirc = ToFree<char>(strdup(module_path));
  auto basec = ToFree<char>(strdup(module_path));

  auto module_dir = dirname(dirc);
  auto module_name = basename(basec);

  verb_jl_eval_string(TString::Format("push!(LOAD_PATH, \"%s\")", module_dir).Data());
  
  //Retrieves the Julia callback function
  verb_jl_eval_string("import EventLoopGlue");

  verb_jl_eval_string(TString::Format("import %s", module_name).Data());


  //  TString cmd = TString::Format("@cfunction(%s.analyze, Cuchar, (Ref{%s.Analysis}, Ptr{EventLoopGlue.Event}))", module_name, module_name);

  auto analyze_jl = get_jl_callback<jl_analyze_func_t>(module_name, "canalyze", "Ptr{EventLoopGlue.Event}");
  auto beginJob_jl = get_jl_callback<jl_begin_job_func_t>(module_name, "cbeginJob");
  auto endJob_jl = get_jl_callback<jl_end_job_func_t>(module_name, "cendJob");


  auto analysis_boxed = (jl_value_t*)
    verb_jl_eval_string(TString::Format("%s.Analysis()", module_name).Data());
  
  if(!analysis_boxed){
    std::cerr << "Analysis struct not found. You need to define a struct derived from the EventLoopGlue.Analysis abtract type in the " << module_name
	      << " module file(" << module_name << ".jl).";
  }

  JL_GC_PUSH1(&analysis_boxed);
  auto analysis_jl = (void*) jl_unbox_voidpointer(analysis_boxed);
  
  struct timeval t0, t1;
  gettimeofday(&t0, 0);
  Long_t passed = 0;
  Long_t processed = 0;
    
  TTree* tree;
  Event* event;

  std::tie(tree, event) = init_root(data_fname);
  std::unique_ptr<Event> event_(event);


  // Load the library
  void *lib = dlopen(cxx_module_path, RTLD_NOW);
  if (lib == nullptr) {
    std::cerr << dlerror() << std::endl;
    return 1;
  }
  
  // Get a pointer to the shearsLoadPlugin() function
  auto createModule = (AnalysisModule* (*)()) dlsym(lib, "createModule");
  if (createModule == nullptr) {
    std::cerr << dlerror() << std::endl;
    dlclose(lib);
    return 1;
  }
		     
  std::unique_ptr<AnalysisModule> module2(createModule());

  beginJob_jl(analysis_jl);
  module2->beginJob();
  
  for(Long_t ievt = 0; ievt < tree->GetEntries(); ++ievt){
    tree->GetEntry(ievt);
      bool rc;
      if((rc = analyze_jl(analysis_jl, event))) ++passed;
      //if(analyze_func(analysis, event)) ++passed;
      if (jl_exception_occurred()){
	printf("%s \n", jl_typeof_str(jl_exception_occurred()));
	break;
      }
      
      if(rc) module2->analyze(event);
      
      ++processed;
  }

  endJob_jl(analysis_jl);
  module2->endJob();
  
  gettimeofday(&t1, 0);
  std::cout << "Duration: " << (t1.tv_sec - t0.tv_sec)*1. + (t1.tv_usec - t0.tv_usec)*1.e-6 << " seconds\n";
  std::cout << "Number of processed events: "  << processed << std::endl;
  std::cout << "Number of selected events: "  << passed << std::endl;

  JL_GC_POP();
  //  JL_GC_POP();

  /* exit */
  jl_atexit_hook(0);


  return 0;
}

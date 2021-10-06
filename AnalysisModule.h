#ifndef ANALYSISMODULE_H
#define ANALYSISMODULE_H

class Event;

class AnalysisModule{
public:
  virtual void beginJob() {}
  virtual void endJob(){}
  virtual bool analyze(Event*){ return true;}
  virtual ~AnalysisModule(){}
};


#endif //ANALYSISMODULE_H not defined

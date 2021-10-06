JL_SHARE = $(shell julia -e 'print(joinpath(Sys.BINDIR, Base.DATAROOTDIR, "julia"))')
#CFLAGS   += $(shell $(JL_SHARE)/julia-config.jl --cflags)
CXXFLAGS += $(subst -std=gnu99,,$(shell $(JL_SHARE)/julia-config.jl --cflags))
CXXFLAGS += -Wall -O0 -g	
LDFLAGS  += $(shell $(JL_SHARE)/julia-config.jl --ldflags)
LDLIBS   += $(shell $(JL_SHARE)/julia-config.jl --ldlibs)

CXXWRAP_CPPFLAGS=-I $(shell julia -e 'using CxxWrap; print(CxxWrap.prefix_path() * "/include")') -std=c++17

CXXFLAGS += -Wno-unused-variable

CPPFLAGS += -MMD

ROOT_FLAGS=$(shell root-config --libs --cflags)
ROOT_LDFLAGS=$(shell root-config --libs)


LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)

CC_FILES=EventLoop.cc EventLoop.Glue.cc

PRODUCTS=EventLoop EventLoopGlue.so AnalysisModule2.so
all: $(PRODUCTS)

clean:
	-rm $(PRODUCTS)

#%: %.c
#	gcc -o $@ $< -I /home/pgras/git.d/julia/usr/include/julia/  -fPIC -L'/home/pgras/git.d/julia/usr/lib' -Wl,--export-dynamic -Wl,-rpath,/home/pgras/git.d/julia/usr/lib -l julia


AnalysisModule2.o: AnalysisModule2.cc
	$(COMPILE.cc) $(ROOT_FLAGS) $<

EventLoopGlue.o: EventLoopGlue.cc
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) $<

EventLoopGlue.so: EventLoopGlue.o
	$(LINK.o) -o $@ $< --shared -fPIC $(ROOT_LDFLAGS)

AnalysisModule2.so: AnalysisModule2.o
	$(LINK.o) -o $@ $< --shared -fPIC $(ROOT_LDFLAGS)


EventLoop.o: EventLoop.cc
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) $< $(ROOT_FLAGS)

EventLoop: EventLoop.o EventLoopGlue.o AnalysisModule2.o
	$(LINK.o) -o $@ $< $(LDLIBS) $(ROOT_FLAGS)

echo_%:
#	@echo "$* = $(subst ",\",$($*))"
	@echo $* = $($*)

-include $(subst .cc,.d, $(CC_FILES))

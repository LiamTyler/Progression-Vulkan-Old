MAINDIR = $(realpath ./)
BUILDDIR = $(MAINDIR)/build
SRCDIR = $(MAINDIR)/src
BINDIR = $(BUILDDIR)/bin
OBJDIR = $(BUILDDIR)/obj
EXTDIR = $(MAINDIR)/ext
CXX = g++
CXXLIBS += -lGLEW -lSDL2 -lGL -lGLU -ldl
INCLUDEDIRS = $(MAINDIR) $(EXTDIR) $(SRCDIR)
CXXFLAGS += $(patsubst %, -I%, $(INCLUDEDIRS)) -std=c++11 -O3

rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))
make-depend-cxx=$(CXX) $(CXXFLAGS) -MM -MF $3 -MP -MT $2 $1

SOURCES = $(SRCDIR)
SRC_CXX = $(call rwildcard,$(SOURCES),*.cpp)
OBJECTS_CXX = $(notdir $(patsubst %.cpp,%.o,$(SRC_CXX)))
LIBNAME = libStarter.a
LIBPATH = $(BUILDDIR)/$(LIBNAME)
LIBLINK = -L$(BUILDDIR) -lStarter

EXAMPLE_SRC_DIR = $(MAINDIR)/examples
EXAMPLE_SRCS = $(notdir $(call rwildcard,$(EXAMPLE_SRC_DIR),*.cpp))
EXAMPLE_EXES = $(basename $(EXAMPLE_SRCS))
EXAMPLE_BUILD_DIR = $(BUILDDIR)/examples


.PHONY: all clean lib examples info

all: $(LIBPATH) examples

clean:
	@rm -rf $(BUILDDIR)
	@rm -f $(EXAMPLEDIR)/*.o

lib: $(LIBPATH)

examples: $(addprefix $(EXAMPLE_BUILD_DIR)/, $(EXAMPLE_EXES))

$(LIBPATH): $(addprefix $(OBJDIR)/, $(OBJECTS_CXX)) | $(BINDIR)
	rm -f $(LIBPATH)
	ar -csq $(LIBPATH) $(OBJDIR)/*.o

ifneq "$MAKECMDGOALS" "clean"
-include $(addprefix $(OBJDIR)/,$(OBJECTS_CXX:.o=.d))
endif

$(addprefix $(OBJDIR)/, $(OBJECTS_CXX)): | $(OBJDIR)

$(BINDIR) $(OBJDIR) $(EXAMPLE_BUILD_DIR):
	mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@$(call make-depend-cxx,$<,$@,$(subst .o,.d,$@))
	$(CXX) $(CXXFLAGS) $(CXXLIBS) -c -o $@ $<

$(EXAMPLE_BUILD_DIR)/%: $(EXAMPLE_SRC_DIR)/*.cpp $(LIBPATH) | $(EXAMPLE_BUILD_DIR)
	$(CXX) $< $(LIBLINK) $(CXXFLAGS) $(CXXLIBS) -o $@

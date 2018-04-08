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
OBJS = $(addprefix $(OBJDIR)/, $(OBJECTS_CXX))
LIBNAME = libStarter.a
LIBPATH = $(BUILDDIR)/$(LIBNAME)
LIBLINK = -L$(BUILDDIR) -lStarter

EXAMPLE_SRC_DIR = $(MAINDIR)/examples
EXAMPLE_SRCS = $(notdir $(call rwildcard,$(EXAMPLE_SRC_DIR),*.cpp))
EXAMPLE_EXES = $(basename $(EXAMPLE_SRCS))
EXAMPLE_BUILD_DIR = $(BUILDDIR)/examples

# Name of the single header file people have to include
header = starter.h
header_files = $(addprefix include/, $(notdir $(wildcard $(SRCDIR)/include/*.h)))

.PHONY: all clean lib examples info

all: $(LIBPATH) examples $(header)

clean:
	@rm -rf $(BUILDDIR)
	@rm -f $(header)

$(header): $(OBJS)
	@$(file > $(header)) $(foreach line, $(header_files), $(file >> $(header), #include "$(line)"))

lib: $(LIBPATH) $(header)

examples: $(addprefix $(EXAMPLE_BUILD_DIR)/, $(EXAMPLE_EXES))

$(LIBPATH): $(OBJS) | $(BINDIR)
	@rm -f $(LIBPATH)
	@ar -csq $(LIBPATH) $(OBJDIR)/*.o

ifneq "$MAKECMDGOALS" "clean"
-include $(addprefix $(OBJDIR)/,$(OBJECTS_CXX:.o=.d))
endif

$(OBJS): | $(OBJDIR)

$(BINDIR) $(OBJDIR) $(EXAMPLE_BUILD_DIR):
	@mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@$(call make-depend-cxx,$<,$@,$(subst .o,.d,$@))
	$(CXX) $(CXXFLAGS) $(CXXLIBS) -c -o $@ $<

$(EXAMPLE_BUILD_DIR)/%: $(EXAMPLE_SRC_DIR)/*.cpp $(LIBPATH) $(header) | $(EXAMPLE_BUILD_DIR)
	$(CXX) $< $(LIBLINK) $(CXXFLAGS) $(CXXLIBS) -o $@

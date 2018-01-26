MAINDIR = $(realpath ./)
BUILDDIR = $(MAINDIR)/build
SRCDIR = $(MAINDIR)/src
BINDIR = $(BUILDDIR)/bin
OBJDIR = $(BUILDDIR)/obj
EXTDIR = $(MAINDIR)/ext
CXX = g++
CXXLIBS += -lGLEW -lSDL2 -lGL -lGLU -ldl
CXXFLAGS += -I$(SRCDIR) -I$(EXTDIR) -std=c++11

rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))
make-depend-cxx=$(CXX) $(CXXFLAGS) -MM -MF $3 -MP -MT $2 $1

SOURCES = $(SRCDIR)
SRC_CXX = $(call rwildcard,$(SOURCES),*.cpp)
OBJECTS_CXX = $(notdir $(patsubst %.cpp,%.o,$(SRC_CXX)))
TARGET = $(BINDIR)/proj

.PHONY: all clean run

all: $(TARGET)

clean:
	@rm -rf $(BUILDDIR)

run: $(TARGET)
	$(TARGET)

ifneq "$MAKECMDGOALS" "clean"
-include $(addprefix $(OBJDIR)/,$(OBJECTS_CXX:.o=.d))
endif

$(addprefix $(OBJDIR)/, $(OBJECTS_CXX)): | $(OBJDIR)

$(TARGET): $(addprefix $(OBJDIR)/, $(OBJECTS_CXX)) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(addprefix $(OBJDIR)/, $(OBJECTS_CXX)) -o $@ $(CXXLIBS)

$(BINDIR) $(OBJDIR):
	@mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@$(call make-depend-cxx,$<,$@,$(subst .o,.d,$@))
	$(CXX) $(CXXFLAGS) $(CXXLIBS) -c -o $@ $<

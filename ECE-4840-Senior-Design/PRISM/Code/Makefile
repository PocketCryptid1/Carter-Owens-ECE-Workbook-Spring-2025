CXX = g++
CXXFLAGS_COMMON = -Wall -Wextra -std=c++17 -O2 -I$(INCDIR)
CXXFLAGS_VTX = $(CXXFLAGS_COMMON) `pkg-config --cflags libcamera libavcodec libavformat libavutil`
CXXFLAGS_VRX = $(CXXFLAGS_COMMON)
DEPFLAGS = -MMD -MP

LDFLAGS_VTX = `pkg-config --libs libcamera libavcodec libavformat libavutil` -lpcap -pthread
LDFLAGS_VRX = -lpcap -pthread -ljpeg `pkg-config --libs libavcodec libavutil libswscale`

INCDIR = include
SRCDIR = src
OBJDIR = obj
BINDIR = bin

VTX_TARGET = $(BINDIR)/prismvtx
VRX_TARGET = $(BINDIR)/prismvrx

VTX_SOURCES = $(SRCDIR)/vtxmain.cpp $(SRCDIR)/camera_app.cpp $(SRCDIR)/h264_encoder.cpp $(SRCDIR)/protocol.cpp
VRX_SOURCES = $(SRCDIR)/vrxmain.cpp $(SRCDIR)/protocol.cpp $(SRCDIR)/display.cpp

VTX_OBJECTS = $(VTX_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/vtx_%.o)
VRX_OBJECTS = $(VRX_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/vrx_%.o)

# Default: build VTX only (what the VTX Pi needs)
all: vtx

vtx: $(VTX_TARGET)
vrx: $(VRX_TARGET)

$(VTX_TARGET): $(VTX_OBJECTS)
	@mkdir -p $(BINDIR)
	$(CXX) $(VTX_OBJECTS) -o $@ $(LDFLAGS_VTX)

$(VRX_TARGET): $(VRX_OBJECTS)
	@mkdir -p $(BINDIR)
	$(CXX) $(VRX_OBJECTS) -o $@ $(LDFLAGS_VRX)

$(OBJDIR)/vtx_%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS_VTX) $(DEPFLAGS) -c $< -o $@

$(OBJDIR)/vrx_%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS_VRX) $(DEPFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

run-vtx: vtx
	sudo ./$(VTX_TARGET)

run-vrx: vrx
	sudo ./$(VRX_TARGET)

.PHONY: all vtx vrx clean run-vtx run-vrx

-include $(VTX_OBJECTS:.o=.d)
-include $(VRX_OBJECTS:.o=.d)
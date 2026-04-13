CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -I$(INCDIR) `pkg-config --cflags libcamera libavcodec libavformat libavutil`
LDFLAGS = `pkg-config --libs libcamera libavcodec libavformat libavutil` -lpcap -pthread

INCDIR = include
SRCDIR = src
OBJDIR = obj
BINDIR = bin

TARGET = $(BINDIR)/prismvtx
SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/camera_app.cpp $(SRCDIR)/h264_encoder.cpp
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BINDIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

run: all
	sudo ./$(TARGET)

.PHONY: all clean run
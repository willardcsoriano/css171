CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
LDLIBS   := -lglut -lGL -lGLU

TARGET := CSS171_MP2_Soriano
VIDEO  := mountain_sample_output.mp4

# Program window size (matches the source); the demo layout is built around it
APP_WIDTH  := 1000
APP_HEIGHT := 700

.PHONY: all run video preview clean

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CXX) $(CXXFLAGS) $(TARGET).cpp -o $@ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

# Record a 1920x1080 demo video: a terminal builds the program from source
# on camera, then runs it for 10 seconds (see tools/record-demo.sh)
video:
	../tools/record-demo.sh $(TARGET) $(APP_WIDTH) $(APP_HEIGHT) 10 $(VIDEO)

# Same as video, plus preview.png and preview.gif cropped to the program window
preview:
	RECORD_PREVIEW=1 ../tools/record-demo.sh $(TARGET) $(APP_WIDTH) $(APP_HEIGHT) 10 $(VIDEO)

clean:
	rm -f $(TARGET)

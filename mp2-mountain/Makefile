CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
LDLIBS   := -lglut -lGL -lGLU

TARGET := CSS171_MP2_Soriano
VIDEO  := mountain_sample_output.mp4

# Program window size (matches the source); previews are cropped to it
APP_WIDTH  := 1000
APP_HEIGHT := 700

.PHONY: all run video preview clean

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CXX) $(CXXFLAGS) $(TARGET).cpp -o $@ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

# Record a demo video on a virtual screen: a terminal builds the program
# from source on camera, then runs it for 10 seconds (see tools/record-demo.sh)
video:
	../tools/record-demo.sh $(TARGET) 10 $(VIDEO)

# Still frame and short GIF of the running program, cropped from the video
preview: video
	ffmpeg -y -loglevel error -sseof -3 -i $(VIDEO) -vf crop=$(APP_WIDTH):$(APP_HEIGHT):0:0 \
		-frames:v 1 -update 1 preview.png
	ffmpeg -y -loglevel error -sseof -6 -t 4 -i $(VIDEO) \
		-vf "crop=$(APP_WIDTH):$(APP_HEIGHT):0:0,fps=10,scale=500:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse" preview.gif
	@echo "Generated preview.png and preview.gif"

clean:
	rm -f $(TARGET)

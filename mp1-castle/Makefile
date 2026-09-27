CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
LDLIBS   := -lglut -lGL -lGLU

TARGET := CSS171_MP1_Soriano
VIDEO  := castle2d_sample_output.mp4

# Program window size (matches the source); previews are cropped to it
APP_WIDTH  := 512
APP_HEIGHT := 412

.PHONY: all run video preview export compare clean

all: $(TARGET)

$(TARGET): $(TARGET).cpp third_party/stb_image_write.h
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

# Render straight to attempt.png on a virtual screen, so it works headless.
# Xvfb -displayfd picks a free display number and reports it once ready.
export: $(TARGET)
	@command -v Xvfb >/dev/null || { echo "export: Xvfb is missing; sudo apt install xvfb" >&2; exit 1; }
	@DISPLAY_FILE=$$(mktemp); \
	Xvfb -displayfd 3 -screen 0 $(APP_WIDTH)x$(APP_HEIGHT)x24 -nolisten tcp 3>"$$DISPLAY_FILE" 2>/dev/null & XVFB_PID=$$!; \
	for i in $$(seq 50); do [ -s "$$DISPLAY_FILE" ] && break; sleep 0.1; done; \
	if [ ! -s "$$DISPLAY_FILE" ]; then echo "export: Xvfb did not start" >&2; kill $$XVFB_PID 2>/dev/null; rm -f "$$DISPLAY_FILE"; exit 1; fi; \
	DISPLAY=:$$(head -n1 "$$DISPLAY_FILE") ./$(TARGET) --export attempt.png; \
	STATUS=$$?; kill $$XVFB_PID 2>/dev/null; rm -f "$$DISPLAY_FILE"; exit $$STATUS
	@echo "Generated attempt.png"

# Pixel diff against the reference drawing to-copy.png (needs ImageMagick)
compare: export
	compare -metric AE attempt.png to-copy.png diff.png || true
	@echo "Diff image written to diff.png (highlighted pixels = mismatches)"

clean:
	rm -f $(TARGET) attempt.png diff.png

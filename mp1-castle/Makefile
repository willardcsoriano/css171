CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
LDLIBS   := -lglut -lGL -lGLU

TARGET := CSS171_MP1_Soriano
VIDEO  := castle2d_sample_output.mp4

# Headless recording: virtual display number and screen size (the screen
# must be at least the window size). Override XVFB_DISPLAY if :96 is taken.
XVFB_DISPLAY  ?= 96
RECORD_SCREEN := 512x412

.PHONY: all run video preview export compare clean

all: $(TARGET)

$(TARGET): $(TARGET).cpp third_party/stb_image_write.h
	$(CXX) $(CXXFLAGS) $(TARGET).cpp -o $@ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

# Record a 10-second sample output video on a virtual X display
video: $(TARGET)
	@Xvfb :$(XVFB_DISPLAY) -screen 0 $(RECORD_SCREEN)x24 >/dev/null 2>&1 & XVFB_PID=$$!; \
	sleep 1; \
	DISPLAY=:$(XVFB_DISPLAY) ./$(TARGET) & APP_PID=$$!; \
	sleep 1.5; \
	ffmpeg -y -loglevel error -draw_mouse 0 -f x11grab -video_size $(RECORD_SCREEN) -framerate 30 \
		-i :$(XVFB_DISPLAY).0 -t 10 -c:v libx264 -pix_fmt yuv420p $(VIDEO); \
	STATUS=$$?; kill $$APP_PID $$XVFB_PID 2>/dev/null; exit $$STATUS
	@echo "Generated $(VIDEO)"

# Still frame and short GIF taken from the sample video
preview: video
	ffmpeg -y -loglevel error -ss 2 -i $(VIDEO) -frames:v 1 -update 1 preview.png
	ffmpeg -y -loglevel error -t 4 -i $(VIDEO) \
		-vf "fps=10,scale=500:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse" preview.gif
	@echo "Generated preview.png and preview.gif"

# Render straight to attempt.png (on a virtual display, so it works headless)
export: $(TARGET)
	@Xvfb :$(XVFB_DISPLAY) -screen 0 $(RECORD_SCREEN)x24 >/dev/null 2>&1 & XVFB_PID=$$!; \
	sleep 1; \
	DISPLAY=:$(XVFB_DISPLAY) ./$(TARGET) --export attempt.png; \
	STATUS=$$?; kill $$XVFB_PID 2>/dev/null; exit $$STATUS
	@echo "Generated attempt.png"

# Pixel diff against the reference drawing to-copy.png (needs ImageMagick)
compare: export
	compare -metric AE attempt.png to-copy.png diff.png || true
	@echo "Diff image written to diff.png (highlighted pixels = mismatches)"

clean:
	rm -f $(TARGET) attempt.png diff.png

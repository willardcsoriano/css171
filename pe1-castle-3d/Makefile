CC       := gcc
CXX      := g++
CFLAGS   := -O2 -Wall -Ithird_party/glad/include
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Ithird_party/glad/include
LDLIBS   := -lglfw -lGL -ldl

TARGET     := CSS171_PE1_Soriano
GLAD_OBJ   := third_party/glad/src/glad.o
VIDEO      := castle_sample_output.mp4

# Headless recording: virtual display number and screen size (the screen
# must be at least the window size). Override XVFB_DISPLAY if :97 is taken.
XVFB_DISPLAY  ?= 97
RECORD_SCREEN := 1000x700

.PHONY: all run video preview textures clean

all: $(TARGET)

$(GLAD_OBJ): third_party/glad/src/glad.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(TARGET).cpp $(GLAD_OBJ) third_party/stb_image.h
	$(CXX) $(CXXFLAGS) $(TARGET).cpp $(GLAD_OBJ) -o $@ $(LDLIBS)

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

# Regenerate the procedural textures (requires Pillow)
textures:
	python3 tools/generate_textures.py textures

clean:
	rm -f $(TARGET) $(GLAD_OBJ)

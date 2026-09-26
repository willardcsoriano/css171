CC       := gcc
CXX      := g++
CFLAGS   := -O2 -Wall -Ithird_party/glad/include
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Ithird_party/glad/include
LDLIBS   := -lglfw -lGL -ldl

TARGET     := MP3_CSS171_Soriano
GLAD_OBJ   := third_party/glad/src/glad.o
VIDEO      := cubes_sample_output.mp4
SUBMISSION := $(TARGET).zip

# Headless recording: virtual display number and screen size (the screen
# must be at least the window size). Override XVFB_DISPLAY if :98 is taken.
XVFB_DISPLAY  ?= 98
RECORD_SCREEN := 1000x700

.PHONY: all run video preview zip clean

all: $(TARGET)

$(GLAD_OBJ): third_party/glad/src/glad.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(TARGET).cpp $(GLAD_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

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

# Submission archive: the complete project folder named MP3_CSS171_Soriano
zip:
	rm -rf build/$(TARGET) $(SUBMISSION)
	mkdir -p build/$(TARGET)/third_party/glad
	cp $(TARGET).cpp Makefile build/$(TARGET)/
	cp -r third_party/glad/include third_party/glad/src build/$(TARGET)/third_party/glad/
	rm -f build/$(TARGET)/third_party/glad/src/*.o
	cd build && python3 -m zipfile -c ../$(SUBMISSION) $(TARGET)
	rm -rf build
	@echo "Generated $(SUBMISSION)"

clean:
	rm -f $(TARGET) $(GLAD_OBJ) $(SUBMISSION)
	rm -rf build

CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
LDLIBS := -lglut -lGL -lGLU

TARGET := CSS171_MP2_Soriano

.PHONY: all run video preview clean

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

video: $(TARGET)
	@echo "Recording 10-second sample output video..."
	@Xvfb :99 -screen 0 1000x700x24 >/dev/null 2>&1 & XVFB_PID=$$!; \
	sleep 1; \
	DISPLAY=:99 ./$(TARGET) >/dev/null 2>&1 & APP_PID=$$!; \
	sleep 1.5; \
	ffmpeg -y -draw_mouse 0 -f x11grab -video_size 1000x700 -framerate 30 -i :99.0 -t 10 -c:v libx264 -pix_fmt yuv420p mountain_sample_output.mp4 >/dev/null 2>&1; \
	kill $$APP_PID $$XVFB_PID 2>/dev/null || true; \
	echo "Video successfully generated: mountain_sample_output.mp4"

preview: $(TARGET)
	@echo "Generating preview image and animated GIF..."
	@Xvfb :99 -screen 0 1000x700x24 >/dev/null 2>&1 & XVFB_PID=$$!; \
	sleep 1; \
	DISPLAY=:99 ./$(TARGET) >/dev/null 2>&1 & APP_PID=$$!; \
	sleep 1.5; \
	ffmpeg -y -draw_mouse 0 -f x11grab -video_size 1000x700 -framerate 30 -i :99.0 -t 5 -c:v libx264 -pix_fmt yuv420p mountain_sample_output.mp4 >/dev/null 2>&1; \
	ffmpeg -y -i mountain_sample_output.mp4 -ss 00:00:01 -vframes 1 preview.png >/dev/null 2>&1; \
	ffmpeg -y -i mountain_sample_output.mp4 -t 4 -vf "fps=10,scale=500:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse" preview.gif >/dev/null 2>&1; \
	kill $$APP_PID $$XVFB_PID 2>/dev/null || true; \
	echo "Generated preview.png, preview.gif, and mountain_sample_output.mp4"

clean:
	rm -f $(TARGET) *.bmp *.mp4 *.gif *.png

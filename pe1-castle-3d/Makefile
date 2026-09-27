CC       := gcc
CXX      := g++
CFLAGS   := -O2 -Wall -Ithird_party/glad/include
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Ithird_party/glad/include
LDLIBS   := -lglfw -lGL -ldl

TARGET     := CSS171_PE1_Soriano
GLAD_OBJ   := third_party/glad/src/glad.o
VIDEO      := castle_sample_output.mp4
SUBMISSION := $(TARGET).zip

# Program window size (matches the source); the demo layout is built around it
APP_WIDTH  := 1000
APP_HEIGHT := 700

.PHONY: all run video preview textures zip submission clean

all: $(TARGET)

$(GLAD_OBJ): third_party/glad/src/glad.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(TARGET).cpp $(GLAD_OBJ) third_party/stb_image.h
	$(CXX) $(CXXFLAGS) $(TARGET).cpp $(GLAD_OBJ) -o $@ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

# Record a 1920x1080 demo video: a terminal builds the program from source
# on camera, then runs it for 10 seconds (see tools/record-demo.sh)
video:
	../tools/record-demo.sh $(TARGET) $(APP_WIDTH) $(APP_HEIGHT) 10 $(VIDEO)

# Same as video, plus preview.png and preview.gif cropped to the program window
preview:
	RECORD_PREVIEW=1 ../tools/record-demo.sh $(TARGET) $(APP_WIDTH) $(APP_HEIGHT) 10 $(VIDEO)

# Regenerate the procedural textures (requires Pillow)
textures:
	python3 tools/generate_textures.py textures

# Complete project folder, zipped: source, textures/, third_party/ (glad and
# stb_image), Makefile and BUILD.txt, everything needed to build and run
zip:
	rm -rf build/$(TARGET) $(SUBMISSION)
	mkdir -p build/$(TARGET)/third_party/glad
	cp $(TARGET).cpp Makefile BUILD.txt build/$(TARGET)/
	cp -r textures build/$(TARGET)/
	cp third_party/stb_image.h build/$(TARGET)/third_party/
	cp -r third_party/glad/include third_party/glad/src build/$(TARGET)/third_party/glad/
	rm -f build/$(TARGET)/third_party/glad/src/*.o
	cd build && python3 -m zipfile -c ../$(SUBMISSION) $(TARGET)
	rm -rf build
	@echo "Generated $(SUBMISSION)"

# Everything to upload via the Google Drive link. The handout conflicts: its
# header asks for a zip, while its text and score sheet ask for
# CSS171_PE1_Surname.cpp and the video, not compressed. This satisfies both:
# the .cpp and the video as loose files, plus the zipped project folder.
submission: zip
	rm -rf submission
	mkdir submission
	cp $(TARGET).cpp $(SUBMISSION) submission/
	cp $(VIDEO) submission/$(TARGET)_sample_output.mp4
	@echo "Ready to upload: submission/"; ls -1 submission
	@echo "How to submit (per the handout): put these files in a Google Drive folder,"
	@echo "share it so your instructor can open it, and submit ONLY the Drive link"
	@echo "on Blackboard (BB). Do not upload the files to BB directly."

clean:
	rm -f $(TARGET) $(GLAD_OBJ) $(SUBMISSION)
	rm -rf build

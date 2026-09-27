CC       := gcc
CXX      := g++
CFLAGS   := -O2 -Wall -Ithird_party/glad/include
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Ithird_party/glad/include
LDLIBS   := -lglfw -lGL -ldl

TARGET     := MP3_CSS171_Soriano
GLAD_OBJ   := third_party/glad/src/glad.o
VIDEO      := cubes_sample_output.mp4
SUBMISSION := $(TARGET).zip

# Program window size (matches the source); the demo layout is built around it
APP_WIDTH  := 1000
APP_HEIGHT := 700

.PHONY: all run video preview zip submission clean

all: $(TARGET)

$(GLAD_OBJ): third_party/glad/src/glad.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(TARGET).cpp $(GLAD_OBJ)
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

# Submission archive: the complete project folder named MP3_CSS171_Soriano
zip:
	rm -rf build/$(TARGET) $(SUBMISSION)
	mkdir -p build/$(TARGET)/third_party/glad
	cp $(TARGET).cpp Makefile BUILD.txt build/$(TARGET)/
	cp -r third_party/glad/include third_party/glad/src build/$(TARGET)/third_party/glad/
	rm -f build/$(TARGET)/third_party/glad/src/*.o
	cd build && python3 -m zipfile -c ../$(SUBMISSION) $(TARGET)
	rm -rf build
	@echo "Generated $(SUBMISSION)"

# Everything to upload: the zipped project folder and the sample output video
submission: zip
	rm -rf submission
	mkdir submission
	cp $(SUBMISSION) submission/
	cp $(VIDEO) submission/$(TARGET)_sample_output.mp4
	@echo "Ready to upload: submission/"; ls -1 submission
	@echo "How to submit: the handout names the file (MP3_CSS171_Surname.zip) but gives"
	@echo "no upload method (no Google Drive link is required). Submit the zip and the"
	@echo "video through the Machine Problem 3.1 item on Blackboard."

clean:
	rm -f $(TARGET) $(GLAD_OBJ) $(SUBMISSION)
	rm -rf build

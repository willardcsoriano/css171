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

.PHONY: all run video preview zip clean

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
	cp $(TARGET).cpp Makefile build/$(TARGET)/
	cp -r third_party/glad/include third_party/glad/src build/$(TARGET)/third_party/glad/
	rm -f build/$(TARGET)/third_party/glad/src/*.o
	cd build && python3 -m zipfile -c ../$(SUBMISSION) $(TARGET)
	rm -rf build
	@echo "Generated $(SUBMISSION)"

clean:
	rm -f $(TARGET) $(GLAD_OBJ) $(SUBMISSION)
	rm -rf build

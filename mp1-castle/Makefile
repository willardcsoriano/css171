CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
LDLIBS := -lglut -lGL -lGLU

TARGET := CSS171_MP1_Soriano

.PHONY: all export compare clean

all: $(TARGET)

$(TARGET): $(TARGET).cpp third_party/stb_image_write.h
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDLIBS)

export: $(TARGET)
	./$(TARGET) --export attempt.png

compare: export
	compare -metric AE attempt.png to-copy.png diff.png || true
	@echo "Diff image written to diff.png (highlighted pixels = mismatches)"

clean:
	rm -f $(TARGET) attempt.png diff.png

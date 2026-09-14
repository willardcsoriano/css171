CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
LDLIBS := -lglut -lGL -lGLU

.PHONY: all export compare clean

all: castle

castle: castle.cpp third_party/stb_image_write.h
	$(CXX) $(CXXFLAGS) castle.cpp -o castle $(LDLIBS)

export: castle
	./castle --export attempt.png

compare: export
	compare -metric AE attempt.png to-copy.png diff.png || true
	@echo "Diff image written to diff.png (highlighted pixels = mismatches)"

clean:
	rm -f castle attempt.png diff.png

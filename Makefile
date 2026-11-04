# Car Showroom Configurator - build with `make`, run with `make run`
# (or ./car_showroom from the project root, so shader/asset paths resolve).

TARGET   := car_showroom
CXX      := g++
CC       := gcc
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
CFLAGS   := -O2
INCLUDES := -Isrc -isystem third_party -isystem third_party/glad/include -isystem third_party/imgui
LIBS     := -lglut -lGL -ldl

BUILD    := build

SRC      := $(wildcard src/*.cpp)
IMGUI    := third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp \
            third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp \
            third_party/imgui/backends/imgui_impl_glut.cpp \
            third_party/imgui/backends/imgui_impl_opengl3.cpp
GLAD     := third_party/glad/src/glad.c

OBJ      := $(SRC:%.cpp=$(BUILD)/%.o) $(IMGUI:%.cpp=$(BUILD)/%.o) $(GLAD:%.c=$(BUILD)/%.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $^ -o $@ $(LIBS)

$(BUILD)/src/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Third-party code: compiled without our warning flags.
$(BUILD)/third_party/%.o: third_party/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -std=c++17 -O2 $(INCLUDES) -c $< -o $@

$(BUILD)/third_party/%.o: third_party/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD) $(TARGET)

-include $(OBJ:.o=.d)

.PHONY: all run clean

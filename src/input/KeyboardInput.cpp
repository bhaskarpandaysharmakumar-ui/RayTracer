#include "KeyboardInput.h"

#include <iostream>

void KeyboardInput::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    for (int i = 0; i < 350; ++i) {
        if (key == i && action == GLFW_PRESS) {
            GetInstance().keys_pressed[i] = true;
            GetInstance().keys_clicked[i] = true;
        }

        if (key == i && action == GLFW_RELEASE) {
            GetInstance().keys_pressed[i] = false;
        }
    }
}

void KeyboardInput::EndFrame() {
    for (bool &key : GetInstance().keys_clicked) {
        key = false;
    }
}

bool KeyboardInput::KeyPressed(unsigned int key) {
    if (!(key > 0 && key < 350)) {
        std::cout << "KeyboardInput: Input data for the key provided does not exist!\n";
        return false;
    }

    return GetInstance().keys_pressed[key];
}

bool KeyboardInput::KeyClicked(unsigned int key) {
    if (!(key > 0 && key < 350)) {
        std::cout << "KeyboardInput: Input data for the key provided does not exist!\n";
        return false;
    }

    return GetInstance().keys_clicked[key];
}

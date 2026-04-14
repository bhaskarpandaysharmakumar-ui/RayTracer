#pragma once

#include <GLFW/glfw3.h>

class KeyboardInput {
public:
    static KeyboardInput &GetInstance() {
        static KeyboardInput instance;
        return instance;
    }

    KeyboardInput(KeyboardInput const&) = delete;
    void operator=(KeyboardInput const&) = delete;

    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void EndFrame();
    static bool KeyPressed(unsigned int key);
    static bool KeyClicked(unsigned int key);

private:
    KeyboardInput() {}

    bool keys_pressed[350];
    bool keys_clicked[350];
};

#pragma once

#include <GLFW/glfw3.h>

class MouseInput {
public:
    static MouseInput &GetInstance() {
        static MouseInput instance;
        return instance;
    }

    MouseInput(MouseInput const&) = delete;
    void operator=(MouseInput const&) = delete;

    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void EndFrame();
    static float GetOffsetX();
    static float GetOffsetY();
    static float GetPosX();
    static float GetPosY();

private:
    MouseInput() {};

    float posX, posY, lastPosX, lastPosY;
    static float Signum(float x);
};
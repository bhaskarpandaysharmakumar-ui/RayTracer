#include "MouseInput.h"

void MouseInput::CursorPosCallback(GLFWwindow *window, double xpos, double ypos) {

    GetInstance().posX = (float) xpos;
    GetInstance().posY = (float) ypos;
}

void MouseInput::EndFrame() {
    GetInstance().lastPosX = GetInstance().posX;
    GetInstance().lastPosY = GetInstance().posY;
}

float MouseInput::GetOffsetX() {
    return GetInstance().posX - GetInstance().lastPosX;
}

float MouseInput::GetOffsetY() {
    return -(GetInstance().posY - GetInstance().lastPosY); // cuz in glfw origin is in the top left corner
}

float MouseInput::GetPosX() {
    return GetInstance().posY;
}

float MouseInput::GetPosY() {
    return GetInstance().posX;
}

float MouseInput::Signum(float x) {
    if (x == 0.f) return x;

    return x > 0.f ? 1.f : -1.f;
}

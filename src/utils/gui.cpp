#include "utils/gui.hpp"

#include <raylib.h>

#include <cstdlib>
#include <cstddef>
#include <string>

using namespace Utils;

namespace {

    void DrawCenteredText(const std::string& message, int y, Color color) {
        const int textWidth = MeasureText(message.c_str(), 20);
        DrawText(message.c_str(), (GetScreenWidth() - textWidth) / 2, y, 20, color);
    }

    int DrawTextBox(std::string message, int x, int y, int width, bool draw) {
        while (!message.empty()) {
            std::string current = message;

            // take one line at a time
            const size_t pos = current.find('\n');
            if (pos != std::string::npos)
                current = current.substr(0, pos);

            // if too long, remove word or character
            while (MeasureText(message.c_str(), 20) > width) {
                const size_t pos = current.find_last_of(' ');
                if (pos == std::string::npos)
                    current = current.substr(0, current.size() - 1);
                else
                    current = current.substr(0, pos);
            }

            // remove the current line from the message text
            message = message.substr(current.size());
            while (message.starts_with(' '))
                message = message.substr(1); // remove leading space
            while (message.starts_with('\n'))
                message = message.substr(1); // remove leading newline

            // draw the current line
            if (draw) DrawText(current.c_str(), x, y, 20, Color { 198, 173, 173, 255 });
            y += 25; // move down for the next line
        }
        return y; // return height of the text box
    }
}

void Utils::showErrorGui(const std::string& message) {
    const int height = DrawTextBox(message, 10, 60, 780, false);
    InitWindow(800, height + 80, "lsfg-vk - Error");

    SetTargetFPS(24); // cinema frame rate lol

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(Color{ 58, 23, 32, 255 });
        DrawCenteredText("lsfg-vk encountered an error", 10, Color{ 225, 115, 115, 255 });
        DrawLine(10, 35, 790, 35, Color{ 218, 87, 87, 255 });
        DrawTextBox(message, 10, 60, 780, true);

        const bool hover = GetMouseY() > height + 30
                && GetMouseY() < height + 70
                && GetMouseX() > 10
                && GetMouseX() < 790;
        if (hover) {
            DrawRectangleLines(10, height + 30, 780, 40, Color{ 250, 170, 151, 255 });
            DrawCenteredText("Press Escape or click here to close", height + 40, Color{ 253, 180, 170, 255 });
        } else {
            DrawRectangleLines(10, height + 30, 780, 40, Color{ 218, 87, 87, 255 });
            DrawCenteredText("Press Escape or click here to close", height + 40, Color{ 225, 115, 115, 255 });
        }

        EndDrawing();

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            break;
    }

    CloseWindow();

    exit(EXIT_FAILURE);
}

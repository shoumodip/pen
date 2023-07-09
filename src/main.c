#include "pen.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>

Color colorFromHex(Hex color) {
  return (Color){
    (color >> (3 * 8)) & 0xFF,
    (color >> (2 * 8)) & 0xFF,
    (color >> (1 * 8)) & 0xFF,
    (color >> (0 * 8)) & 0xFF,
  };
}

void platformClear(Hex color) {
  ClearBackground(colorFromHex(color));
}

void platformError(char *data) {
  TraceLog(LOG_WARNING, "%s", data);
}

void platformDrawLine(int x1, int y1, int x2, int y2, Hex color) {
  DrawLine(x1, y1, x2, y2, colorFromHex(color));
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "ERROR: file path not provided\n");
    fprintf(stderr, "USAGE: %s <file>\n", *argv);
    return 1;
  }
  char *file_path = argv[1];

  InitWindow(WIDTH, HEIGHT, "Pen");

  char *data = LoadFileText(file_path);
  if (data) {
    penUpdate(data, strlen(data));
    UnloadFileText(data);
  }

  while (!WindowShouldClose()) {
    BeginDrawing();
    penRender();
    EndDrawing();

    if (IsKeyPressed(KEY_R)) {
      char *data = LoadFileText(file_path);
      if (data) {
        penUpdate(data, strlen(data));
        UnloadFileText(data);
      }
    }
  }
  CloseWindow();
}

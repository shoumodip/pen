#include "pen.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>

void platformClear(void) {
  ClearBackground(RAYWHITE);
}

void platformError(char *data) {
  TraceLog(LOG_WARNING, "%s", data);
}

void platformDrawLine(int x1, int y1, int x2, int y2) {
  DrawLine(x1, y1, x2, y2, BLACK);
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

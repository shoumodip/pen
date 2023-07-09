#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <raylib.h>

// Str
typedef struct {
  char *data;
  int size;
} Str;

#define STR(data) ((Str){data, sizeof(data) - 1})

int strEq(Str a, Str b) {
  return a.size == b.size && !memcmp(a.data, b.data, b.size);
}

Str strDrop(Str s, int n) {
  if (n >= s.size) {
    return (Str){0};
  }
  return (Str){s.data + n, s.size - n};
}

Str strTrim(Str s, char ch) {
  int n = 0;
  while (n < s.size && s.data[n] == ch) {
    n++;
  }
  return strDrop(s, n);
}

Str strSplit(Str *s, char ch) {
  Str result = *s;

  for (int i = 0; i < s->size; i++) {
    if (s->data[i] == ch) {
      result.size = i;
      break;
    }
  }

  *s = strDrop(*s, result.size + 1);
  return result;
}

int strParseInt(Str s, int *out) {
  *out = 0;

  for (int i = 0; i < s.size; i++) {
    char ch = s.data[i];
    if (ch < '0' || ch > '9') {
      return 0;
    }
    *out = *out * 10 + ch - '0';
  }

  return 1;
}

#define WIDTH 800
#define HEIGHT 600

// Op
typedef enum {
  OP_LEFT,
  OP_RIGHT,
  OP_FORWARD,
  OP_BACKWARD
} OpType;

typedef struct {
  OpType type;
  int data;
} Op;

// Canvas
#define CANVAS_CAP 1024

typedef struct {
  Vector2 data[CANVAS_CAP];
  int count;
} Canvas;

int canvasPush(Canvas *c, Vector2 v) {
  if (c->count >= CANVAS_CAP) {
    fprintf(stderr, "error: canvas overflow\n");
    return 0;
  }

  c->data[c->count++] = v;
  return 1;
}

// Program
#define PROGRAM_CAP 1024

typedef struct {
  Op data[PROGRAM_CAP];
  int count;
} Program;

int programPush(Program *p, Op op) {
  if (p->count >= PROGRAM_CAP) {
    fprintf(stderr, "error: program overflow\n");
    return 0;
  }

  p->data[p->count++] = op;
  return 1;
}

int programEval(Program *p, Canvas *c) {
  float angle = 0.0;
  Vector2 point = {WIDTH / 2.0, HEIGHT / 2.0};

  c->count = 0;
  if (!canvasPush(c, point)) {
    return 0;
  }

  for (int i = 0; i < p->count; i++) {
    Op op = p->data[i];
    switch (op.type) {
    case OP_LEFT:
      angle -= op.data * DEG2RAD;
      break;

    case OP_RIGHT:
      angle += op.data * DEG2RAD;
      break;

    case OP_FORWARD:
      point.x += op.data * cosf(angle);
      point.y += op.data * sinf(angle);
      if (!canvasPush(c, point)) {
        return 0;
      }
      break;

    case OP_BACKWARD:
      point.x -= op.data * cosf(angle);
      point.y -= op.data * sinf(angle);
      if (!canvasPush(c, point)) {
        return 0;
      }
      break;
    }
  }

  return 1;
}

int programRead(Program *p, Str s) {
  Str words[] = {
    [OP_LEFT] = STR("left"),
    [OP_RIGHT] = STR("right"),
    [OP_FORWARD] = STR("forward"),
    [OP_BACKWARD] = STR("backward"),
  };

  int ok = 1;
  for (int row = 1; s.size; row++) {
    Str line = strTrim(strSplit(&s, '\n'), ' ');
    if (!line.size) {
      continue;
    }
    Str word = strTrim(strSplit(&line, ' '), ' ');

    Op op;
    int valid = 0;
    for (int i = 0; i < sizeof(words) / sizeof(*words); i++) {
      if (strEq(word, words[i])) {
        valid = 1;
        op.type = i;
        break;
      }
    }

    if (!valid) {
      fprintf(stderr, "error: invalid word '%.*s' in line %d\n", word.size, word.data, row);
      ok = 0;
    }

    word = strTrim(line, ' ');

    if (!strParseInt(word, &op.data)) {
      fprintf(stderr, "error: invalid count '%.*s' in line %d\n", word.size, word.data, row);
      ok = 0;
    }

    if (ok && !programPush(p, op)) {
      return 0;
    }
  }

  return ok;
}

// Main
int readFile(char *path, Str *out) {
  FILE *file = fopen(path, "r");
  if (!file) {
    return 0;
  }

  if (fseek(file, 0, SEEK_END) == -1) {
    fclose(file);
    return 0;
  }

  out->size = ftell(file);
  if (out->size == -1) {
    fclose(file);
    return 0;
  }
  rewind(file);

  out->data = malloc(out->size);
  if (!out->data) {
    fclose(file);
    return 0;
  }

  if (fread(out->data, 1, out->size, file) != out->size) {
    free(out->data);
    fclose(file);
    return 0;
  }

  return 1;
}

Canvas canvas;
Program program;

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "error: file path not provided\n");
    fprintf(stderr, "usage: %s <file>\n", *argv);
    exit(1);
  }
  char *path = argv[1];

  Str contents;
  if (!readFile(path, &contents)) {
    fprintf(stderr, "error: could not read file '%s'\n", path);
    exit(1);
  }

  if (!programRead(&program, contents)) {
    exit(1);
  }

  if (!programEval(&program, &canvas)) {
    exit(1);
  }

  InitWindow(WIDTH, HEIGHT, "Pen");
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    for (int i = 1; i < canvas.count; i++) {
      DrawLineV(canvas.data[i - 1], canvas.data[i], BLACK);
    }
    EndDrawing();
  }
  CloseWindow();
}

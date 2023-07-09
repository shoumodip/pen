#include <assert.h>
#include <math.h>
#include <raylib.h>

#define WIDTH 800
#define HEIGHT 600

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

#define CANVAS_CAP 1024

typedef struct {
  Vector2 data[CANVAS_CAP];
  int count;
} Canvas;

void canvasPush(Canvas *c, Vector2 v) {
  assert(c->count < CANVAS_CAP);
  c->data[c->count++] = v;
}

#define PROGRAM_CAP 1024

typedef struct {
  Op data[PROGRAM_CAP];
  int count;
} Program;

void programPush(Program *p, Op op) {
  assert(p->count < PROGRAM_CAP);
  p->data[p->count++] = op;
}

void programEval(Program *p, Canvas *c) {
  float angle = 0.0;
  Vector2 point = {WIDTH / 2.0, HEIGHT / 2.0};

  canvasPush(c, point);
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
      canvasPush(c, point);
      break;

    case OP_BACKWARD:
      point.x -= op.data * cosf(angle);
      point.y -= op.data * sinf(angle);
      canvasPush(c, point);
      break;
    }
  }
}

Canvas canvas;
Program program;

int main(void) {
  programPush(&program, (Op){OP_FORWARD, 50});
  programPush(&program, (Op){OP_LEFT, 72});
  programPush(&program, (Op){OP_FORWARD, 50});
  programPush(&program, (Op){OP_LEFT, 72});
  programPush(&program, (Op){OP_FORWARD, 50});
  programPush(&program, (Op){OP_LEFT, 72});
  programPush(&program, (Op){OP_FORWARD, 50});
  programPush(&program, (Op){OP_LEFT, 72});
  programPush(&program, (Op){OP_FORWARD, 50});
  programEval(&program, &canvas);

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

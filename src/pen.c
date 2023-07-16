#include "pen.h"

#define ELANG_IMPLEMENTATION
#include "elang.h"

// Math
#define PI 3.14159265
#define EPS 1e-6

float remf(float x, float y) {
  return x - (int)(x / y) * y;
}

float absf(float x) {
  if (x < 0) {
    return -x;
  }
  return x;
}

float sinf(float x) {
  float s = x;
  float t = x;
  for (int i = 1; absf(t / s) > EPS; i++) {
    t *= -x * x / ((2 * i + 1) * 2 * i);
    s += t;
  }
  return s;
}

float cosf(float x) {
  float s = 1;
  float t = 1;
  for (int i = 1; absf(t / s) > EPS; i++) {
    t *= -x * x / ((2 * i - 1) * 2 * i);
    s += t;
  }
  return s;
}

// Canvas
#define CANVAS_CAP 1024

int canvasCount;
float canvasXs[CANVAS_CAP];
float canvasYs[CANVAS_CAP];
float canvasAngle;

float canvasMove(float *arg) {
  if (canvasCount < CANVAS_CAP) {
    canvasXs[canvasCount] = canvasXs[canvasCount - 1] + *arg * cosf(canvasAngle);
    canvasYs[canvasCount] = canvasYs[canvasCount - 1] + *arg * sinf(canvasAngle);
    canvasCount++;
  }
  return 0;
}

float canvasRotate(float *arg) {
  canvasAngle = remf(canvasAngle - *arg * PI / 180, PI * 2);
  return 0;
}

// Exports
void penInit(void) {
  elangRegisterNative("move", 1, canvasMove);
  elangRegisterNative("rotate", 1, canvasRotate);
}

void penRender(int w, int h) {
  w /= 2;
  h /= 2;

  platformClear();
  for (int i = 1; i < canvasCount; i++) {
    platformDrawLine(w + (int)canvasXs[i - 1], h + (int)canvasYs[i - 1], w + (int)canvasXs[i],
                     h + (int)canvasYs[i]);
  }
}

void penUpdate(char *data, int size) {
  canvasAngle = 0;
  canvasCount = 1;
  elangCompile(data, size) && elangRun();
}

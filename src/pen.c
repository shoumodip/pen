#include "pen.h"

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

// Str
typedef struct {
  char *data;
  int count;
} Str;

#define STR(data) ((Str){data, sizeof(data) - 1})

int strEq(Str a, Str b) {
  if (a.count != b.count) {
    return 0;
  }

  for (int i = 0; i < a.count; i++) {
    if (a.data[i] != b.data[i]) {
      return 0;
    }
  }

  return 1;
}

Str strDrop(Str s, int n) {
  if (n >= s.count) {
    return (Str){0};
  }
  return (Str){s.data + n, s.count - n};
}

Str strTrim(Str s, char ch) {
  int n = 0;
  while (n < s.count && s.data[n] == ch) {
    n++;
  }
  return strDrop(s, n);
}

Str strSplit(Str *s, char ch) {
  Str result = *s;

  for (int i = 0; i < s->count; i++) {
    if (s->data[i] == ch) {
      result.count = i;
      break;
    }
  }

  *s = strDrop(*s, result.count + 1);
  return result;
}

int strParseInt(Str s, int *out) {
  if (!s.count) {
    return 0;
  }

  *out = 0;

  for (int i = 0; i < s.count; i++) {
    char ch = s.data[i];
    if (ch < '0' || ch > '9') {
      return 0;
    }
    *out = *out * 10 + ch - '0';
  }

  return 1;
}

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

// Buffer
#define BUFFER_CAP 1024
char bufferData[BUFFER_CAP];
int bufferCount;

char *bufferFinish(void) {
  if (bufferCount >= BUFFER_CAP) {
    platformError("Buffer overflow");
  }

  bufferData[bufferCount++] = '\0';
  return bufferData;
}

void bufferPushInt(int n) {
  int size = 0;
  if (n) {
    for (int i = n; i; i /= 10) {
      size++;
    }
  } else {
    size = 1;
  }

  if (bufferCount + size >= BUFFER_CAP) {
    platformError("Buffer overflow");
    return;
  }

  bufferCount += size;
  if (n) {
    for (int i = 1; n; i++) {
      bufferData[bufferCount - i] = '0' + n % 10;
      n /= 10;
    }
  } else {
    bufferData[bufferCount - 1] = '0';
  }
}

void bufferPushStr(Str s) {
  if (bufferCount + s.count >= BUFFER_CAP) {
    platformError("Buffer overflow");
    return;
  }

  for (int i = 0; i < s.count; i++) {
    bufferData[bufferCount++] = s.data[i];
  }
}

// Canvas
#define CANVAS_CAP 1024
int canvasXs[CANVAS_CAP];
int canvasYs[CANVAS_CAP];
int canvasCount;

int canvasPush(int x, int y) {
  if (canvasCount >= CANVAS_CAP) {
    platformError("Canvas overflow");
    return 0;
  }

  canvasXs[canvasCount] = x;
  canvasYs[canvasCount] = y;
  canvasCount++;
  return 1;
}

// Program
#define PROGRAM_CAP 1024
Op programOps[PROGRAM_CAP];
int programCount;

int programPush(Op op) {
  if (programCount >= PROGRAM_CAP) {
    platformError("Program overflow");
    return 0;
  }

  programOps[programCount++] = op;
  return 1;
}

int programEval(void) {
  float a = 0;
  float x = 0;
  float y = 0;

  canvasCount = 0;
  canvasPush(x, y);

  for (int i = 0; i < programCount; i++) {
    Op op = programOps[i];
    switch (op.type) {
    case OP_LEFT:
      a = remf(a - op.data * PI / 180, PI * 2);
      break;

    case OP_RIGHT:
      a = remf(a + op.data * PI / 180, PI * 2);
      break;

    case OP_FORWARD:
      x += op.data * cosf(a);
      y += op.data * sinf(a);
      if (!canvasPush(x, y)) {
        return 0;
      }
      break;

    case OP_BACKWARD:
      x -= op.data * cosf(a);
      y -= op.data * sinf(a);
      if (!canvasPush(x, y)) {
        return 0;
      }
      break;
    }
  }

  return 1;
}

int programRead(Str s) {
  programCount = 0;

  static Str words[] = {
    [OP_LEFT] = STR("left"),
    [OP_RIGHT] = STR("right"),
    [OP_FORWARD] = STR("forward"),
    [OP_BACKWARD] = STR("backward"),
  };

  for (int row = 1; s.count; row++) {
    Str line = strTrim(strSplit(&s, '\n'), ' ');
    if (!line.count) {
      continue;
    }

    Op op;
    int ok = 0;
    Str word = strTrim(strSplit(&line, ' '), ' ');

    for (int i = 0; i < sizeof(words) / sizeof(*words); i++) {
      if (strEq(word, words[i])) {
        ok = 1;
        op.type = i;
        break;
      }
    }

    if (!ok) {
      bufferCount = 0;
      bufferPushStr(STR("Invalid word '"));
      bufferPushStr(word);
      bufferPushStr(STR("' in line "));
      bufferPushInt(row);
      platformError(bufferFinish());
      programCount = 0;
      return 0;
    }

    word = strTrim(line, ' ');
    if (!strParseInt(word, &op.data)) {
      bufferCount = 0;
      bufferPushStr(STR("Invalid count '"));
      bufferPushStr(word);
      bufferPushStr(STR("' in line "));
      bufferPushInt(row);
      platformError(bufferFinish());
      programCount = 0;
      return 0;
    }

    if (!programPush(op)) {
      programCount = 0;
      return 0;
    }
  }

  return 1;
}

// Exports
void penRender(int w, int h) {
  w /= 2;
  h /= 2;

  platformClear();
  for (int i = 1; i < canvasCount; i++) {
    platformDrawLine(w + canvasXs[i - 1], h + canvasYs[i - 1], w + canvasXs[i], h + canvasYs[i]);
  }
}

void penUpdate(char *data, int size) {
  if (programRead((Str){data, size})) {
    programEval();
  } else {
    canvasCount = 0;
  }
}

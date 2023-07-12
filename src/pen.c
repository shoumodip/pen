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

Str strFromInt(int n, char *buffer) {
  int size = 0;
  if (n) {
    for (int i = n; i; i /= 10) {
      size++;
    }
  } else {
    size = 1;
  }

  if (n) {
    for (int i = 1; n; i++) {
      buffer[size - i] = '0' + n % 10;
      n /= 10;
    }
  } else {
    buffer[size - 1] = '0';
  }

  return (Str){buffer, size};
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

// Error
void logError(Str *data, int count) {
  platformErrorStart();
  for (int i = 0; i < count; i++) {
    platformErrorPush(data[i].data, data[i].count);
  }
  platformErrorEnd();
}

// Token
typedef enum {
  TOKEN_EOF,
  TOKEN_NUM,
  TOKEN_IDENT,

  TOKEN_GT,
  TOKEN_GE,
  TOKEN_LT,
  TOKEN_LE,
  TOKEN_EQ,
  TOKEN_NE,
  TOKEN_NOT,

  TOKEN_ADD,
  TOKEN_SUB,
  TOKEN_MUL,
  TOKEN_DIV,
  TOKEN_SET,

  TOKEN_COMMA,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACE,
  TOKEN_RBRACE,

  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_FN,
  TOKEN_RETURN,

  TOKEN_MOVE,
  TOKEN_ROTATE,
} TokenType;

Str strFromTokenType(TokenType type) {
  switch (type) {
  case TOKEN_EOF:
    return STR("end of file");

  case TOKEN_NUM:
    return STR("number");

  case TOKEN_IDENT:
    return STR("identifier");

  case TOKEN_GT:
    return STR("'>'");

  case TOKEN_GE:
    return STR("'>='");

  case TOKEN_LT:
    return STR("'<'");

  case TOKEN_LE:
    return STR("'<='");

  case TOKEN_EQ:
    return STR("'=='");

  case TOKEN_NE:
    return STR("'!='");

  case TOKEN_NOT:
    return STR("'!'");

  case TOKEN_ADD:
    return STR("'+'");

  case TOKEN_SUB:
    return STR("'-'");

  case TOKEN_MUL:
    return STR("'*'");

  case TOKEN_DIV:
    return STR("'/'");

  case TOKEN_SET:
    return STR("'='");

  case TOKEN_COMMA:
    return STR("','");

  case TOKEN_LPAREN:
    return STR("'('");

  case TOKEN_RPAREN:
    return STR("')'");

  case TOKEN_LBRACE:
    return STR("'{'");

  case TOKEN_RBRACE:
    return STR("'}'");

  case TOKEN_IF:
    return STR("'if'");

  case TOKEN_ELSE:
    return STR("'else'");

  case TOKEN_WHILE:
    return STR("'while'");

  case TOKEN_FN:
    return STR("'fn'");

  case TOKEN_RETURN:
    return STR("'return'");

  case TOKEN_MOVE:
    return STR("'move'");

  case TOKEN_ROTATE:
    return STR("'rotate'");
  }
}

typedef struct {
  TokenType type;
  int row;
  Str str;
} Token;

// Lexer
int isdigit(int ch) {
  return ch >= '0' && ch <= '9';
}

int isalpha(int ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

int isident(int ch) {
  return ch == '_' || isalpha(ch);
}

typedef struct {
  int row;
  Str str;

  int buffer;
  Token token;
} Lexer;

Lexer lexer;

void lexerInit(Str str) {
  lexer.row = 1;
  lexer.str = str;
  lexer.buffer = 0;
}

void lexerRead(void) {
  if (*lexer.str.data == '\n') {
    lexer.row++;
  }
  lexer.str = strDrop(lexer.str, 1);
}

void lexerTrim(void) {
  while (lexer.str.count) {
    switch (*lexer.str.data) {
    case ' ':
    case '\n':
      lexerRead();
      break;

    case '#':
      while (lexer.str.count && *lexer.str.data != '\n') {
        lexerRead();
      }
      break;

    default:
      return;
    }
  }
}

int lexerMatch(char ch) {
  if (lexer.str.count && *lexer.str.data == ch) {
    lexerRead();
    return 1;
  }

  return 0;
}

int lexerNext(Token *token) {
  if (lexer.buffer) {
    lexer.buffer = 0;
    *token = lexer.token;
    return 1;
  }

  lexerTrim();

  token->row = lexer.row;
  token->str = lexer.str;

  if (!lexer.str.count) {
    token->type = TOKEN_EOF;
    return 1;
  }

  switch (*lexer.str.data) {
  case '!':
    lexerRead();
    token->type = lexerMatch('=') ? TOKEN_NE : TOKEN_NOT;
    break;

  case '>':
    lexerRead();
    token->type = lexerMatch('=') ? TOKEN_GE : TOKEN_GT;
    break;

  case '<':
    lexerRead();
    token->type = lexerMatch('=') ? TOKEN_LE : TOKEN_LT;
    break;

  case '+':
    lexerRead();
    token->type = TOKEN_ADD;
    break;

  case '-':
    lexerRead();
    token->type = TOKEN_SUB;
    break;

  case '*':
    lexerRead();
    token->type = TOKEN_MUL;
    break;

  case '/':
    lexerRead();
    token->type = TOKEN_DIV;
    break;

  case '=':
    lexerRead();
    token->type = lexerMatch('=') ? TOKEN_EQ : TOKEN_SET;
    break;

  case ',':
    lexerRead();
    token->type = TOKEN_COMMA;
    break;

  case '(':
    lexerRead();
    token->type = TOKEN_LPAREN;
    break;

  case ')':
    lexerRead();
    token->type = TOKEN_RPAREN;
    break;

  case '{':
    lexerRead();
    token->type = TOKEN_LBRACE;
    break;

  case '}':
    lexerRead();
    token->type = TOKEN_RBRACE;
    break;

  default:
    if (isdigit(*lexer.str.data)) {
      while (lexer.str.count && isdigit(*lexer.str.data)) {
        lexerRead();
      }

      token->type = TOKEN_NUM;
    } else if (isident(*lexer.str.data)) {
      while (lexer.str.count && isident(*lexer.str.data)) {
        lexerRead();
      }

      token->type = TOKEN_IDENT;
    } else {
      char buffer[20];
      Str errors[] = {
        STR("Invalid character '"),
        (Str){token->str.data, 1},
        STR("' in line "),
        strFromInt(token->row, buffer),
      };
      logError(errors, sizeof(errors) / sizeof(*errors));
      return 0;
    }
  }

  token->str.count -= lexer.str.count;

  if (token->type == TOKEN_IDENT) {
    if (strEq(token->str, STR("if"))) {
      token->type = TOKEN_IF;
    } else if (strEq(token->str, STR("else"))) {
      token->type = TOKEN_ELSE;
    } else if (strEq(token->str, STR("while"))) {
      token->type = TOKEN_WHILE;
    } else if (strEq(token->str, STR("fn"))) {
      token->type = TOKEN_FN;
    } else if (strEq(token->str, STR("return"))) {
      token->type = TOKEN_RETURN;
    } else if (strEq(token->str, STR("move"))) {
      token->type = TOKEN_MOVE;
    } else if (strEq(token->str, STR("rotate"))) {
      token->type = TOKEN_ROTATE;
    }
  }

  return 1;
}

int lexerPeek(Token *token) {
  if (!lexer.buffer) {
    if (!lexerNext(&lexer.token)) {
      return 0;
    }
    lexer.buffer = 1;
  }

  *token = lexer.token;
  return 1;
}

int lexerExpect(Token *token, TokenType type) {
  if (!lexerNext(token)) {
    return 0;
  }

  if (token->type != type) {
    char buffer[20];
    Str errors[] = {
      STR("Expected "), strFromTokenType(type),
      STR(", found "),  strFromTokenType(token->type),
      STR(" in line "), strFromInt(token->row, buffer),
    };
    logError(errors, sizeof(errors) / sizeof(*errors));
    return 0;
  }

  return 1;
}

int lexerExpectPeek(TokenType type) {
  if (!lexerExpect(&lexer.token, type)) {
    return 0;
  }

  lexer.buffer = 1;
  return 1;
}

// Op
typedef enum {
  OP_NUM,

  OP_GT,
  OP_GE,
  OP_LT,
  OP_LE,
  OP_EQ,
  OP_NE,

  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,

  OP_NOT,
  OP_NEG,

  OP_ELSE,
  OP_GOTO,
  OP_CALL,
  OP_RETURN,

  OP_DROP,
  OP_GETG,
  OP_SETG,
  OP_GETL,
  OP_SETL,

  OP_MOVE,
  OP_ROTATE
} OpType;

typedef struct {
  OpType type;
  int data;
} Op;

// Canvas
#define CANVAS_CAP 1024
int canvasXs[CANVAS_CAP];
int canvasYs[CANVAS_CAP];
int canvasCount;

int canvasPush(int x, int y) {
  if (canvasCount >= CANVAS_CAP) {
    Str errors[] = {STR("Canvas overflow")};
    logError(errors, sizeof(errors) / sizeof(*errors));
    return 0;
  }

  canvasXs[canvasCount] = x;
  canvasYs[canvasCount] = y;
  canvasCount++;
  return 1;
}

// Stack
#define STACK_CAP 1024
int stackData[STACK_CAP];
int stackCount;

int stackPop(int *out) {
  if (!stackCount) {
    return 0;
  }

  *out = stackData[--stackCount];
  return 1;
}

int stackPush(int value) {
  if (stackCount >= STACK_CAP) {
    Str errors[] = {STR("Stack overflow")};
    logError(errors, sizeof(errors) / sizeof(*errors));
    return 0;
  }

  stackData[stackCount++] = value;
  return 1;
}

// Scope
#define SCOPE_CAP 1024
typedef struct {
  int data[SCOPE_CAP];
  Str names[SCOPE_CAP];
  int count;
} Scope;

int scopePush(Scope *s, Str name) {
  if (s->count >= SCOPE_CAP) {
    Str errors[] = {STR("Scope overflow")};
    logError(errors, sizeof(errors) / sizeof(*errors));
    return 0;
  }

  s->names[s->count++] = name;
  return 1;
}

int scopeFind(Scope *s, Str name, int *out) {
  for (int i = 0; i < s->count; i++) {
    if (strEq(name, s->names[i])) {
      *out = i;
      return 1;
    }
  }
  return 0;
}

int functionsBody[SCOPE_CAP];
int functionsArity[SCOPE_CAP];
int insideFunction;
Scope functions;

int variablesMax;
int variablesBase;
Scope variables;

// Program
#define PROGRAM_CAP 1024
Op programOps[PROGRAM_CAP];
int programCount;

int programPush(OpType type, int data) {
  if (programCount >= PROGRAM_CAP) {
    Str errors[] = {STR("Program overflow")};
    logError(errors, sizeof(errors) / sizeof(*errors));
    return 0;
  }

  programOps[programCount++] = (Op){type, data};
  return 1;
}

int programEval(void) {
  int lhs = 0;
  int rhs = 0;
  int rbp = 0;

  float a = 0;
  float x = 0;
  float y = 0;

  canvasCount = 0;
  canvasPush(x, y);

  for (int i = 0; i < programCount; i++) {
    Op op = programOps[i];
    switch (op.type) {
    case OP_NUM:
      if (!stackPush(op.data)) {
        return 0;
      }
      break;

    case OP_GT:
      if (!stackPop(&rhs)) {
        return 0;
      }

      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!stackPush(lhs > rhs)) {
        return 0;
      }
      break;

    case OP_GE:
      if (!stackPop(&rhs)) {
        return 0;
      }

      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!stackPush(lhs >= rhs)) {
        return 0;
      }
      break;

    case OP_LT:
      if (!stackPop(&rhs)) {
        return 0;
      }

      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!stackPush(lhs < rhs)) {
        return 0;
      }
      break;

    case OP_LE:
      if (!stackPop(&rhs)) {
        return 0;
      }

      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!stackPush(lhs <= rhs)) {
        return 0;
      }
      break;

    case OP_EQ:
      if (!stackPop(&rhs)) {
        return 0;
      }

      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!stackPush(lhs == rhs)) {
        return 0;
      }
      break;

    case OP_NE:
      if (!stackPop(&rhs)) {
        return 0;
      }

      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!stackPush(lhs != rhs)) {
        return 0;
      }
      break;

    case OP_ADD:
      if (!stackPop(&rhs)) {
        return 0;
      }

      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!stackPush(lhs + rhs)) {
        return 0;
      }
      break;

    case OP_SUB:
      if (!stackPop(&rhs)) {
        return 0;
      }

      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!stackPush(lhs - rhs)) {
        return 0;
      }
      break;

    case OP_MUL:
      if (!stackPop(&rhs)) {
        return 0;
      }

      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!stackPush(lhs * rhs)) {
        return 0;
      }
      break;

    case OP_DIV:
      if (!stackPop(&rhs)) {
        return 0;
      }

      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!stackPush(lhs / rhs)) {
        return 0;
      }
      break;

    case OP_NOT:
      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!stackPush(!lhs)) {
        return 0;
      }
      break;

    case OP_NEG:
      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!stackPush(-lhs)) {
        return 0;
      }
      break;

    case OP_ELSE:
      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!lhs) {
        i = op.data - 1;
      }
      break;

    case OP_GOTO:
      i = op.data - 1;
      break;

    case OP_CALL:
      stackCount += functionsBody[op.data] - functionsArity[functions.count - 1];
      if (stackCount > STACK_CAP) {
        return 0;
      }

      if (!stackPush(i)) {
        return 0;
      }

      if (!stackPush(rbp)) {
        return 0;
      }

      rbp = stackCount - functionsBody[op.data] - 2;
      i = functions.data[op.data] - 1;
      break;

    case OP_RETURN:
      if (!stackPop(&lhs)) {
        return 0;
      }

      if (!stackPop(&rbp)) {
        return 0;
      }

      if (!stackPop(&i)) {
        return 0;
      }

      stackCount -= functionsBody[op.data];
      if (stackCount < 0) {
        return 0;
      }

      if (!stackPush(lhs)) {
        return 0;
      }
      break;

    case OP_DROP:
      if (!stackPop(&lhs)) {
        return 0;
      }
      break;

    case OP_GETG:
      if (!stackPush(variables.data[op.data])) {
        return 0;
      }
      break;

    case OP_SETG:
      if (!stackPop(&lhs)) {
        return 0;
      }

      variables.data[op.data] = lhs;
      break;

    case OP_GETL:
      if (!stackPush(stackData[rbp + op.data])) {
        return 0;
      }
      break;

    case OP_SETL:
      if (!stackPop(&lhs)) {
        return 0;
      }

      stackData[rbp + op.data] = lhs;
      break;

    case OP_MOVE:
      if (!stackPop(&lhs)) {
        return 0;
      }

      x += lhs * cosf(a);
      y += lhs * sinf(a);
      if (!canvasPush(x, y)) {
        return 0;
      }
      break;

    case OP_ROTATE:
      if (!stackPop(&lhs)) {
        return 0;
      }

      a = remf(a - lhs * PI / 180, PI * 2);
      break;
    }
  }

  return 1;
}

// Compiler
typedef enum {
  POWER_NIL,
  POWER_SET,
  POWER_CMP,
  POWER_ADD,
  POWER_MUL,
  POWER_PRE
} Power;

Power powerFromTokenType(TokenType type) {
  switch (type) {
  case TOKEN_GT:
  case TOKEN_GE:
  case TOKEN_LT:
  case TOKEN_LE:
  case TOKEN_EQ:
  case TOKEN_NE:
    return POWER_CMP;

  case TOKEN_ADD:
  case TOKEN_SUB:
    return POWER_ADD;

  case TOKEN_MUL:
  case TOKEN_DIV:
    return POWER_MUL;

  case TOKEN_SET:
    return POWER_SET;

  default:
    return POWER_NIL;
  }
}

void errorUnexpected(Token token) {
  char buffer[20];
  Str errors[] = {
    STR("Unexpected "),
    strFromTokenType(token.type),
    STR(" in line "),
    strFromInt(token.row, buffer),
  };
  logError(errors, sizeof(errors) / sizeof(*errors));
}

void errorUndefined(Token token, char *label) {
  char buffer[20];
  Str errors[] = {
    STR("Undefined "), strFromTokenType(token.type),  STR(" '"), token.str,
    STR("' in line "), strFromInt(token.row, buffer),
  };
  logError(errors, sizeof(errors) / sizeof(*errors));
}

int compileExpr(Power base) {
  Token token;
  if (!lexerNext(&token)) {
    return 0;
  }

  switch (token.type) {
  case TOKEN_NUM: {
    int data;
    if (!strParseInt(token.str, &data)) {
      return 0;
    }

    if (!programPush(OP_NUM, data)) {
      return 0;
    }
  } break;

  case TOKEN_IDENT: {
    Token new;
    if (!lexerPeek(&new)) {
      return 0;
    }

    if (new.type == TOKEN_LPAREN) {
      lexer.buffer = 0;

      int index;
      if (!scopeFind(&functions, token.str, &index)) {
        errorUndefined(token, "function");
        return 0;
      }

      for (int i = 0; i < functionsArity[index]; i++) {
        if (i && !lexerExpect(&token, TOKEN_COMMA)) {
          return 0;
        }

        if (!compileExpr(POWER_SET)) {
          return 0;
        }
      }

      if (!lexerExpect(&new, TOKEN_RPAREN)) {
        return 0;
      }

      if (!programPush(OP_CALL, index)) {
        return 0;
      }
    } else if (new.type == TOKEN_SET) {
      if (base != POWER_NIL) {
        errorUnexpected(new);
        return 0;
      }
      lexer.buffer = 0;

      if (!compileExpr(POWER_SET)) {
        return 0;
      }

      int index = variables.count;
      if (!scopeFind(&variables, token.str, &index)) {
        if (!scopePush(&variables, token.str)) {
          return 0;
        }
        variables.data[index] = insideFunction;
      }

      if (variables.data[index]) {
        return programPush(OP_SETL, index - variablesBase);
      } else {
        return programPush(OP_SETG, index);
      }
    } else {
      int index;
      if (!scopeFind(&variables, token.str, &index)) {
        errorUndefined(token, "variable");
        return 0;
      }

      if (variables.data[index]) {
        if (!programPush(OP_GETL, index - variablesBase)) {
          return 0;
        }
      } else {
        if (!programPush(OP_GETG, index)) {
          return 0;
        }
      }
    }
  } break;

  case TOKEN_NOT:
    if (!compileExpr(POWER_PRE)) {
      return 0;
    }

    if (!programPush(OP_NOT, 0)) {
      return 0;
    }
    break;

  case TOKEN_SUB:
    if (!compileExpr(POWER_PRE)) {
      return 0;
    }

    if (!programPush(OP_NEG, 0)) {
      return 0;
    }
    break;

  case TOKEN_LPAREN:
    if (!compileExpr(POWER_SET)) {
      return 0;
    }

    if (!lexerExpect(&token, TOKEN_RPAREN)) {
      return 0;
    }
    break;

  default:
    errorUnexpected(token);
    return 0;
  }

  while (1) {
    if (!lexerPeek(&token)) {
      return 0;
    }

    Power left = powerFromTokenType(token.type);
    if (left <= base) {
      break;
    }
    lexer.buffer = 0;

    if (!compileExpr(left)) {
      return 0;
    }

    switch (token.type) {
    case TOKEN_GT:
      if (!programPush(OP_GT, 0)) {
        return 0;
      }
      break;

    case TOKEN_GE:
      if (!programPush(OP_GE, 0)) {
        return 0;
      }
      break;

    case TOKEN_LT:
      if (!programPush(OP_LT, 0)) {
        return 0;
      }
      break;

    case TOKEN_LE:
      if (!programPush(OP_LE, 0)) {
        return 0;
      }
      break;

    case TOKEN_EQ:
      if (!programPush(OP_EQ, 0)) {
        return 0;
      }
      break;

    case TOKEN_NE:
      if (!programPush(OP_NE, 0)) {
        return 0;
      }
      break;

    case TOKEN_ADD:
      if (!programPush(OP_ADD, 0)) {
        return 0;
      }
      break;

    case TOKEN_SUB:
      if (!programPush(OP_SUB, 0)) {
        return 0;
      }
      break;

    case TOKEN_MUL:
      if (!programPush(OP_MUL, 0)) {
        return 0;
      }
      break;

    case TOKEN_DIV:
      if (!programPush(OP_DIV, 0)) {
        return 0;
      }
      break;

    default:
      break;
    }
  }

  return 1;
}

int compileStmt(void) {
  Token token;
  if (!lexerPeek(&token)) {
    return 0;
  }

  switch (token.type) {
  case TOKEN_LBRACE: {
    int scopeStart = variables.count;

    lexer.buffer = 0;
    while (1) {
      if (!lexerPeek(&token)) {
        return 0;
      }

      if (token.type == TOKEN_RBRACE) {
        break;
      }

      if (!compileStmt()) {
        return 0;
      }
    }
    lexer.buffer = 0;

    if (variablesMax < variables.count) {
      variablesMax = variables.count;
    }

    variables.count = scopeStart;
  } break;

  case TOKEN_IF: {
    lexer.buffer = 0;
    if (!compileExpr(POWER_SET)) {
      return 0;
    }

    if (!lexerExpectPeek(TOKEN_LBRACE)) {
      return 0;
    }

    int thenAddr = programCount;
    if (!programPush(OP_ELSE, 0)) {
      return 0;
    }

    if (!compileStmt()) {
      return 0;
    }

    if (!lexerPeek(&token)) {
      return 0;
    }

    if (token.type == TOKEN_ELSE) {
      lexer.buffer = 0;

      if (!lexerExpectPeek(TOKEN_LBRACE)) {
        return 0;
      }

      int elseAddr = programCount;
      if (!programPush(OP_GOTO, 0)) {
        return 0;
      }

      programOps[thenAddr].data = programCount;

      if (!compileStmt()) {
        return 0;
      }

      programOps[elseAddr].data = programCount;
    } else {
      programOps[thenAddr].data = programCount;
    }
  } break;

  case TOKEN_WHILE: {
    lexer.buffer = 0;

    int condAddr = programCount;
    if (!compileExpr(POWER_SET)) {
      return 0;
    }

    if (!lexerExpectPeek(TOKEN_LBRACE)) {
      return 0;
    }

    int bodyAddr = programCount;
    if (!programPush(OP_ELSE, 0)) {
      return 0;
    }

    if (!compileStmt()) {
      return 0;
    }

    if (!programPush(OP_GOTO, condAddr)) {
      return 0;
    }

    programOps[bodyAddr].data = programCount;
  } break;

  case TOKEN_FN: {
    lexer.buffer = 0;

    if (insideFunction) {
      errorUnexpected(token);
      return 0;
    }
    insideFunction = 1;

    variablesMax = variables.count;
    variablesBase = variables.count;

    if (!lexerExpect(&token, TOKEN_IDENT)) {
      return 0;
    }

    int index;
    if (scopeFind(&functions, token.str, &index)) {
      char buffer[20];
      Str errors[] = {
        STR("Redefinition of function '"),
        token.str,
        STR("' in line "),
        strFromInt(token.row, buffer),
      };
      logError(errors, sizeof(errors) / sizeof(*errors));
      return 0;
    }

    if (!scopePush(&functions, token.str)) {
      return 0;
    }

    if (!lexerExpect(&token, TOKEN_LPAREN)) {
      return 0;
    }

    functionsArity[functions.count - 1] = 0;
    while (1) {
      if (!lexerPeek(&token)) {
        return 0;
      }

      if (token.type == TOKEN_RPAREN) {
        lexer.buffer = 0;
        break;
      }

      if (functionsArity[functions.count - 1] && !lexerExpect(&token, TOKEN_COMMA)) {
        return 0;
      }

      if (!lexerExpect(&token, TOKEN_IDENT)) {
        return 0;
      }

      if (!scopePush(&variables, token.str)) {
        return 0;
      }
      variables.data[variables.count - 1] = insideFunction;

      functionsArity[functions.count - 1]++;
    }

    if (!lexerExpectPeek(TOKEN_LBRACE)) {
      return 0;
    }

    int bodyAddr = programCount;
    if (!programPush(OP_GOTO, 0)) {
      return 0;
    }
    functions.data[functions.count - 1] = programCount;

    if (!compileStmt()) {
      return 0;
    }
    functionsBody[functions.count - 1] = variablesMax - variablesBase;

    if (!programPush(OP_NUM, 0)) {
      return 0;
    }

    if (!programPush(OP_RETURN, functions.count - 1)) {
      return 0;
    }

    insideFunction = 0;
    programOps[bodyAddr].data = programCount;
    variables.count = variablesBase;
  } break;

  case TOKEN_RETURN:
    lexer.buffer = 0;

    if (!insideFunction) {
      errorUnexpected(token);
      return 0;
    }

    if (!compileExpr(POWER_SET)) {
      return 0;
    }

    return programPush(OP_RETURN, functions.count - 1);

  case TOKEN_MOVE:
    lexer.buffer = 0;
    if (!lexerExpectPeek(TOKEN_LPAREN)) {
      return 0;
    }

    if (!compileExpr(POWER_SET)) {
      return 0;
    }

    return programPush(OP_MOVE, 0);

  case TOKEN_ROTATE:
    lexer.buffer = 0;
    if (!lexerExpectPeek(TOKEN_LPAREN)) {
      return 0;
    }

    if (!compileExpr(POWER_SET)) {
      return 0;
    }

    return programPush(OP_ROTATE, 0);

  default: {
    if (!compileExpr(POWER_NIL)) {
      return 0;
    }

    OpType last = programOps[programCount - 1].type;
    if (last != OP_SETG && last != OP_SETL) {
      return programPush(OP_DROP, 0);
    }
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
  canvasCount = 0;
  programCount = 0;

  functions.count = 0;
  variables.count = 0;

  insideFunction = 0;
  variablesMax = 0;
  variablesBase = 0;

  lexerInit((Str){data, size});

  Token token;
  while (1) {
    if (!lexerPeek(&token)) {
      return;
    }

    if (token.type == TOKEN_EOF) {
      break;
    }

    if (!compileStmt()) {
      return;
    }
  }

  programEval();
}

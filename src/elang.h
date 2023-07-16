// Copyright 2023 Shoumodip Kar <shoumodipkar@gmail.com>

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ELANG_H
#define ELANG_H

void platformErrorStart(void);
void platformErrorPush(char *data, int count);
void platformErrorEnd(void);

typedef float (*Native)(float *);

int elangRun(void);
int elangCompile(char *data, int size);
int elangRegisterNative(char *name, int arity, Native native);

#endif

#ifdef ELANG_IMPLEMENTATION

// Str
typedef struct {
  char *data;
  int count;
} Str;

#define STR(s) ((Str){.data = s, .count = sizeof(s) - 1})

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

  return (Str){.data = buffer, .count = size};
}

int strParseFloat(Str s, float *out) {
  if (!s.count) {
    return 0;
  }

  int a = 0;
  int b = 0;
  int c = 1;
  int *d = &a;

  for (int i = 0; i < s.count; i++) {
    if (s.data[i] == '.') {
      c = 1;
      d = &b;
    } else {
      c *= 10;
      *d = *d * 10 + s.data[i] - '0';
    }
  }

  *out = a + (float)b / c;
  return 1;
}

// Error
void logErrorImpl(Str *data, int count) {
  platformErrorStart();
  for (int i = 0; i < count; i++) {
    platformErrorPush(data[i].data, data[i].count);
  }
  platformErrorEnd();
}

#define LOG_ERROR(...)                                                                             \
  do {                                                                                             \
    Str list[] = {__VA_ARGS__};                                                                    \
    logErrorImpl(list, sizeof(list) / sizeof(*list));                                              \
  } while (0)

#define LOG_ERROR_LINE(line, ...)                                                                  \
  do {                                                                                             \
    char buffer[20];                                                                               \
    Str list[] = {__VA_ARGS__, STR(" in line "), strFromInt(line, buffer)};                        \
    logErrorImpl(list, sizeof(list) / sizeof(*list));                                              \
  } while (0)

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
  TOKEN_RETURN
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

int lexerRow;
Str lexerStr;
int lexerBuffer;
Token lexerToken;

void lexerInit(Str str) {
  lexerRow = 1;
  lexerStr = str;
  lexerBuffer = 0;
}

void lexerRead() {
  if (*lexerStr.data == '\n') {
    lexerRow++;
  }
  lexerStr.data++;
  lexerStr.count--;
}

void lexerTrim() {
  while (lexerStr.count) {
    switch (*lexerStr.data) {
    case ' ':
    case '\n':
      lexerRead();
      break;

    case '#':
      while (lexerStr.count && *lexerStr.data != '\n') {
        lexerRead();
      }
      break;

    default:
      return;
    }
  }
}

int lexerMatch(char ch) {
  if (lexerStr.count && *lexerStr.data == ch) {
    lexerRead();
    return 1;
  }

  return 0;
}

int lexerNext(Token *token) {
  if (lexerBuffer) {
    lexerBuffer = 0;
    *token = lexerToken;
    return 1;
  }

  lexerTrim();

  token->row = lexerRow;
  token->str = lexerStr;

  if (!lexerStr.count) {
    token->type = TOKEN_EOF;
    return 1;
  }

  switch (*lexerStr.data) {
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
    if (isdigit(*lexerStr.data)) {
      while (lexerStr.count && isdigit(*lexerStr.data)) {
        lexerRead();
      }

      if (lexerStr.count && *lexerStr.data == '.') {
        lexerRead();
        while (lexerStr.count && isdigit(*lexerStr.data)) {
          lexerRead();
        }
      }

      token->type = TOKEN_NUM;
    } else if (isident(*lexerStr.data)) {
      while (lexerStr.count && isident(*lexerStr.data)) {
        lexerRead();
      }

      token->type = TOKEN_IDENT;
    } else {
      LOG_ERROR_LINE(token->row, STR("Invalid character '"),
                     (Str){.data = token->str.data, .count = 1}, STR("'"));
      return 0;
    }
  }

  token->str.count -= lexerStr.count;

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
    }
  }

  return 1;
}

int lexerPeek(Token *token) {
  if (!lexerBuffer) {
    if (!lexerNext(&lexerToken)) {
      return 0;
    }
    lexerBuffer = 1;
  }

  *token = lexerToken;
  return 1;
}

int lexerNextExpect(Token *token, TokenType type) {
  if (!lexerNext(token)) {
    return 0;
  }

  if (token->type != type) {
    LOG_ERROR_LINE(token->row, STR("Expected "), strFromTokenType(type), STR(", found "),
                   strFromTokenType(token->type));
    return 0;
  }

  return 1;
}

int lexerPeekExpect(TokenType type) {
  if (!lexerNextExpect(&lexerToken, type)) {
    return 0;
  }

  lexerBuffer = 1;
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
  OP_NATIVE,
  OP_RETURN,

  OP_DROP,
  OP_GETG,
  OP_SETG,
  OP_GETL,
  OP_SETL
} OpType;

typedef struct {
  OpType type;
  float data;
} Op;

// Program
typedef float (*Native)(float *);

typedef struct {
  Str name;
  int body;
  int arity;
  int start;
} Function;

typedef struct {
  Str name;
  float data;
} Variable;

#define PROGRAM_CAP 1024

Op ops[PROGRAM_CAP];
int opsCount;

int opsPush(OpType type, float data) {
  if (opsCount >= PROGRAM_CAP) {
    LOG_ERROR(STR("Program overflow"));
    return 0;
  }

  ops[opsCount++] = (Op){.type = type, .data = data};
  return 1;
}

Function functions[PROGRAM_CAP];
int functionsCount;
int functionsLocal;

int functionsPush(Str name, int arity, int start) {
  if (functionsCount >= PROGRAM_CAP) {
    LOG_ERROR(STR("Functions overflow"));
    return 0;
  }

  functions[functionsCount++] = (Function){
    .name = name,
    .arity = arity,
    .start = start,
  };
  return 1;
}

int functionsFind(Str name, int *out) {
  for (int i = 0; i < functionsCount; i++) {
    if (strEq(name, functions[i].name)) {
      *out = i;
      return 1;
    }
  }
  return 0;
}

Variable variables[PROGRAM_CAP];
int variablesMax;
int variablesBase;
int variablesCount;

int variablesPush(Str name, float data) {
  if (variablesCount >= PROGRAM_CAP) {
    LOG_ERROR(STR("Variables overflow"));
    return 0;
  }

  variables[variablesCount++] = (Variable){.name = name, .data = data};
  return 1;
}

int variablesFind(Str name, int *out) {
  for (int i = 0; i < variablesCount; i++) {
    if (strEq(name, variables[i].name)) {
      *out = i;
      return 1;
    }
  }
  return 0;
}

Native natives[PROGRAM_CAP];
int nativesCount;

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
  LOG_ERROR_LINE(token.row, STR("Unexpected "), strFromTokenType(token.type));
}

void errorUndefined(Token token, Str label) {
  LOG_ERROR_LINE(token.row, STR("Undefined "), label, STR(" '"), token.str, STR("'"));
}

int compileExpr(Power base) {
  Token token;
  if (!lexerNext(&token)) {
    return 0;
  }

  switch (token.type) {
  case TOKEN_NUM: {
    float data;
    if (!strParseFloat(token.str, &data)) {
      return 0;
    }

    if (!opsPush(OP_NUM, data)) {
      return 0;
    }
  } break;

  case TOKEN_IDENT: {
    Token new;
    if (!lexerPeek(&new)) {
      return 0;
    }

    if (new.type == TOKEN_LPAREN) {
      lexerBuffer = 0;

      int index;
      if (!functionsFind(token.str, &index)) {
        errorUndefined(token, STR("function"));
        return 0;
      }

      for (int i = 0; i < functions[index].arity; i++) {
        if (i && !lexerNextExpect(&token, TOKEN_COMMA)) {
          return 0;
        }

        if (!compileExpr(POWER_SET)) {
          return 0;
        }
      }

      if (!lexerNextExpect(&new, TOKEN_RPAREN)) {
        return 0;
      }

      if (functions[index].start < 0) {
        if (!opsPush(OP_NATIVE, index)) {
          return 0;
        }
      } else {
        if (!opsPush(OP_CALL, index)) {
          return 0;
        }
      }
    } else if (new.type == TOKEN_SET) {
      if (base != POWER_NIL) {
        errorUnexpected(new);
        return 0;
      }
      lexerBuffer = 0;

      if (!compileExpr(POWER_SET)) {
        return 0;
      }

      int index = variablesCount;
      if (!variablesFind(token.str, &index)) {
        if (!variablesPush(token.str, functionsLocal)) {
          return 0;
        }
      }

      if (variables[index].data) {
        return opsPush(OP_SETL, index - variablesBase);
      } else {
        return opsPush(OP_SETG, index);
      }
    } else {
      int index;
      if (!variablesFind(token.str, &index)) {
        errorUndefined(token, STR("variable"));
        return 0;
      }

      if (variables[index].data) {
        if (!opsPush(OP_GETL, index - variablesBase)) {
          return 0;
        }
      } else {
        if (!opsPush(OP_GETG, index)) {
          return 0;
        }
      }
    }
  } break;

  case TOKEN_NOT:
    if (!compileExpr(POWER_PRE)) {
      return 0;
    }

    if (!opsPush(OP_NOT, 0)) {
      return 0;
    }
    break;

  case TOKEN_SUB:
    if (!compileExpr(POWER_PRE)) {
      return 0;
    }

    if (!opsPush(OP_NEG, 0)) {
      return 0;
    }
    break;

  case TOKEN_LPAREN:
    if (!compileExpr(POWER_SET)) {
      return 0;
    }

    if (!lexerNextExpect(&token, TOKEN_RPAREN)) {
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
    lexerBuffer = 0;

    if (!compileExpr(left)) {
      return 0;
    }

    switch (token.type) {
    case TOKEN_GT:
      if (!opsPush(OP_GT, 0)) {
        return 0;
      }
      break;

    case TOKEN_GE:
      if (!opsPush(OP_GE, 0)) {
        return 0;
      }
      break;

    case TOKEN_LT:
      if (!opsPush(OP_LT, 0)) {
        return 0;
      }
      break;

    case TOKEN_LE:
      if (!opsPush(OP_LE, 0)) {
        return 0;
      }
      break;

    case TOKEN_EQ:
      if (!opsPush(OP_EQ, 0)) {
        return 0;
      }
      break;

    case TOKEN_NE:
      if (!opsPush(OP_NE, 0)) {
        return 0;
      }
      break;

    case TOKEN_ADD:
      if (!opsPush(OP_ADD, 0)) {
        return 0;
      }
      break;

    case TOKEN_SUB:
      if (!opsPush(OP_SUB, 0)) {
        return 0;
      }
      break;

    case TOKEN_MUL:
      if (!opsPush(OP_MUL, 0)) {
        return 0;
      }
      break;

    case TOKEN_DIV:
      if (!opsPush(OP_DIV, 0)) {
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
    int scopeStart = variablesCount;

    lexerBuffer = 0;
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
    lexerBuffer = 0;

    if (variablesMax < variablesCount) {
      variablesMax = variablesCount;
    }

    variablesCount = scopeStart;
  } break;

  case TOKEN_IF: {
    lexerBuffer = 0;
    if (!compileExpr(POWER_SET)) {
      return 0;
    }

    if (!lexerPeekExpect(TOKEN_LBRACE)) {
      return 0;
    }

    int thenAddr = opsCount;
    if (!opsPush(OP_ELSE, 0)) {
      return 0;
    }

    if (!compileStmt()) {
      return 0;
    }

    if (!lexerPeek(&token)) {
      return 0;
    }

    if (token.type == TOKEN_ELSE) {
      lexerBuffer = 0;

      if (!lexerPeekExpect(TOKEN_LBRACE)) {
        return 0;
      }

      int elseAddr = opsCount;
      if (!opsPush(OP_GOTO, 0)) {
        return 0;
      }

      ops[thenAddr].data = opsCount;

      if (!compileStmt()) {
        return 0;
      }

      ops[elseAddr].data = opsCount;
    } else {
      ops[thenAddr].data = opsCount;
    }
  } break;

  case TOKEN_WHILE: {
    lexerBuffer = 0;

    int condAddr = opsCount;
    if (!compileExpr(POWER_SET)) {
      return 0;
    }

    if (!lexerPeekExpect(TOKEN_LBRACE)) {
      return 0;
    }

    int bodyAddr = opsCount;
    if (!opsPush(OP_ELSE, 0)) {
      return 0;
    }

    if (!compileStmt()) {
      return 0;
    }

    if (!opsPush(OP_GOTO, condAddr)) {
      return 0;
    }

    ops[bodyAddr].data = opsCount;
  } break;

  case TOKEN_FN: {
    lexerBuffer = 0;

    if (functionsLocal) {
      errorUnexpected(token);
      return 0;
    }
    functionsLocal = 1;

    variablesMax = variablesCount;
    variablesBase = variablesCount;

    if (!lexerNextExpect(&token, TOKEN_IDENT)) {
      return 0;
    }
    Str name = token.str;

    int index;
    if (functionsFind(token.str, &index)) {
      LOG_ERROR_LINE(token.row, STR("Redefinition of function '"), token.str, STR("'"));
      return 0;
    }

    if (!lexerNextExpect(&token, TOKEN_LPAREN)) {
      return 0;
    }

    int arity = 0;
    while (1) {
      if (!lexerPeek(&token)) {
        return 0;
      }

      if (token.type == TOKEN_RPAREN) {
        lexerBuffer = 0;
        break;
      }

      if (arity && !lexerNextExpect(&token, TOKEN_COMMA)) {
        return 0;
      }

      if (!lexerNextExpect(&token, TOKEN_IDENT)) {
        return 0;
      }

      if (!variablesPush(token.str, functionsLocal)) {
        return 0;
      }

      arity++;
    }

    if (!lexerPeekExpect(TOKEN_LBRACE)) {
      return 0;
    }

    int bodyAddr = opsCount;
    if (!opsPush(OP_GOTO, 0)) {
      return 0;
    }

    if (!functionsPush(name, arity, opsCount)) {
      return 0;
    }

    if (!compileStmt()) {
      return 0;
    }

    functions[functionsCount - 1].body = variablesMax - variablesBase;

    if (!opsPush(OP_NUM, 0)) {
      return 0;
    }

    if (!opsPush(OP_RETURN, functionsCount - 1)) {
      return 0;
    }
    ops[bodyAddr].data = opsCount;

    functionsLocal = 0;
    variablesCount = variablesBase;
  } break;

  case TOKEN_RETURN:
    lexerBuffer = 0;

    if (!functionsLocal) {
      errorUnexpected(token);
      return 0;
    }

    if (!compileExpr(POWER_SET)) {
      return 0;
    }

    return opsPush(OP_RETURN, functionsCount - 1);

  default: {
    if (!compileExpr(POWER_NIL)) {
      return 0;
    }

    OpType last = ops[opsCount - 1].type;
    if (last != OP_SETG && last != OP_SETL) {
      return opsPush(OP_DROP, 0);
    }
  }
  }

  return 1;
}

// Stack
#define STACK_CAP 1024

float stack[STACK_CAP];
int stackCount;

int stackPop(float *out) {
  if (!stackCount) {
    return 0;
  }

  *out = stack[--stackCount];
  return 1;
}

int stackPush(float value) {
  if (stackCount >= STACK_CAP) {
    LOG_ERROR(STR("Stack overflow"));
    return 0;
  }

  stack[stackCount++] = value;
  return 1;
}

// Elang
#define UNARY_OP(op)                                                                               \
  do {                                                                                             \
    if (!stackPop(&a)) {                                                                           \
      return 0;                                                                                    \
    }                                                                                              \
                                                                                                   \
    if (!stackPush(op(a))) {                                                                       \
      return 0;                                                                                    \
    }                                                                                              \
  } while (0)

#define BINARY_OP(op)                                                                              \
  do {                                                                                             \
    if (!stackPop(&b)) {                                                                           \
      return 0;                                                                                    \
    }                                                                                              \
                                                                                                   \
    if (!stackPop(&a)) {                                                                           \
      return 0;                                                                                    \
    }                                                                                              \
                                                                                                   \
    if (!stackPush(a op b)) {                                                                      \
      return 0;                                                                                    \
    }                                                                                              \
  } while (0)

int elangRun(void) {
  stackCount = 0;

  int frame = 0;
  float a, b;
  for (int i = 0; i < opsCount; i++) {
    Op op = ops[i];
    switch (op.type) {
    case OP_NUM:
      if (!stackPush(op.data)) {
        return 0;
      }
      break;

    case OP_GT:
      BINARY_OP(>);
      break;

    case OP_GE:
      BINARY_OP(>=);
      break;

    case OP_LT:
      BINARY_OP(<);
      break;

    case OP_LE:
      BINARY_OP(<=);
      break;

    case OP_EQ:
      BINARY_OP(==);
      break;

    case OP_NE:
      BINARY_OP(!=);
      break;

    case OP_ADD:
      BINARY_OP(+);
      break;

    case OP_SUB:
      BINARY_OP(-);
      break;

    case OP_MUL:
      BINARY_OP(*);
      break;

    case OP_DIV:
      BINARY_OP(/);
      break;

    case OP_NOT:
      UNARY_OP(!);
      break;

    case OP_NEG:
      UNARY_OP(-);
      break;

    case OP_ELSE:
      if (!stackPop(&a)) {
        return 0;
      }

      if (!a) {
        i = op.data - 1;
      }
      break;

    case OP_GOTO:
      i = op.data - 1;
      break;

    case OP_CALL: {
      Function *f = &functions[(int)op.data];

      stackCount += f->body - f->arity;
      if (stackCount > STACK_CAP) {
        return 0;
      }

      if (!stackPush(i)) {
        return 0;
      }

      if (!stackPush(frame)) {
        return 0;
      }

      frame = stackCount - f->body - 2;
      i = f->start - 1;
    } break;

    case OP_NATIVE: {
      Function *f = &functions[(int)op.data];

      stackCount -= f->arity;
      if (stackCount < 0) {
        return 0;
      }

      float result = natives[-f->start - 1](stack + stackCount);
      if (!stackPush(result)) {
        return 0;
      }
    } break;

    case OP_RETURN:
      if (!stackPop(&a)) {
        return 0;
      }

      if (!stackPop(&b)) {
        return 0;
      }
      frame = b;

      if (!stackPop(&b)) {
        return 0;
      }
      i = b;

      stackCount -= functions[(int)op.data].body;
      if (stackCount < 0) {
        return 0;
      }

      if (!stackPush(a)) {
        return 0;
      }
      break;

    case OP_DROP:
      if (!stackPop(&a)) {
        return 0;
      }
      break;

    case OP_GETG:
      if (!stackPush(variables[(int)op.data].data)) {
        return 0;
      }
      break;

    case OP_SETG:
      if (!stackPop(&a)) {
        return 0;
      }

      variables[(int)op.data].data = a;
      break;

    case OP_GETL:
      if (!stackPush(stack[frame + (int)op.data])) {
        return 0;
      }
      break;

    case OP_SETL:
      if (!stackPop(&a)) {
        return 0;
      }

      stack[frame + (int)op.data] = a;
      break;
    }
  }

  return 1;
}

int elangCompile(char *data, int size) {
  opsCount = 0;

  functionsCount = nativesCount;
  functionsLocal = 0;

  variablesMax = 0;
  variablesBase = 0;
  variablesCount = 0;

  lexerInit((Str){.data = data, .count = size});

  Token token;
  while (1) {
    if (!lexerPeek(&token)) {
      return 0;
    }

    if (token.type == TOKEN_EOF) {
      break;
    }

    if (!compileStmt()) {
      return 0;
    }
  }

  return 1;
}

int elangRegisterNative(char *name, int arity, Native native) {
  if (nativesCount >= PROGRAM_CAP) {
    LOG_ERROR(STR("Natives overflow"));
    return 0;
  }

  Str str = {.data = name, .count = 0};
  while (str.data[str.count] != '\0') {
    str.count++;
  }

  natives[nativesCount++] = native;
  return functionsPush(str, arity, -nativesCount);
}

#endif

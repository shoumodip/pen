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

typedef struct {
  int row;
  Str str;

  int buffer;
  Token token;
} Lexer;

void lexerInit(Lexer *l, Str str) {
  l->row = 1;
  l->str = str;
  l->buffer = 0;
}

void lexerRead(Lexer *l) {
  if (*l->str.data == '\n') {
    l->row++;
  }
  l->str.data++;
  l->str.count--;
}

void lexerTrim(Lexer *l) {
  while (l->str.count) {
    switch (*l->str.data) {
    case ' ':
    case '\n':
      lexerRead(l);
      break;

    case '#':
      while (l->str.count && *l->str.data != '\n') {
        lexerRead(l);
      }
      break;

    default:
      return;
    }
  }
}

int lexerMatch(Lexer *l, char ch) {
  if (l->str.count && *l->str.data == ch) {
    lexerRead(l);
    return 1;
  }

  return 0;
}

int lexerNext(Lexer *l, Token *token) {
  if (l->buffer) {
    l->buffer = 0;
    *token = l->token;
    return 1;
  }

  lexerTrim(l);

  token->row = l->row;
  token->str = l->str;

  if (!l->str.count) {
    token->type = TOKEN_EOF;
    return 1;
  }

  switch (*l->str.data) {
  case '!':
    lexerRead(l);
    token->type = lexerMatch(l, '=') ? TOKEN_NE : TOKEN_NOT;
    break;

  case '>':
    lexerRead(l);
    token->type = lexerMatch(l, '=') ? TOKEN_GE : TOKEN_GT;
    break;

  case '<':
    lexerRead(l);
    token->type = lexerMatch(l, '=') ? TOKEN_LE : TOKEN_LT;
    break;

  case '+':
    lexerRead(l);
    token->type = TOKEN_ADD;
    break;

  case '-':
    lexerRead(l);
    token->type = TOKEN_SUB;
    break;

  case '*':
    lexerRead(l);
    token->type = TOKEN_MUL;
    break;

  case '/':
    lexerRead(l);
    token->type = TOKEN_DIV;
    break;

  case '=':
    lexerRead(l);
    token->type = lexerMatch(l, '=') ? TOKEN_EQ : TOKEN_SET;
    break;

  case ',':
    lexerRead(l);
    token->type = TOKEN_COMMA;
    break;

  case '(':
    lexerRead(l);
    token->type = TOKEN_LPAREN;
    break;

  case ')':
    lexerRead(l);
    token->type = TOKEN_RPAREN;
    break;

  case '{':
    lexerRead(l);
    token->type = TOKEN_LBRACE;
    break;

  case '}':
    lexerRead(l);
    token->type = TOKEN_RBRACE;
    break;

  default:
    if (isdigit(*l->str.data)) {
      while (l->str.count && isdigit(*l->str.data)) {
        lexerRead(l);
      }

      if (l->str.count && *l->str.data == '.') {
        lexerRead(l);
        while (l->str.count && isdigit(*l->str.data)) {
          lexerRead(l);
        }
      }

      token->type = TOKEN_NUM;
    } else if (isident(*l->str.data)) {
      while (l->str.count && isident(*l->str.data)) {
        lexerRead(l);
      }

      token->type = TOKEN_IDENT;
    } else {
      LOG_ERROR_LINE(token->row, STR("Invalid character '"),
                     (Str){.data = token->str.data, .count = 1}, STR("'"));
      return 0;
    }
  }

  token->str.count -= l->str.count;

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

int lexerPeek(Lexer *l, Token *token) {
  if (!l->buffer) {
    if (!lexerNext(l, &l->token)) {
      return 0;
    }
    l->buffer = 1;
  }

  *token = l->token;
  return 1;
}

int lexerNextExpect(Lexer *l, Token *token, TokenType type) {
  if (!lexerNext(l, token)) {
    return 0;
  }

  if (token->type != type) {
    LOG_ERROR_LINE(token->row, STR("Expected "), strFromTokenType(type), STR(", found "),
                   strFromTokenType(token->type));
    return 0;
  }

  return 1;
}

int lexerPeekExpect(Lexer *l, TokenType type) {
  if (!lexerNextExpect(l, &l->token, type)) {
    return 0;
  }

  l->buffer = 1;
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

// Stack
#define STACK_CAP 1024

typedef struct {
  float data[STACK_CAP];
  int count;
} Stack;

int stackPop(Stack *s, float *out) {
  if (!s->count) {
    return 0;
  }

  *out = s->data[--s->count];
  return 1;
}

int stackPush(Stack *s, float value) {
  if (s->count >= STACK_CAP) {
    LOG_ERROR(STR("Stack overflow"));
    return 0;
  }

  s->data[s->count++] = value;
  return 1;
}

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

typedef struct {
  Op ops[PROGRAM_CAP];
  int opsCount;

  Function functions[PROGRAM_CAP];
  int functionsCount;
  int functionsLocal;

  Variable variables[PROGRAM_CAP];
  int variablesMax;
  int variablesBase;
  int variablesCount;

  Native natives[PROGRAM_CAP];
  int nativesCount;
} Program;

int programPushOp(Program *p, OpType type, float data) {
  if (p->opsCount >= PROGRAM_CAP) {
    LOG_ERROR(STR("Program overflow"));
    return 0;
  }

  p->ops[p->opsCount++] = (Op){.type = type, .data = data};
  return 1;
}

int programPushFunction(Program *p, Str name, int arity, int start) {
  if (p->functionsCount >= PROGRAM_CAP) {
    LOG_ERROR(STR("Functions overflow"));
    return 0;
  }

  p->functions[p->functionsCount++] = (Function){
    .name = name,
    .arity = arity,
    .start = start,
  };
  return 1;
}

int programFindFunction(Program *p, Str name, int *out) {
  for (int i = 0; i < p->functionsCount; i++) {
    if (strEq(name, p->functions[i].name)) {
      *out = i;
      return 1;
    }
  }
  return 0;
}

int programPushVariable(Program *p, Str name, float data) {
  if (p->variablesCount >= PROGRAM_CAP) {
    LOG_ERROR(STR("Variables overflow"));
    return 0;
  }

  p->variables[p->variablesCount++] = (Variable){.name = name, .data = data};
  return 1;
}

int programFindVariable(Program *p, Str name, int *out) {
  for (int i = 0; i < p->variablesCount; i++) {
    if (strEq(name, p->variables[i].name)) {
      *out = i;
      return 1;
    }
  }
  return 0;
}

int programPushNative(Program *p, Str name, int arity, Native native) {
  if (p->nativesCount >= PROGRAM_CAP) {
    LOG_ERROR(STR("Natives overflow"));
    return 0;
  }

  p->natives[p->nativesCount++] = native;
  return programPushFunction(p, name, arity, -p->nativesCount);
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
  LOG_ERROR_LINE(token.row, STR("Unexpected "), strFromTokenType(token.type));
}

void errorUndefined(Token token, Str label) {
  LOG_ERROR_LINE(token.row, STR("Undefined "), label, STR(" '"), token.str, STR("'"));
}

int compileExpr(Lexer *l, Program *p, Power base) {
  Token token;
  if (!lexerNext(l, &token)) {
    return 0;
  }

  switch (token.type) {
  case TOKEN_NUM: {
    float data;
    if (!strParseFloat(token.str, &data)) {
      return 0;
    }

    if (!programPushOp(p, OP_NUM, data)) {
      return 0;
    }
  } break;

  case TOKEN_IDENT: {
    Token new;
    if (!lexerPeek(l, &new)) {
      return 0;
    }

    if (new.type == TOKEN_LPAREN) {
      l->buffer = 0;

      int index;
      if (!programFindFunction(p, token.str, &index)) {
        errorUndefined(token, STR("function"));
        return 0;
      }

      for (int i = 0; i < p->functions[index].arity; i++) {
        if (i && !lexerNextExpect(l, &token, TOKEN_COMMA)) {
          return 0;
        }

        if (!compileExpr(l, p, POWER_SET)) {
          return 0;
        }
      }

      if (!lexerNextExpect(l, &new, TOKEN_RPAREN)) {
        return 0;
      }

      if (p->functions[index].start < 0) {
        if (!programPushOp(p, OP_NATIVE, index)) {
          return 0;
        }
      } else {
        if (!programPushOp(p, OP_CALL, index)) {
          return 0;
        }
      }
    } else if (new.type == TOKEN_SET) {
      if (base != POWER_NIL) {
        errorUnexpected(new);
        return 0;
      }
      l->buffer = 0;

      if (!compileExpr(l, p, POWER_SET)) {
        return 0;
      }

      int index = p->variablesCount;
      if (!programFindVariable(p, token.str, &index)) {
        if (!programPushVariable(p, token.str, p->functionsLocal)) {
          return 0;
        }
      }

      if (p->variables[index].data) {
        return programPushOp(p, OP_SETL, index - p->variablesBase);
      } else {
        return programPushOp(p, OP_SETG, index);
      }
    } else {
      int index;
      if (!programFindVariable(p, token.str, &index)) {
        errorUndefined(token, STR("variable"));
        return 0;
      }

      if (p->variables[index].data) {
        if (!programPushOp(p, OP_GETL, index - p->variablesBase)) {
          return 0;
        }
      } else {
        if (!programPushOp(p, OP_GETG, index)) {
          return 0;
        }
      }
    }
  } break;

  case TOKEN_NOT:
    if (!compileExpr(l, p, POWER_PRE)) {
      return 0;
    }

    if (!programPushOp(p, OP_NOT, 0)) {
      return 0;
    }
    break;

  case TOKEN_SUB:
    if (!compileExpr(l, p, POWER_PRE)) {
      return 0;
    }

    if (!programPushOp(p, OP_NEG, 0)) {
      return 0;
    }
    break;

  case TOKEN_LPAREN:
    if (!compileExpr(l, p, POWER_SET)) {
      return 0;
    }

    if (!lexerNextExpect(l, &token, TOKEN_RPAREN)) {
      return 0;
    }
    break;

  default:
    errorUnexpected(token);
    return 0;
  }

  while (1) {
    if (!lexerPeek(l, &token)) {
      return 0;
    }

    Power left = powerFromTokenType(token.type);
    if (left <= base) {
      break;
    }
    l->buffer = 0;

    if (!compileExpr(l, p, left)) {
      return 0;
    }

    switch (token.type) {
    case TOKEN_GT:
      if (!programPushOp(p, OP_GT, 0)) {
        return 0;
      }
      break;

    case TOKEN_GE:
      if (!programPushOp(p, OP_GE, 0)) {
        return 0;
      }
      break;

    case TOKEN_LT:
      if (!programPushOp(p, OP_LT, 0)) {
        return 0;
      }
      break;

    case TOKEN_LE:
      if (!programPushOp(p, OP_LE, 0)) {
        return 0;
      }
      break;

    case TOKEN_EQ:
      if (!programPushOp(p, OP_EQ, 0)) {
        return 0;
      }
      break;

    case TOKEN_NE:
      if (!programPushOp(p, OP_NE, 0)) {
        return 0;
      }
      break;

    case TOKEN_ADD:
      if (!programPushOp(p, OP_ADD, 0)) {
        return 0;
      }
      break;

    case TOKEN_SUB:
      if (!programPushOp(p, OP_SUB, 0)) {
        return 0;
      }
      break;

    case TOKEN_MUL:
      if (!programPushOp(p, OP_MUL, 0)) {
        return 0;
      }
      break;

    case TOKEN_DIV:
      if (!programPushOp(p, OP_DIV, 0)) {
        return 0;
      }
      break;

    default:
      break;
    }
  }

  return 1;
}

int compileStmt(Lexer *l, Program *p) {
  Token token;
  if (!lexerPeek(l, &token)) {
    return 0;
  }

  switch (token.type) {
  case TOKEN_LBRACE: {
    int scopeStart = p->variablesCount;

    l->buffer = 0;
    while (1) {
      if (!lexerPeek(l, &token)) {
        return 0;
      }

      if (token.type == TOKEN_RBRACE) {
        break;
      }

      if (!compileStmt(l, p)) {
        return 0;
      }
    }
    l->buffer = 0;

    if (p->variablesMax < p->variablesCount) {
      p->variablesMax = p->variablesCount;
    }

    p->variablesCount = scopeStart;
  } break;

  case TOKEN_IF: {
    l->buffer = 0;
    if (!compileExpr(l, p, POWER_SET)) {
      return 0;
    }

    if (!lexerPeekExpect(l, TOKEN_LBRACE)) {
      return 0;
    }

    int thenAddr = p->opsCount;
    if (!programPushOp(p, OP_ELSE, 0)) {
      return 0;
    }

    if (!compileStmt(l, p)) {
      return 0;
    }

    if (!lexerPeek(l, &token)) {
      return 0;
    }

    if (token.type == TOKEN_ELSE) {
      l->buffer = 0;

      if (!lexerPeekExpect(l, TOKEN_LBRACE)) {
        return 0;
      }

      int elseAddr = p->opsCount;
      if (!programPushOp(p, OP_GOTO, 0)) {
        return 0;
      }

      p->ops[thenAddr].data = p->opsCount;

      if (!compileStmt(l, p)) {
        return 0;
      }

      p->ops[elseAddr].data = p->opsCount;
    } else {
      p->ops[thenAddr].data = p->opsCount;
    }
  } break;

  case TOKEN_WHILE: {
    l->buffer = 0;

    int condAddr = p->opsCount;
    if (!compileExpr(l, p, POWER_SET)) {
      return 0;
    }

    if (!lexerPeekExpect(l, TOKEN_LBRACE)) {
      return 0;
    }

    int bodyAddr = p->opsCount;
    if (!programPushOp(p, OP_ELSE, 0)) {
      return 0;
    }

    if (!compileStmt(l, p)) {
      return 0;
    }

    if (!programPushOp(p, OP_GOTO, condAddr)) {
      return 0;
    }

    p->ops[bodyAddr].data = p->opsCount;
  } break;

  case TOKEN_FN: {
    l->buffer = 0;

    if (p->functionsLocal) {
      errorUnexpected(token);
      return 0;
    }
    p->functionsLocal = 1;

    p->variablesMax = p->variablesCount;
    p->variablesBase = p->variablesCount;

    if (!lexerNextExpect(l, &token, TOKEN_IDENT)) {
      return 0;
    }
    Str name = token.str;

    int index;
    if (programFindFunction(p, token.str, &index)) {
      LOG_ERROR_LINE(token.row, STR("Redefinition of function '"), token.str, STR("'"));
      return 0;
    }

    if (!lexerNextExpect(l, &token, TOKEN_LPAREN)) {
      return 0;
    }

    int arity = 0;
    while (1) {
      if (!lexerPeek(l, &token)) {
        return 0;
      }

      if (token.type == TOKEN_RPAREN) {
        l->buffer = 0;
        break;
      }

      if (arity && !lexerNextExpect(l, &token, TOKEN_COMMA)) {
        return 0;
      }

      if (!lexerNextExpect(l, &token, TOKEN_IDENT)) {
        return 0;
      }

      if (!programPushVariable(p, token.str, p->functionsLocal)) {
        return 0;
      }

      arity++;
    }

    if (!lexerPeekExpect(l, TOKEN_LBRACE)) {
      return 0;
    }

    int bodyAddr = p->opsCount;
    if (!programPushOp(p, OP_GOTO, 0)) {
      return 0;
    }

    if (!programPushFunction(p, name, arity, p->opsCount)) {
      return 0;
    }

    if (!compileStmt(l, p)) {
      return 0;
    }

    p->functions[p->functionsCount - 1].body = p->variablesMax - p->variablesBase;

    if (!programPushOp(p, OP_NUM, 0)) {
      return 0;
    }

    if (!programPushOp(p, OP_RETURN, p->functionsCount - 1)) {
      return 0;
    }
    p->ops[bodyAddr].data = p->opsCount;

    p->functionsLocal = 0;
    p->variablesCount = p->variablesBase;
  } break;

  case TOKEN_RETURN:
    l->buffer = 0;

    if (!p->functionsLocal) {
      errorUnexpected(token);
      return 0;
    }

    if (!compileExpr(l, p, POWER_SET)) {
      return 0;
    }

    return programPushOp(p, OP_RETURN, p->functionsCount - 1);

  default: {
    if (!compileExpr(l, p, POWER_NIL)) {
      return 0;
    }

    OpType last = p->ops[p->opsCount - 1].type;
    if (last != OP_SETG && last != OP_SETL) {
      return programPushOp(p, OP_DROP, 0);
    }
  }
  }

  return 1;
}

// Evaluator
#define UNARY_OP(op)                                                                               \
  do {                                                                                             \
    if (!stackPop(s, &a)) {                                                                        \
      return 0;                                                                                    \
    }                                                                                              \
                                                                                                   \
    if (!stackPush(s, op(a))) {                                                                    \
      return 0;                                                                                    \
    }                                                                                              \
  } while (0)

#define BINARY_OP(op)                                                                              \
  do {                                                                                             \
    if (!stackPop(s, &b)) {                                                                        \
      return 0;                                                                                    \
    }                                                                                              \
                                                                                                   \
    if (!stackPop(s, &a)) {                                                                        \
      return 0;                                                                                    \
    }                                                                                              \
                                                                                                   \
    if (!stackPush(s, a op b)) {                                                                   \
      return 0;                                                                                    \
    }                                                                                              \
  } while (0)

int programEval(Program *p, Stack *s) {
  int frame = 0;
  float a;
  float b;
  for (int i = 0; i < p->opsCount; i++) {
    Op op = p->ops[i];
    switch (op.type) {
    case OP_NUM:
      if (!stackPush(s, op.data)) {
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
      if (!stackPop(s, &a)) {
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
      Function *f = &p->functions[(int)op.data];

      s->count += f->body - f->arity;
      if (s->count > STACK_CAP) {
        return 0;
      }

      if (!stackPush(s, i)) {
        return 0;
      }

      if (!stackPush(s, frame)) {
        return 0;
      }

      frame = s->count - f->body - 2;
      i = f->start - 1;
    } break;

    case OP_NATIVE: {
      Function *f = &p->functions[(int)op.data];

      s->count -= f->arity;
      if (s->count < 0) {
        return 0;
      }

      float result = p->natives[-f->start - 1](s->data + s->count);
      if (!stackPush(s, result)) {
        return 0;
      }
    } break;

    case OP_RETURN:
      if (!stackPop(s, &a)) {
        return 0;
      }

      if (!stackPop(s, &b)) {
        return 0;
      }
      frame = b;

      if (!stackPop(s, &b)) {
        return 0;
      }
      i = b;

      s->count -= p->functions[(int)op.data].body;
      if (s->count < 0) {
        return 0;
      }

      if (!stackPush(s, a)) {
        return 0;
      }
      break;

    case OP_DROP:
      if (!stackPop(s, &a)) {
        return 0;
      }
      break;

    case OP_GETG:
      if (!stackPush(s, p->variables[(int)op.data].data)) {
        return 0;
      }
      break;

    case OP_SETG:
      if (!stackPop(s, &a)) {
        return 0;
      }

      p->variables[(int)op.data].data = a;
      break;

    case OP_GETL:
      if (!stackPush(s, s->data[frame + (int)op.data])) {
        return 0;
      }
      break;

    case OP_SETL:
      if (!stackPop(s, &a)) {
        return 0;
      }

      s->data[frame + (int)op.data] = a;
      break;
    }
  }

  return 1;
}

// Canvas
#define CANVAS_CAP 1024

int canvasCount;
float canvasXs[CANVAS_CAP];
float canvasYs[CANVAS_CAP];
float canvasAngle;

float canvasMove(float *arg) {
  if (canvasCount >= CANVAS_CAP) {
    LOG_ERROR(STR("Canvas overflow"));
  } else {
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
void penRender(int w, int h) {
  w /= 2;
  h /= 2;

  platformClear();
  for (int i = 1; i < canvasCount; i++) {
    platformDrawLine(w + (int)canvasXs[i - 1], h + (int)canvasYs[i - 1], w + (int)canvasXs[i],
                     h + (int)canvasYs[i]);
  }
}

Lexer lexer;
Stack stack;
Program program;

void penUpdate(char *data, int size) {
  stack.count = 0;
  canvasCount = 0;
  program.opsCount = 0;

  program.functionsCount = 0;
  program.variablesCount = 0;

  program.functionsLocal = 0;
  program.variablesMax = 0;
  program.variablesBase = 0;

  lexerInit(&lexer, (Str){.data = data, .count = size});
  programPushNative(&program, STR("move"), 1, canvasMove);
  programPushNative(&program, STR("rotate"), 1, canvasRotate);

  Token token;
  while (1) {
    if (!lexerPeek(&lexer, &token)) {
      return;
    }

    if (token.type == TOKEN_EOF) {
      break;
    }

    if (!compileStmt(&lexer, &program)) {
      return;
    }
  }

  canvasAngle = 0;
  canvasCount = 1;
  programEval(&program, &stack);
}

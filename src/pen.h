#ifndef PEN_H
#define PEN_H

void platformClear(void);
void platformErrorStart(void);
void platformErrorPush(char *data, int count);
void platformErrorEnd(void);
void platformDrawLine(int x1, int y1, int x2, int y2);

void penRender(int w, int h);
void penUpdate(char *data, int size);

#endif

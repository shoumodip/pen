#ifndef PEN_H
#define PEN_H

#define WIDTH 800
#define HEIGHT 600

void platformClear(void);
void platformError(char *data);
void platformDrawLine(int x1, int y1, int x2, int y2);

void penUpdate(char *data, int size);
void penRender(void);

#endif

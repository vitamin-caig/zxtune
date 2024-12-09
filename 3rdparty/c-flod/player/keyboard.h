#ifndef KEYBOARD_H
#define KEYBOARD_H

void init_keyboard(void);
void close_keyboard(void);
int kbhit(void);
int readch(void);

#endif
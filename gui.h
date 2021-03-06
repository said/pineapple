#define SETLO(v,x) v = ((v) & 0xf0) | (x)
#define SETHI(v,x) v = ((v) & 0x0f) | ((x) << 4)
#define CTRL(c) ((c) & 037)
#define KEY_ESCAPE 27
#define KEY_TAB 9   // this also happens to be ^i...
#define ENTER 13

#ifndef WINDOWS
#define BACKSPACE 0x07
#endif

int currmode;
char currcmd;
int instrx, instry, instroffs;
int octave;
int songx, songy, songoffs;
int trackx, tracky, trackoffs;
int currtrack, currinstr;
int currtab;
int saved;
int disptick;
int step;

char cmdstr[500];
char filename[1024];
char *validcmds;
char *keymap[2];

void initsonglines(void);
void inittracks(void);
void initinstrs(void);

void drawsonged(int x, int y, int height);
void drawtracked(int x, int y, int height);
void drawinstred(int x, int y, int height);
void drawgui(void);

int freqkey(int c);
int hexinc(int x);
int hexdec(int x);
void insertc (int c);
char nextchar(void);
void setdisplay(char *str);

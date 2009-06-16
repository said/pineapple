/* vi:set ts=4 sts=4 sw=4 noexpandtab: */
#include <stdint.h>

#ifndef PINEAPPLE_H
#define PINEAPPLE_H
#define TRACKLEN 32

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

u8 callbacktime;
char filename[1024];
char *infinitemsg;
char comment[1024];

/* MODES */
enum {
	PM_NORMAL,
	PM_VISUAL,
	PM_VISUALLINE,
	PM_CMDLINE,
	PM_INSERT,
	PM_JAMMER
};
void normalmode(int c);
void cmdlinemode(void);
void insertmode(void);
void jammermode(void);
void visualmode(void);
void visuallinemode(void);

typedef enum {
	WF_TRI = 0,
	WF_SAW,
	WF_PUL,
	WF_NOI,
	WF_SINE
} waveform_t;

volatile struct oscillator {
	u16	freq;
	u16	phase;
	u16	duty;
	waveform_t waveform;
	u8 volume;	// 0-255
} osc[4];

struct trackline {
	u8	note;
	u8	instr;
	u8	cmd[2];
	u8	param[2];
};

struct track {
	struct trackline	line[TRACKLEN];
};

struct instrline {
	u8			cmd;
	u8			param;
};

struct instrument {
	int			length;
	struct instrline	line[256];
};

struct songline {
	u8			track[4];
	u8			transp[4];
};

struct instrument instrument[256], iclip[256];
struct track track[256], tclip[256];
struct songline song[256];

int songlen, tracklen;

void initchip(void);
u8 interrupthandler(void);

void readsong(int pos, int ch, u8 *dest);
void readtrack(int num, int pos, struct trackline *tl);
void readinstr(int num, int pos, u8 *il);

void silence(void);
void iedplonk(int, int);

void initgui(void);
void guiloop(void);

void display(void);

void startplaysong(int);
void startplaytrack(int);

void parsecmd(char cmd[]);

// just some poorly-named variables for hackin together the repeat command
int cmdrepeat;
int cmdrepeatnum;
int lastaction;
int lastrepeatnum;

/* a linked list */
/*typedef struct list_node{
	void *element;
	struct list_node *next;
} NODE;

NODE *list_create(void *e);
NODE *list_contains(NODE *n, int(*func)(void *, void *), void *match);
int findu8(void *a, void *b);

NODE *highlightlines;*/

/* lines to be highlighted in visual line mode */
int highlight_firstline, highlight_lastline, highlight_lineamount;

/* lines to be highlighted in visual mode */
int highlight_firstx, highlight_firsty, highlight_lasty, highlight_lastx; 

/* ACTIONS */
void act_bigmvdown(void);
void act_bigmvup(void);
void act_clritall(void);
void act_clronething(void);
void act_clrinsongtab(int y);
void act_clrintracktab(int t, int y);
void act_clrininstrtab(int instr, int y);
void act_fxdec(void);
void act_fxinc(void);
void act_instrdec(void);
void act_instrinc(void);
void act_mvdown(void);
void act_mvleft(void);
void act_mvright(void);
void act_mvup(void);
void act_notedec(void);
void act_noteinc(void);
void act_octavedec(void);
void act_octaveinc(void);
void act_paramdec(void);
void act_paraminc(void);
void act_trackdec(void);
void act_trackinc(void);
void act_transpdec(void);
void act_transpinc(void);
void act_undo(void);
void act_viewinstrdec(void);
void act_viewinstrinc(void);
void act_viewtrackdec(void);
void act_viewtrackinc(void);

u8 trackpos;
u8 playtrack;
u8 playsong;
u8 songpos;
int songlen;

#endif /* PINEAPPLE_H */

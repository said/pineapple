/* vi:set ts=4 sts=4 sw=4 noexpandtab: */

/* welcome to gui.c, enjoy your stay 8-) */

#include "pineapple.h"
#include "gui.h"

/*                  */
// ** LOCAL VARS ** //
/*                  */
char *dispmesg = "";

static char *notenames[] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "H-"};

/*                       */
// ** LOCAL FUNCTIONS ** //
/*                       */
int _char2int(char ch);
void _display(void);

/*                              */
// ** END LOCAL DECLARATIONS ** //
/*                              */

char cmdstr[500] = "";

int disptick = 0;
int currmode = PM_NORMAL;
int octave = 4;
int songlen = 1;
int tracklen = TRACKLEN;
int currtrack = 1;
int currinstr = 1;
int currtab = 0;
int saved = 1;

int step = 1;

int cmdrepeat = 0;
int cmdrepeatnum = 1;
int lastrepeat = 1;

// 0 is like a blank command
char *validcmds = "0dfi@smtvw~+=*";

/*char *keymap[2] = {
	";oqejkixdbhmwnvsz",
	"'2,3.p5y6f7gc9r0l/="
};*/

char *keymap[2] = {
	"zsxdcvgbhnjm,l.;/",
	"q2w3er5t6y7ui9o0p"
};

/* hexinc and hexdec wrap around */
int hexinc(int x){
	return (x >= 0 && x <= 14)? x+1 : 0;
}
int hexdec(int x){
	return (x >= 1 && x <= 15)? x-1 : 15;
}

/* Wait for the next keyboard char and return it.
 * This stops the screen from being updated. */
char nextchar(){
	char ch;
	ch = getch();
	while (ch == ERR){
		ch = getch();
		if(ch != ERR ){
			return ch;
		}
		usleep(10000);
	}
	return ch;
}

int _char2int(char ch){
	if(isdigit(ch)){
		return (int)ch - '0';
	}
	return -1;
}

int freqkey(int c){
	char *s;
	int f = -1;

	if(c == '-' || c == KEY_DC) return 0;
	if(c > 0 && c < 256){
		s = strchr(keymap[0], c);
		if(s){
			f = (s - (keymap[0])) + octave * 12 + 1;
		}else{
			s = strchr(keymap[1], c);
			if(s){
				f = (s - (keymap[1])) + octave * 12 + 12 + 1;
			}
		}
	}
	if(f > 12 * 9 + 1) return -1;
	return f;
}

void initsonglines(PT_TUNE *pt){
	for(int i=0; i < pt->songlen; i++){
		memmove(&pt->song[i + 0], &pt->song[i + 1], sizeof(struct songline) * (songlen - i - 1));
		if(i < 4){
			pt->song[0].track[i] = 0x000;
			pt->song[0].transp[i] = 0x000;
		}
	}
	pt->songlen = 1;
}

void inittracks(PT_TUNE *pt){
	for(int i=0; i < 256; i++){
		for(int j=0; j < TRACKLEN; j++){
			pt->track[i].line[j].note = 0x0000;
			pt->track[i].line[j].instr = 0x0000;
			for(int k=0; k < 2; k++){
				pt->track[i].line[j].cmd[k] = 0x0000;
				pt->track[i].line[j].param[k] = 0x0000;
			}
		}
	}
}

void initinstrs(PT_TUNE *pt){
	for(int i=1; i < 256; i++){
		pt->instrument[i].length = 1;
		pt->instrument[i].line[0].cmd = '0';
		pt->instrument[i].line[0].param = 0;
	}
}

void readsong(PT_TUNE *pt, int pos, int ch, u8 *dest){ 
	dest[0] = pt->song[pos].track[ch];
	dest[1] = pt->song[pos].transp[ch];
}

void readtrack(PT_TUNE *pt, int num, int pos, struct trackline *tl){
	tl->note = pt->track[num].line[pos].note;
	tl->instr = pt->track[num].line[pos].instr;
	tl->cmd[0] = pt->track[num].line[pos].cmd[0];
	tl->cmd[1] = pt->track[num].line[pos].cmd[1];
	tl->param[0] = pt->track[num].line[pos].param[0];
	tl->param[1] = pt->track[num].line[pos].param[1];
}

void readinstr(PT_TUNE *pt, int num, int pos, u8 *il){
	if(pos >= pt->instrument[num].length){
		il[0] = 0;
		il[1] = 0;
	}else{
		il[0] = pt->instrument[num].line[pos].cmd;
		il[1] = pt->instrument[num].line[pos].param;
	}
}

void exitgui(){
	endwin();
}

void initgui(PT_TUNE *pt){
	initscr();

	//if(setlocale(LC_CTYPE,"en_US.utf8") != NULL) setdisplay("UTF-8 enabled!");

	// don't send newline on Enter key, and don't echo chars to the screen.
	nonl();
	noecho();

	// make sure behaviour for special keys like ^H isn't overridden
	keypad(stdscr, FALSE);

	// nodelay() makes getch() non-blocking. This will cause the cpu to spin
	// whenever we use getch() in a loop. This is necessary so the screen will
	// update when you aren't pressing keys. halfdelay()'s minimum timeout time
	// is one tenth of a second, which is too long for our purposes.
	//
	// Right now we are calling usleep() whenever we use getch() in a loop so
	// the cpu won't spin. This solution isn't the best, for three reasons:
	//    1. We're still wasting a little bit of cpu!!!!!
	//    2. It is possible to enter keys faster than the usleep time. It's
	//       especially easy to do this by setting your key repeat rate really
	//       high and moving up or down, and the screen will lag a little.
	//    3. nextchar() prevents the screen from being updated.
	//
	// Because of these two small problems, maybe we should eventually use
	// keyboard interrupts to trigger gui events. I haven't done any research
	// on that yet.
	nodelay(stdscr, TRUE);

	initinstrs(pt);

	atexit(exitgui);
}

void drawsonged(PT_TUNE *pt, int x, int y, int height){
	int i, j;
	char buf[1024];
	//NODE *match;

	if(pt->songy < pt->songoffs) pt->songoffs = pt->songy;
	if(pt->songy >= pt->songoffs + height) pt->songoffs = pt->songy - height + 1;

	for(i = 0; i < pt->songlen; i++){
		if(i >= pt->songoffs && i - pt->songoffs < height){
			move(y + i - pt->songoffs, x + 0);
			if(i == pt->songy) attrset(A_BOLD);

			snprintf(buf, sizeof(buf), "%02x", i);

			if(i == 0){ addch(ACS_ULCORNER); }
			else if(i == pt->songlen-1){ addch(ACS_LLCORNER); }
			else if(i%4 == 0){ addch(ACS_LTEE); }
			else if(i < songlen-1){ addch(ACS_VLINE); }
			addch(' ');

			// should this line be highlighted?
			//if( (match = list_contains(highlightlines, findu8, &i)) ){
			if( currtab == 0 && currmode == PM_VISUALLINE &&
				((i <= highlight_firstline && i >= highlight_lastline)
				|| (i >= highlight_firstline && i <= highlight_lastline)) ){
				attrset(A_REVERSE);
			}

			addstr(buf);
			for(j = 0; j < 4; j++){
				snprintf(buf, sizeof(buf), "%02x:%02x", pt->song[i].track[j], pt->song[i].transp[j]);
				addstr(buf);
				if(j != 3) addch(' ');
			}
			if(playsong && pt->songpos == (i + 1)){
				attrset(A_STANDOUT);
				addch('*');
			}
			attrset(A_NORMAL);
		}
	}
}

void drawtracked(PT_TUNE *pt, int x, int y, int height){
	u8 i, j;
	char buf[1024];

	if(pt->tracky < pt->trackoffs) pt->trackoffs = pt->tracky;
	if(pt->tracky >= pt->trackoffs + height) pt->trackoffs = pt->tracky - height + 1;

	for(i = 0; i < tracklen; i++){
		if(i >= pt->trackoffs && i - pt->trackoffs < height){
			move(y + i - pt->trackoffs, x + 0);
			if(i == pt->tracky) attrset(A_BOLD);

			snprintf(buf, sizeof(buf), "%02x", i);
			addstr(buf);

			if(i == 0){ addch(ACS_LLCORNER); }
			else if(i == 1){ addch(ACS_ULCORNER); }
			else if(i == tracklen-1){ addch(ACS_LLCORNER); }
			else if(i%4 == 0){ addch(ACS_LTEE); }
			else if(i < tracklen-1){ addch(ACS_VLINE); }
			addch(' ');

			// should this line be highlighted?
			//if( (match = list_contains(highlightlines, findu8, &i)) ){
			if( currtab == 1 && currmode == PM_VISUALLINE &&
				((i <= highlight_firstline && i >= highlight_lastline)
				|| (i >= highlight_firstline && i <= highlight_lastline)) ){
				attrset(A_REVERSE);
			}

			if(pt->track[currtrack].line[i].note){
				snprintf(buf, sizeof(buf), "%s%d",
					notenames[(pt->track[currtrack].line[i].note - 1) % 12],
					(pt->track[currtrack].line[i].note - 1) / 12);
			}else{
				snprintf(buf, sizeof(buf), "---");
			}
			addstr(buf);
			snprintf(buf, sizeof(buf), " %02x", pt->track[currtrack].line[i].instr);
			addstr(buf);
			for(j = 0; j < 2; j++){
				if(pt->track[currtrack].line[i].cmd[j]){
					snprintf(buf, sizeof(buf), " %c%02x",
						pt->track[currtrack].line[i].cmd[j],
						pt->track[currtrack].line[i].param[j]);
				}else{
					snprintf(buf, sizeof(buf), " ...");
				}
				addstr(buf);
			}
			if(playtrack && ((i + 1) % tracklen) == pt->trackpos){
				attrset(A_STANDOUT);
				addch('*');
			}
			attrset(A_NORMAL);
		}
	}
}

void drawinstred(PT_TUNE *pt, int x, int y, int height){
	u8 i;
	char buf[1024];

	if(pt->instry >= pt->instrument[currinstr].length) pt->instry = pt->instrument[currinstr].length - 1;

	if(pt->instry < pt->instroffs) pt->instroffs = pt->instry;
	if(pt->instry >= pt->instroffs + height) pt->instroffs = pt->instry - height + 1;

	for(i = 0; i < pt->instrument[currinstr].length; i++){
		if(i >= pt->instroffs && i - pt->instroffs < height){
			move(y + i - pt->instroffs, x + 0);
			if(i == pt->instry) attrset(A_BOLD);

			snprintf(buf, sizeof(buf), "%02x", i);
			addstr(buf);

			if(i == 0){ addch(ACS_LLCORNER); }
			else if(i == 1){ addch(ACS_ULCORNER); }
			else if(i == pt->instrument[currinstr].length-1){ addch(ACS_LLCORNER); }
			else if(i < pt->instrument[currinstr].length-1){ addch(ACS_VLINE); }
			addch(' ');

			// should this line be highlighted?
			//if( (match = list_contains(highlightlines, findu8, &i)) ){
			if( pt->currtab == 2 && currmode == PM_VISUALLINE &&
				((i <= highlight_firstline && i >= highlight_lastline)
				|| (i >= highlight_firstline && i <= highlight_lastline)) ){
				attrset(A_REVERSE);
			}

			snprintf(buf, sizeof(buf), "%c ", pt->instrument[currinstr].line[i].cmd);
			addstr(buf);
			if(pt->instrument[currinstr].line[i].cmd == '+' || pt->instrument[currinstr].line[i].cmd == '='){
				if(pt->instrument[currinstr].line[i].param){
					snprintf(buf, sizeof(buf), "%s%d",
						notenames[(pt->instrument[currinstr].line[i].param - 1) % 12],
						(pt->instrument[currinstr].line[i].param - 1) / 12);
				}else{
					snprintf(buf, sizeof(buf), "---");
				}
			}else{
				snprintf(buf, sizeof(buf), "%02x", pt->instrument[currinstr].line[i].param);
			}
			addstr(buf);
			attrset(A_NORMAL);
		}
	}
}

/* you guys KNOW we arn't using this and probably won't....
 * ....
 * ....
 * EVER
 */

/*static FILE *exportfile = 0;
static int exportbits = 0;
static int exportcount = 0;
static int exportseek = 0;

void putbit(int x){
	if(x){
		exportbits |= (1 << exportcount);
	}
	exportcount++;
	if(exportcount == 8){
		if(exportfile){
			fprintf(exportfile, "\t.byte\t0x%02x\n", exportbits);
		}
		exportseek++;
		exportbits = 0;
		exportcount = 0;
	}
}

void exportchunk(int data, int bits){
	int i;

	for(i = 0; i < bits; i++){
		putbit(!!(data & (1 << i)));
	}
}

int alignbyte(){
	if(exportcount){
		if(exportfile){
			fprintf(exportfile, "\t.byte\t0x%02x\n", exportbits);
		}
		exportseek++;
		exportbits = 0;
		exportcount = 0;
	}
	if(exportfile) fprintf(exportfile, "\n");
	return exportseek;
}

int packcmd(u8 ch){
	if(!ch) return 0;
	if(strchr(validcmds, ch)){
		return strchr(validcmds, ch) - validcmds;
	}
	return 0;
}

void exportdata(FILE *f, int maxtrack, int *resources){
	int i, j;
	int nres = 0;

	exportfile = f;
	exportbits = 0;
	exportcount = 0;
	exportseek = 0;

	for(i = 0; i < 16 + maxtrack; i++){
		exportchunk(resources[i], 13);
	}

	resources[nres++] = alignbyte();

	for(i = 0; i < songlen; i++){
		for(j = 0; j < 4; j++){
			if(song[i].transp[j]){
				exportchunk(1, 1);
				exportchunk(song[i].track[j], 6);
				exportchunk(song[i].transp[j], 4);
			}else{
				exportchunk(0, 1);
				exportchunk(song[i].track[j], 6);
			}
		}
	}

	for(i = 1; i < 16; i++){
		resources[nres++] = alignbyte();

		if(instrument[i].length > 1){
			for(j = 0; j < instrument[i].length; j++){
				exportchunk(packcmd(instrument[i].line[j].cmd), 8);
				exportchunk(instrument[i].line[j].param, 8);
			}
		}

		exportchunk(0, 8);
	}

	for(i = 1; i <= maxtrack; i++){
		resources[nres++] = alignbyte();

		for(j = 0; j < tracklen; j++){
			u8 cmd = packcmd(track[i].line[j].cmd[0]);

			exportchunk(!!track[i].line[j].note, 1);
			exportchunk(!!track[i].line[j].instr, 1);
			exportchunk(!!cmd, 1);

			if(track[i].line[j].note){
				exportchunk(track[i].line[j].note, 7);
			}

			if(track[i].line[j].instr){
				exportchunk(track[i].line[j].instr, 4);
			}

			if(cmd){
				exportchunk(cmd, 4);
				exportchunk(track[i].line[j].param[0], 8);
			}
		}
	}
}

void export(){
	FILE *f = fopen("exported.s", "w");
	FILE *hf = fopen("exported.h", "w");
	int i, j;
	int maxtrack = 0;
	int resources[256];

	exportfile = 0;
	exportbits = 0;
	exportcount = 0;
	exportseek = 0;

	for(i = 0; i < songlen; i++){
		for(j = 0; j < 4; j++){
			if(maxtrack < song[i].track[j]) maxtrack = song[i].track[j];
		}
	}

	fprintf(f, "\t.global\tsongdata\n\n");

	fprintf(hf, "#define MAXTRACK\t0x%02x\n", maxtrack);
	fprintf(hf, "#define SONGLEN\t\t0x%02x\n", songlen);

	fprintf(f, "songdata:\n");

	exportdata(0, maxtrack, resources);

	fprintf(f, "# ");
	for(i = 0; i < 16 + maxtrack; i++){
		fprintf(f, "%04x ", resources[i]);
	}
	fprintf(f, "\n");

	exportdata(f, maxtrack, resources);

	fclose(f);
	fclose(hf);
}
*/

/* main input loop */
void handleinput(PT_TUNE *pt){
	int c;

	/*if(currmode == PM_NORMAL){*/
	if((c = getch()) != ERR){

		/* Repeat */
		if(isdigit(c)){
			if(!cmdrepeat){
				cmdrepeat = 1;
				cmdrepeatnum = _char2int(c);
			}else{
				cmdrepeatnum = (cmdrepeatnum*10) + _char2int(c);
			}
		}else{
			normalmode(pt, c);
		}
	}
	usleep(10000);
}

void setdisplay(char *str){
	disptick = 350;
	dispmesg = str;
}

// display dispmesg in the center of the screen
void _display(void){
	int cx = (getmaxx(stdscr)/2)-(strlen(dispmesg)/2)-1;
	int cy = getmaxy(stdscr)/2;

	mvaddch(cy-1, cx, ACS_ULCORNER);
	for(int i=cx+1; i<cx+strlen(dispmesg)+1; i++)
		mvaddch(cy-1, i, ACS_HLINE);
	mvaddch(cy-1, cx+strlen(dispmesg)+1, ACS_URCORNER);

	mvaddch(cy, cx, ACS_VLINE);
	mvaddstr(cy, cx+1, dispmesg);
	mvaddch(cy, cx+strlen(dispmesg)+1, ACS_VLINE);

	mvaddch(cy+1, cx, ACS_LLCORNER);
	for(int i=cx+1; i<cx+strlen(dispmesg)+1; i++)
		mvaddch(cy+1, i, ACS_HLINE);
	mvaddch(cy+1, cx+strlen(dispmesg)+1, ACS_LRCORNER);
}

void drawgui(PT_TUNE *pt){
	char buf[1024];
	int lines = LINES;
	int songcols[] = {0, 1, 3, 4, 6, 7, 9, 10, 12, 13, 15, 16, 18, 19, 21, 22};
	int trackcols[] = {0, 2, 4, 5, 7, 8, 9, 11, 12, 13};
	int instrcols[] = {0, 2, 3};
	u8 tempo;

	erase(); 
	attrset(A_UNDERLINE);
	mvaddstr(0, 0, "PINEAPPLEtRACKER");
	attrset(A_NORMAL);

	// display track num
	mvaddch(0, 31, ACS_ULCORNER);
	snprintf(buf, sizeof(buf), "%02x{}", currtrack);
	mvaddstr(0, 32, buf);
	drawtracked(pt, 29, 1, lines - 2);

	// display instrument num
	mvaddch(0, 51, ACS_ULCORNER);
	snprintf(buf, sizeof(buf), "%02x[]", currinstr);
	mvaddstr(0, 52, buf);
	drawinstred(pt, 49, 1, lines - 2);

	mvaddstr(1, 0, "Song");
	drawsonged(pt, 0, 1, lines - 2);

	// just a wild guess here..
	tempo = pt->callbacktime * (-1) + 300;
	// display tempo
	mvaddch(0, 17, ACS_DEGREE);
	snprintf(buf, sizeof(buf), "%d()", tempo);
	mvaddstr(0, 18, buf);

	// display octave
	mvaddch(0, 24, ACS_PI);
	snprintf(buf, sizeof(buf), "%d<>", octave);
	mvaddstr(0, 25, buf);

	// display step amount
	mvaddstr(0, 60, "step -=");
	snprintf(buf, sizeof(buf), "%0x", step); 
	mvaddstr(0, 68, buf);

	// display comment
	mvaddstr(2, 60, "comment:");
	snprintf(buf, sizeof(buf), "%s", pt->comment);
	mvaddstr(3, 60, buf);

	if(currmode == PM_NORMAL){
		mvaddstr(getmaxy(stdscr)-1, 0, filename);
		if(!saved && currmode != PM_INSERT){
			addstr(" [+]");
			infinitemsg = NULL;
		}
	}

	if(disptick > 0){
		_display();
		disptick--;
	}

	if(currmode == PM_INSERT){
		infinitemsg = NULL;

		move(getmaxy(stdscr)-1,0);
		clrtoeol();
		mvaddstr(getmaxy(stdscr)-1, 0, "-- INSERT --");
	}else if(currmode == PM_VISUAL){
		infinitemsg = NULL;

		move(getmaxy(stdscr)-1,0);
		clrtoeol();
		mvaddstr(getmaxy(stdscr)-1, 0, "-- VISUAL --");
	}else if(currmode == PM_VISUALLINE){
		infinitemsg = NULL;

		move(getmaxy(stdscr)-1,0);
		clrtoeol();
		mvaddstr(getmaxy(stdscr)-1, 0, "-- VISUAL LINE --");
	}else if(currmode == PM_JAMMER){
		infinitemsg = NULL;

		move(getmaxy(stdscr)-1,0);
		clrtoeol();
		mvaddstr(getmaxy(stdscr)-1, 0, "-- JAMMER --");
	}else if(currmode == PM_CMDLINE){
		infinitemsg = NULL;

		move(getmaxy(stdscr)-1,0);
		clrtoeol();
		mvaddstr(getmaxy(stdscr) - 1, 0, cmdstr);
	}else if(infinitemsg != NULL){
		move(getmaxy(stdscr)-1,0);
		clrtoeol();
		mvaddstr(getmaxy(stdscr) - 1, 0, infinitemsg);
	}
    
	switch(currtab){
		case 0:
			move(1 + pt->songy - pt->songoffs, 0 + 4 + songcols[pt->songx]);
			break;
		case 1:
			move(1 + pt->tracky - pt->trackoffs, 29 + 4 + trackcols[pt->trackx]);
			break;
		case 2:
			move(1 + pt->instry - pt->instroffs, 49 + 4 + instrcols[pt->instrx]);
			break;
	}

	refresh();

	if(disptick > 0){
		disptick--;
	}
}

void guiloop(PT_TUNE *pt){
#ifndef WINDOWS
	// don't treat the escape key like a meta key
	ESCDELAY = 50;
#endif
	for(;;){
		drawgui(pt);
		handleinput(pt);
	}
}


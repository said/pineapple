/* vi:set syntax= ts=8 sts=8 sw=8 noexpandtab: */
/* WARNING: this file is ROSS STYLE */

#include "pineapple.h"
#include "gui.h"
#include "musicchip_file.h"

int f;
int tcliplen, icliplen = 0;
int lastinsert = 0;

int _hexdigit(char c);
int _nextfreetrack(void);
int _nextfreeinstr(void);

/* Let's make a linked list! */
/* Here's a great generic implementation:
 * http://en.literateprograms.org/Singly_linked_list_(C)
 */
/*NODE *list_create(void *e){
	NODE *n;
	if(!(n=malloc(sizeof(NODE)))) return NULL;
	n->element = e;
	n->next = NULL;
	return n;
}

NODE *list_insertafter(NODE *oldnode, void *newelement){
	NODE *newnode;
	newnode = list_create(newelement);
	newnode->next = oldnode->next;
	oldnode->next = newnode;
	return newnode;
}

// have a look at this wizardry! wow. what does int(*func) do?
NODE *list_contains(NODE *n, int (*func) (void *, void *), void *match){
	while(n){
		if(func(n->element, match) > 0) return n;
		n = n->next;
	}
	return NULL;
}

int findu8(void *a, void *b){
	return a==b;
}*/

int _hexdigit(char c){
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'a' && c <= 'f') return c - 'a' + 10;
	return -1;
}

int _nextfreetrack(){
	int skiptherest = 0;

	for(int i = 1; i <= 0xff; i++){
		for(int j = 0; j < tracklen; j++){
			if(track[i].line[j].note) skiptherest = 1;
			for(int k = 0; k < 2; k++){
				if(track[i].line[j].cmd[k]) skiptherest = 1;
				if(track[i].line[j].param[k]) skiptherest = 1;
			}

			// skip the rest of this track?
			if(skiptherest){
				skiptherest = 0;
				break;
			}

			// this track is free, so return the index
			if(j == tracklen-1) return i;
		}
	}

	setdisplay("_nextfreetrack() failed somehow..");
	return -1;
}

int _nextfreeinstr(){
	for(int i = 1; i <= 0xff; i++){
		if(instrument[i].line[0].cmd == '0')
			return i;
	}

	setdisplay("_nextfreeinstr() failed somehow..");
	return -1;
}

void _insertc(int c){
	int x;

	x = _hexdigit(c);
	if(x >= 0){
		if(currtab == 2
		&& instrx > 0
		&& instrument[currinstr].line[instry].cmd != '+'
		&& instrument[currinstr].line[instry].cmd != '='){
			switch(instrx){
				case 1: SETHI(instrument[currinstr].line[instry].param, x); break;
				case 2: SETLO(instrument[currinstr].line[instry].param, x); break;
			}
		}
		if(currtab == 1 && trackx > 1){
			switch(trackx){
				case 2: SETHI(track[currtrack].line[tracky].instr, x); break;
				case 3: SETLO(track[currtrack].line[tracky].instr, x); break;
				case 5: if(track[currtrack].line[tracky].cmd[0])
					SETHI(track[currtrack].line[tracky].param[0], x); break;
				case 6: if(track[currtrack].line[tracky].cmd[0])
					SETLO(track[currtrack].line[tracky].param[0], x); break;
				case 8: if(track[currtrack].line[tracky].cmd[1])
					SETHI(track[currtrack].line[tracky].param[1], x); break;
				case 9: if(track[currtrack].line[tracky].cmd[1])
					SETLO(track[currtrack].line[tracky].param[1], x); break;
			}
		}
		if(currtab == 0){
			switch(songx & 3){
				case 0: SETHI(song[songy].track[songx / 4], x); break;
				case 1: SETLO(song[songy].track[songx / 4], x); break;
				case 2: SETHI(song[songy].transp[songx / 4], x); break;
				case 3: SETLO(song[songy].transp[songx / 4], x); break;
			}
		}
	}
	x = freqkey(c);
	if(x >= 0){
		if(currtab == 2
		&& instrx
		&& (instrument[currinstr].line[instry].cmd == '+' || instrument[currinstr].line[instry].cmd == '=')){
			instrument[currinstr].line[instry].param = x;
		}
		if(currtab == 1 && !trackx){
			track[currtrack].line[tracky].note = x;
			if(x){
				track[currtrack].line[tracky].instr = currinstr;
			}else{
				track[currtrack].line[tracky].instr = 0;
			}
			if(x) iedplonk(x, currinstr);
		}
	}
	if(currtab == 2 && instrx == 0){
		if(strchr(validcmds, c))
			instrument[currinstr].line[instry].cmd = c;
	}
	if(currtab == 1 && (trackx == 4 || trackx == 7)){
		if(strchr(validcmds, c)){
			if(c == '.' || c == '0') c = 0;
			track[currtrack].line[tracky].cmd[(trackx - 3) / 3] = c;
		}
	}
	// for repeat
	lastinsert = c;
}

void _parsecmd(char cmd[]){
	//if(cmd[1] == 'w'){
	//switch(strcmp(cmd,
	if(strcmp(cmd, ":w") == 0){
		savefile(filename);
		saved = 1;
	}else if(strcmp(cmd, ":q") == 0){
		erase();
		refresh();
		endwin();
		exit(0);
	}else if(strcmp(cmd, ":write") == 0){
		savefile(filename);
		saved = 1;
	}else if(strcmp(cmd, ":wq") == 0){
		savefile(filename);
		saved = 1;
		erase();
		refresh();
		endwin();
		exit(0);
	}else if(strcmp(cmd, ":quit") == 0){
		erase();
		refresh();
		endwin();
		exit(0);
	}else if(cmd[1]=='e' && cmd[2]==' '){
		// if the file doesn't exist, clear the song
		if(loadfile(cmd+3)){
			initsonglines();
			inittracks();
			initinstrs();
		}
	}else if(isdigit(cmd[1])){
		int gotoline = atoi(cmd+1);

		switch(currtab){
			case 0:
				if(gotoline>songlen){ songy=songlen-1; }
				else{ songy = gotoline; }
				break;
			case 1:
				if(gotoline>tracklen){ tracky=tracklen-1; }
				else{ tracky = gotoline; }
				break;
			case 2:
				if(gotoline>instrument[currinstr].length){
					instry=instrument[currinstr].length-1; }
				else{ instry = gotoline; }
				break;
		}
	}else if(cmd[1] == 'c' && cmd[2] == ' '){
		strncpy(comment, cmd+3, sizeof(comment));
	}else 
		setdisplay("not a tracker command!");
	return;
}

/* normal mode */
void normalmode(int c){
	int i;


	// don't save the action for repeat if it's a movement or a repeat, or
	// something else that doesnt make sense to repeat
	if(c != 'h' &&
		c != 'j' && 
		c != 'k' && 
		c != 'l' && 
		c != CTRL('D') && 
		c != CTRL('U') && 
		c != CTRL('H') && 
		c != CTRL('L') && 
		c != 'H' && 
		c != 'M' && 
		c != 'L' && 
		c != 'g' && 
		c != 'G' && 
		c != '.'){
		lastaction = c;
		lastrepeatnum = cmdrepeatnum;
	}

	for(i=0; i<cmdrepeatnum; i++){
		switch(c){
		/* add line */
		case 'a':
			if(currtab == 2){
				struct instrument *in = &instrument[currinstr];

				if(in->length < 256){
					memmove(&in->line[instry + 2], &in->line[instry + 1], sizeof(struct instrline) * (in->length - instry - 1));
					instry++;
					in->length++;
					in->line[instry].cmd = '0';
					in->line[instry].param = 0;
				}
			}else if(currtab == 0){
				if(songlen < 256){
					memmove(&song[songy + 2], &song[songy + 1], sizeof(struct songline) * (songlen - songy - 1));
					songy++;
					songlen++;
					memset(&song[songy], 0, sizeof(struct songline));
				}
			}
			break;
		case '.':
			// if the last command was a replace, just insert the last thing
			// inserted instead of calling insertmode()
			if(lastaction == 'r')
				_insertc(lastinsert);
			else
				normalmode(lastaction);
			cmdrepeatnum = lastrepeatnum;
			break;
		case KEY_ESCAPE:
			disptick = 0;
			break;
		case CTRL('Y'):
			switch(currtab){
				case 0:
					if(songoffs>0){
						if(songy==getmaxy(stdscr)-3+songoffs)
							songy--;
						songoffs--;
					}
					break;
				case 1:
					if(trackoffs>0){
						if(tracky==getmaxy(stdscr)-3+trackoffs)
							tracky--;
						trackoffs--;
					}
					break;
				case 2:
					if(instroffs>0){
						if(instry==getmaxy(stdscr)-3+instroffs)
							instry--;
						instroffs--;
					}
					break;
			}
			break;
		case CTRL('E'):
			switch(currtab){
				case 0:
					if(songy<=songlen-2){
						if(songy==songoffs)
							songy++;
						songoffs++;
					}
					break;
				case 1:
					if(tracky<=tracklen-2){
						if(tracky==trackoffs)
							tracky++;
						trackoffs++;
					}
					break;
				case 2:
					if(instry<=instrument[currinstr].length-2){
						if(instry==instroffs)
							instry++;
						instroffs++;
					}
					break;
			}
			break;
		case 'H':
			switch(currtab){
				case 0:
					songy = songoffs;
					break;
				case 1:
					tracky = trackoffs;
					break;
				case 2:
					instry = instroffs;
					break;
			}
			break;

		// the second cases (to the right of the colon) for M and L
		// took some serious guesswork, so I'm not sure if they're
		// correct but they seem to work.
		case 'M':
			switch(currtab){
				case 0:
					songy = (songlen <= getmaxy(stdscr)-2)?
							songlen/2
							: ((getmaxy(stdscr)-6)/2) + songoffs;
					break;
				case 1:
					tracky = (tracklen <= getmaxy(stdscr)-2)?
							tracklen/2
							: ((getmaxy(stdscr)-6)/2) + trackoffs;
					break;
				case 2:
					instry = (instrument[currinstr].length <= getmaxy(stdscr)-2)?
							instrument[currinstr].length/2
							: ((getmaxy(stdscr)-6)/2) + instroffs;
					break;
			}
			break;
		case 'L':
			switch(currtab){
				case 0:
					songy = (songlen <= getmaxy(stdscr)-2)?
							songlen-1
							: getmaxy(stdscr)-3+songoffs;
					break;
				case 1:
					tracky = (tracklen <= getmaxy(stdscr)-2)?
							tracklen-1
							: getmaxy(stdscr)-3+trackoffs;
					break;
				case 2:
					instry = (instrument[currinstr].length <= getmaxy(stdscr)-2)?
							instrument[currinstr].length-1
							: getmaxy(stdscr)-3+instroffs;
					break;
			}
			break;
		case 'g':
			if(nextchar() == 'g'){
				switch(currtab){
					case 0:
						songy = 0;
						break;
					case 1:
						tracky = 0;
						break;
					case 2:
						instry = 0;
						break;
				}
			}
			break;
		case 'G':
			switch(currtab){
				case 0:
					songy = songlen - 1;
					break;
				case 1:
					tracky = tracklen - 1;
					break;
				case 2:
					instry = instrument[currinstr].length - 1;
					break;
			}
			break;

		// yank
		case 'y':
			c = nextchar();
			switch(c){
				case 'y':
					//tclip = malloc(1);
					if(currtab == 0){
						tcliplen = 1;
						memcpy(&tclip, &song[songy], sizeof(struct songline));
					}else if(currtab == 1){
						tcliplen = 1;
						memcpy(&tclip, &track[currtrack].line[tracky], sizeof(struct trackline));
					}else if(currtab == 2){
						icliplen = 1;
						memcpy(&iclip, &instrument[currinstr].line[instry], sizeof(struct instrline));
					}
					break;
				case 'j':
					//tclip = malloc(2);
					if(currtab == 0){
						tcliplen = 2;
						memcpy(&tclip[0], &song[songy], sizeof(struct songline));
						act_mvdown();
						memcpy(&tclip[1], &song[songy], sizeof(struct songline));
					}else if(currtab == 1){
						tcliplen = 2;
						memcpy(&tclip[0], &track[currtrack].line[tracky], sizeof(struct trackline));
						act_mvdown();
						memcpy(&tclip[1], &track[currtrack].line[tracky], sizeof(struct trackline));
					}else if(currtab == 2){
						icliplen = 2;
						memcpy(&iclip[0], &instrument[currinstr].line[instry], sizeof(struct instrline));
						act_mvdown();
						memcpy(&iclip[1], &instrument[currinstr].line[instry], sizeof(struct instrline));
					}
					break;
				case 'k':
					//tclip = malloc(2);
					if(currtab == 0){
						tcliplen = 2;
						memcpy(&tclip[1], &song[songy], sizeof(struct songline));
						act_mvup();
						memcpy(&tclip[0], &song[songy], sizeof(struct songline));
					}else if(currtab == 1){
						tcliplen = 2;
						memcpy(&tclip[1], &track[currtrack].line[tracky], sizeof(struct trackline));
						act_mvup();
						memcpy(&tclip[0], &track[currtrack].line[tracky], sizeof(struct trackline));
					}else if(currtab == 2){
						icliplen = 2;
						memcpy(&iclip[1], &instrument[currinstr].line[instry], sizeof(struct instrline));
						act_mvup();
						memcpy(&iclip[0], &instrument[currinstr].line[instry], sizeof(struct instrline));
					}
					break;
			}
			break;

		//paste
		case 'p':
			if(currtab == 0){
				if(songlen < 256){
					// insert new line
					memmove(&song[songy + 2], &song[songy + 1], sizeof(struct songline) * (songlen - songy - 1));
					songy++;
					songlen++;
					memset(&song[songy], 0, sizeof(struct songline));

					// paste to new line
					memcpy(&song[songy], &tclip, sizeof(struct songline));
				}
			}else if(currtab == 1){
					for(int i = 0; i < tcliplen; i++){
						memcpy(&track[currtrack].line[tracky], &tclip[i], sizeof(struct trackline));
						if(tracky < tracklen-step) tracky += step;
						else tracky = tracklen-1;
					}
			}else if(currtab == 2){
				if(instrument[currinstr].length < 256){
					// insert new line
					struct instrument *in = &instrument[currinstr];

					instry++;
					memmove(&in->line[instry + 1], &in->line[instry + 0], sizeof(struct instrline) * (in->length - instry));
					in->length++;
					in->line[instry].cmd = '0';
					in->line[instry].param = 0;

					// paste to new line
					memcpy(&instrument[currinstr].line[instry], &iclip, sizeof(struct instrline));
				}
				//if(instry < instrument[currinstr].length-1) instry++;
			}
			break;

		// copy everything in the current phrase or instrument into the next free one
		case '^':
			if(currtab == 1){
				f = _nextfreetrack();
				memcpy(&track[f], &track[currtrack], sizeof(struct track));
				currtrack = f;
			}else if(currtab == 2){
				f = _nextfreeinstr();
				memcpy(&instrument[f], &instrument[currinstr], sizeof(struct instrument));
				currinstr = f;
			}
			break;

		// TODO: Y and P can be removed after we make visual mode
		// copy whole phrase or instrument
		case 'Y':
			if(currtab == 1){
				memcpy(&tclip, &track[currtrack], sizeof(struct track));
			}else if(currtab == 2){
				memcpy(&iclip, &instrument[currinstr], sizeof(struct instrument));
			}
			break;
		// paste whole phrase or instrument
		case 'P':
			if(currtab == 1){
				memcpy(&track[currtrack], &tclip, sizeof(struct track));
			}else if(currtab == 2){
				memcpy(&instrument[currinstr], &iclip, sizeof(struct instrument));
			}
			break;

		/* delete line */
		// TODO: clean this SHIT up
		// TODO: add an ACT_ function for delete
		case 'd':
			c = nextchar();
			switch(c){
				case 'd':
					if(currtab == 2){
						struct instrument *in = &instrument[currinstr];

						if(in->length > 1){
							memmove(&in->line[instry + 0], &in->line[instry + 1], sizeof(struct instrline) * (in->length - instry - 1));
							in->length--;
							if(instry >= in->length) instry = in->length - 1;
						}
					}else if(currtab == 0){
						if(songlen > 1){
							memmove(&song[songy + 0], &song[songy + 1], sizeof(struct songline) * (songlen - songy - 1));
							songlen--;
							if(songy >= songlen) songy = songlen - 1;
						}
					}
					break;
				case 'k':
					if(currtab == 2){
						struct instrument *in = &instrument[currinstr];
						instry--;
						int i;
						for(i=0; i<2; i++){
							if(in->length > 1){
								memmove(&in->line[instry + 0], &in->line[instry + 1], sizeof(struct instrline) * (in->length - instry - 1));
								in->length--;
								if(instry >= in->length) instry = in->length - 1;
							}
						}
					}else if(currtab == 0){
						songy--;
						int i;
						for(i=0; i<2; i++){
							if(songlen > 1){
								memmove(&song[songy + 0], &song[songy + 1], sizeof(struct songline) * (songlen - songy - 1));
								songlen--;
								if(songy >= songlen) songy = songlen - 1;
							}
						}
					}
					break;
				case 'j':
					if(currtab == 2){
						struct instrument *in = &instrument[currinstr];

						int i;
						for(i=0; i<2; i++){
							if(in->length > 1){
								memmove(&in->line[instry + 0], &in->line[instry + 1], sizeof(struct instrline) * (in->length - instry - 1));
								in->length--;
								if(instry >= in->length) instry = in->length - 1;
							}
						}
					}else if(currtab == 0){
						int i;
						for(i=0; i<2; i++){
							if(songlen > 1){
								memmove(&song[songy + 0], &song[songy + 1], sizeof(struct songline) * (songlen - songy - 1));
								songlen--;
								if(songy >= songlen) songy = songlen - 1;
							}
						}
					}
					break;
			}
			break;
		/* undo */
		case 'u':
			act_undo();
		/* Clear */
		case 'x':
			act_clronething();
			break;
		case 'X':
			act_clritall();
			break;
		case ENTER:
			if(currtab != 2){
				if(currtab == 1){
					silence();
					startplaytrack(currtrack);
				}else if(currtab == 0){
					silence();
					startplaysong(songy);
				}
			}
			break;
		case 'Z':
			c = nextchar();
			switch(c){
				case 'Z':
					savefile(filename);
					erase();
					refresh();
					endwin();
					exit(0);
					break;
				case 'Q':
					erase();
					refresh();
					endwin();
					exit(0);
					break;
			}
			break;
		/* Enter command mode */
		case ':':
			cmdlinemode();
			break;
		case ' ':
			silence();
			break;
		// TODO: make an act_ function for '`'
		case '`':
			if(currtab == 0){
				int t = song[songy].track[songx / 4];
				if(t) currtrack = t;
				currtab = 1;
				if(playtrack){
					startplaytrack(currtrack);
				}
			}else if((currtab == 1) && ((trackx == 2) || (trackx == 3))){
				int i = track[currtrack].line[tracky].instr;
				if(i) currinstr = i;
				currtab = 2;
			}	else if(currtab == 1){
				currtab = 0;
			}else if(currtab == 2){
				currtab = 1;
			}
			break;
		/* Enter insert mode */
		case 'i':
			insertmode();
			break;
		/* Enter visual mode */
		case 'v':
			visualmode();
			break;
		/* Enter visual line mode */
		case 'V':
			visuallinemode();
			break;
		/* enter jammer mode */
		case CTRL('A'):
			jammermode();
			break;
		/* Add new line and enter insert mode */
		case 'o':
			if(currtab == 2){
				struct instrument *in = &instrument[currinstr];

				if(in->length < 256){
					memmove(&in->line[instry + 2], &in->line[instry + 1], sizeof(struct instrline) * (in->length - instry - 1));
					instry++;
					in->length++;
					in->line[instry].cmd = '0';
					in->line[instry].param = 0;
				}
			}else if(currtab == 0){
				if(songlen < 256){
					memmove(&song[songy + 2], &song[songy + 1], sizeof(struct songline) * (songlen - songy - 1));
					songy++;
					songlen++;
					memset(&song[songy], 0, sizeof(struct songline));
				}
			}
			insertmode();
			break;
		case 'h':
		case KEY_LEFT:
			act_mvleft();
			break;
		case 'j':
		case KEY_DOWN:
			act_mvdown();
			break;
		case 'k':
		case KEY_UP:
			act_mvup();
			break;
		case 'l':
		case KEY_RIGHT:
			act_mvright();
			break;
		case '<':
			if(octave) octave--;
			break;
		case '>':
			if(octave < 8) octave++;
			break;
		case '{':
			if(currtrack > 1) currtrack--;
			break;
		case '}':
			if(currtrack < 255) currtrack++;
			break;
		case 'J':
			if(currtab == 0){
				if( (songx%4) < 2){
					act_trackdec();
				}else{
					act_transpdec();
				}
			}else if(currtab == 1){
				switch(trackx){
					case 0:
						act_notedec();
						break;
					case 1:
						act_octavedec();
						break;
					case 2:
						act_instrdec();
						break;
					case 3:
						act_instrdec();
						break;
					case 4:
						act_fxdec();	
						break;
					case 5:
					case 6:
						act_paramdec();	
						break;
					case 7:
						act_fxdec();	
						break;
					case 8:
					case 9:
						act_paramdec();	
						break;
					default:
						setdisplay("in J");
						break;
					}
			}else if(currtab == 2){
				switch(instrx){
					case 0:
						act_fxdec();	
						break;
					case 1:
						if(instrument[currinstr].line[instry].cmd == '+' || instrument[currinstr].line[instry].cmd == '='){
							act_notedec();
						}else{
							act_paramdec();	
						}
						break;
					case 2:
						if(instrument[currinstr].line[instry].cmd == '+' || instrument[currinstr].line[instry].cmd == '='){
							act_notedec();
						}else{
							act_paramdec();	
						}
						break;
				}
			}
			break;
		case 'K':
			if(currtab == 0){
				if( (songx%4) < 2){
					act_trackinc();
				}else{
					act_transpinc();
				}
			}else if(currtab == 1){
				switch(trackx){
					case 0:
						act_noteinc();
						break;
					case 1:
						act_octaveinc();	
						break;
					case 2:
						act_instrinc();
						break;
					case 3:
						act_instrinc();
						break;
					case 4:
						act_fxinc();	
						break;
					case 5:
					case 6:
						act_paraminc();	
						break;
					case 7:
						act_fxinc();	
						break;
					case 8:
					case 9:
						act_paraminc();	
						break;
					default:
						setdisplay("in K");
						break;
				}
			}else if(currtab == 2){
				switch(instrx){
					case 0:
						act_fxinc();	
						break;
					case 1:
						if(instrument[currinstr].line[instry].cmd == '+' || instrument[currinstr].line[instry].cmd == '='){
							act_noteinc();
						}else{
							act_paraminc();	
						}
						break;
					case 2:
						if(instrument[currinstr].line[instry].cmd == '+' || instrument[currinstr].line[instry].cmd == '='){
							act_noteinc();
						}else{
							act_paraminc();	
						}
						break;
				}
			}
			break;
		case CTRL('J'):
			if(currtab == 2){
				act_viewinstrdec();
			}else if(currtab == 1){
				act_viewtrackdec();
			}
			break;
		case CTRL('K'):
			if(currtab == 2){
				act_viewinstrinc();
			}else if(currtab == 1){
				act_viewtrackinc();
			}
			break;
		case '[':
			act_viewinstrdec();
			break;
		case ']':
			act_viewinstrinc();
			break;
		case '(':
			callbacktime++;
			break;
		case ')':
			callbacktime--;
			break;
		case '-':
			if(step > 0) 
			  step--;
			break;
		case '=':
			if(step < 0x0f) 
			  step++;
			break;
		case CTRL('H'):
			currtab--;
			if(currtab < 0)
				currtab = 2;
			break;
		case CTRL('L'):
			currtab++;
			currtab %= 3;
			break;
		case KEY_TAB:
			currtab++;
			currtab %= 3;
			break;
		case CTRL('U'):
			act_bigmvup();
			break;
		case CTRL('D'):
			act_bigmvdown();
			break;
		/*case CTRL('P'):
			vimode = false;
			break;*/

		// replace
		case 'r':
			_insertc(nextchar());
			break;

		default:
			break;
		} // end switch
	} // end for
	cmdrepeatnum = 1;
	cmdrepeat = 0;
}

/* vi cmdline mode */
void cmdlinemode(void){
	u16 c;
	keypad(stdscr, TRUE);

	currmode = PM_CMDLINE;
	strncat(cmdstr, ":", 50);
	for(;;){
		drawgui();

		c = nextchar();
		switch(c){
			case KEY_ESCAPE:
				//cmdstr = "";
				currmode = PM_NORMAL;
				goto end;
			case ENTER:
				_parsecmd(cmdstr);
				goto end;
#ifndef WINDOWS
			case BACKSPACE:
				setdisplay("\\o/");
				cmdstr[strlen(cmdstr)-1] = '\0';
				break;
#endif
			case '\t':
				break;
			default:
				strncat(cmdstr, &c, 50);
				break;
		}
	}
end:
	strcpy(cmdstr, "");
	keypad(stdscr, FALSE);
	return;
}

/* vi insert mode */
void insertmode(void){
	int c;
	currmode = PM_INSERT;
	drawgui();
	for(;;){
		if((c = getch()) != ERR) switch(c){
			case KEY_ESCAPE:
				currmode = PM_NORMAL;
				guiloop();
			case 'h':
			case KEY_LEFT:
				act_mvleft();
				break;
			case 'j':
			case KEY_DOWN:
				act_mvdown();
				break;
			case 'k':
			case KEY_UP:
				act_mvup();
				break;
			case 'l':
			case KEY_RIGHT:
				act_mvright();
				break;
			/* change octave */
			case '<':
				if(octave) octave--;
				break;
			case '>':
				if(octave < 8) octave++;
				break;
			/* change instrument */
			case CTRL('J'):
				if(currtab == 2){
					act_viewinstrdec();
				}else if(currtab == 1){
					act_viewtrackdec();
				}
				break;
			case CTRL('K'):
				if(currtab == 2){
					act_viewinstrinc();
				}else if(currtab == 1){
					act_viewtrackinc();
				}
				break;
			case '[':
				act_viewinstrdec();
				break;
			case ']':
				act_viewinstrinc();
				break;
			case CTRL('H'):
				currtab--;
				if(currtab < 0)
					currtab = 2;
				break;
			case CTRL('L'):
				currtab++;
				currtab %= 3;
				break;
			case 'Z':
				c = nextchar();
				switch(c){
					case 'Z':
						savefile(filename);
						erase();
						refresh();
						endwin();
						exit(0);
						break;
					case 'Q':
						erase();
						refresh();
						endwin();
						exit(0);
						break;
				}
				break;
			case ' ':
				silence();
				currmode = PM_NORMAL;
				guiloop();
				break;
			case ENTER:
				if(currtab != 2){
					if(currtab == 1){
						silence();
						startplaytrack(currtrack);
					}else if(currtab == 0){
						silence();
						startplaysong(songy);
					}
				}
				break;
			case '`':
				if(currtab == 0){
					int t = song[songy].track[songx / 4];
					if(t) currtrack = t;
					currtab = 1;
				}else if(currtab == 1){
					currtab = 0;
				}
				break;
			default:
				_insertc(c);
				if(currtab == 1){
					tracky+=step;
					tracky %= tracklen;
				}else if(currtab == 2){
					//if(instry < instrument[currinstr].length-1) instry++;
					if(instrx < 2) instrx++;
					else instrx--;
					instry %= instrument[currinstr].length;
				}
				saved = 0;
		}
		drawgui();
		usleep(10000);
	}
}

/* jammer mode */
void jammermode(void){
	int c, x;
	currmode = PM_JAMMER;
	while(currmode == PM_JAMMER){
		if((c = getch()) != ERR) switch(c){
			case KEY_ESCAPE:
				currmode = PM_NORMAL;
				break;
			case '[':
				act_viewinstrdec();
				break;
			case ']':
				act_viewinstrinc();
				break;
			case '<':
				if(octave) octave--;
				break;
			case '>':
				if(octave < 8) octave++;
				break;
			default:
				x = freqkey(c);

				if(x > 0){
					iedplonk(x, currinstr);
				}

				break;
		}
		drawgui();
		usleep(10000);
	}
}

/* visual mode */
void visualmode(void){
	int c;

	currmode = PM_VISUAL;
	attrset(A_REVERSE);
	while(currmode == PM_VISUAL){
		if((c = getch()) != ERR) switch(c){
			case 'v':
			case KEY_ESCAPE:
				currmode = PM_NORMAL;
				break;
			case 'V':
				visuallinemode();
				break;
			case 'h':
				act_mvleft();
				break;
			case 'j':
				act_mvdown();
				break;
			case 'k':
				act_mvup();
				break;
			case 'l':
				act_mvright();
				break;
		}
		drawgui();
	}
	attrset(A_BOLD);
	return;
}

/* visual line mode */
void visuallinemode(void){
	int c;
	int min, max;
	char buf[1024];
	//NODE *firstnode, *lastnode;

	currmode = PM_VISUALLINE;

	/* Store the current line as the first and last node of a linked list */
	if(currtab==0){
		//firstnode = list_create((void *) songy);
		//lastnode = list_insertafter(firstnode, (void *)songy);
		highlight_firstline = songy;
		highlight_lastline = songy;
	}else if(currtab==1){
		//firstnode = list_create((void *) tracky);
		//lastnode = list_insertafter(firstnode, (void *)tracky);
		highlight_firstline = tracky;
		highlight_lastline = tracky;
	}else if(currtab==2){
		//firstnode = list_create((void *) instry);
		//lastnode = list_insertafter(firstnode, (void *)instry);
		highlight_firstline = instry;
		highlight_lastline = instry;
	}else{
		//firstnode = NULL;
		//lastnode = NULL;
		highlight_firstline = -1;
		highlight_lastline = -1;
	}

	// initialize difference
	highlight_lineamount = 1;

	// make it visible to gui.c
	//highlightlines = firstnode;

	while(currmode == PM_VISUALLINE){
		if((c = getch()) != ERR) switch(c){
			case 'V':
			case KEY_ESCAPE:
				currmode = PM_NORMAL;
				break;
			case 'v':
				visualmode();
			case 'h':
				act_mvleft();
				break;
			case 'j':
				act_mvdown();
				// update lastnode
				if(currtab==0){
					//lastnode = list_insertafter(firstnode, (void *)songy);
					highlight_lastline = songy;
				}else if(currtab==1){
					//lastnode = list_insertafter(firstnode, (void *)tracky);
					highlight_lastline = tracky;
				}else if(currtab==2){
					//lastnode = list_insertafter(firstnode, (void *)instry);
					highlight_lastline = instry;
				}
				break;
			case 'k':
				act_mvup();
				// update lastnode
				if(currtab==0){
					//lastnode = list_insertafter(firstnode, (void *)songy);
					highlight_lastline = songy;
				}else if(currtab==1){
					//lastnode = list_insertafter(firstnode, (void *)tracky);
					highlight_lastline = tracky;
				}else if(currtab==2){
					//lastnode = list_insertafter(firstnode, (void *)instry);
					highlight_lastline = instry;
				}
				break;
			case 'l':
				act_mvright();
				break;
			// d: copy every line that is highlighted to the paste buffer and clear them, too
			case 'd':
				min = (highlight_firstline < highlight_lastline)?
						highlight_firstline
						: highlight_lastline;
				max = (highlight_firstline < highlight_lastline)?
						highlight_lastline
						: highlight_firstline;
				if(currtab == 0){
					for(int i=min; i<=max; i++)
						act_clrinsongtab(i);
				}else if(currtab == 1){
					for(int i=min; i<=max; i++)
						act_clrintracktab(currtrack, i);
				}else if(currtab == 2){
					for(int i=min; i<=max; i++)
						act_clrininstrtab(currinstr, i);
				}
				//snprintf(buf, sizeof(buf), "%d fewer lines", highlight_lineamount);
				//infinitemsg = buf;
				currmode = PM_NORMAL;
				break;
			// y: copy every line that is highlighted to the paste buffer
			case 'y':
				if(currtab == 0){
					tcliplen = 1;
					memcpy(&tclip, &song[songy], sizeof(struct songline)*highlight_lineamount);
				}else if(currtab == 1){
					tcliplen = 1;
					memcpy(&tclip, &track[currtrack].line[tracky], sizeof(struct trackline)*highlight_lineamount);
				}else if(currtab == 2){
					icliplen = 1;
					memcpy(&iclip, &instrument[currinstr].line[instry], sizeof(struct instrline)*highlight_lineamount);
				}

				snprintf(buf, sizeof(buf), "%d lines yanked", highlight_lineamount);
				infinitemsg = buf;
				currmode = PM_NORMAL;
				break;
		}
		drawgui();

		// update the highlighted length
		highlight_lineamount = (highlight_firstline>highlight_lastline)?
				highlight_firstline - highlight_lastline +1
				: highlight_lastline - highlight_firstline +1;
	}
	highlight_firstline = -1;
	highlight_lastline = -1;

	return;
}

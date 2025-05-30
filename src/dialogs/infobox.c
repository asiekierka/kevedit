/* infobox.c - board/world information dialogs
 * $Id: infobox.c,v 1.2 2005/05/28 03:17:45 bitman Exp $
 * Copyright (C) 2000 Ryan Phillips <bitman@scn.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place Suite 330; Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "infobox.h"

#include "kevedit/screen.h"

#include "libzzt2/zzt.h"
#include "help/help.h"
#include "dialog.h"
#include "texteditor/editbox.h"
#include "themes/theme.h"

#include "display/display.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Label colorings */
#define LABEL_COLOR  0x0A
#define OPTION_COLOR 0x0B

/* addBoundedDelta()
 * Adds delta to value without going over max or under zero */
#define addBoundedDelta(value, delta, max) if ((value) < -(delta)) (value) = 0; else if ((value) <= (max) && (value) > (max) - (delta)) (value) = (max); else (value) += (delta);

/*************** Board Info *******************/

/* Types of board info */
#define ID_NONE          0
#define BRDINFO_TITLE    1
#define BRDINFO_MESSAGE  2

#define BRDINFO_DARKNESS 3
#define BRDINFO_REENTER  4
#define BRDINFO_REENTERX 5
#define BRDINFO_REENTERY 6
#define BRDINFO_TIMELIM  7
#define BRDINFO_MAXSHOTS 8

#define BRDINFO_BRDNORTH 9
#define BRDINFO_BRDSOUTH 10
#define BRDINFO_BRDEAST  11
#define BRDINFO_BRDWEST  12

#define UPDATE_NONE 0
#define UPDATE_DIALOG 1
#define UPDATE_SCREEN 2
#define UPDATE_QUIT 3

dialog buildboardinfodialog(ZZTworld * myworld);
int boardinfoeditoption(displaymethod * d, ZZTworld* myworld, dialogComponent* opt);
int boardinfodeltaoption(displaymethod * d, ZZTworld* myworld, dialogComponent* opt, int delta);
int boardinfostaroption(ZZTworld* myworld, dialogComponent* opt);
int boardinfodeleteoption(ZZTworld* myworld, dialogComponent* opt);

int editboardinfo(ZZTworld* myworld, displaymethod* d)
{
	dialog dia;
	int key;

	/* Build the dialog */
	dia = buildboardinfodialog(myworld);

	/* Draw the side panel */
	drawsidepanel(d, PANEL_BOARDINFO);

	do {
		int rebuild = 0;
		/* Draw the dialog each time around */
		dialogDraw(d, dia);

		/* Get the key */
		key = d->getch();

		switch (key) {
			case DKEY_DOWN: dialogNextOption(&dia); break;
			case DKEY_UP:   dialogPrevOption(&dia); break;

			case DKEY_ENTER:
				rebuild = boardinfoeditoption(d, myworld, dialogGetCurOption(dia));
				break;

			case DKEY_LEFT:
				rebuild = boardinfodeltaoption(d, myworld, dialogGetCurOption(dia), -10);
				break;

			case DKEY_RIGHT:
				rebuild = boardinfodeltaoption(d, myworld, dialogGetCurOption(dia), 10);
				break;

			case '-':
				rebuild = boardinfodeltaoption(d, myworld, dialogGetCurOption(dia), -1);
				break;

			case '+':
				rebuild = boardinfodeltaoption(d, myworld, dialogGetCurOption(dia), 1);
				break;

			case '*':
				rebuild = boardinfostaroption(myworld, dialogGetCurOption(dia));
				break;

			case DKEY_BACKSPACE:
			case DKEY_DELETE:
				rebuild = boardinfodeleteoption(myworld, dialogGetCurOption(dia));
				break;

			case DKEY_F1:
				helpsectiontopic("kbrdinfo", NULL, d);
				/* Draw the side panel again */
				rebuild = 2;
				break;
		}

		if (rebuild == UPDATE_QUIT) {
			key = DKEY_QUIT;
			break;
		}

		/* Rebuild of 2 means update panel as well as dialog */
		if (rebuild == UPDATE_SCREEN)
			drawsidepanel(d, PANEL_BOARDINFO);

		if (rebuild) {
			/* Rebuild dialog */
			int curoption = dia.curoption;
			dialogFree(&dia);
			dia = buildboardinfodialog(myworld);
			dia.curoption = curoption;
		}
	} while (key != DKEY_ESC && key != DKEY_QUIT);

	dialogFree(&dia);
	return key;
}

dialog buildboardinfodialog(ZZTworld * myworld)
{
	char buffer[24];   /* Number to string buffer */
	int curboard = zztBoardGetCurrent(myworld);
	uint32_t boardsize = zztBoardGetSize(zztBoardGetCurPtr(myworld));
	dialog dia;

	dialogComponent label  = dialogComponentMake(DIALOG_COMP_LABEL,   3, 2, LABEL_COLOR,  NULL, ID_NONE);
	dialogComponent option = dialogComponentMake(DIALOG_COMP_OPTION, 25, 2, OPTION_COLOR, NULL, ID_NONE);

	/* Handy macros for using template label & option */
#define _addlabel(TEXT)      { label.text  = (TEXT); dialogAddComponent(&dia, label); label.y++; }
#define _addoption(TEXT, ID) { option.text = (TEXT); option.id = (ID); dialogAddComponent(&dia, option); option.y++; }

	/* Initialize */
	dialogInit(&dia);

	/* Dialog title */
	dialogAddComponent(&dia, dialogComponentMake(DIALOG_COMP_TITLE, 0, 0, 0x0F, "Board Info", ID_NONE));

	/* Board title */
	dialogAddComponent(&dia, dialogComponentMake(DIALOG_COMP_OPTION,
				21 - (strlen((char *)zztBoardGetTitle(myworld)) / 2),
				0, OPTION_COLOR, (char *)zztBoardGetTitle(myworld), BRDINFO_TITLE));

	/* Basic board info */
	_addlabel("       Board is dark:");
	_addlabel("Re-Enter When Zapped:");
	_addlabel("          Re-Enter X:");
	_addlabel("          Re-Enter Y:");
	_addlabel("          Time Limit:");
	_addlabel("       Maximum Shots:");

	_addoption(zztBoardGetDarkness(myworld) ? "Yes" : "No", BRDINFO_DARKNESS);
	_addoption(zztBoardGetReenter(myworld)  ? "Yes" : "No", BRDINFO_REENTER);

	/* Numerical */

	/* Re-enter x/y */
	sprintf(buffer, "%d", zztBoardGetReenter_x(myworld) + 1);
	_addoption(buffer, BRDINFO_REENTERX);
	sprintf(buffer, "%d", zztBoardGetReenter_y(myworld) + 1);
	_addoption(buffer, BRDINFO_REENTERY);

	/* Time Limit */
	if (zztBoardGetTimelimit(myworld) > 0)
		sprintf(buffer, "%d", zztBoardGetTimelimit(myworld));
	else
		strcpy(buffer, "Infinite");
	_addoption(buffer, BRDINFO_TIMELIM);

	/* Maximum shots */
	sprintf(buffer, "%d", zztBoardGetMaxshots(myworld));
	_addoption(buffer, BRDINFO_MAXSHOTS);

	/* Message */
	/* Don't bother trying to edit this. A special stat has to be created
	 * at position (0,0) which stores the remaining message time in
	 * param[1], of all places. If said stat is missing, no message is
	 * displayed. TODO: support this automatically in libzzt2. */
	label.x = 0;
	_addlabel((char *)zztBoardGetMessage(myworld));

	/* Advance template cursors */
	label.y += 1; option.y += 2;
	label.x = 2; option.x = 5;

	/* Board links */

	dialogAddComponent(&dia, dialogComponentMake(DIALOG_COMP_HEADING, 0, label.y - 1, 0x0F, "Adjacent Boards", ID_NONE));

	_addlabel("\x18:");
	_addlabel("\x19:");
	_addlabel("\x1A:");
	_addlabel("\x1B:");

#define _addstar() dialogAddComponent(&dia, dialogComponentMake(DIALOG_COMP_LABEL, 0, option.y - 1, OPTION_COLOR, "*", ID_NONE))

	/* North */
	if (zztBoardGetBoard_n(myworld) > 0) {
		_addoption((char *)myworld->boards[zztBoardGetBoard_n(myworld)].title, BRDINFO_BRDNORTH);
		if (curboard > 0 && myworld->boards[zztBoardGetBoard_n(myworld)].info.board_s == curboard)
			_addstar();
	} else {
		_addoption("(None)", BRDINFO_BRDNORTH);
	}

	/* South */
	if (zztBoardGetBoard_s(myworld) > 0) {
		_addoption((char *)myworld->boards[zztBoardGetBoard_s(myworld)].title, BRDINFO_BRDSOUTH);
		if (curboard > 0 && myworld->boards[zztBoardGetBoard_s(myworld)].info.board_n == curboard)
			_addstar();
	} else {
		_addoption("(None)", BRDINFO_BRDSOUTH);
	}

	/* East */
	if (zztBoardGetBoard_e(myworld) > 0) {
		_addoption((char *)myworld->boards[zztBoardGetBoard_e(myworld)].title, BRDINFO_BRDEAST);
		if (curboard > 0 && myworld->boards[zztBoardGetBoard_e(myworld)].info.board_w == curboard)
			_addstar();
	} else {
		_addoption("(None)", BRDINFO_BRDEAST);
	}

	/* West */
	if (zztBoardGetBoard_w(myworld) > 0) {
		_addoption((char *)myworld->boards[zztBoardGetBoard_w(myworld)].title, BRDINFO_BRDWEST);
		if (curboard > 0 && myworld->boards[zztBoardGetBoard_w(myworld)].info.board_e == curboard)
			_addstar();
	} else {
		_addoption("(None)", BRDINFO_BRDWEST);
	}

	/* Board size statistics */

	label.x = 4; _addlabel("Board Size:");
	/* Red color for dangerously large board size */
	label.color = (boardsize <= ZZT_BOARD_MAX_FILESIZE ? 0x0B : 0x0C);
	label.y--; label.x = 16;

	sprintf(buffer, "%d bytes, %.3f KB", boardsize, (float)boardsize / 1024);
	_addlabel(buffer);

	return dia;
}

int boardinfoeditoption(displaymethod * d, ZZTworld* myworld, dialogComponent* opt)
{
	int edit_result = -1;
	switch (opt->id) {
		case BRDINFO_TITLE:
			/* Change the title of the board */
			/* Board titles can actually be up to 50 chars long,
			 * but this would display poorly in any editor, so we'll
			 * use the width of a text-box (42) as the maximum. */
			opt->x = 0; opt->color = 0x0F;
			edit_result = dialogComponentEdit(d, opt, 42, LINED_NORMAL);
			if(edit_result == LINED_OK) {
				zztBoardSetTitle(myworld, opt->text);
				return UPDATE_DIALOG;
			}
			break;

		case BRDINFO_MESSAGE:
			/* 58 chars is the actual limit, but 42 is enough */
			edit_result = dialogComponentEdit(d, opt, 42, LINED_NORMAL);
			if(edit_result == LINED_OK) {
				zztBoardSetMessage(myworld, opt->text);
				return UPDATE_DIALOG;
			}
			break;

		case BRDINFO_BRDNORTH: {
			int board = boarddialog(myworld, zztBoardGetBoard_n(myworld), "Board to the North", BOARDDIALOG_FIRST_NONE, d);
			if(board == DKEY_QUIT) {
				return UPDATE_QUIT;
			} else {
				zztBoardSetBoard_n(myworld, board);
				return UPDATE_SCREEN;
			}
		}

		case BRDINFO_BRDSOUTH:{
			int board = boarddialog(myworld, zztBoardGetBoard_s(myworld), "Board to the South", BOARDDIALOG_FIRST_NONE, d);
			if(board == DKEY_QUIT) {
				return UPDATE_QUIT;
			} else {
				zztBoardSetBoard_s(myworld, board);
				return UPDATE_SCREEN;
			}
		}

		case BRDINFO_BRDEAST:{
			int board = boarddialog(myworld, zztBoardGetBoard_e(myworld), "Board to the East", BOARDDIALOG_FIRST_NONE, d);
			if(board == DKEY_QUIT) {
				return UPDATE_QUIT;
			} else {
				zztBoardSetBoard_e(myworld, board);
				return UPDATE_SCREEN;
			}
		}

		case BRDINFO_BRDWEST:{
			int board = boarddialog(myworld, zztBoardGetBoard_w(myworld), "Board to the West", BOARDDIALOG_FIRST_NONE, d);
			if(board == DKEY_QUIT) {
				return UPDATE_QUIT;
			} else {
				zztBoardSetBoard_w(myworld, board);
				return UPDATE_SCREEN;
			}
		}

		case BRDINFO_DARKNESS:
			zztBoardSetDarkness(myworld, !zztBoardGetDarkness(myworld));
			return UPDATE_DIALOG;

		case BRDINFO_REENTER:
			zztBoardSetReenter(myworld, !zztBoardGetReenter(myworld));
			return UPDATE_DIALOG;

		case BRDINFO_REENTERX:
		case BRDINFO_REENTERY:
		case BRDINFO_MAXSHOTS:
			if (opt->text[0] == '0') /* If the number is zero */
				opt->text[0] = '\x0';  /* Clear the text */
            edit_result = dialogComponentEdit(d, opt, 3, LINED_NUMBER);
			if (edit_result == LINED_OK) {
				int number;
				sscanf(opt->text, "%d", &number);
				if (strlen(opt->text) == 0)
					number = 0;
				else if (number > 255)
					number = 255;

				switch (opt->id) {
					case BRDINFO_REENTERX: zztBoardSetReenter_x(myworld, number-1); break;
					case BRDINFO_REENTERY: zztBoardSetReenter_y(myworld, number-1); break;
					case BRDINFO_MAXSHOTS: zztBoardSetMaxshots (myworld, number); break;
				}
                return UPDATE_DIALOG;
			}
            break;

		case BRDINFO_TIMELIM:
			if (zztBoardGetTimelimit(myworld) == 0) {
				opt->text[0] = '\x0';
				/* Clear the word "Infinite" */
				d->print(opt->x + 9, opt->y + 6, 0x00, "          ");
			}

			edit_result = dialogComponentEdit(d, opt, 5, LINED_NUMBER);
            if (edit_result == LINED_OK) {
				long int timelimit;
				sscanf(opt->text, "%ld", &timelimit);
				if (strlen(opt->text) == 0)
					zztBoardSetTimelimit(myworld, 0);
				else if (timelimit > 32767)
					zztBoardSetTimelimit(myworld, 32767);
				else
					zztBoardSetTimelimit(myworld, timelimit);
                return UPDATE_DIALOG;
			}
            break;
	}
	if(edit_result == -1)
		return UPDATE_NONE;
	if(edit_result == LINED_QUIT)
		return UPDATE_QUIT;
	return UPDATE_DIALOG;
}

int boardinfodeltaoption(displaymethod * d, ZZTworld* myworld, dialogComponent* opt, int delta)
{
	/* It's easier this way */
	ZZTboardinfo* info = &(myworld->boards[zztBoardGetCurrent(myworld)].info);

	switch (opt->id) {
		case BRDINFO_TIMELIM:
			addBoundedDelta(info->timelimit, delta, 32767);
			return 1;

		case BRDINFO_MAXSHOTS:
			addBoundedDelta(info->maxshots, delta, 255);
			return 1;

		case BRDINFO_BRDNORTH:
			if (info->board_n + delta >= zztWorldGetBoardcount(myworld) ) {
				info->board_n = zztWorldGetBoardcount(myworld) - 1;
			}
			else if( info->board_n + delta < 0) {
				info->board_n = 0;
			}
			else {
				info->board_n += delta;
			}
			return 1;

		case BRDINFO_BRDSOUTH:
			if (info->board_s + delta >= zztWorldGetBoardcount(myworld) ) {
				info->board_s = zztWorldGetBoardcount(myworld) - 1;
			}
			else if( info->board_s + delta < 0) {
				info->board_s = 0;
			}
			else {
				info->board_s += delta;
			}
			return 1;

		case BRDINFO_BRDEAST:
			if (info->board_e + delta >= zztWorldGetBoardcount(myworld) ) {
				info->board_e = zztWorldGetBoardcount(myworld) - 1;
			}
			else if( info->board_e + delta < 0) {
				info->board_e = 0;
			}
			else {
				info->board_e += delta;
			}
			return 1;

		case BRDINFO_BRDWEST:
			if (info->board_w + delta >= zztWorldGetBoardcount(myworld) ) {
				info->board_w = zztWorldGetBoardcount(myworld) - 1;
			}
			else if( info->board_w + delta < 0) {
				info->board_w = 0;
			}
			else {
				info->board_w += delta;
			}
			return 1;

		default:
			return boardinfoeditoption(d, myworld, opt);
	}
}

int boardinfostaroption(ZZTworld* myworld, dialogComponent* opt)
{
	int curboard = zztBoardGetCurrent(myworld);
	ZZTboardinfo* info = &(myworld->boards[zztBoardGetCurrent(myworld)].info);

	if (curboard == 0)
		return 0;

	switch (opt->id) {
		case BRDINFO_BRDNORTH:
			if (info->board_n > 0)
				myworld->boards[info->board_n].info.board_s = curboard;
			return 1;

		case BRDINFO_BRDSOUTH:
			if (info->board_s > 0)
				myworld->boards[info->board_s].info.board_n = curboard;
			return 1;

		case BRDINFO_BRDEAST:
			if (info->board_e > 0)
				myworld->boards[info->board_e].info.board_w = curboard;
			return 1;

		case BRDINFO_BRDWEST:
			if (info->board_w > 0)
				myworld->boards[info->board_w].info.board_e = curboard;
			return 1;
	}

	return 0;
}

int boardinfodeleteoption(ZZTworld* myworld, dialogComponent* opt)
{
	switch (opt->id) {
		case BRDINFO_TITLE:
			zztBoardSetTitle(myworld, "");
			return UPDATE_DIALOG;

		case BRDINFO_MESSAGE:
			zztBoardSetMessage(myworld, "");
			return UPDATE_DIALOG;

		case BRDINFO_BRDNORTH:
			zztBoardSetBoard_n(myworld, 0);
			return UPDATE_DIALOG;

		case BRDINFO_BRDSOUTH:
			zztBoardSetBoard_s(myworld, 0);
			return UPDATE_DIALOG;

		case BRDINFO_BRDEAST:
			zztBoardSetBoard_e(myworld, 0);
			return UPDATE_DIALOG;

		case BRDINFO_BRDWEST:
			zztBoardSetBoard_w(myworld, 0);
			return UPDATE_DIALOG;

		case BRDINFO_DARKNESS:
			zztBoardSetDarkness(myworld, 0);
			return UPDATE_DIALOG;

		case BRDINFO_REENTER:
			zztBoardSetReenter(myworld, 0);
			return UPDATE_DIALOG;

		case BRDINFO_REENTERX:
			zztBoardSetReenter_x(myworld, 0);
			return UPDATE_DIALOG;

		case BRDINFO_REENTERY:
			zztBoardSetReenter_y(myworld, 0);
			return UPDATE_DIALOG;

		case BRDINFO_MAXSHOTS:
			zztBoardSetMaxshots(myworld, 255);
			return UPDATE_DIALOG;

		case BRDINFO_TIMELIM:
			zztBoardSetTimelimit(myworld, 0);
			return UPDATE_DIALOG;
	}

	return UPDATE_NONE;
}

/************* World Info ******************/

/* Types of World Info */
#define WLDINFO_NAME       0
#define WLDINFO_KEYS       1
#define WLDINFO_AMMO       2
#define WLDINFO_GEMS       3
#define WLDINFO_HEALTH     4
#define WLDINFO_TORCHES    5
#define WLDINFO_SCORE      6
#define WLDINFO_TCYCLES    7
#define WLDINFO_ECYCLES    8
#define WLDINFO_TIMEPASSED 9
#define WLDINFO_ISSAVED    10
#define WLDINFO_CHARSET    11
#define WLDINFO_PALETTE    12
#define WLDINFO_FLAGS      13

void drawstaticworldinfo(displaymethod* d);
void drawworldinfo(ZZTworld* myworld, keveditor *myeditor);
int worldinfoeditoption(int curoption, ZZTworld* myworld,
		int cursorx, int cursory, keveditor *myeditor);
int worldinfodirectionoption(int curoption, ZZTworld* myworld,
		int cursorx, int cursory, int dir, keveditor *myeditor);
void worldinfoclearkeys(ZZTworld* myworld);
void worldinfotogglekey(ZZTworld* myworld, int whichkey);
int editworldflags(ZZTworld* myworld, displaymethod* d);
static int worldinfodeleteoption(int curoption, ZZTworld *myworld, keveditor *myeditor);
static void worldinfopickfile(int curoption, ZZTworld *myworld, int cursorx, int cursory,
                              keveditor *myeditor, bool *quit);

/* editworldinfo() - brings up dialog box for editing world info */
int editworldinfo(keveditor *myeditor)
{
        ZZTworld *myworld = myeditor->myworld;
        displaymethod *d = myeditor->mydisplay;

	int curoption = WLDINFO_NAME;
	int cursorx, cursory;
	int done = 0;

	drawstaticworldinfo(d);
	drawworldinfo(myworld, myeditor);

	cursorx = 31;

	int key;
	do {
		/* Position the cursors */
		cursory = curoption + 6;
		if (curoption > WLDINFO_SCORE)
			cursory++;

		if (curoption == WLDINFO_FLAGS)
			cursorx = 23;
		else if (curoption != WLDINFO_KEYS)
			/* Don't change the cursorx when messing w/ keys */
			cursorx = 31;

		/* Draw the selector */
		d->putch( 7, cursory, '\xAF', 0x02);
		d->putch(51, cursory, '\xAE', 0x02);
		d->cursorgo(cursorx, cursory);

		/* TODO: there is a better way to update the cursor */
		d->update(3, 4, 51, 19);

		/* Get the key */
		key = d->getch();

		/* Erase the selector */
		d->putch( 7, cursory, ' ', 0x00);
		d->putch(51, cursory, ' ', 0x00);

		switch (key) {
			case DKEY_UP:
				if (curoption > 0) curoption--; else curoption = WLDINFO_FLAGS;
				break;

			case DKEY_DOWN:
				if (curoption < WLDINFO_FLAGS) curoption++; else curoption = 0;
				break;

			case DKEY_ESC:
			case DKEY_QUIT:
				done = 1;
				break;
		}

		if (curoption != WLDINFO_KEYS) {
			int result = 0;
			switch (key) {
				case DKEY_ENTER:
					result = worldinfoeditoption(curoption, myworld, cursorx, cursory, myeditor);
					break;

				case DKEY_LEFT:
					result = worldinfodirectionoption(curoption, myworld, cursorx, cursory, -10, myeditor);
					break;

				case '-':
					result = worldinfodirectionoption(curoption, myworld, cursorx, cursory, -1, myeditor);
					break;

				case DKEY_RIGHT:
					result = worldinfodirectionoption(curoption, myworld, cursorx, cursory, 10, myeditor);
					break;

				case '+':
					result = worldinfodirectionoption(curoption, myworld, cursorx, cursory, 1, myeditor);
					break;

				case DKEY_F1:
					helpsectiontopic("kwldinfo", NULL, d);
					result = 1;
					break;

                                case DKEY_BACKSPACE:
                                case DKEY_DELETE:
                                        result = worldinfodeleteoption(curoption, myworld, myeditor);
                                        break;
                        }
			if(result != 0) {
				drawstaticworldinfo(d);
				drawworldinfo(myworld, myeditor);
			}
			if(result == DKEY_QUIT) {
				done = 1;
				key = DKEY_QUIT;
			}
		} else {
			/* Current option is BRDINFO_KEYS */

			switch (key) {
				case DKEY_LEFT:  if (cursorx > 31) cursorx--;
													 else cursorx = 31 + ZZT_KEY_WHITE; break;
				case DKEY_RIGHT: if (cursorx < 31 + ZZT_KEY_WHITE) cursorx++;
													 else cursorx = 31; break;
                                case DKEY_BACKSPACE:
                                case DKEY_DELETE:
					worldinfoclearkeys(myworld);
					drawstaticworldinfo(d);
					drawworldinfo(myworld, myeditor);
					break;

				case DKEY_ENTER:
				case ' ':
					worldinfotogglekey(myworld, cursorx - 31);
					drawstaticworldinfo(d);
					drawworldinfo(myworld, myeditor);
					break;
			}
		}
	} while (!done);
	return key;
}

void drawstaticworldinfo(displaymethod* d)
{
	/* Draw the side panel */
	drawsidepanel(d, PANEL_WORLDINFO);

	/* Draw the scroll box */
	drawscrollbox(d, 0, 0, 0);
	d->putch_discrete( 7, 13, ' ', 0x00);
	d->putch_discrete(51, 13, ' ', 0x00);

	/* Draw the static contents */
	d->print_discrete(25, 4, 0x0A, "World Info");

	d->print_discrete(13,  6, 0x0F, "      World Name:");

	d->print_discrete(13,  7, 0x0A, "            Keys:");
	d->print_discrete(13,  8, 0x0A, "            Ammo:");
	d->print_discrete(13,  9, 0x0A, "            Gems:");
	d->print_discrete(13, 10, 0x0A, "          Health:");
	d->print_discrete(13, 11, 0x0A, "         Torches:");
	d->print_discrete(13, 12, 0x0A, "           Score:");

	d->print_discrete(13, 14, 0x0A, "    Torch Cycles:");
	d->print_discrete(13, 15, 0x0A, "Energizer Cycles:");
	d->print_discrete(13, 16, 0x0A, "    Time Elapsed:");
	d->print_discrete(13, 17, 0x0A, "   Is Saved Game:");
        d->print_discrete(13, 18, 0x0A, "   Character Set:");
        d->print_discrete(13, 19, 0x0A, "         Palette:");
	d->print_discrete(23, 20, 0x0F, "Set/Clear Flags");
}

void drawworldinfo(ZZTworld* myworld, keveditor *myeditor)
{
        displaymethod *d = myeditor->mydisplay;
	char buffer[10];    /* Buffer for translating numbers to strings */
	int i;

	/* Start at the top */
	d->print_discrete(31,  6, 0x0B, (char *)zztWorldGetTitle(myworld));

	/* List the keys */
	for (i = ZZT_KEY_BLUE; i <= ZZT_KEY_WHITE; i++) {
		if (zztWorldGetKey(myworld, i) != 0) {
			d->putch_discrete(31 + i, 7, '\x0C', 0x08 +  i + 1);
		} else {
			d->putch_discrete(31 + i, 7, '\x0A', 0x0F + ((i + 1) << 4));
		}
	}

	/* Inventory */
	sprintf(buffer, "%d", zztWorldGetAmmo(myworld));     d->print_discrete(31,  8, 0x0B, buffer);
	sprintf(buffer, "%d", zztWorldGetGems(myworld));     d->print_discrete(31,  9, 0x0B, buffer);
	sprintf(buffer, "%d", zztWorldGetHealth(myworld));   d->print_discrete(31, 10, 0x0B, buffer);
	sprintf(buffer, "%d", zztWorldGetTorches(myworld));  d->print_discrete(31, 11, 0x0B, buffer);
	sprintf(buffer, "%d", zztWorldGetScore(myworld));    d->print_discrete(31, 12, 0x0B, buffer);

	/* Misc */
	sprintf(buffer, "%d", zztWorldGetTorchcycles(myworld));    d->print_discrete(31, 14, 0x0B, buffer);
	sprintf(buffer, "%d", zztWorldGetEnergizercycles(myworld));d->print_discrete(31, 15, 0x0B, buffer);
	sprintf(buffer, "%d", zztWorldGetTimepassed(myworld));     d->print_discrete(31, 16, 0x0B, buffer);

	/* Saved Game boolean */
	d->print_discrete(31, 17, 0x0B, zztWorldGetSavegame(myworld) ? "Yes" : "No");

        /* Charset and palette paths */
        struct { int y; char *path; } path_lines[] = {
                {.y = 18, .path = myeditor->char_set ? myeditor->char_set->path : NULL },
                {.y = 19, .path = myeditor->palette ? myeditor->palette->path : NULL },
        };
        for(i = 0; i < sizeof(path_lines) / sizeof(*path_lines); i++) {
                int y = path_lines[i].y;
                char *path = path_lines[i].path;
                if(!path) {
                        d->print_discrete(31, y, 0x07, "(default)           ");
                } else {
                        /* Clear out old file if we just changed it */
                        d->print_discrete(31, y, 0x07, "                    ");
                        char short_path[20];
                        fileof(short_path, path, sizeof(short_path) - 1);
                        d->print_discrete(31, y, 0x0b, short_path);
                }
        }

	/* Update the display */
	d->update(3, 3, 52, 19);
}

int worldinfoeditoption(int curoption, ZZTworld* myworld,
		int cursorx, int cursory, keveditor *myeditor)
{
        displaymethod *d = myeditor->mydisplay;
	int tempnum;
	char buffer[35];
	ZZTworldinfo * header = myworld->header;

	int lined_result = -1;
	switch (curoption) {
		case WLDINFO_NAME:
			/* Change the title of the board */
			strcpy(buffer, (char *)header->title);
			lined_result = line_editor(cursorx, cursory, 0x0f, buffer, 19, LINED_NORMAL, d);
			if(lined_result == LINED_OK) {
				zztWorldSetTitle(myworld, buffer);
			}
			break;

		case WLDINFO_ISSAVED:
			header->savegame = !header->savegame;
			return 1;

		case WLDINFO_FLAGS:
			if(editworldflags(myworld, d) == DKEY_QUIT) {
				return DKEY_QUIT;
			}
			return 1;

		case WLDINFO_AMMO:
			tempnum = header->ammo;
			lined_result = line_editsnumber(cursorx, cursory, 0x0f, &tempnum, -32768, 32767, d);
			header->ammo = tempnum;
			break;

		case WLDINFO_GEMS:
			tempnum = header->gems;
			lined_result = line_editsnumber(cursorx, cursory, 0x0f, &tempnum, -32768, 32767, d);
			header->gems = tempnum;
			break;

		case WLDINFO_HEALTH:
			tempnum = header->health;
			lined_result = line_editsnumber(cursorx, cursory, 0x0f, &tempnum, -32768, 32767, d);
			header->health = tempnum;
			break;

		case WLDINFO_TORCHES:
			tempnum = header->torches;
			lined_result = line_editsnumber(cursorx, cursory, 0x0f, &tempnum, -32768, 32767, d);
			header->torches = tempnum;
			break;

		case WLDINFO_SCORE:
			tempnum = header->score;
			lined_result = line_editsnumber(cursorx, cursory, 0x0f, &tempnum, -32768, 32767, d);
			header->score = tempnum;
			break;

		case WLDINFO_TCYCLES:
			tempnum = header->torchcycles;
			lined_result = line_editsnumber(cursorx, cursory, 0x0f, &tempnum, -32768, 32767, d);
			header->torchcycles = tempnum;
			break;

		case WLDINFO_ECYCLES:
			tempnum = header->energizercycles;
			lined_result = line_editsnumber(cursorx, cursory, 0x0f, &tempnum, -32768, 32767, d);
			header->energizercycles = tempnum;
			break;

		case WLDINFO_TIMEPASSED:
			tempnum = header->timepassed;
			lined_result = line_editsnumber(cursorx, cursory, 0x0f, &tempnum, -32768, 32767, d);
			header->timepassed = tempnum;
			break;

                case WLDINFO_CHARSET:
                case WLDINFO_PALETTE: {
                        bool quit = false;
                        worldinfopickfile(curoption, myworld, cursorx, cursory, myeditor, &quit);
                        break;
                }
        }

	if(lined_result == LINED_OK)
		return 1;
	if(lined_result == LINED_QUIT)
		return DKEY_QUIT;
	return 0;
}

int worldinfodirectionoption(int curoption, ZZTworld* myworld, int cursorx, int cursory, int dir, keveditor *myeditor)
{
	ZZTworldinfo * header = myworld->header;

	switch (curoption) {
		case WLDINFO_AMMO:
			addBoundedDelta(header->ammo, dir, 32767);
			return 1;

		case WLDINFO_GEMS:
			addBoundedDelta(header->gems, dir, 32767);
			return 1;

		case WLDINFO_HEALTH:
			addBoundedDelta(header->health, dir, 32767);
			return 1;

		case WLDINFO_TORCHES:
			addBoundedDelta(header->torches, dir, 32767);
			return 1;

		case WLDINFO_SCORE:
			addBoundedDelta(header->score, dir, 32767);
			return 1;

		case WLDINFO_TCYCLES:
			addBoundedDelta(header->torchcycles, dir, 32767);
			return 1;

		case WLDINFO_ECYCLES:
			addBoundedDelta(header->energizercycles, dir, 32767);
			return 1;

		case WLDINFO_TIMEPASSED:
			addBoundedDelta(header->timepassed, dir, 32767);
			return 1;

		default:
			return worldinfoeditoption(curoption, myworld, cursorx, cursory, myeditor);
	}
}

void worldinfoclearkeys(ZZTworld* myworld)
{
	int i;

	for (i = ZZT_KEY_BLUE; i <= ZZT_KEY_WHITE; i++)
		zztWorldSetKey(myworld, i, 0);
}

void worldinfotogglekey(ZZTworld* myworld, int whichkey)
{
	zztWorldSetKey(myworld, whichkey, !zztWorldGetKey(myworld, whichkey));
}

/* World Info Flags */

void drawstaticflags(displaymethod * d);
void drawflags(ZZTworld * myworld, displaymethod * d);
void worldflagedit(int curoption, ZZTworld* myworld,
									 int cursorx, int cursory, displaymethod* d);
void worldflagclear(int curoption, ZZTworld* myworld);

int editworldflags(ZZTworld* myworld, displaymethod* d)
{
	int curoption = 0;
	int cursorx, cursory;
	int done = 0;

	drawstaticflags(d);
	drawflags(myworld, d);

	cursorx = 31;

	int key;
	do {
		/* Position the cursor */
		cursory = curoption + 9;

		/* Draw the selector */
		d->putch( 7, cursory, '\xAF', 0x02);
		d->putch(51, cursory, '\xAE', 0x02);
		d->cursorgo(cursorx, cursory);

		/* TODO: there is a better way to update the cursor */
		d->update(3, 4, 51, 19);

		/* Get the key */
		key = d->getch();

		/* Erase the selector */
		d->putch( 7, cursory, ' ', 0x00);
		d->putch(51, cursory, ' ', 0x00);

		switch (key) {
			case DKEY_UP:
				if (curoption > 0) curoption--; else curoption = 9;
				break;

			case DKEY_DOWN:
				if (curoption < 9) curoption++; else curoption = 0;
				break;

			case DKEY_ESC:
			case DKEY_QUIT:
				done = 1;
				break;

			case DKEY_ENTER:
			case DKEY_LEFT:
			case DKEY_RIGHT:
				worldflagedit(curoption, myworld, cursorx, cursory, d);
				break;

			case DKEY_DELETE:
			case DKEY_BACKSPACE:
				worldflagclear(curoption, myworld);
				drawstaticflags(d);
				drawflags(myworld, d);
				break;
		}
	} while (!done);
	return key;
}

int worldinfodeleteoption(int curoption, ZZTworld *myworld, keveditor *myeditor) {
	ZZTworldinfo * header = myworld->header;

        switch(curoption) {
		case WLDINFO_ISSAVED:
			header->savegame = 0;
			return 1;

		case WLDINFO_AMMO:
			header->ammo = 0;
			return 1;

		case WLDINFO_GEMS:
			header->gems = 0;
			return 1;

		case WLDINFO_HEALTH:
			header->health = ZZT_WORLD_START_HEALTH;
			return 1;

		case WLDINFO_TORCHES:
			header->torches = 0;
			return 1;

		case WLDINFO_SCORE:
			header->score = 0;
			return 1;

		case WLDINFO_TCYCLES:
			header->torchcycles = 0;
			return 1;

		case WLDINFO_ECYCLES:
			header->energizercycles = 0;
			return 1;

		case WLDINFO_TIMEPASSED:
			header->timepassed = 0;
			return 1;

                case WLDINFO_CHARSET:
                        // Restore the default character set
                        if(myeditor->char_set != NULL) {
                                charset_free(myeditor->char_set);
                        }
                        myeditor->char_set = NULL;
                        myeditor->mydisplay->set_charset(&default_charset);
                        return 1;

                case WLDINFO_PALETTE:
                        // Restore the default palette
                        if(myeditor->palette != NULL) {
                                palette_free(myeditor->palette);
                        }
                        myeditor->palette = NULL;
                        myeditor->mydisplay->set_palette(&default_palette);
                        return 1;

                default:
                        return 0;
        }
}

void worldinfopickfile(int curoption, ZZTworld *myworld, int cursorx, int cursory,
                       keveditor *myeditor, bool *quit) {
        displaymethod *d = myeditor->mydisplay;

        // Show the file picker for *.pal files (palette) or *.chr and *.com files (charset)
        char *title;
        stringvector extensions;
        initstringvector(&extensions);
        if(curoption == WLDINFO_CHARSET) {
                pushstring(&extensions, "chr");
                pushstring(&extensions, "com");
                title = "Choose a Character Set File";
        } else {
                pushstring(&extensions, "pal");
                title = "Choose a Palette File";
        }
        char *path = filedialog_multiext(".", &extensions, title, FTYPE_ALL, myeditor->mydisplay, quit);
        removestringvector(&extensions);
        if(*quit) {
                return;
        }

        // Redraw the world info screen (was overwritten by file picker)
        drawstaticworldinfo(d);
        drawworldinfo(myworld, myeditor);
        d->update(3, 4, 51, 19);

        // If the user backed out, we're done
        if(path == NULL) {
                return;
        }

        // If they picked a file, try to load it
        bool failed = false;
        switch(curoption) {
                case WLDINFO_CHARSET: {
                        charset *result = charset_load(path);
                        if(!result) {
                                failed = true;
                                break;
                        }
                        if(myeditor->char_set) {
                                charset_free(myeditor->char_set);
                        }
                        myeditor->char_set = result;
                        d->set_charset(result);
                        break;
                }
                case WLDINFO_PALETTE: {
                        palette *result = palette_load(path);
                        if(!result) {
                                failed = true;
                                break;
                        }
                        if(myeditor->palette) {
                                palette_free(myeditor->palette);
                        }
                        myeditor->palette = result;
                        d->set_palette(result);
                        break;
                }
        }

        if(failed) {
                // Show an error message on the current field
                d->print_discrete(31, cursory, 0x0c, "Unable to load file!");
                d->update(31, cursory, 20, 1);
                d->cursorgo(cursorx, cursory);
                int key = myeditor->mydisplay->getch();
                if(key == DKEY_QUIT) {
                        *quit = true;
                        return;
                }
        }

        // In either case, we need to redraw the content
        drawworldinfo(myworld, myeditor);
        d->update(3, 4, 51, 19);
}

void drawstaticflags(displaymethod * d)
{
	/* Draw the scroll box */
	drawscrollbox(d, 0, 0, 0);
	d->putch_discrete( 7, 13, ' ', 0x00);
	d->putch_discrete(51, 13, ' ', 0x00);

	/* Draw the static contents */
	d->print_discrete(25, 4, 0x0A, "World Info");

	d->print_discrete(23,  7, 0x0F, "Set/Clear Flags");
	d->print_discrete(22, 9+0, 0x0A, "Flag  1:");
	d->print_discrete(22, 9+1, 0x0A, "Flag  2:");
	d->print_discrete(22, 9+2, 0x0A, "Flag  3:");
	d->print_discrete(22, 9+3, 0x0A, "Flag  4:");
	d->print_discrete(22, 9+4, 0x0A, "Flag  5:");
	d->print_discrete(22, 9+5, 0x0A, "Flag  6:");
	d->print_discrete(22, 9+6, 0x0A, "Flag  7:");
	d->print_discrete(22, 9+7, 0x0A, "Flag  8:");
	d->print_discrete(22, 9+8, 0x0A, "Flag  9:");
	d->print_discrete(22, 9+9, 0x0A, "Flag 10:");
}

void drawflags(ZZTworld * myworld, displaymethod * d)
{
	int i;

	for (i = 0; i < ZZT_MAX_FLAGS; i++) {
		d->print_discrete(31, 9 + i, 0x0B, (char *)zztWorldGetFlag(myworld, i));
	}

	/* Update the display */
	d->update(3, 3, 52, 19);
}

void worldflagedit(int curoption, ZZTworld* myworld,
									 int cursorx, int cursory, displaymethod* d)
{
	char buffer[ZZT_FLAG_SIZE + 1];

	strcpy(buffer, (char *)zztWorldGetFlag(myworld, curoption));
	if (line_editor(cursorx, cursory, 0x0B, buffer, ZZT_FLAG_SIZE, LINED_NOLOWER, d) == LINED_OK) {
		zztWorldSetFlag(myworld, curoption, buffer);
	}
}

void worldflagclear(int curoption, ZZTworld* myworld)
{
	/* Very nice */
	zztWorldSetFlag(myworld, curoption, "");
}

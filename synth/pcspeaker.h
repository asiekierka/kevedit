/* pcspeaker.c	-- PC speaker music synthesizer
 * $Id: pcspeaker.h,v 1.1 2002/08/23 21:34:15 bitman Exp $
 * Copyright (C) 2001 Kev Vance <kev@kvance.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef PC_SPEAKER_H
#define PC_SPEAKER_H 1

#include "notes.h"

/* Play a single note. Program will halt until playback has finished. */
void pcSpeakerPlayNote(musicalNote note, musicSettings settings);

/* Call immediately after note playback to silence the speaker */
void pcSpeakerFinish(void);

#endif

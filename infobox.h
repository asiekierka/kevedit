/* infobox.h - board/world information dialogs
 * $id$
 * Copyright (C) 2001 Ryan Phillips <bitman@users.sourceforge.net>
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

#ifndef __INFOBOX_H
#define __INFOBOX_H

#include "zzt.h"
#include "display.h"
#include "kevedit.h"

/* editboardinfo() - brings up dialog box for editing board info */
void editboardinfo(world* myworld, int curboard, displaymethod* d);

/* editworldinfo() - brings up dialog box for editing world info */
void editworldinfo(world* myworld, displaymethod* d);

#endif
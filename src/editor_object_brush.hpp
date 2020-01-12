/*
 * Copyright (C) 1995-2002 FSGames. Ported by Sean Ford and Yan Shosh
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef __EDITOR_OBJECT_BRUSH_HPP__
#define __EDITOR_OBJECT_BRUSH_HPP__

#include "walker-fwd.hpp"

#include <SDL2/SDL.h>

class EditorObjectBrush
{
public:
    bool snap_to_grid;
    Sint32 order;
    Sint32 family;
    Uint8 team;
    Uint16 level;
    bool picking;

    EditorObjectBrush();
    void set(Walker *target);
};

#endif
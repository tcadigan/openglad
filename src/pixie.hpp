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
#ifndef __PIXIE_HPP__
#define __PIXIE_HPP__

// Definition of PIXIE class

#include "base.hpp"

class pixie
{
public:
    pixie(PixieData const &data);
    pixie(PixieData const &data, Sint32 doaccel);
    virtual ~pixie();
    virtual Sint16 move(Sint16 x, Sint16 y);
    Sint16 setxy(Sint16 x, Sint16 y);
    Sint16 draw(viewscreen *view_buf);
    Sint16 draw(Sint16 x, Sint16 y, viewscreen *view_buf);
    Sint16 drawMix(viewscreen *view_buf);
    Sint16 drawMix(Sint16 x, Sint16 y, viewscreen *view_buf);
    Sint16 put_screen(Sint16 x, Sint16 y);
    void init_sdl_surface(void);
    void set_accel(Sint32 a);
    void set_data(PixieData const &data);
    Sint16 on_screen();
    // On a specific viewscreen?
    Sint16 on_screen(viewscreen *viewp);

    Sint16 sizex;
    Sint16 sizey;
    Sint16 xpos;
    Sint16 ypos;

    // buffers: Is SDL_Surface acceleration on/off, 1/0
    Sint32 accel;

protected:
    Uint16 size;
    Uint8 *bmp;
    Uint8 *oldbmp;

    // buffers: Same data as bmp in a convenient SDL_Surface
    SDL_Surface *bmp_surface;
};

#endif
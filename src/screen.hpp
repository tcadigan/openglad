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
#ifndef __SCREEN_HPP__
#define __SCREEN_HPP__

// Definition of SCREEN class

#include <map>
#include <set>
#include <string>

#include "base.hpp"
#include "gloader.hpp"
#include "level_data.hpp"
#include "obmap.hpp"
#include "save_data.hpp"
#include "smooth.h"
#include "text.hpp"
#include "video.hpp"

class screen : public video
{
public:
    // Called with '1' for numviews
    screen();
    screen(Sint16 howmany);
    virtual ~screen();

    void reset(Sint16 howmany);
    void ready_for_battle(Sint16 howmany);
    void initialize_views();
    void cleanup(Sint16 howmany);
    void clear();
    video *get_video_ob();
    bool query_passable(float x, float y, walker *ob);
    bool query_object_passable(float x, float y, walker *ob);
    bool query_grid_passable(float x, float y, walker *ob);
    Sint16 redraw();
    void refresh();
    walker *first_of(Uint8 whatorder, Uint8 whatfamily, int team_num=1);
    Sint16 input(SDL_event const &event);
    Sint16 continuous_input();
    Sint16 act();

    Sint16 endgame(Sint16 ending);
    Sint16 endgame(Sint16 ending, Sint16 nextlevel); // What level next?
    void draw_panels(Sint16 howmany);
    walker *find_near_foe(walker *ob);
    walker *find_foe_far(walker *ob);
    walker *find_nearest_blood(walker *who);
    walker *find_nearest_player(walker *ob);
    std::list<walker *> find_in_range(std::list<walker *> &somelist, Sint32 range, Sint16 *howmany, walker *ob);
    std::list<walker *> find_foes_in_range(std::list<walker *> &somelist, Sint32 range, Sint16 *howmany, walker *ob);
    std::list<walker *> find_foe_weapons_in_range(std::list< walker *> &somelist, Sint32 range, Sint16 *howmany, walker *ob);
    Uint8 damage_tile(Sint16 xloc, Sint16 yloc); // Damage the specified tile
    void do_notify(Uint8 const *message, walker *who); // Printing text
    void report_mem();
    walker *set_walker(walker *ob, Uint8 order, Uint8 family);
    Uint8 const *get_scen_title(Uint8 const *filename, screen *master);
    void add_level_completed(std::string const &campaign, Sint32 level_index);

    Sint32 get_num_levels_completed(std::string const &campaign) const;
    bool is_level_completed(Sint32 level_index) const;

    // General drawing data
    Uint8 newpalette[768];
    Sint16 palmode;

    // Level data
    Leveldata level_data;

    // Save data
    SaveData save_data;

    // Game state
    float control_hp; // Last turn's hitpoints
    Uint8 end;
    Uint8 timer_wait;

    // Set true when all our foes are dead
    Uint16 level_done;

    // We should reset the level and go again
    bool retry;

    Uint8 special_name[NUM_FAMILIES][NUM_SPECIALS][20];
    Uint8 alternate_name[NUM_FAMILIES][NUM_SPECIALS][20];

    // Stops enemies from acting
    Uint8 enemy_freeze;

    oundob *soundp;
    Uint16 redrawme;
    viewscreen *viewob[5];
    Uint16 numviews;
    Uint32 timerstart;
    Uint32 framecount;
};

#endif
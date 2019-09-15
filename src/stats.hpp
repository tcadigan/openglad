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
#ifndef __STATS_HPP__
#define __STATS_HPP__

// Defition of STATS class

//
// Include file for the stats object
//

#include <list>

#include "base.hpp"

// These are for the bit flags
#define BIT_FLYING static_cast<Sint32>(1) // Fly over water, trees
#define BIT_SWIMMING static_cast<Sint32>(2) // Move over water
#define BIT_ANIMATE static_cast<Sint32>(4) // Animate even when not moving
#define BIT_INVINCIBLE static_cast<Sint32>(8) // Can't be harmed
#define BIT_NO_RANGED static_cast<Sint32>(16) // No ranged attack
#define BIT_IMMORTAL static_cast<Sint32>(32) // For weapons that don't die when they hit
#define BIT_NO_COLLIDE static_cast<Sint32>(64) // Fly through walkers
#define BIT_PHANTOM static_cast<Sint32>(128) // Use phantomputbuffer instead of walkerputbuffer
#define BIT_NAMED static_cast<Sint32>(256) // Has a name (will have outline)
#define BIT_FORESTWALK static_cast<Sint32>(512) // Can walk through forests
#define BIT_MAGICAL static_cast<Sint32>(1024) // Generally for magical weapons
#define BIT_FIRE static_cast<Sint32>(2048) // For any flame weapons
#define BIT_ETHEREAL static_cast<Sint32>(4096) // Fly "through" walls
#define BIT_LAST static_cast<Sint32>(8192)

// Other special effects, etc.
#define FAERIE_FREEZE_TIME 40

// Class statistics, for (guess what?) controlling stats, etc....
class statistics
{
public:
    statistics(walker *);
    ~statistics();

    Sint16 try_command(Sint16 whatcommand, Sint16 iterations, Sint16 info1, Sint16 info2);
    Sint16 try_command(Sint16 whatcommand, Sint16 iterations);
    void set_command(Sint16 whatcommand, Sint16 iterations);
    void set_command(Sint16 whatcommand, Sint16 iterations, Sint16 info1, Sint16 info2);
    void add_command(Sint16 whatcommand, Sint16 iterations, Sint16 info1, Sint16 info2);
    void force_command(Sint16 whatcommand, Sint16 iterations, Sint16 info1, Sint16 info2);
    bool has_commands();
    void clear_command();
    Sint16 do_command();
    void hit_response(walker *who);
    // Yell and run away
    void yell_for_help(walker *foe);
    Sint16 query_bit_flags(Sint32 myvalue);
    void clear_bit_flags();
    // Sets a single flag
    void set_bit_flags(Sint32 someflag, Sint16 newvalue);
    // Is our right blocked?
    bool right_blocked();
    bool right_forward_blocked();
    bool right_back_blocked();
    // Are we blocked in front?
    bool forward_blocked();
    // Not in use???
    // Sint16 distance_to_foe();
    // Walk using right hand rule
    bool right_walk();
    // Walk in a line toward foe...
    bool direct_walk();

    // For NPCs normally...
    Uint8 name[12];
    Uint8 old_order;
    Uint8 old_family;
    Uint32 last_distance;
    // Distances (to foe) are used for AI walking
    Sint32 current_distance;
    // Holds (currently) 32 bit flags
    Sint32 bit_flags;
    Sint16 delete_me;

    float hitpoints;
    float max_hitpoints;
    float magicpoints;
    float max_magic_points;

    Sint32 max_heal_delay;
    Sint32 current_heal_delay;
    Sint32 max_magic_display;
    Sint32 current_magic_delay;
    // Magic points we regain each round
    Sint32 magic_per_round;
    // hp we regain each round
    Sint32 heal_per_round;
    // Reduces damage against us
    float armor;

    Uint16 level;
    // Use for paralyzing...
    Sint16 froze_delay;
    // Cost of our special ability
    Uint16 special_cost[NUM_SPECIALS];
    // Cost of our weapon
    Sint16 weapon_cost;
    walker *controller;
    std::list<command> commands;

private:
    // Parameters to command
    // Sint16 com1;
    // Sint16 com2;
    // Number of rounds we've spent right walking
    Sint32 walkrounds;
};

class command
{
public:
    command();

    Sint16 commandtype;
    Sint16 commandcount;
    Sint16 com1;
    Sint16 com2;
};

#endif
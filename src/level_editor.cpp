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
/*
 * Changelog
 *     08/08/02: Zardus: Added scrolling-by-minimap
 *               Zardus: Added scrolling-by-keyboard
 */
#include "level_editor.hpp"

#include "input.hpp"
#include "io.hpp"
#include "object_type.hpp"
#include "options.hpp"
#include "pal32.hpp"
#include "picker.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "util.hpp"
#include "view.hpp"
#include "walker.hpp"

#ifdef OUYA
#include "ouya_controller.hpp"
#endif

#include <algorithm>
#include <sstream>

#define OK 4 // This function was successful, continue normal operation

#define MINIMUM_TIME 0

#define S_LEFT 1
#define S_DOWN 188

#define SCROLLSIZE 8

#define VERSION_NUM static_cast<Uint8>(8) // Save scenario type info

#define PIX_LEFT (S_RIGHT + 18)
// #define PIX_DOWN ((PIX_MAX / PIX_OVER) + 1)
#define PIX_RIGHT (PIX_LEFT + (PIX_OVER * GRID_SIZE))

#define L_W(x) ((x * 8) + 9)
#define L_H(x) (x * 8)

#define PAN_LIMIT_UP -60
#define PAN_LIMIT_DOWN (((GRID_SIZE * data.level->grid.h) - 200) + 80)
#define PAN_LIMIT_LEFT -60
#define PAN_LIMIT_RIGHT (((GRID_SIZE * data.level->grid.w) - 320) + 80)

void quit(Sint32 arg1);
void set_screen_pos(VideoScreen *myscreen, Sint32 x, Sint32 y);
bool some_hit(Sint32 x, Sint32 y, Walker *ob, LevelData *data);
Uint8 get_random_matching_tile(Sint32 whatback);
void info_box(Walker *target, VideoScreen *myscreen);
void set_name(Walker *target, VideoScreen *myscreen);
bool save_level_and_map(VideoScreen *ascreen);
bool are_objects_outside_area(LevelData *level, Sint32 x, Sint32 y, Sint32 w, Sint32 h);

extern Sint16 scroll_amount; // For scrolling up and down text popups
extern VideoScreen *myscreen; // Global for scen?
// Zardus: Our prefs object from view.cpp
extern Options *theprefs;

Uint8 scenpalette[768];
Sint32 cyclemode = 1; // For color cycling

Sint32 mouse_up_button = 0;

// Deltas for motion
Sint32 mouse_motion_x = 0;
Sint32 mouse_motion_y = 0;

bool pan_left = false;
bool pan_right = false;
bool pan_up = false;
bool pan_down = false;

bool does_campaign_exist(std::string const &campaign_id)
{
    std::list<std::string> ls = list_campaigns();

    return std::any_of(ls.begin(),
                       ls.end(),
                       [&campaign_id](auto const &e) {
                           return (campaign_id == e);
                       });
}

bool create_new_campaign(std::string const &campaign_id)
{
    // Delete the temp directory
    cleanup_unpacked_campaign();

    // Create necessities in the temp directory
    create_dir(get_user_path() + "temp/");
    create_dir(get_user_path() + "temp/pix");
    create_dir(get_user_path() + "temp/scen");
    create_dir(get_user_path() + "temp/sound");
    create_new_pix(get_user_path() + "temp/icon.pix", 32, 32);
    create_new_campaign_descriptor(get_user_path() + "temp/campaign.yaml");
    create_new_scen_file(get_user_path() + "temp/scen/scen1.fss", "scen0001");
    // Create the map file (grid)
    create_new_map_pix(get_user_path() + "temp/pix/scen001.pix", 40, 60);

    bool result = repack_campaign(campaign_id);

    if (!result) {
        return result;
    }

    cleanup_unpacked_campaign();

    return true;
}

void importCampaignPicker()
{
    // TODO: Browse campaigns online and download some
}

void shareCampaign(VideoScreen *myscreen)
{
    // TODO: Send current campaign to the internets!
}

bool prompt_for_string_block(std::string const &message, std::list<std::string> &result)
{
    myscreen->darken_screen();

    Sint32 max_chars = 40;
    Sint32 max_lines = 8;

    Sint32 w = max_chars * 6;
    Sint32 h = max_lines * 10;
    Sint32 x = 160 - (w / 2);
    Sint32 y = 100 - (h / 2);

    Text &mytext = myscreen->text_normal;

    // Background
    myscreen->draw_button(x - 5, y - 20, (x + w) + 10, (y + h) + 10, 1);

    Uint8 forecolor = DARK_BLUE;

    SDL_Rect done_button = { 320 - 52, 0, 50, 14 };
    SDL_Rect cancel_button = { 320 - 104, 0, 50, 14 };

#if defined(USE_TOUCH_INPUT) || defined(USE_CONTROLLER_INPUT)
    SDL_Rect newline_button = { 320 - 75, 16, 50, 14 };
    SDL_Rect up_button = { 14, 0, 14, 14 };
    SDL_Rect down_button = { 14, 14, 14, 14 };
    SDL_Rect left_button = { 0, 14, 14, 14 };
    SDL_Rect right_button = { 28, 14, 14, 14 };
#endif

    std::list<std::string> original_text = result;

    clear_keyboard();
    clear_key_press_event();
    clear_text_input_event();

    MouseState &mymouse = query_mouse_no_poll();

    SDL_StartTextInput();

    if (result.empty()) {
        result.push_back("");
    }

    std::list<std::string>::iterator s = result.begin();
    size_t cursor_pos = 0;
    size_t current_line = 0;

    bool cancel = false;
    bool done = false;

    while (!done) {
        get_input_events(POLL);

        if (query_key_press_event()) {
            Uint8 c = query_key();
            clear_key_press_event();

            if (c == SDLK_RETURN) {
#if defined(USE_TOUCH_INPUT) || defined(USE_CONTROLLER_INPUT)
                // Some soft keyboards might disappear anyhow if you press return...
                done = true;

                break;
#else
                std::string rest_of_line(s->substr(cursor_pos));
                s->erase(cursor_pos);
                ++s;
                s = result.insert(s, rest_of_line);
                ++current_line;
                cursor_pos = 0;
#endif
            } else if (c == SDLK_BACKSPACE) {
                // At the beginning of the line?
                if (cursor_pos == 0) {
                    // Deleting a line break
                    // Not at the first line?
                    if (!result.empty() && (current_line > 0)) {
                        // Then move up into the previous line, copying the old line
                        --current_line;
                        std::string old_line(*s);
                        s = result.erase(s);
                        --s;
                        cursor_pos = s->size();
                        // Append the old line
                        *s += old_line;
                    }
                } else {
                    // Delete previous character
                    --cursor_pos;
                    s->erase(cursor_pos, 1);
                }
            }
        } else if (mymouse.left) {
            mymouse.left = false;

            if (mymouse.in(done_button)) {
                done = true;
            } else if (mymouse.in(cancel_button)) {
                result = original_text;
                done = true;
            }
#if defined(USE_TOUCH_INPUT) || defined(USE_CONTROLLER_INPUT)
            else if (mymouse.in(newline_button)) {
                std::string rest_of_line(s->substr(cursor_pos));
                s->erase(cursor_pos);
                ++s;
                s = result.insert(s, rest_of_line);
                ++current_line;
                cursor_pos = 0;
            } else if (mymouse.in(up_button)) {
                if (current_line > 0) {
                    --current_line;
                    --s;

                    if (s->size() < cursor_pos) {
                        cursor_pos = s->size();
                    }
                }
            } else if (mymouse.in(down_button)) {
                if ((current_line + 1) < result.size()) {
                    ++current_line;
                    ++s;
                } else {
                    // At the bottom already
                    cursor_pos = s->size();
                }

                if (s->size() < cursor_pos) {
                    cursor_pos = s->size();
                }
            } else if (mymouse.in(left_button)) {
                if (cursor_pos > 0) {
                    --cursor_pos;
                } else if (current_line > 0) {
                    --current_line;
                    --s;
                    cursor_pos = s->size();
                }
            } else if (mymouse.in(right_button)) {
                ++cursor_pos;

                if (cursor_pos > s->size()) {
                    if ((current_line + 1) < result.size()) {
                        // Go to next line
                        ++current_line;
                        ++s;
                        cursor_pos = 0;
                    } else {
                        // No next line
                        cursor_pos = s->size();
                    }
                }
            }
#endif
        }

        if (keystates[KEYSTATE_ESCAPE]) {
            while (keystates[KEYSTATE_ESCAPE]) {
                get_input_events(WAIT);
            }

            done = true;

            break;
        }

        if (keystates[KEYSTATE_DELETE]) {
            if (cursor_pos < s->size()) {
                s->erase(cursor_pos, 1);
            }

            while (keystates[KEYSTATE_DELETE]) {
                get_input_events(WAIT);
            }
        }

        if (keystates[KEYSTATE_UP]) {
            if (current_line > 0) {
                --current_line;
                --s;

                if (s->size() < cursor_pos) {
                    cursor_pos = s->size();
                }
            }

            while (keystates[KEYSTATE_UP]) {
                get_input_events(WAIT);
            }
        }

        if (keystates[KEYSTATE_DOWN]) {
            if ((current_line + 1) < result.size()) {
                ++current_line;
                ++s;
            } else {
                // At the bottom already
                cursor_pos = s->size();
            }

            if (s->size() < cursor_pos) {
                cursor_pos = s->size();
            }

            while (keystates[KEYSTATE_DOWN]) {
                get_input_events(WAIT);
            }
        }

        if (keystates[KEYSTATE_LEFT]) {
            if (cursor_pos > 0) {
                --cursor_pos;
            } else if (current_line > 0) {
                --current_line;
                --s;
                cursor_pos = s->size();
            }

            while (keystates[KEYSTATE_LEFT]) {
                get_input_events(WAIT);
            }
        }

        if (keystates[KEYSTATE_RIGHT]) {
            ++cursor_pos;

            if (cursor_pos > s->size()) {
                if ((current_line + 1) < result.size()) {
                    // Go to next line
                    ++current_line;
                    ++s;
                    cursor_pos = 0;
                } else {
                    // No next line
                    cursor_pos = s->size();
                }
            }

            while (keystates[KEYSTATE_RIGHT]) {
                get_input_events(WAIT);
            }
        }

        if (query_text_input_event()) {
            std::string temptext(query_text_input());

            if (!temptext.empty()) {
                s->insert(cursor_pos, temptext);
                cursor_pos += temptext.size();
            }
        }

        clear_text_input_event();
        myscreen->draw_button(x - 5, y - 20, (x + w) + 10, (y + h) + 10, 1);
        mytext.write_xy(x, y - 13, message.c_str(), BLACK, 1);
        myscreen->hor_line(x, y - 5, w, BLACK);

        myscreen->draw_button(done_button.x, done_button.y, done_button.x + done_button.w,
                              done_button.y + done_button.h, 1);

        mytext.write_xy((done_button.x + (done_button.w / 2)) - 12,
                        (done_button.y + (done_button.h / 2)) - 3,
                        "DONE", DARK_BLUE, 1);

        myscreen->draw_button(cancel_button.x, cancel_button.y, cancel_button.x + cancel_button.w,
                              cancel_button.y + cancel_button.h, 1);

        mytext.write_xy((cancel_button.x + (cancel_button.w / 2)) - 18,
                        (cancel_button.y + (cancel_button.h / 2)) - 3,
                        "CANCEL", DARK_BLUE, 1);

#if defined(USE_TOUCH_INPUT) || defined(USE_CONTROLLER_INPUT)
        myscreen->draw_button(newline_button.x, newline_button.y,
                              newline_button.x + newline_button.w,
                              newline_button.y + newline_button.h, 1);

        mytext.write_xy((newline_button.x + (newline_button.w / 2)) - 18,
                        (newline_button.y + (newline_button.h / 2)) - 3,
                        "NEWLINE", DARK_BLUE, 1);

        myscreen->draw_button(up_button.x, up_button.y, up_button.x + up_button.w,
                              up_button.y + up_button.h, 1);

        mytext.write_xy((up_button.x + (up_button.w / 2)) - 6,
                        (up_button.y + (up_button.h / 2)) - 3,
                        "UP", DARK_BLUE, 1);

        myscreen->draw_button(left_button.x, left_button.y, left_button.x + left_button.w,
                              left_button.y + left_button.h, 1);

        mytext.write_xy((left_button.x + (left_button.w / 2)) - 6,
                        (left_button.y + (left_button.h / 2)) - 3,
                        "LT", DARK_BLUE, 1);

        myscreen->draw_button(down_button.x, down_button.y, down_button.x + down_button.w,
                              down_button.y + down_button.h, 1);

        mytext.write_xy((down_button.x + (down_button.w / 2)) - 6,
                        (down_button.y + (down_button.h / 2)) - 3,
                        "DN", DARK_BLUE, 1);

        myscreen->draw_button(right_button.x, right_button.y, right_button.x + right_button.w,
                              right_button.y + right_button.h, 1);

        mytext.write_xy((right_button.x + (right_button.w / 2)) - 6,
                        (right_button.y + (right_button.h / 2)) - 3,
                        "RT", DARK_BLUE, 1);
#endif

        Sint32 offset = 0;

        if (current_line > 3) {
            offset = (current_line - 3) * 10;
        }

        Sint32 j = 0;

        for (auto const &e : result) {
            Sint32 ypos = (y + (j * 10)) - offset;

            if ((y <= ypos) && (ypos <= (y + h))) {
                mytext.write_xy(x, ypos, e, forecolor, 1);
            }

            ++j;
        }

        myscreen->ver_line(x + (cursor_pos * 6), ((y + (current_line * 10)) - 2) - offset,
                           10, RED);

        myscreen->buffer_to_screen(0, 0, 320, 200);

        SDL_Delay(10);
    }

    SDL_StopTextInput();

    clear_keyboard();

    return !cancel;
}

bool prompt_for_string(std::string const &message, std::string &result)
{
    myscreen->darken_screen();

    Sint32 max_chars = 29;

    Sint32 x = 58;
    Sint32 y = 60;
    Sint32 w = max_chars * 6;
    Sint32 h = 10;

    myscreen->draw_button(x - 5, y - 20, (x + w) + 10, (y + h) + 10, 1);

    std::string str(myscreen->text_normal.input_string_ex(x, y, max_chars, message.c_str(), result.c_str()));

    if (str.empty()) {
        return false;
    }

    result = str;

    return true;
}

bool button_showing(std::list<std::pair<SimpleButton *, std::set<SimpleButton *>>> const &ls, SimpleButton *elem)
{
    return std::any_of(ls.begin(),
                       ls.end(),
                       [&elem](auto const &e) {
                           return (e.second.find(elem) != e.second.end());
                       });
}

bool activate_sub_menu_button(Sint32 mx, Sint32 my, std::list<std::pair<SimpleButton *, std::set<SimpleButton *>>> &current_menu, SimpleButton &button, bool is_in_top_menu)
{
    // Make sure it is showing
    if (!button.contains(mx, my) || (!is_in_top_menu && !button_showing(current_menu, &button))) {
        return false;
    }

    MouseState &mymouse = query_mouse_no_poll();

    while (mymouse.left) {
        get_input_events(WAIT);
    }

    if (!current_menu.empty()) {
        // Close menu if already open
        if (current_menu.back().first == &button) {
            current_menu.pop_back();

            return false;
        }

        // Remove all menus up to the parent
        while (!current_menu.empty()) {
            std::set<SimpleButton *> &s = current_menu.back().second;

            if (s.find(&button) == s.end()) {
                current_menu.pop_back();
            } else {
                // Open this menu
                return true;
            }
        }
    }

    // No parent!
    return is_in_top_menu;
}

bool activate_menu_choice(Sint32 mx, Sint32 my, LevelEditorData &data, SimpleButton &button, bool is_in_top_menu)
{
    // Make sure it is showing
    if (!button.contains(mx, my) || (!is_in_top_menu && !button_showing(data.current_menu, &button))) {
        return false;
    }

    MouseState &mymouse = query_mouse_no_poll();

    while (mymouse.left) {
        get_input_events(WAIT);
    }

    // Close menu
    data.current_menu.clear();
    data.draw(myscreen);
    myscreen->refresh();

    return true;
}

bool activate_menu_toggle_choice(Sint32 mx, Sint32 my, LevelEditorData &data, SimpleButton &button, bool is_in_top_menu)
{
    // Make sure it is showing
    if (!button.contains(mx, my) || (!is_in_top_menu && !button_showing(data.current_menu, &button))) {
        return false;
    }

    MouseState &mymouse = query_mouse_no_poll();

    while (mymouse.left) {
        get_input_events(WAIT);
    }

    // Close menu
    data.draw(myscreen);
    myscreen->refresh();

    return true;
}

// Recursively get the connected levels
void get_connected_level_exits(Sint32 current_level, std::list<Sint32> const &levels, std::set<Sint32> &connected, std::list<std::string> &problems)
{
    std::stringstream buf;

    // Stopping condition
    if (connected.find(current_level) != connected.end()) {
        return;
    }

    connected.insert(current_level);

    // Load level
    LevelData d(current_level);

    if (!d.load()) {
        buf << "Level " << current_level << " failed to load.";

        std::string prob(buf.str());
        buf.clear();
        prob.resize(40);
        problems.push_back(prob);

        return;
    }

    // Get the exits
    std::set<Sint32> exits;

    for (auto const &w : d.fxlist) {
        if ((w->query_order() == ORDER_TREASURE) && (w->query_family() == FAMILY_EXIT) && (w->stats != nullptr)) {
            exits.insert(w->stats->level);
        }
    }

    // With no exits, we'll progress directly to the next sequential level
    if (exits.empty()) {
        // Does the next sequential level exist?

        bool has_next = false;

        for (auto const &e : levels) {
            if ((current_level + 1) == e) {
                has_next = true;

                break;
            }
        }

        if (has_next) {
            exits.insert(current_level + 1);
        } else {
            buf << "Level " << current_level << " has no exits.";
            std::string prob(buf.str());
            buf.clear();
            prob.resize(40);
            problems.push_back(prob);

            return;
        }
    }

    // Recursively call on exits
    for (auto const &e : exits) {
        get_connected_level_exits(e, levels, connected, problems);
    }
}

bool is_in_selection(Walker *w, std::vector<SelectionInfo> const &selection)
{
    return std::any_of(selection.begin(),
                       selection.end(),
                       [&w](auto const &e) {
                           return (e.target == w);
                       });
}

// Make sure to use reset_mode_buttons() after this
void add_contained_objects_to_selection(LevelData *level, Rectf const &area, std::vector<SelectionInfo> &selection)
{
    for (auto const &w : level->oblist) {
        if (w && area.contains(w->xpos + (w->sizex / 2), w->ypos + (w->sizey / 2))) {
            if (!is_in_selection(w, selection)) {
                selection.push_back(SelectionInfo(w));
            }
        }
    }

    for (auto const &w : level->fxlist) {
        if (w && area.contains(w->xpos + (w->sizex / 2), w->ypos + (w->sizey / 2))) {
            if (!is_in_selection(w, selection)) {
                selection.push_back(SelectionInfo(w));
            }
        }
    }

    for (auto const &w : level->weaplist) {
        if (w && area.contains(w->xpos + (w->sizex / 2), w->ypos + (w->sizey / 2))) {
            if (!is_in_selection(w, selection)) {
                selection.push_back(SelectionInfo(w));
            }
        }
    }
}

bool are_objects_outside_area(LevelData *level, Sint32 x, Sint32 y, Sint32 w, Sint32 h)
{
    x *= GRID_SIZE;
    y *= GRID_SIZE;
    w *= GRID_SIZE;
    h *= GRID_SIZE;

    for (auto const &e : level->oblist) {
        Walker *ob = e;

        if (ob && ((x > ob->xpos) || (ob->xpos >= (x + w))
                   || (y >= ob->ypos) || (ob->ypos >= (y + h)))) {
            return true;
        }
    }

    for (auto const &e : level->fxlist) {
        Walker *ob = e;

        if (ob && ((x > ob->xpos) || (ob->xpos >= (x + w))
                   || (y > ob->ypos) || (ob->ypos >= (y + h)))) {
            return true;
        }
    }

    for (auto const &e : level->weaplist) {
        Walker *ob = e;

        if (ob && ((x > ob->xpos) || (ob->xpos >= (x + w))
                   || (y > ob->ypos) || (ob->ypos >= (y + h)))) {
            return true;
        }
    }

    return false;
}

EventTypeEnum handle_basic_editor_event(SDL_Event const &event)
{
    switch (event.type) {
    case SDL_WINDOWEVENT:
        handle_window_event(event);

        return HANDLED_EVENT;
    case SDL_TEXTINPUT:
        handle_text_event(event);

        return TEXT_EVENT;
    case SDL_MOUSEWHEEL:
        handle_mouse_event(event);

        return SCROLL_EVENT;
    case SDL_FINGERMOTION:
        handle_mouse_event(event);
        mouse_motion_x = event.tfinger.dx * 320;
        mouse_motion_y = event.tfinger.dy * 200;

        return MOUSE_MOTION_EVENT;
    case SDL_FINGERUP:
    {
        MouseState &mymouse = query_mouse_no_poll();
        Sint32 left_state = mymouse.left;
        Sint32 right_state = mymouse.right;
        handle_mouse_event(event);

        if (left_state != mymouse.left) {
            mouse_up_button = MOUSE_LEFT;
        } else if (right_state != mymouse.right) {
            mouse_up_button = MOUSE_RIGHT;
        } else {
            mouse_up_button = 0;
        }

        return MOUSE_UP_EVENT;
    }
    case SDL_FINGERDOWN:
        handle_mouse_event(event);

        return MOUSE_DOWN_EVENT;
    case SDL_KEYDOWN:
        handle_key_event(event);

        return KEY_DOWN_EVENT;
    case SDL_KEYUP:
        handle_key_event(event);

        return HANDLED_EVENT;
    case SDL_MOUSEMOTION:
        handle_mouse_event(event);
        mouse_motion_x = event.motion.xrel * (320 / viewport_w);
        mouse_motion_y = event.motion.yrel * (200 / viewport_h);

        return MOUSE_MOTION_EVENT;
    case SDL_MOUSEBUTTONUP:
    {
        MouseState &mymouse = query_mouse_no_poll();
        Sint32 left_state = mymouse.left;
        Sint32 right_state = mymouse.right;
        handle_mouse_event(event);

        if (left_state != mymouse.left) {
            mouse_up_button = MOUSE_LEFT;
        } else if (right_state != mymouse.right) {
            mouse_up_button = MOUSE_RIGHT;
        } else {
            mouse_up_button = 0;
        }

        return MOUSE_UP_EVENT;
    }
    case SDL_MOUSEBUTTONDOWN:
        handle_mouse_event(event);

        return MOUSE_DOWN_EVENT;
    case SDL_JOYAXISMOTION:
        handle_joy_event(event);

        return HANDLED_EVENT;
    case SDL_JOYBUTTONDOWN:
        handle_joy_event(event);

        return HANDLED_EVENT;
    case SDL_JOYBUTTONUP:
        handle_joy_event(event);

        return HANDLED_EVENT;
    case SDL_QUIT:
        quit(0);

        return HANDLED_EVENT;
    default:

        return HANDLED_EVENT;
    }
}

Sint32 level_editor()
{
    static LevelEditorData data;
    EditorTerrainBrush &terrain_brush = data.terrain_brush;
    EditorObjectBrush &object_brush = data.object_brush;
    EditModeEnum &mode = data.mode;
    Radar &myradar = data.myradar;
    Sint32 i;
    Sint32 j;
    Sint32 windowx;
    Sint32 windowy;
    Sint32 mx;
    Sint32 my;

    // Initialize palette for cycling
    load_and_set_palette("our.pal", scenpalette);

    if (data.reloadCampaign()) {
        Log("Loaded campaign data successfully.\n");
    } else {
        Log("Failed to load campaign data.\n");
    }

    std::string old_campaign(get_mounted_campaign());

    if (!old_campaign.empty()) {
        unmount_campaign_package(old_campaign);
    }

    mount_campaign_package(data.campaign->id);

    std::list<Sint32> levels = list_levels();

    if (!levels.empty()) {
        if (data.loadLevel(levels.front())) {
            Log("Loaded level data successfully.\n");
        } else {
            Log("Failed to load level data.\n");
        }
    } else {
        Log("Campaign has no valid levels!");
    }

    // Redraw right away
    redraw = 1;
    object_pane.clear();

    for (Sint32 i = 0; i < NUM_FAMILIES; ++i) {
        object_pane.push_back(ObjectType(ORDER_LIVING, i));
    }

    for (Sint32 i = 0; i < (MAX_TREASURE + 1); ++i) {
        object_pane.push_back(ObjectType(ORDER_TREASURE, i));
    }

    for (Sint32 i = 0; i < 4; ++i) {
        object_pane.push_back(ObjectType(ORDER_GENERATOR, i));
    }

    object_pane.push_back(ObjectType(ORDER_WEAPON, FAMILY_DOOR));
    object_pane.push_back(ObjectType(ORDER_SPECIAL, FAMILY_RESERVED_TEAM));

    // Minimap
    myradar.start(data.level);

    data.reset_mode_buttons();

    MouseState &mymouse = query_mouse_no_poll();

#ifdef USE_CONTROLLER_INPUT
    mymouse.x = 160;
    mymouse.y = 100;
#endif

    mouse_last_x = mymouse.x;
    mouse_last_y = mymouse.y;

    float cycletimer = 0.0f;
    grab_mouse();
    Uint32 last_ticks = SDL_GetTicks();
    Uint32 start_ticks = last_ticks;

    // This is the main program loop
    bool done = false;
    SDL_Event event;

    while (!done) {
        // Reset the timer count to zero...
        reset_timer();

        if (myscreen->end) {
            done = true;

            break;
        }

        while (SDL_PollEvent(&event)) {
#ifdef USE_CONTROLLER_INPUT
            if (didPlayerPressKey(0, KEY_FIRE, event)) {
                // Send fake mouse down event
                SDL_Event event;

                event.type = SDL_MOUSEBUTTONDOWN;
                event.button.button = SDL_BUTTON_LEFT;
                event.button.x = (mymouse.x * (viewport_w / 320)) + viewport_offset_x;
                event.button.y = (mymouse.y * (viewport_h / 200)) + viewport_offset_y;
                SDL_PushEvent(&event);

                continue;
            }

            if (didPlayerReleaseKey(0, KEY_FIRE, event)) {
                // Send fake mouse up event
                SDL_Event event;

                event.type = SDL_MOUSEBUTTONUP;
                event.button.button = SDL_BUTTON_LEFT;
                event.button.x = (mymouse.x * (viewport_w / 320)) + viewport_offset_x;
                event.button.y = (mymouse.y * (viewport_h / 200)) + viewport_offset_y;
                SDL_PushEvent(&event);

                continue;
            }
#endif

            switch (handle_basic_editor_event(event)) {
            case MOUSE_MOTION_EVENT:
                data.mouse_motion(mymouse.x, mymouse.y, mouse_motion_x, mouse_motion_y);

                break;
            case MOUSE_DOWN_EVENT:
                if (mymouse.left) {
                    mouse_last_x = mymouse.x;
                    mouse_last_y = mymouse.y;

                    data.mouse_down(mymouse.x, mymouse.y);
                }

                break;
            case MOUSE_UP_EVENT:
                if (mouse_up_button == MOUSE_LEFT) {
                    data.mouse_up(mymouse.x, mymouse.y, mouse_last_x, mouse_last_y, done);
                    redraw = 1;
                } else if (mouse_up_button == MOUSE_RIGHT) {
                    // Picking with right mouse button
                    data.pick_by_mouse(mymouse.x, mymouse.y);
                    redraw = 1;
                }

                break;
            case KEY_DOWN_EVENT:
                redraw = 1;

                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if ((!levelchanged && !campaignchanged)
                        || yes_or_no_prompt("Exit", "Quit without saving?", false)) {
                        done = true;

                        break;
                    }

                    // Change teams...
                } else if (event.key.keysym.sym == SDLK_0) {
                    object_brush.team = 0;
                } else if (event.key.keysym.sym == SDLK_1) {
                    object_brush.team = 1;
                } else if (event.key.keysym.sym == SDLK_2) {
                    object_brush.team = 2;
                } else if (event.key.keysym.sym == SDLK_3) {
                    object_brush.team = 3;
                } else if (event.key.keysym.sym == SDLK_4) {
                    object_brush.team = 4;
                } else if (event.key.keysym.sym == SDLK_5) {
                    object_brush.team = 5;
                } else if (event.key.keysym.sym == SDLK_6) {
                    object_brush.team = 6;
                } else if (event.key.keysym.sym == SDLK_7) {
                    object_brush.team = 7;
                } else if (event.key.keysym.sym == SDLK_g) {
                    // Toggle grid alignment
                    if ((mode == OBJECT) || (mode == SELECT)) {
                        data.activate_mode_button(&data.gridSnapButton);
                    }
                } else if ((event.key.keysym.sym == SDLK_s) && (event.key.keysym.mod & KMOD_CTRL)) {
                    // Save scenario
                    bool saved = false;

                    if (levelchanged) {
                        if (data.saveLevel()) {
                            levelchanged = 0;
                            saved = true;
                        } else {
                            timed_dialog("Failed to save level.");
                        }
                    }

                    if (campaignchanged) {
                        if (data.saveCampaign()) {
                            campaignchanged = 0;
                            saved = true;
                        } else {
                            timed_dialog("Failed to save campaign.");
                        }
                    }

                    if (saved) {
                        timed_dialog("Saved.");
                    } else if (!levelchanged && !campaignchanged) {
                        timed_dialog("No changes to save.");
                    }

                    // End of saving routines
                } else if (event.key.keysym.sym == SDLK_RIGHTBRACKET) {
                    // Change level of current guy being placed...
                    if (mode == OBJECT) {
                        ++object_brush.level;
                    }
                } else if (event.key.keysym.sym == SDLK_LEFTBRACKET) {
                    if ((mode == OBJECT) && (object_brush.level > 1)) {
                        --object_brush.level;
                    }
                } else if (event.key.keysym.sym == SDLK_DELETE) {
                    if (mode == SELECT) {
                        data.activate_mode_button(&data.deleteButton);
                    }
                } else if (event.key.keysym.sym == SDLK_o) {
                    if (mode == OBJECT) {
                        mode = SELECT;
                        data.modeButton.label = "Edit (Select)";
                    } else {
                        mode = OBJECT;
                        data.modeButton.label = "Edit (Objects)";
                    }

                    data.reset_mode_buttons();
                } else if (event.key.keysym.sym == SDLK_t) {
                    if (mode == TERRAIN) {
                        mode = SELECT;
                        data.modeButton.label = "Edit (Select)";
                    } else {
                        mode = TERRAIN;
                        data.modeButton.label = "Edit (Terrain)";
                    }

                    data.reset_mode_buttons();
                } else if (event.key.keysym.sym == SDLK_F5) {
                    // Smooth current map, F5
                    data.resmooth_terrain();
                    levelchanged = 1;
                } else if (event.key.keysym.sym == SDLK_F9) {
                    // Change to new palette...
                    load_and_set_palette("our.pal", scenpalette);
                }

                break;
            default:

                break;
            }
        }

#ifdef USE_CONTROLLER_INPUT
        Sint32 dx = 0;
        Sint32 dy = 0;
        OuyaController &c = OuyaControllerManager::getController(0);

        if (!c.isStickBeyondDeadzone(OuyaController::AXIS_LS_X)) {
            if (isPlayerHoldingKey(0, KEY_UP)
                || isPlayerHoldingKey(0, KEY_UP_LEFT)
                || isPlayerHoldingKey(0, KEY_UP_RIGHT)) {
                dy = -5;
            }

            if (isPlayerHoldingKey(0, KEY_DOWN)
                || isPlayerHoldingKey(0, KEY_DOWN_LEFT)
                || isPlayerHoldingKey(0, KEY_DOWN_RIGHT)) {
                dy = 5;
            }

            if (isPlayerHoldingKey(0, KEY_LEFT)
                || isPlayerHoldingKey(0, KEY_UP_LEFT)
                || isPlayerHoldingKey(0, KEY_DOWN_LEFT)) {
                dx = -5;
            }

            if (isPlayerHoldingKey(0, KEY_RIGHT)
                || isPlayerHoldingKey(0, KEY_UP_RIGHT)
                || isPlayerHoldingKey(0, KEY_DOWN_RIGHT)) {
                dx = 5;
            }
        } else {
            dx = 5 * c.getAxisValue(OuyaController::AXIS_LS_X);
            dy = 5 * c.getAxisvalue(OuyaController::AXIS_LS_Y);
        }

        if ((mymouse.x + dx) < 0) {
            mymouse.x = 0;
        }

        if ((mymouse.x + dx) > 320) {
            mymouse.x = 320;
        }

        if ((mymouse.y + dy) < 0) {
            mymouse.y = 0;
        }

        if ((mymouse.y + dy) > 200) {
            mymouse.y = 200;
        }

        if ((dx != 0) || (dy != 0)) {
            Sint32 x;
            Sint32 y;
            SDL_Event event;

            event.type = SDL_MOUSEMOTION;
            event.motion.type = SDL_MOUSE_MOTION;
            event.motion.windowID = 0;
            event.motion.which = 0;
            event.motion.state = SDL_GetMouseState(&x, &y);
            event.motion.xrel = dx * (viewport_w / 320);
            event.motion.yrel = dy * (viewport_h / 200);
            event.motion.x = ((mymouse.x * (viewport_w / 320)) + viewport_offset_x) + event.motion.xrel;
            event.motion.y = ((mymouse.y * (viewport_h / 200)) + viewport_offset_y) + event.motion.yrel;
            SDL_PushEvent(&event);
        }

#ifdef OUYA
        Ouyacontroller const &c = OuyaControllerManager::getController(0);
        float vx = c.getAxisValue(OuyaController::AXIS_RS_X);
        float vy = c.getAxisValue(OuyaController::AXIS_RS_Y);

        // Scroll the tile selector when over it
        if (Rect(S_RIGHT, PIX_TOP, 4 * GRID_SIZE, 4 * GRID_SIZE).contains(mymouse.x, mymouse.y)) {
            if (fabs(vy) > OuyaController::DEADZONE) {
                scroll_amount = -2 * vy;
            } else {
                scroll_amount = 0;
            }
        } else {
            scroll_amount = 0;

            // Panning
            if (vx < -OuyaController::DEADZONE) {
                pan_left = true;
            } else {
                pan_left = false;
            }

            if (vx > Ouyacontroller::DEADZONE) {
                pan_right = true;
            } else {
                pan_right = false;
            }

            if (vy < -OuyaController::DEADZONE) {
                pan_up = true;
            } else {
                pan_up = false;
            }

            if (vy > OuyaController::DEADZONE) {
                pan_down = true;
            } else {
                pan_down = false;
            }
        }
#endif
#endif

        Sint16 scroll_amount = get_and_reset_scroll_amount();

        bool scroll = true;

#if defined(USE_TOUCH_PAD)
        scroll = false;

        // Only scroll the tile selector when touching it and you've already
        // moved a bi
        if (mymouse.left
            && Rect(S_RIGHT, PIX_TOP, 4 * GRID_SIZE, 4 * GRID_SIZE).contains(mymouse.x, mymouse.y)
            && (fabs(mouse_last_y - mymouse.y) > 4)) {
            scroll = true;
        }
#endif

        if (scroll && (keystates[KEYSTATE_DOWN] || (scroll_amount < 0))) {
            ++rowsdown;

            if (rowsdown >= maxrows) {
                rowsdown -= maxrows;
            }

            redraw = 1;

            while (keystates[KEYSTATE_DOWN]) {
                get_input_events(WAIT);
            }
        }

        // Slide tile selector up...
        if (keystates[KEYSTATE_UP] || (scroll_amount > 0)) {
            --rowsdown;

            if (rowsdown < 0) {
                rowsdown += maxrows;
            }

            // Bad case
            if ((rowsdown < 0) || (rowsdown >= maxrows)) {
                rowsdown = 0;
            }

            redraw = 1;

            while (keystates[KEYSTATE_UP]) {
                get_input_events(WAIT);
            }
        }

        // Scroll the screen (panning)
#ifndef OUYA
        if (keystates[KEYSTATE_KP_4]
            || keystates[KEYSTATE_KP_7]
            || keystates[KEYSTATE_KP_1]
            || keystates[KEYSTATE_a]) {
            pan_left = true;
        } else {
            pan_left = false;
        }

        if (keystates[KEYSTATE_KP_6]
            || keystates[KEYSTATE_KP_3]
            || keystates[KEYSTATE_KP_9]
            || keystates[KEYSTATE_d]) {
            pan_right = true;
        } else {
            pan_right = false;
        }

        if (keystates[KEYSTATE_KP_8]
            || keystates[KEYSTATE_KP_7]
            || keystates[KEYSTATE_KP_9]
            || keystates[KEYSTATE_w]) {
            pan_up = true;
        } else {
            pan_up = false;
        }

        if (keystates[KEYSTATE_KP_2]
            || keystates[KEYSTATE_KP_1]
            || keystates[KEYSTATE_KP_3]
            || keystates[KEYSTATE_s]) {
            pan_down = true;
        } else {
            pan_down = false;
        }
#endif

        // Top of the screen
        if (pan_up && (data.level->topy >= PAN_LIMIT_UP)) {
            redraw = 1;
            data.level->add_draw_pos(0, -SCROLLSIZE);
        }

        // Scroll down
        if (pan_down && (data.level->topy <= PAN_LIMIT_DOWN)) {
            redraw = 1;
            data.level->add_draw_pos(0, SCROLLSIZE);
        }

        // Scroll left
        if (pan_left && (data.level->topx >= PAN_LIMIT_LEFT)) {
            redraw = 1;
            data.level->add_draw_pos(-SCROLLSIZE, 0);
        }

        // Scroll right
        if (pan_right && (data.level->topx <= PAN_LIMIT_RIGHT)) {
            redraw = 1;
            data.level->add_draw_pos(SCROLLSIZE, 0);
        }

        // Mouse stuff...
        mymouse = query_mouse_no_poll();

        // Put or remove the current guy
        if (mymouse.left) {
            redraw = 1;
            mx = mymouse.x;
            my = mymouse.y;

            // Hodling on menu items
            bool mouse_on_menu = data.mouse_on_menus(mx, my);
            bool old_mouse_on_menu = data.mouse_on_menus(mouse_last_x, mouse_last_y);
            bool on_menu = mouse_on_menu && old_mouse_on_menu;
            bool off_menu = !mouse_on_menu && !old_mouse_on_menu;

            if (on_menu) {
                // Panning with mouse (touch)
                // Top of the screen
                if (data.panUpButton.contains(mx, my) && (data.level->topy >= PAN_LIMIT_UP)) {
                    redraw = 1;
                    data.level->add_draw_pos(0, -SCROLLSIZE);
                } else if (data.panUpRightButton.contains(mx, my)) {
                    redraw = 1;

                    if (data.level->topy >= PAN_LIMIT_UP) {
                        data.level->add_draw_pos(0, -SCROLLSIZE);
                    }

                    if (data.level->topx <= PAN_LIMIT_RIGHT) {
                        data.level->add_draw_pos(SCROLLSIZE, 0);
                    }
                } else if (data.panUpLeftButton.contains(mx, my)) {
                    redraw = 1;

                    if (data.level->topy >= PAN_LIMIT_UP) {
                        data.level->add_draw_pos(0, -SCROLLSIZE);
                    }

                    if (data.level->topx >= PAN_LIMIT_LEFT) {
                        data.level->add_draw_pos(-SCROLLSIZE, 0);
                    }
                } else if (data.panDownButton.contains(mx, my) && (data.level->topy <= PAN_LIMIT_DOWN)) {
                    // Scroll down
                    redraw = 1;
                    data.level->add_draw_pos(0, SCROLLSIZE);
                } else if (data.panDownRightButton.contains(mx, my)) {
                    redraw = 1;

                    if (data.level->topy <= PAN_LIMIT_DOWN) {
                        data.level->add_draw_pos(0, SCROLLSIZE);
                    }

                    if (data.level->topx <= PAN_LIMIT_RIGHT) {
                        data.level->add_draw_pos(SCROLLSIZE, 0);
                    }
                } else if (data.panDownLeftButton.contains(mx, my)) {
                    redraw = 1;

                    if (data.level->topy <= PAN_LIMIT_DOWN) {
                        data.level->add_draw_pos(0, SCROLLSIZE);
                    }

                    if (data.level->topx >= PAN_LIMIT_LEFT) {
                        data.level->add_draw_pos(-SCROLLSIZE, 0);
                    }
                } else if (data.panLeftButton.contains(mx, my) && (data.level->topx >= PAN_LIMIT_LEFT)) {
                    // Scroll left
                    redraw = 1;
                    data.level->add_draw_pos(-SCROLLSIZE, 0);
                } else if (data.panRightButton.contains(mx, my) && (data.level->topx <= PAN_LIMIT_RIGHT)) {
                    // Scroll right
                    redraw = 1;
                    data.level->add_draw_pos(SCROLLSIZE, 0);
                }
            } else if (off_menu) {
                // Zardus: ADD: Can move map by clicking on minimap
                if (((mode != SELECT) || (!data.rect_selecting && !data.dragging))
                    && (mx > ((myscreen->viewob[0]->endx - myradar.xview) - 4))
                    && (my > ((myscreen->viewob[0]->endy - myradar.yview) - 4))
                    && (mx < (myscreen->viewob[0]->endx - 4))
                    && (my < (myscreen->viewob[0]->endy - 4))) {
                    mx -= ((myscreen->viewob[0]->endx - myradar.xview) - 4);
                    my -= ((myscreen->viewob[0]->endy - myradar.yview) - 4);

                    // Zardus: Above set_screen_pos doesn't take into account
                    // that minimap scrolls too. This one does.
                    data.level->set_draw_pos(((myradar.radarx * GRID_SIZE) + (mx * GRID_SIZE)) - 160, ((myradar.radary * GRID_SIZE) + (my * GRID_SIZE) - 100));
                } else {
                    // In the main window
                    windowx = (mymouse.x + data.level->topx) - myscreen->viewob[0]->xloc; // - S_LEFT
                    windowx -= (windowx % GRID_SIZE);
                    windowy = (mymouse.y + data.level->topy) - myscreen->viewob[0]->yloc; // - S_UP
                    windowy -= (windowy % GRID_SIZE);

                    if (mode == TERRAIN) {
                        if ((mx >= S_RIGHT) && (my >= PIX_TOP) && (my <= PIX_BOTTOM)) {
                            // Picking the tile is done in LevelEditorData::mouse_up()
                            // End of the background grid window
                        } else {
                            // Get the map position...
                            windowx /= GRID_SIZE;
                            windowy /= GRID_SIZE;

                            if (!terrain_brush.picking) {
                                // Set to our current selection (apply brush)
                                data.set_terrain(windowx, windowy, get_random_matching_tile(terrain_brush.terrain));
                                levelchanged = 1;

                                // Smooth a few squares, if not control
                                if (terrain_brush.use_smoothing) {
                                    for (i = (windowx - 1); i <= (windowx + 1); ++i) {
                                        for (j = (windowy - 1); j <= (windowy + 1); ++j) {
                                            if ((i >= 0)
                                                && (i < data.level->grid.w)
                                                && (j >= 0)
                                                && (j < data.level->grid.h)) {
                                                data.level->mysmoother.smooth(i, j);
                                            }
                                        }
                                    }
                                }

                                myradar.update(data.level);
                            }
                        }
                    } // End of setting grid square
                } // End of main window
            }
        } // End of left mouse button

        // No perform color cycling if selected
        if (cyclemode) {
            cycletimer -= ((start_ticks - last_ticks) / 1000.0f);

            if (cycletimer <= 0) {
                cycletimer = 0.5f;
                cycle_palette(scenpalette, WATER_START, WATER_END, 1);
                cycle_palette(scenpalette, ORANGE_START, ORANGE_END, 1);
            }

            redraw = 1;
        }

        // Redraw screen
        if (redraw) {
            redraw = 0;
            data.draw(myscreen);

#ifdef USE_CONTROLLER_INPUT
            myscreen->fastbox(mymouse.x - 1, mymouse.y - 1, 4, 4, PURE_WHITE);
            myscreen->fastbox(mymouse.x, mymouse.y, 2, 2, PURE_BLACK);
#endif

            myscreen->refresh();
        }

        SDL_Delay(10);

        last_ticks = start_ticks;
        start_ticks = SDL_GetTicks();
    }

    // Reset the screen position so it doesn't ruin the main menu
    data.level->set_draw_pos(0, 0);
    // Update the screen's potion
    data.level->draw(myscreen);
    // Clear the background
    myscreen->clearbuffer();

    unmount_campaign_package(data.campaign->id);
    mount_campaign_package(old_campaign);

    return OK;
}

void set_screen_pos(VideoScreen *myscreen, Sint32 x, Sint32 y)
{
    myscreen->level_data.topx = x;
    myscreen->level_data.topy = y;
    redraw = 1;
}

Uint8 get_random_matching_tile(Sint32 whatback)
{
    Sint32 i;

    // Max number of types of any particular...
    i = random(4);

    switch (whatback) {
    case PIX_GRASS1:
        switch (i) {
        case 0:

            return PIX_GRASS1;
        case 1:

            return PIX_GRASS2;
        case 2:

            return PIX_GRASS3;
        case 3:

            return PIX_GRASS4;
        default:

            return PIX_GRASS1;
        }

        break;
    case PIX_GRASS_DARK_1:
        switch (i) {
        case 0:

            return PIX_GRASS_DARK_1;
        case 1:

            return PIX_GRASS_DARK_2;
        case 2:

            return PIX_GRASS_DARK_3;
        case 3:

            return PIX_GRASS_DARK_4;
        default:

            return PIX_GRASS_DARK_1;
        }

        break;
    case PIX_GRASS_DARK_B1:
    case PIX_GRASS_DARK_B2:
        switch (i) {
        case 0:
        case 1:

            return PIX_GRASS_DARK_B1;
        case 2:
        case 3:
        default:

            return PIX_GRASS_DARK_B2;
        }

        break;
    case PIX_GRASS_DARK_R1:
    case PIX_GRASS_DARK_R2:
        switch (i) {
        case 0:
        case 1:

            return PIX_GRASS_DARK_R1;
        case 2:
        case 3:
        default:

            return PIX_GRASS_DARK_R2;
        }

        break;
    case PIX_WATER1:
        switch (i) {
        case 0:

            return PIX_WATER1;
        case 1:

            return PIX_WATER2;
        case 2:

            return PIX_WATER3;
        default:

            return PIX_WATER1;
        }

        break;
    case PIX_PAVEMENT1:
        switch (random(12)) {
        case 0:

            return PIX_PAVEMENT1;
        case 1:

            return PIX_PAVEMENT2;
        case 2:

            return PIX_PAVEMENT3;
        default:

            return PIX_PAVEMENT1;
        }

        break;
    case PIX_COBBLE_1:
        switch (random(i)) {
        case 0:

            return PIX_COBBLE_1;
        case 1:

            return PIX_COBBLE_2;
        case 2:

            return PIX_COBBLE_3;
        case 3:

            return PIX_COBBLE_4;
        default:

            return PIX_COBBLE_1;
        }

        break;
    case PIX_BOULDER_1:
        switch (random(i)) {
        case 0:

            return PIX_BOULDER_1;
        case 1:

            return PIX_BOULDER_2;
        case 2:

            return PIX_BOULDER_3;
        case 3:

            return PIX_BOULDER_4;
        default:

            return PIX_BOULDER_1;
        }

        break;
    case PIX_JAGGED_GROUND_1:
        switch (i) {
        case 0:

            return PIX_JAGGED_GROUND_1;
        case 1:

            return PIX_JAGGED_GROUND_2;
        case 2:

            return PIX_JAGGED_GROUND_3;
        case 3:

            return PIX_JAGGED_GROUND_4;
        default:

            return PIX_JAGGED_GROUND_1;
        }

        break;
    default:

        return whatback;
    }
}

// Copy of collide from obmap, used manually... :(
Sint32 check_collide(Sint32 x, Sint32 y, Sint32 xsize, Sint32 ysize,
                     Sint32 x2, Sint32 y2, Sint32 xsize2, Sint32 ysize2)
{
    if (x < x2) {
        if (y < y2) {
            if (((x2 - x) < xsize)  && ((y2 - y) < ysize)) {
                return 1;
            }
        } else {
            // y >= y2
            if (((x2 - x) < xsize) && ((y - y2) < ysize2)) {
                return 1;
            }
        }
    } else {
        // x >= x2
        if (y < y2) {
            if (((x - x2) < xsize2) && ((y2 - y) < ysize)) {
                return 1;
            }
        } else {
            // y >= y2
            if (((x - x2) < xsize2) && ((y - y2) < ysize2)) {
                return 1;
            }
        }
    }

    return 0;
}

// The old-fashioned hit check...
bool some_hit(Sint32 x, Sint32 y, Walker *ob, LevelData *data)
{
    for (auto const &w : data->oblist) {
        if (w && (w != ob) && check_collide(x, y, ob->sizex, ob->sizey, w->xpos, w->ypos, w->sizex, w->sizey)) {
            ob->collide_ob = w;

            return true;
        }
    }

    for (auto const &w : data->fxlist) {
        if (w && (w != ob) && check_collide(x, y, ob->sizex, ob->sizey, w->xpos, w->ypos, w->sizex, w->sizey)) {
            ob->collide_ob = w;

            return true;
        }
    }

    for (auto const &w : data->weaplist) {
        if (w && (w != ob) && check_collide(x, y, ob->sizex, ob->sizey, w->xpos, w->ypos, w->sizex, w->sizey)) {
            ob->collide_ob = w;

            return true;
        }
    }

    ob->collide_ob = nullptr;

    return false;
}
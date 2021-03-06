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
#ifndef __CAMPAIGN_RESULT_HPP__
#define __CAMPAIGN_RESULT_HPP__

#include <filesystem>
#include <map>

#include <SDL2/SDL.h>

#include "save_data.hpp"

class CampaignResult
{
public:
    std::filesystem::path id;
    Sint32 first_level;

    CampaignResult();
};

CampaignResult pick_campaign(SaveData *save_data, bool enable_delete = false);

Sint32 load_campaign(std::filesystem::path const &campaign,
                     std::map<std::filesystem::path, int> &current_levels,
                     Sint32 first_level = 1);

#endif // __CAMPAIGN_RESULT_HPP__

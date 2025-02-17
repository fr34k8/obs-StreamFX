/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2018 Michael Fabian Dirks
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "obs-tools.hpp"
#include "obs-source.hpp"
#include "obs-weak-source.hpp"
#include "plugin.hpp"

#include "warning-disable.hpp"
#include <map>
#include <set>
#include <stdexcept>
#include "warning-enable.hpp"

struct __sfs_data {
	std::set<::streamfx::obs::weak_source> sources;
};

void __source_find_source_enumerate(obs_source_t* haystack, __sfs_data* cbd)
{
	auto tp = obs_source_get_type(haystack);

	// Check if this source is already present in the set.
	::streamfx::obs::weak_source weak_child{haystack};
	if (!weak_child || (cbd->sources.find(weak_child) != cbd->sources.end())) {
		return;
	}

	// If it was not in the list, add it now.
	cbd->sources.insert(weak_child);

	// Enumerate direct reference tree.
	obs_source_enum_full_tree(
		haystack,
		[](obs_source_t* parent, obs_source_t* child, void* param) {
			try {
				__source_find_source_enumerate(child, reinterpret_cast<__sfs_data*>(param));
			} catch (...) {
			}
		},
		cbd);

	switch (tp) {
	case OBS_SOURCE_TYPE_SCENE: {
		obs_scene_enum_items(
			obs_scene_from_source(haystack),
			[](obs_scene_t* scene, obs_sceneitem_t* item, void* param) {
				try {
					__sfs_data* cbd = reinterpret_cast<__sfs_data*>(param);
					__source_find_source_enumerate(obs_sceneitem_get_source(item), cbd);
					return true;
				} catch (...) {
					return true;
				}
			},
			cbd);
	}
#if __cplusplus >= 201700L
		[[fallthrough]];
#endif
	case OBS_SOURCE_TYPE_INPUT: {
		// Enumerate filter tree.
		obs_source_enum_filters(
			haystack,
			[](obs_source_t* parent, obs_source_t* child, void* param) {
				try {
					__sfs_data* cbd = reinterpret_cast<__sfs_data*>(param);
					__source_find_source_enumerate(child, cbd);
				} catch (...) {
				}
			},
			cbd);
	}
#if __cplusplus >= 201700L
		[[fallthrough]];
#endif
	default:
		break;
	}
}

bool streamfx::obs::tools::source_find_source(::streamfx::obs::source haystack, ::streamfx::obs::source needle)
{
	__sfs_data cbd = {};
	try {
		__source_find_source_enumerate(haystack.get(), &cbd);
	} catch (...) {
	}

	for (auto weak_source : cbd.sources) {
		if (!weak_source)
			continue;

		if (weak_source == needle)
			return true;
	}

	return false;
}

/*
    Copyright 2026 Davey Hughes

    melonDS DS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS DS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS DS. If not, see http://www.gnu.org/licenses/.
*/
#ifndef MELONDS_DS_LIBRETRO_VIEWS_H
#define MELONDS_DS_LIBRETRO_VIEWS_H

#include <limits.h>
#include <libretro.h>

// RetroArch's video views API as of libretro/RetroArch#19811,
// until the libretro.h that libretro-common ships has it.
#ifndef RETRO_ENVIRONMENT_SET_VIDEO_VIEWS

#define RETRO_ENVIRONMENT_SET_VIDEO_VIEWS (95 | RETRO_ENVIRONMENT_EXPERIMENTAL)
#define RETRO_ENVIRONMENT_GET_VIDEO_VIEWS_STATUS (96 | RETRO_ENVIRONMENT_EXPERIMENTAL)

#define RETRO_VIDEO_VIEWS_STATUS_PRESENTS (1 << 0)
#define RETRO_VIDEO_VIEWS_STATUS_STEREO (1 << 1)
// A headset is tracking; only set together with PRESENTS and STEREO.
#define RETRO_VIDEO_VIEWS_STATUS_HMD (1 << 2)

#define RETRO_VIDEO_VIEW_EYE_NONE 0
#define RETRO_VIDEO_VIEW_EYE_LEFT 1
#define RETRO_VIDEO_VIEW_EYE_RIGHT 2
#define RETRO_VIDEO_VIEWS_MAX 8
// The map's LEFT and RIGHT views were drawn from the frontend's eye poses; melonDS DS's never are.
#define RETRO_VIDEO_VIEWS_FLAG_TRACKED (1 << 0)

enum retro_vr_reference_space {
    RETRO_VR_REFERENCE_SPACE_LOCAL = 0,
    RETRO_VR_REFERENCE_SPACE_STAGE = 1,
    RETRO_VR_REFERENCE_SPACE_DUMMY = INT_MAX
};

struct retro_video_view {
    unsigned x;
    unsigned y;
    unsigned width;
    unsigned height;
    unsigned screen;
    unsigned eye;
    float aspect_ratio;
};

struct retro_video_views {
    const struct retro_video_view* views;
    unsigned num_views;
    unsigned flags;
    enum retro_vr_reference_space reference_space;
    // Outputs: the frontend writes these back while a headset is tracking.
    unsigned recommended_view_width;
    unsigned recommended_view_height;
};

#endif

#endif //MELONDS_DS_LIBRETRO_VIEWS_H

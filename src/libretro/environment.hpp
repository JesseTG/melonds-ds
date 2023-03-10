//
// Created by Jesse on 3/6/2023.
//

#ifndef MELONDS_DS_ENVIRONMENT_HPP
#define MELONDS_DS_ENVIRONMENT_HPP

#include <string>
#include <libretro.h>

namespace retro {
    /// For use by other parts of the core
    bool environment(unsigned cmd, void *data);

    void log(enum retro_log_level level, const char *fmt, ...);

    bool supports_bitmasks();
    void input_poll();
    int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id);
    size_t audio_sample_batch(const int16_t *data, size_t frames);
    void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch);

}

#endif //MELONDS_DS_ENVIRONMENT_HPP

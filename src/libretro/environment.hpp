//
// Created by Jesse on 3/6/2023.
//

#ifndef MELONDS_DS_ENVIRONMENT_HPP
#define MELONDS_DS_ENVIRONMENT_HPP

#include <string>

namespace retro {
    /// For use by other parts of the core
    bool environment(unsigned cmd, void *data);

    void log(enum retro_log_level level, const char *fmt, ...);

    bool supports_bitmasks();

    const std::string& base_directory();
    const std::string& save_directory();
}

#endif //MELONDS_DS_ENVIRONMENT_HPP

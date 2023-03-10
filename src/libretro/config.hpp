//
// Created by Jesse on 3/6/2023.
//

#ifndef MELONDS_DS_CONFIG_HPP
#define MELONDS_DS_CONFIG_HPP

namespace melonds {
    bool update_option_visibility();
    void check_variables(bool init);
    extern struct retro_core_options_v2 options_us;
    extern struct retro_core_option_v2_definition option_defs_us[];
#ifndef HAVE_NO_LANGEXTRA
    extern struct retro_core_options_v2 *options_intl[];
#endif

    enum ConsoleType {
        DS = 0,
        DSi = 1,
    };

    enum class ScreenSwapMode {
        Hold,
        Toggle,
    };

    enum RendererType {
        Software,
        OpenGl,
    };

    GPU::RenderSettings &render_settings();
}

#endif //MELONDS_DS_CONFIG_HPP

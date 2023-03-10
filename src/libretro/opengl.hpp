//
// Created by Jesse on 3/7/2023.
//

#ifndef MELONDS_DS_OPENGL_HPP
#define MELONDS_DS_OPENGL_HPP

namespace melonds {
    enum class CurrentRenderer
    {
        None,
        Software,
        OpenGLRenderer,
    };

    extern CurrentRenderer current_renderer;
}

namespace melonds::opengl {
    bool initialize();

    void deinitialize();

    /// Returns true if OpenGL is configured to be used \em and is actively being used.
    bool using_opengl();

    void render_frame(bool software);
}
#endif //MELONDS_DS_OPENGL_HPP

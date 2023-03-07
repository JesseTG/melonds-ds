//
// Created by Jesse on 3/7/2023.
//

#ifndef MELONDS_DS_GPU_HPP
#define MELONDS_DS_GPU_HPP

namespace melonds {
    enum class CurrentRenderer
    {
        None,
        Software,
        OpenGLRenderer,
    };

    extern CurrentRenderer current_renderer;
}
#endif //MELONDS_DS_GPU_HPP

#ifndef MELONDS_DS_LIBRETRO_HPP
#define MELONDS_DS_LIBRETRO_HPP

#include <libretro.h>

/**!
 * Contains global state that's accessible to the entire core.
 */

#define PUBLIC_SYMBOL [[maybe_unused]]

#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif

#endif //MELONDS_DS_LIBRETRO_HPP

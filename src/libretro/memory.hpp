//
// Created by Jesse on 3/7/2023.
//

#ifndef MELONDS_DS_MEMORY_HPP
#define MELONDS_DS_MEMORY_HPP

namespace melonds {


    constexpr size_t DEFAULT_SERIALIZE_TEST_SIZE = 16 * 1024 * 1024; // 16 MiB

    void init_savestate_buffer(size_t length = DEFAULT_SERIALIZE_TEST_SIZE);

    void free_savestate_buffer();
}
#endif //MELONDS_DS_MEMORY_HPP

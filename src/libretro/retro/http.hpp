/*
    Copyright 2023 Jesse Talavera-Greenberg

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

#ifndef MELONDSDS_RETRO_HTTP_HPP
#define MELONDSDS_RETRO_HTTP_HPP

#include <string_view>
#include "std/span.hpp"

struct http_connection_t;
struct http_t;
namespace retro {
    class Http;
    class HttpConnection {
    public:
        HttpConnection(std::string_view url, std::string_view method, std::string_view data = {});
        ~HttpConnection() noexcept;
        HttpConnection(const HttpConnection&) = delete;
        HttpConnection(HttpConnection&&) = delete;
        HttpConnection& operator=(const HttpConnection&) = delete;
        HttpConnection& operator=(HttpConnection&&) = delete;

        bool Update(size_t& progress, size_t& total) noexcept;
        bool IsError() const noexcept;
        int Status() const noexcept;
        std::span<const std::byte> Data(bool acceptError) const noexcept;
    private:
        http_connection_t* _connection = nullptr;
        http_t* _http = nullptr;
    };
}

#endif // MELONDSDS_RETRO_HTTP_HPP

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

#include "http.hpp"

#include <net/net_http.h>
#include <retro_assert.h>

retro::HttpConnection::HttpConnection(std::string_view url, std::string_view method, std::string_view data) :
    _connection(net_http_connection_new(url.data(), method.data(), data.data())) {

    if (!_connection) {
        if (!(url.starts_with("http://") || url.starts_with("https://"))) {
            throw std::invalid_argument("URL must start with http:// or https://");
        }

        throw std::bad_alloc();
    }

    bool url_parsed = net_http_connection_iterate(_connection);
    retro_assert(url_parsed);

    // Signify that we're ready to send the request
    if (!net_http_connection_done(_connection)) {
        // If initializing the connection failed...
        throw std::exception();
        // TODO: Details
    }

    _http = net_http_new(_connection);
    if (!_http) {
        throw std::exception();
        // TODO: Details
    }
}

retro::HttpConnection::~HttpConnection() noexcept {
    if (_http) {
        net_http_delete(_http);
        _http = nullptr;
    }

    if (_connection) {
        net_http_connection_free(_connection);
        _connection = nullptr;
    }
}

bool retro::HttpConnection::Update(size_t& progress, size_t& total) noexcept {
    return net_http_update(_http, &progress, &total);
}

bool retro::HttpConnection::IsError() const noexcept {
    return net_http_error(_http);
}

int retro::HttpConnection::Status() const noexcept {
    return net_http_status(_http);
}

std::span<const std::byte> retro::HttpConnection::Data(bool acceptError) const noexcept {
    size_t length = 0;
    uint8_t* payload = net_http_data(_http, &length, acceptError);

    return std::span((const std::byte*)payload, length);
}
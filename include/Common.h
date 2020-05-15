/**
 *  AntStick -- communicate with an ANT+ USB stick
 *  Copyright (C) 2017 - 2020 Alex Harsanyi (AlexHarsanyi@gmail.com),
 *                            Alexey Kokoshnikov (alexeikokoshnikov@gmail.com)
 *                            Alexander Saechnikov (saechnikov.a@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <iostream>
#include <algorithm>
#include <sstream>

#include "Defaults.h"

// Show debug info
#define DEBUG

// Show functions call
#define LOG_FUNC_CALL

#if defined(DEBUG)

#include <string.h>

class LogMessageObject
{
public:
    LogMessageObject(std::string const &funcname, std::string const &path_to_file, unsigned line) {

        auto found = path_to_file.rfind("/");

        // Extra symbols make the output coloured
        std::cout << "+ \x1b[31m" << funcname << " \x1b[33m["
                  << (found == std::string::npos ? path_to_file : path_to_file.substr(found + 1))
                  << ":" << line << "]\x1b[0m" << std::endl;

        this->funcname_ = funcname;
    };

    ~LogMessageObject() {
        std::cout << "- \x1b[31m" << this->funcname_ << "\x1b[0m" << std::endl;
    };

private:
    std::string funcname_;
};

#define LOG_MSG(msg) std::cout << msg << std::endl;
#define LOG_ERR(msg) std::cerr << msg << std::endl;
#ifdef LOG_FUNC_CALL
    #define LOG_FUNC LogMessageObject lmsgo__(__func__, __FILE__, __LINE__);
#else
    #define LOG_FUNC
#endif
#else
#define LOG_MSG(msg)
#define LOG_ERR(msg)
#endif


inline uint8_t MessageChecksum (std::vector<uint8_t> const &msg)
{
    LOG_FUNC;

    uint8_t checksum = 0;
    std::for_each(msg.begin(), msg.end(), [&checksum](uint8_t item) { checksum ^= item; });

    return checksum;
}


inline std::vector<uint8_t> Message(ant::MessageId id, std::vector<uint8_t> const &data)
{
    LOG_FUNC;

    std::vector<uint8_t> yield;

    yield.push_back(static_cast<uint8_t>(ant::SYNC_BYTE));
    yield.push_back(static_cast<uint8_t>(data.size()));
    yield.push_back(static_cast<uint8_t>(id));
    yield.insert(yield.end(), data.begin(), data.end());
    yield.push_back(MessageChecksum(yield));

    return yield;
}


inline std::string MessageDump(const std::vector<uint8_t>& data)
{
    std::stringstream dump;

    for (auto itt = data.begin(); itt != data.end(); ++itt) {
        if (itt == data.begin()) dump << "0x" << std::hex; else dump << " 0x";
        dump << (unsigned)(*itt);
    }

    return dump.str();
}

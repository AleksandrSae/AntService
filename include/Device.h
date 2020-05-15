/**
 *  AntStick -- communicate with an ANT+ USB stick
 *  Copyright (C) 2017 - 2020 Alexander Saechnikov (saechnikov.a@gmail.com)
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

#include "Common.h"

class Device {
public:
    virtual bool Read(std::vector<uint8_t> &) = 0;
    virtual bool Write(std::vector<uint8_t> const &) = 0;
    virtual bool Connect() = 0;
    virtual bool IsConnected() = 0;
    virtual bool Disconnect() = 0;
    virtual ~Device() {}
};

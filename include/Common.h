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
class LogMessageObject {
public:
    LogMessageObject(const char * funcname, std::string file, int line) {
        std::cout << "+ \x1b[31m" << funcname << " \x1b[33m[" << file << ":" << line << "]\x1b[0m" << std::endl;
        this->m_funcname = funcname;
    };
    ~LogMessageObject() {
        std::cout << "- \x1b[31m" << this->m_funcname << "\x1b[0m" << std::endl;
    };
private:
    std::string m_funcname;
};
#define LOG_MSG(msg) std::cout << msg << std::endl;
#define LOG_ERR(msg) std::cerr << msg << std::endl;
#ifdef LOG_FUNC_CALL
#define LOG_FUNC LogMessageObject lmsgo__(__func__, __FILENAME__, __LINE__);
#else
#define LOG_FUNC
#endif
#else
#define LOG_MSG(msg)
#define LOG_ERR(msg)
#endif


inline uint8_t MessageChecksum (const std::vector<uint8_t>& msg)
{
    LOG_FUNC;

    uint8_t checksum = 0;
    std::for_each (msg.begin(), msg.end(), [&](uint8_t item) { checksum ^= item; });
    return checksum;
}


inline std::vector<uint8_t> Message(ant::MessageId id, const std::vector<uint8_t>& data) {
    LOG_FUNC;

    std::vector<uint8_t> yield;

    yield.push_back(static_cast<uint8_t>(ant::SYNC_BYTE));
    yield.push_back(static_cast<uint8_t>(data.size()));
    yield.push_back(static_cast<uint8_t>(id));
    yield.insert(yield.end(), data.begin(), data.end());
    yield.push_back(MessageChecksum(yield));

    return std::move(yield);
}


inline std::string MessageDump(const std::vector<uint8_t>& data) {
    std::stringstream dump;

    for (auto item : data) {
        dump << "0x" << std::hex << (unsigned)item << " ";
    }

    return dump.str();
}

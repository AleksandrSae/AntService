#pragma once

#include <vector>
#include <cstdint>

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

typedef std::vector<uint8_t> Buffer;
typedef uint8_t BufferItemType;


inline BufferItemType MessageChecksum (const Buffer& msg)
{
    LOG_FUNC;

    BufferItemType checksum = 0;
    std::for_each (msg.begin(), msg.end(), [&](BufferItemType item) { checksum ^= item; });
    return checksum;
}


inline Buffer Message(ant::MessageId id, const Buffer& data) {
    LOG_FUNC;

    Buffer yield;

    yield.push_back(static_cast<BufferItemType>(ant::SYNC_BYTE));
    yield.push_back(static_cast<BufferItemType>(data.size()));
    yield.push_back(static_cast<BufferItemType>(id));
    yield.insert(yield.end(), data.begin(), data.end());
    yield.push_back(MessageChecksum(yield));

    return std::move(yield);
}


inline std::string MessageDump(const Buffer& data) {
    std::stringstream dump;

    for (auto item : data) {
        dump << "0x" << std::hex << (unsigned)item << " ";
    }

    return dump.str();
}


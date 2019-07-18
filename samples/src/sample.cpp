/**
 *  TelemetryServer -- manage a bike trainer
 *  Copyright (C) 2019 Alexey Kokoshnikov (alexeikokoshnikov@gmail.com)
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
#include <stdio.h>
#include <chrono>
#include <mutex>
#include <thread>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include "TrainerControl.h"

template <typename T>
static inline T _print_err(T sts, const char* file, int line, const char* func)
{
    if (sts)
    {
        std::stringstream error;
        error << file << ": " << line << ": " << func << " ERROR: " << sts << "\n";
        throw std::runtime_error(error.str().c_str());
    }
}
#define CHECK_RES(sts) _print_err(sts, __FILE__, __LINE__, __FUNCTION__)

class AntServiceAPI {
protected:
    bool STOP_ALL;
    std::mutex m_Guard;
    std::mutex m_LocalGuard;
    void* m_pAntHandle;
    int m_MaxChannels;
    void* m_pSearchService;
    std::thread m_SearchThread;
    std::unordered_set<AntDevice*> m_ActiveDevices;
    AntDevice** m_ppDeviceList;
    std::unordered_map<AntSession*, std::thread*> m_ActiveSessions;
    std::unordered_map<AntSession*, Telemetry> m_Telemetry;
    std::list<std::string> m_FatalErrors;
    void UpdateActiveDevices() {
        unsigned int num_devices = 0;
        unsigned int num_active_devices = 0;
        if (-1 == GetDeviceList(m_pSearchService, m_ppDeviceList, num_devices, num_active_devices))
            return;
        m_ActiveDevices.clear();
        for (unsigned int i = 0; i < num_active_devices; i++) {
            m_ActiveDevices.insert(m_ppDeviceList[i]);
        }
    }
public:
    ~AntServiceAPI() {
        STOP_ALL = true;
        if (nullptr != m_pSearchService) {
            StopSearch(&m_pSearchService, m_SearchThread);
        }
        for (auto session : m_ActiveSessions) {
            StopSession(session.first);
        }
        m_ActiveSessions.clear();
        m_ActiveDevices.clear();
        for (int i = 0; i < m_MaxChannels; i++) {
            delete m_ppDeviceList[i];
            m_ppDeviceList[i] = nullptr;
        }
        delete m_ppDeviceList;
        m_ppDeviceList = nullptr;
        PrintErrors();
    }
    AntServiceAPI() :
        STOP_ALL(false),
        m_Guard(),
        m_LocalGuard(),
        m_pAntHandle(nullptr),
        m_MaxChannels(0),
        m_pSearchService(nullptr),
        m_SearchThread(),
        m_ActiveDevices(),
        m_ppDeviceList(nullptr),
        m_ActiveSessions(),
        m_FatalErrors()
    {
        try {
            // reinit AntStick
            CHECK_RES(InitAntService(&m_pAntHandle, m_MaxChannels));
            CHECK_RES(CloseAntService());

            CHECK_RES(InitAntService(&m_pAntHandle, m_MaxChannels));

            AntDevice** m_ppDeviceList = new AntDevice * [m_MaxChannels];
            for (int i = 0; i < m_MaxChannels; i++)
                m_ppDeviceList[i] = new AntDevice();
        }
        catch(std::runtime_error error) {
            m_FatalErrors.push_back(error.what());
            CloseAntService();
        }
    }
    int RunSearchService() {
        try {
            CHECK_RES(RunSearch(m_pAntHandle, &m_pSearchService, m_SearchThread, m_Guard));
        }
        catch(std::runtime_error error) {
            m_FatalErrors.push_back(error.what());
            StopSearch(&m_pSearchService, m_SearchThread);
            return -1;
        }
    }
    int AddDevice(AntDeviceType type) {
        try {
            CHECK_RES(AddDeviceForSearch(&m_pSearchService, type));
        }
        catch (std::runtime_error error) {
            m_FatalErrors.push_back(error.what());
            return -1;
        }
    }
    unsigned int GetNumActiveDevices() {
        UpdateActiveDevices();
        return m_ActiveDevices.size();
    }
    std::unordered_set<AntDevice*> GetActiveDevices() {
        UpdateActiveDevices();
        return m_ActiveDevices;
    }
    AntSession* RunSession(AntDevice* pDevice) {
        try {
            AntSession* ant_session = new AntSession();
            *ant_session = InitSession(m_pAntHandle, &pDevice, 1, m_Guard);
            if (nullptr == ant_session->m_AntStick ||
                nullptr == ant_session->m_TelemtryServer) {
                throw std::runtime_error("Initialization of Ant Session FAILED");
            }

            auto funk = [this, ant_session]()
            {
                CHECK_RES(Run(*ant_session));
                while (!STOP_ALL && ant_session->m_bIsRun)
                {
                    if (((std::thread*)ant_session->m_ServiceThread)->joinable()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        std::lock_guard<std::mutex> guard(m_LocalGuard);
                        m_Telemetry.insert(std::make_pair(ant_session, GetTelemetry(*ant_session)));
                    }
                    else {
                        CHECK_RES(Stop(*ant_session));
                    }
                }
            };
            std::thread* client_thread = new std::thread(funk);
            m_ActiveSessions.emplace(ant_session, client_thread);
            return ant_session;
        }
        catch (std::runtime_error error) {
            m_FatalErrors.push_back(error.what());
            return nullptr;
        }
    }
    int StopSession(AntSession* session) {
        if (nullptr == session) {
            return -1;
        }
        auto it = m_ActiveSessions.find(session);
        if (m_ActiveSessions.end() != it) {
            try {
                CHECK_RES(Stop(*it->first));
                if (it->second->joinable()) {
                    it->second->join();
                }
                CHECK_RES(CloseSession(*it->first));
                delete it->first;
                //it->first = nullptr;
                delete it->second;
                //it->second = nullptr;
                m_ActiveSessions.erase(it);
                return 0;
            }
            catch (std::runtime_error error) {
                m_FatalErrors.push_back(error.what());
                return -1;
            }
        }
        return -1;
    }
    Telemetry GetCurrTelemetry(AntSession* session) {
        std::lock_guard<std::mutex> guard(m_LocalGuard);
        auto it = m_Telemetry.find(session);
        if (m_Telemetry.end() != it) {
            return it->second;
        }
        return Telemetry();
    }
    int GetMaxChannels() {
        return m_MaxChannels;
    }
    unsigned int GetNumErrors() {
        return m_FatalErrors.size();
    }
    void PrintErrors() {
        for (auto err : m_FatalErrors) {
            printf("%s\n", err.c_str());
        }
        m_FatalErrors.clear();
    }
};

int main() {
    AntServiceAPI ant_service;
    if (ant_service.GetNumErrors()) {
        ant_service.PrintErrors();
        return -1;
    }
    if (ant_service.RunSearchService()) {
        ant_service.PrintErrors();
        return -1;
    }
    if (ant_service.AddDevice(HRM_Type)) {
        ant_service.PrintErrors();
        return -1;
    }
    while (0 == ant_service.GetNumActiveDevices()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    std::unordered_set<AntDevice*> active_devices = ant_service.GetActiveDevices();
    auto device_it = std::find_if(active_devices.begin(), active_devices.end(), [](AntDevice* item) { return item && item->m_type == HRM_Type; });
    if (active_devices.end() != device_it) {
        AntSession* session = ant_service.RunSession(*device_it);
        if (nullptr == session) {
            ant_service.PrintErrors();
            return -1;
        }
        while (session->m_bIsRun) {
            Telemetry t = ant_service.GetCurrTelemetry(session);
            printf("hr = %lf\n", t.hr);
        }
        if (ant_service.StopSession(session)) {
            ant_service.PrintErrors();
            return -1;
        }
    }
    return -1;
}

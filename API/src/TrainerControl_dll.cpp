/**
 *  TrainServer -- prototype bike trainer application
 *  Copyright (C) 2018 - 2019 Alexey Kokoshnikov (alexeikokoshnikov@gmail.com)
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
#include "stdafx.h"

#include "TrainerControl.h"

#include "AntStick.h"
#include "TelemetryServer.h"
#include "SearchService.h"
#include "Tools.h"
#include <algorithm>
#include <map>
#include <ctime>
#include <iomanip>
#include <iostream>

std::map<std::thread::id, bool *> stop_flags;

int InitAntService(void ** ant_instanance, int & max_channels)
{
    LOG_MSG("InitAntService()+\n");
    if (ant_instanance == nullptr)
        return -1;
    try {
        libusb_context *ctx = nullptr;
        int r = libusb_init(&ctx);
        if (r < 0)
            throw LibusbError("libusb_init", r);
#ifdef WIN32
        libusb_set_option(ctx, LIBUSB_OPTION_USE_USBDK);
#endif
        *ant_instanance = new AntStick();
        LOG_MSG(" USB Stick: Serial#: "); LOG_D(((AntStick*)*ant_instanance)->GetSerialNumber());
        LOG_MSG(", version "); LOG_MSG(((AntStick*)*ant_instanance)->GetVersion().c_str());
        LOG_MSG(", max "); LOG_D(((AntStick*)*ant_instanance)->GetMaxNetworks());
        LOG_MSG(" networks, max "); LOG_D(((AntStick*)*ant_instanance)->GetMaxChannels());
        LOG_MSG(" channels\n");
        ((AntStick*)*ant_instanance)->SetNetworkKey(AntStick::g_AntPlusNetworkKey);
        max_channels = ((AntStick*)*ant_instanance)->GetMaxChannels();
    }
    catch (const std::exception &e) {
        LOG_MSG(e.what()); LOG_MSG("\n");
        return -1;
    }
    LOG_MSG("InitAntService() - OK\n");
    return 0;
}

int CloseAntService()
{
    LOG_MSG("CloseAntService()+\n");
    LOG_MSG("CloseAntService() - OK\n");
    return 0;
}

AntSession InitSession(void * ant_instanance, AntDevice ** devices, int num_devices, std::mutex & guard)
{
    LOG_MSG("InitSession()+\n");
    AntSession session = {};
    if (ant_instanance == nullptr)
        return session;
    session.m_AntStick = ant_instanance;
    if (devices == nullptr)
        return session;

    try {
        for (int i = 0; i < num_devices; i++) {
            if (devices[i] == nullptr)
                return session;
        }
        if (num_devices == 1 && devices[0]->m_type == HRM_Type)
            session.m_TelemtryServer = new TelemetryServer((AntStick*)ant_instanance, (std::unique_ptr<AntChannel>*)devices[0]->m_device, guard);
        else
            return session;
    }
    catch (const std::exception &e) {
        LOG_MSG(e.what()); LOG_MSG("\n");
        session.m_TelemtryServer = nullptr;
        return session;
    }
    LOG_MSG("InitSession() - OK\n");
    return session;
}

int CloseSession(AntSession & session)
{
    LOG_MSG("CloseSession()+\n");
    session.m_AntStick = nullptr; // deleted in ~SearchService()
    delete session.m_TelemtryServer;
    session.m_TelemtryServer = nullptr;
    session.m_bIsRun = false;
    LOG_MSG("CloseSession() - OK\n");
    return 0;
}

int RunSearch(void * ant_instanance, void ** pp_search_service, std::thread & thread, std::mutex & guard)
{
    LOG_MSG("RunSearch()+\n");
    if (ant_instanance == nullptr)
        return -1;
    if (pp_search_service == nullptr)
        return -1;
    bool* stop_flag = nullptr;

    try {
        *pp_search_service = new SearchService((AntStick*)ant_instanance, guard);
        stop_flag = new bool(false);

        thread = std::thread([ant_instanance, pp_search_service, stop_flag]()
        {
            while (true && stop_flag != nullptr && !(*stop_flag))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                ((SearchService*)*pp_search_service)->Tick();
            }
        });
        stop_flags.insert(std::pair<std::thread::id, bool *>(thread.get_id(), stop_flag));
    }
    catch (const std::exception &e) {
        LOG_MSG(e.what()); LOG_MSG("\n");
        pp_search_service = nullptr;
        delete stop_flag;
        stop_flag = nullptr;
        return -1;
    }
    LOG_MSG("RunSearch() - OK\n");
    return 0;
}

int AddDeviceForSearch(void * p_search_service, AntDeviceType type)
{
    LOG_MSG("AddDeviceForSearch()+\n");
    if (p_search_service == nullptr)
        return -1;

    int res = 0;
    try {
        res = ((SearchService *)p_search_service)->AddDeviceForSearch(type);
    }
    catch (const std::exception &e) {
        LOG_MSG(e.what()); LOG_MSG("\n");
        return -1;
    }

    return res;
}

int StopSearch(void ** pp_search_service, std::thread & thread)
{
    LOG_MSG("StopSearch()+\n");
    if (pp_search_service == nullptr)
        return -1;
    if (!thread.joinable())
        return -1;

    auto pair = stop_flags.find(thread.get_id());
    if (pair == stop_flags.end() || pair->second == nullptr)
        return -1;

    *(pair->second) = true;
    thread.join();

    delete (SearchService*)*pp_search_service;
    *pp_search_service = nullptr;

    delete pair->second;
    stop_flags.erase(pair);

    LOG_MSG("StopSearch() - OK\n");
    return 0;
}

// internal function
inline int GetChannelList(void * p_search_service, AntDevice ** devices, unsigned int & num_devices, unsigned int & num_active_devices)
{
    if (p_search_service == nullptr)
        return -1;
    if (devices == nullptr)
        return -1;
    try {
        const std::vector<std::unique_ptr<AntChannel>>& _devices = ((SearchService*)p_search_service)->GetDevices();
        num_devices = 0;
        for (auto & it : _devices)
        {
            if (it.get())
            {
                if (it->ChannelState() == AntChannel::CH_OPEN)
                {
                    devices[num_devices]->m_device = (void*)&it;
                    if (it->ChannelId().DeviceType == HRM::ANT_DEVICE_TYPE)
                        devices[num_devices]->m_type = HRM_Type;
                    else if (it->ChannelId().DeviceType == HRM::ANT_DEVICE_TYPE)
                        devices[num_devices]->m_type = BIKE_Type;
                    else
                        devices[num_devices]->m_type = NONE_Type;
                    devices[num_devices]->m_device_number = it->ChannelId().DeviceNumber;
                    num_active_devices++;
                }
                num_devices++;
            }
        }
        return 0;
    }
    catch (const std::exception &e) {
        LOG_MSG(e.what()); LOG_MSG("\n");
        return -1;
    }
}

int GetDeviceList(void * p_search_service, AntDevice ** devices, unsigned int & num_devices, unsigned int & num_active_devices)
{
    LOG_MSG("GetDeviceList()+\n");
    if (GetChannelList(p_search_service, devices, num_devices, num_active_devices) != 0)
        return -1;
    return 0;
    LOG_MSG("GetDeviceList() - OK\n");
    return 0;
}

int Run(AntSession & session)
{
    LOG_MSG("Run()+\n");
    if (session.m_bIsRun)
        return -1;
    if (session.m_AntStick == nullptr ||
        session.m_TelemtryServer == nullptr)
        return -1;

    bool *stop_flag = nullptr;

    try {
        stop_flag = new bool(false);
        session.m_ServiceThread = new std::thread([session, stop_flag]()
        {
            while(true && stop_flag != nullptr && !(*stop_flag))
                ((TelemetryServer*)session.m_TelemtryServer)->Tick();
        });
        stop_flags.insert(std::pair<std::thread::id, bool *>(((std::thread*)session.m_ServiceThread)->get_id(), stop_flag));
    }
    catch (const std::exception &e) {
        delete stop_flag;
        stop_flag = nullptr;
        LOG_MSG(e.what()); LOG_MSG("\n");
        return -1;
    }
    session.m_bIsRun = true;
    LOG_MSG("Run() - OK\n");
    return 0;
}
int Stop(AntSession & session)
{
    LOG_MSG("Stop()+\n");
    if (!session.m_bIsRun)
        return -1;
    if (session.m_AntStick == nullptr ||
        session.m_TelemtryServer == nullptr ||
        session.m_ServiceThread == nullptr) {
        return -1;
    }
    std::thread* service_thread = static_cast<std::thread*>(session.m_ServiceThread);
    if (!service_thread->joinable())
        return -1;

    auto pair = stop_flags.find(service_thread->get_id());
    if (pair == stop_flags.end() || pair->second == nullptr)
        return -1;

    *pair->second = true;
    service_thread->join();

    session.m_bIsRun = false;

    delete pair->second;
    stop_flags.erase(pair);

    LOG_MSG("Stop() - OK\n");
    return 0;
}
Telemetry GetTelemetry(AntSession & session)
{
    LOG_MSG("GetTelemetry()+\n");
    Telemetry t = {};
    if (!session.m_bIsRun)
        return t;
    if (session.m_AntStick == nullptr ||
        session.m_TelemtryServer == nullptr)
        return t;
    LOG_MSG("GetTelemetry() - OK\n");
    return ((TelemetryServer*)session.m_TelemtryServer)->GetTelemetry();
}

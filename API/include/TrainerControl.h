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
#pragma once

#include <thread>
#include <mutex>
#include "structures.h"

#if defined(_MSC_VER)
    #define DLL_API __declspec(dllexport)
#else
    #define DLL_API __attribute__((visibility("default")))
#endif

DLL_API int InitAntService(void ** ant_instanance, int & max_channels);
DLL_API int CloseAntService();
DLL_API int RunSearch(void * ant_instanance, void ** pp_search_service, std::thread & thread, std::mutex & guard);
DLL_API int AddDeviceForSearch(void * p_search_service, AntDeviceType type);
DLL_API int StopSearch(void ** pp_search_service, std::thread & thread);
DLL_API AntSession InitSession(void * ant_instanance, AntDevice ** devices, int num_devices, std::mutex & guard);
DLL_API int GetDeviceList(void * p_search_service, AntDevice ** devices, unsigned int & num_devices, unsigned int & num_active_devices);
DLL_API int CloseSession(AntSession & session);
/*create separate thread assign with session*/
DLL_API int Run(AntSession & session);
DLL_API int Stop(AntSession & session);
DLL_API Telemetry GetTelemetry(AntSession & session);

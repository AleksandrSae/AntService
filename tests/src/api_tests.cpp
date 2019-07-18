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
#include "gtest/gtest.h"
#include "TrainerControl.h"

#ifdef WIN32
#pragma comment (lib, "AntService.lib")
#pragma comment (lib, "gtest.lib")
#endif

TEST(InitAntServiceTest, NullPtr) {
    int max_channels = 0;
    EXPECT_EQ(-1, InitAntService(nullptr, max_channels));
    EXPECT_TRUE(0 == max_channels);
}

TEST(InitAntServiceTest, Normal) {
    void* ant_handle = nullptr;
    int max_channels = 0;
    EXPECT_EQ(0, InitAntService(&ant_handle, max_channels));
    EXPECT_TRUE(nullptr != ant_handle);
    EXPECT_TRUE(0 != max_channels);

    EXPECT_EQ(0, CloseAntService());
}

TEST(RunSearch, NullptrIn) {
    std::thread search_thread;
    std::mutex guard;
    void* search_service = nullptr;
    EXPECT_EQ(-1, RunSearch(nullptr, &search_service, search_thread, guard));
    EXPECT_FALSE(search_thread.joinable());
    EXPECT_EQ(nullptr, search_service);
}

TEST(RunSearch, NullptrOut) {
    void* ant_handle;
    int max_channels;
    EXPECT_EQ(0, InitAntService(&ant_handle, max_channels));
    EXPECT_TRUE(nullptr != ant_handle);
    EXPECT_TRUE(0 != max_channels);

    std::thread search_thread;
    std::mutex guard;
    EXPECT_EQ(-1, RunSearch(ant_handle, nullptr, search_thread, guard));
    EXPECT_FALSE(search_thread.joinable());

    EXPECT_EQ(0, CloseAntService());
}

TEST(StopSearch, Nullptr) {
    std::thread search_thread;
    EXPECT_EQ(-1, StopSearch(nullptr, search_thread));
}

TEST(RunStopSearch, Normal) {
    void* ant_handle;
    int max_channels;
    EXPECT_EQ(0, InitAntService(&ant_handle, max_channels));
    EXPECT_TRUE(nullptr != ant_handle);
    EXPECT_TRUE(0 != max_channels);

    std::thread search_thread;
    std::mutex guard;
    std::mutex local_guard;
    void* search_service;
    EXPECT_EQ(0, RunSearch(ant_handle, &search_service, search_thread, guard));
    EXPECT_TRUE(search_thread.joinable());
    EXPECT_TRUE(nullptr != search_service);

    EXPECT_EQ(0, StopSearch(&search_service, search_thread));
    EXPECT_FALSE(search_thread.joinable());
    EXPECT_EQ(nullptr, search_service);

    EXPECT_EQ(0, CloseAntService());
}

TEST(AddDeviceForSearch, Nullptr) {
    EXPECT_EQ(-1, AddDeviceForSearch(nullptr, HRM_Type));
}

TEST(AddDeviceForSearch, NoneType) {
    EXPECT_EQ(-1, AddDeviceForSearch(nullptr, NONE_Type));
}

TEST(AddDeviceForSearch, Normal) {
    void* ant_handle;
    int max_channels;
    EXPECT_EQ(0, InitAntService(&ant_handle, max_channels));
    EXPECT_TRUE(nullptr != ant_handle);
    EXPECT_TRUE(0 != max_channels);

    std::thread search_thread;
    std::mutex guard;
    void* search_service;
    EXPECT_EQ(0, RunSearch(ant_handle, &search_service, search_thread, guard));
    EXPECT_TRUE(search_thread.joinable());
    EXPECT_TRUE(nullptr != search_service);

    EXPECT_EQ(0, AddDeviceForSearch(search_service, HRM_Type));

    EXPECT_EQ(0, StopSearch(&search_service, search_thread));
    EXPECT_FALSE(search_thread.joinable());
    EXPECT_EQ(nullptr, search_service);

    EXPECT_EQ(0, CloseAntService());
}

TEST(InitSession, NullAntINstance) {
    AntDevice* device_list[1];
    std::mutex guard;
    AntSession ant_session = InitSession(nullptr, device_list, 1, guard);
    EXPECT_EQ(nullptr, ant_session.m_AntStick);
    EXPECT_EQ(nullptr, ant_session.m_TelemtryServer);
    EXPECT_EQ(false, ant_session.m_bIsRun);
}

TEST(InitSession, NullDeviceList) {
    void* ant_handle;
    int max_channels;
    EXPECT_EQ(0, InitAntService(&ant_handle, max_channels));
    EXPECT_TRUE(nullptr != ant_handle);
    EXPECT_TRUE(0 != max_channels);

    std::mutex guard;
    AntSession ant_session = InitSession(ant_handle, nullptr, 1, guard);
    EXPECT_EQ(nullptr, ant_session.m_AntStick);
    EXPECT_EQ(nullptr, ant_session.m_TelemtryServer);
    EXPECT_EQ(false, ant_session.m_bIsRun);

    EXPECT_EQ(0, CloseAntService());
}

TEST(CloseSession, NullDeviceList) {
    AntSession ant_session = {nullptr, nullptr, nullptr, true};
    EXPECT_EQ(0, CloseSession(ant_session));
    EXPECT_EQ(nullptr, ant_session.m_AntStick);
    EXPECT_EQ(nullptr, ant_session.m_TelemtryServer);
    EXPECT_EQ(false, ant_session.m_bIsRun);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

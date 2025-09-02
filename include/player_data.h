// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include <endstone/endstone.hpp>
#include <chrono>
#include <memory>

struct PlayerData {
    // Original location and rotation
    endstone::Location originalLocation;
    float originalYaw = 0.0f;
    float originalPitch = 0.0f;
    
    // Authentication state
    bool isAuthenticated = false;
    std::chrono::steady_clock::time_point joinTime;
    
    // Timer tasks
    std::shared_ptr<endstone::Task> authTimerTask;
    std::shared_ptr<endstone::Task> reminderTask;
    
    // Constructor
    PlayerData() = default;
    PlayerData(const endstone::Location& loc, float yaw, float pitch)
        : originalLocation(loc), originalYaw(yaw), originalPitch(pitch),
          isAuthenticated(false), joinTime(std::chrono::steady_clock::now()) {}
};
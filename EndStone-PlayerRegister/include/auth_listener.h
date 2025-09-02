// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include <endstone/endstone.hpp>

class PlayerRegisterPlugin;

class AuthListener {
public:
    explicit AuthListener(PlayerRegisterPlugin& plugin);
    
    void onPlayerJoin(endstone::PlayerJoinEvent &event);
    void onPlayerQuit(endstone::PlayerQuitEvent &event);
    void onPlayerChat(endstone::PlayerChatEvent &event);

private:
    PlayerRegisterPlugin& plugin_;
};
// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include <endstone/endstone.hpp>

class PlayerRegisterListener {
public:
    explicit PlayerRegisterListener(endstone::Plugin &plugin) : plugin_(plugin) {}

    void onServerLoad(endstone::ServerLoadEvent &event);
    void onPlayerJoin(endstone::PlayerJoinEvent &event);
    void onPlayerQuit(endstone::PlayerQuitEvent &event);
    void onPlayerChat(endstone::PlayerChatEvent &event);
    void onPlayerCommand(endstone::PlayerCommandEvent &event);

private:
    endstone::Plugin &plugin_;
};

// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "auth_listener.h"
#include "player_register_plugin.h"

AuthListener::AuthListener(PlayerRegisterPlugin& plugin) : plugin_(plugin) {}

void AuthListener::onPlayerJoin(endstone::PlayerJoinEvent &event) {
    plugin_.onPlayerJoin(event);
}

void AuthListener::onPlayerQuit(endstone::PlayerQuitEvent &event) {
    plugin_.onPlayerQuit(event);
}

void AuthListener::onPlayerChat(endstone::PlayerChatEvent &event) {
    plugin_.onPlayerChat(event);
}
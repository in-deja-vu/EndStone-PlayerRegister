// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include <endstone/endstone.hpp>

class PlayerRegisterListener {
public:
    explicit PlayerRegisterListener(endstone::Plugin &plugin) : plugin_(plugin) {}

    void onServerLoad(endstone::ServerLoadEvent &event)
    {
        plugin_.getLogger().info("ServerLoadEvent is passed to PlayerRegisterListener::onServerLoad");
    }

private:
    endstone::Plugin &plugin_;
};

// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include <endstone/endstone.hpp>

class PlayerRegisterPlugin;

class AuthCommandExecutor : public endstone::CommandExecutor {
public:
    explicit AuthCommandExecutor(PlayerRegisterPlugin& plugin);
    
    bool onCommand(endstone::CommandSender &sender, const endstone::Command &command,
                   const std::vector<std::string> &args) override;

private:
    bool handleRegister(endstone::CommandSender &sender, const std::vector<std::string> &args);
    bool handleLogin(endstone::CommandSender &sender, const std::vector<std::string> &args);
    
    PlayerRegisterPlugin& plugin_;
};
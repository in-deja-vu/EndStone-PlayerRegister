// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include "player_manager.h"

namespace PlayerRegister {

class AccountManager {
public:
    static bool createAccount(endstone::Player& pl, const std::string& name, const std::string& password, bool create_new = false);
    static bool loginAccount(endstone::Player& pl, const std::string& name, const std::string& password);
    static bool changePassword(const std::string& name, const std::string& new_password);
    static bool changePassword(endstone::Player& pl, const std::string& old_password, const std::string& new_password);
    
    static void showRegisterHelp(endstone::Player& pl);
    static void showLoginHelp(endstone::Player& pl);
    static void showAccountInfo(endstone::Player& pl);
    static void showChangePasswordHelp(endstone::Player& pl);

private:
    static void trimString(std::string& s);
    static bool validatePassword(const std::string& password);
    static bool validateUsername(const std::string& username);
};

} // namespace PlayerRegister
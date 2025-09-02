// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include <endstone/endstone.hpp>
#include <string>
#include <optional>

namespace PlayerRegister {

class AccountManager {
public:
    static bool init(const std::string& dataDir);
    static bool createAccount(const std::string& username, const std::string& password);
    static bool accountExists(const std::string& username);
    static bool verifyPassword(const std::string& username, const std::string& password);
    static bool removeAccount(const std::string& username);
    
    // Helper methods
    static std::string hashPassword(const std::string& password);
    static bool isValidPassword(const std::string& password);
    static void sendRegisterError(endstone::Player& player, const std::string& error);
    static void sendLoginError(endstone::Player& player, const std::string& error);
    static void sendSuccessMessage(endstone::Player& player, const std::string& message);

private:
    static std::string dataDir_;
    static std::string getAccountFilePath(const std::string& username);
    static void ensureDirectoryExists(const std::string& path);
    static bool saveAccountData(const std::string& username, const std::string& hashedPassword);
    static std::optional<std::string> loadAccountData(const std::string& username);
};

} // namespace PlayerRegister
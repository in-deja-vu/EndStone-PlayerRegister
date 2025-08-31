// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "player_register.h"

// The ENDSTONE_PLUGIN macro defines the metadata for the plugin.
ENDSTONE_PLUGIN(/*name=*/"player_register", /*version=*/"1.4.0", /*main_class=*/PlayerRegisterPlugin)
{
    prefix = "PlayerRegister";
    description = "Player registration and account management plugin for Endstone servers";
    website = "https://github.com/in-deja-vu/EndStone-PlayerRegister";
    authors = {"edshPC", "PlayerRegister Contributors"};

    command("register") //
        .description("Register a new player account.")
        .usages("/register <username> <password> <confirm_password>")
        .permissions("player_register.command.register");

    command("login") //
        .description("Login to your player account.")
        .usages("/login <username> <password>")
        .permissions("player_register.command.login");

    command("changepassword") //
        .description("Change your account password.")
        .usages("/changepassword <old_password> <new_password> <confirm_new_password>")
        .aliases("changepass", "cp")
        .permissions("player_register.command.changepassword");

    command("account") //
        .description("Account management and information.")
        .usages("/account [info]")
        .permissions("player_register.command.account");

    command("resetpassword") //
        .description("Reset a player's password (Operator only).")
        .usages("/resetpassword <username>")
        .permissions("player_register.command.resetpassword");

    command("logout") //
        .description("Logout from your current account.")
        .usages("/logout")
        .permissions("player_register.command.logout");

    permission("player_register.command")
        .description("Allow users to use all commands provided by the player register plugin")
        .children("player_register.command.register", true)
        .children("player_register.command.login", true)
        .children("player_register.command.changepassword", true)
        .children("player_register.command.account", true)
        .children("player_register.command.logout", true);

    permission("player_register.command.register")
        .description("Allow users to register new accounts")
        .default_(endstone::PermissionDefault::True);

    permission("player_register.command.login")
        .description("Allow users to login to their accounts")
        .default_(endstone::PermissionDefault::True);

    permission("player_register.command.changepassword")
        .description("Allow users to change their passwords")
        .default_(endstone::PermissionDefault::True);

    permission("player_register.command.account")
        .description("Allow users to view account information")
        .default_(endstone::PermissionDefault::True);

    permission("player_register.command.logout")
        .description("Allow users to logout from their accounts")
        .default_(endstone::PermissionDefault::True);

    permission("player_register.command.resetpassword")
        .description("Allow operators to reset player passwords")
        .default_(endstone::PermissionDefault::Operator);
}

// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "player_register_plugin.h"

// The ENDSTONE_PLUGIN macro defines the metadata for the plugin.
ENDSTONE_PLUGIN(/*name=*/"player_register", /*version=*/"2.0.0", /*main_class=*/PlayerRegisterPlugin)
{
    prefix = "PlayerRegister";
    description = "Complete player registration and authentication system for Endstone servers";
    website = "https://github.com/in-deja-vu/EndStone-PlayerRegister";
    authors = {"EndStone-PlayerRegister Contributors"};

    command("register") //
        .description("Создать новый аккаунт.")
        .usages("/register <пароль: string> <повтор: string>")
        .permissions("player_register.command.register");

    command("login") //
        .description("Войти в аккаунт.")
        .usages("/login <пароль: string>")
        .permissions("player_register.command.login");

    permission("player_register.command")
        .description("Разрешить пользователям использовать команды регистрации")
        .children("player_register.command.register", true)
        .children("player_register.command.login", true);

    permission("player_register.command.register")
        .description("Разрешить пользователям создавать новые аккаунты")
        .default_(endstone::PermissionDefault::True);

    permission("player_register.command.login")
        .description("Разрешить пользователям входить в свои аккаунты")
        .default_(endstone::PermissionDefault::True);
}
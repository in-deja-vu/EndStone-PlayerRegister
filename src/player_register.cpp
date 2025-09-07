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
        .description("Создать новый аккаунт.")
        .usages("/register <пароль> <подтверждение_пароля>")
        .permissions("player_register.command.register");

    command("login") //
        .description("Войти в аккаунт.")
        .usages("/login <пароль>")
        .permissions("player_register.command.login");

    command("changepassword") //
        .description("Сменить пароль аккаунта.")
        .usages("/changepassword <старый_пароль> <новый_пароль> <подтверждение_нового_пароля>")
        .aliases("changepass", "cp")
        .permissions("player_register.command.changepassword");

    command("account") //
        .description("Управление аккаунтом и информация.")
        .usages("/account [info]")
        .permissions("player_register.command.account");

    command("resetpassword") //
        .description("Сбросить пароль игрока (только для операторов).")
        .usages("/resetpassword <ник>")
        .permissions("player_register.command.resetpassword");

    command("logout") //
        .description("Выйти из текущего аккаунта.")
        .usages("/logout")
        .permissions("player_register.command.logout");

    permission("player_register.command")
        .description("Разрешить пользователям использовать все команды плагина регистрации")
        .children("player_register.command.register", true)
        .children("player_register.command.login", true)
        .children("player_register.command.changepassword", true)
        .children("player_register.command.account", true)
        .children("player_register.command.logout", true);

    permission("player_register.command.register")
        .description("Разрешить пользователям создавать новые аккаунты")
        .default_(endstone::PermissionDefault::True);

    permission("player_register.command.login")
        .description("Разрешить пользователям входить в свои аккаунты")
        .default_(endstone::PermissionDefault::True);

    permission("player_register.command.changepassword")
        .description("Разрешить пользователям менять свои пароли")
        .default_(endstone::PermissionDefault::True);

    permission("player_register.command.account")
        .description("Разрешить пользователям просматривать информацию об аккаунте")
        .default_(endstone::PermissionDefault::True);

    permission("player_register.command.logout")
        .description("Разрешить пользователям выходить из своих аккаунтов")
        .default_(endstone::PermissionDefault::True);

    permission("player_register.command.resetpassword")
        .description("Разрешить операторам сбрасывать пароли игроков")
        .default_(endstone::PermissionDefault::Operator);
}

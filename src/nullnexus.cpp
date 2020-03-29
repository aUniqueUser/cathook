#include "config.h"
#if ENABLE_NULLNEXUS
#include "common.hpp"
#include "libnullnexus/nullnexus.hpp"
#include "nullnexus.hpp"
#include "netadr.h"
#if ENABLE_VISUALS
#include "colors.hpp"
#include "MiscTemporary.hpp"
#include "boost/beast/core/detail/base64.hpp"
#endif

namespace nullnexus
{
static settings::Boolean enabled("nullnexus.enabled", "true");
static settings::Boolean anon("nullnexus.user.anon", "false");
static settings::String address("nullnexus.host", "localhost");
static settings::String port("nullnexus.port", "3000");
static settings::String endpoint("nullnexus.endpoint", "/client/v1");
static settings::Boolean authenticate("nullnexus.auth", "true");
#if ENABLE_VISUALS
static settings::Rgba colour("nullnexus.user.colour");
#endif

static NullNexus nexus;

void printmsg(std::string &usr, std::string &msg, int colour = 0xff9340)
{
#if !ENFORCE_STREAM_SAFETY && ENABLE_VISUALS
    if (msg.size() > 128 || usr.size() > 32)
    {
        logging::Info("Nullnexus: Message too large.");
        return;
    }
    if (g_Settings.bInvalid)
        g_ICvar->ConsoleColorPrintf(MENU_COLOR, "[Nullnexus] %s: %s\n", usr.c_str(), msg.c_str());
    else
        PrintChat(false, "\x07%06X[\x07%06XNullnexus\x07%06X] \x07%06X%s\x01: %s", 0x5e3252, 0xba3d9a, 0x5e3252, colour, usr.c_str(), msg.c_str());
#endif
}
void printmsgcopy(std::string usr, std::string msg)
{
    printmsg(usr, msg);
}

namespace handlers
{
void message(std::string usr, std::string msg, int colour)
{
    printmsg(usr, msg, colour);
}
void authedplayers(std::vector<std::string> steamids)
{
    // Check if we are in a game
    if (g_Settings.bInvalid)
        return;
    for (int i = 0; i <= g_IEngine->GetMaxClients(); i++)
    {
        // if (i == g_pLocalPlayer->entity_idx)
        //    continue;
        player_info_s pinfo{};
        if (g_IEngine->GetPlayerInfo(i, &pinfo))
        {
            if (pinfo.friendsID == 0)
                continue;
            MD5Value_t result;
            std::string steamidhash = std::to_string(pinfo.friendsID) + pinfo.name;
            MD5_ProcessSingleBuffer(steamidhash.c_str(), strlen(steamidhash.c_str()), result);
            steamidhash.clear();
            for (auto i : result.bits)
            {
                for (int j = 0; j < 8; j++)
                    steamidhash.append(std::to_string((i >> j) & 1));
            }
            std::remove_if(steamids.begin(), steamids.end(), [&steamidhash, &pinfo](std::string &steamid) {
                std::cout << "Comparing hashes: " << steamid << " and " << steamidhash << std::endl;
                if (steamid == steamidhash)
                {
                    // Use actual steamid to set cat status
                    if (playerlist::ChangeState(pinfo.friendsID, playerlist::k_EState::CAT))
                        PrintChat("\x07%06X%s\x01 Marked as CAT (Nullnexus user)", 0xe05938, pinfo.name);
                    return true;
                }
                return false;
            });
        }
    }
}
} // namespace handlers

// Update info about the current server we are on.
void updateServer(NullNexus::UserSettings &settings)
{
    INetChannel *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
    if (ch && *authenticate)
    {
        auto addr = ch->GetRemoteAddress();
        // Local address! Don't send that to nullnexus.
        if (!addr.IsReservedAdr())
        {
            player_info_s pinfo{};
            if (g_IEngine->GetPlayerInfo(g_pLocalPlayer->entity_idx, &pinfo))
            {
                MD5Value_t result;
                std::string steamidhash = std::to_string(pinfo.friendsID) + pinfo.name;
                MD5_ProcessSingleBuffer(steamidhash.c_str(), strlen(steamidhash.c_str()), result);
                steamidhash.clear();
                for (auto i : result.bits)
                {
                    for (int j = 0; j < 8; j++)
                        steamidhash.append(std::to_string((i >> j) & 1));
                }
                settings.tf2server = { true, addr.ToString(true), std::to_string(addr.port), steamidhash };
                return;
            }
        }
    }
    // Not connected
    settings.tf2server = { false };
}

// Update info about the current server we are on.
void updateServer()
{
    NullNexus::UserSettings settings;
    updateServer(settings);
    nexus.changeData(settings);
}

void updateData()
{
    std::optional<std::string> username = std::nullopt;
    std::optional<int> newcolour        = std::nullopt;
    username                            = *anon ? "anon" : g_ISteamFriends->GetPersonaName();
    if ((*colour).r || (*colour).g || (*colour).b)
    {
        int r     = (*colour).r * 255;
        int g     = (*colour).g * 255;
        int b     = (*colour).b * 255;
        newcolour = (r << 16) + (g << 8) + b;
    }
    NullNexus::UserSettings settings;
    settings.username = *username;
    settings.colour   = newcolour;
    // Tell nullnexus about the current server we are connected to.
    updateServer(settings);

    nexus.changeData(settings);
}

bool sendmsg(std::string &msg)
{
    if (nexus.sendChat(msg))
        return true;
    printmsgcopy("Cathook", "Error! Couldn't send message.");
    return false;
}

template <typename T> void rvarCallback(settings::VariableBase<T> &, T)
{
    std::thread reload([]() {
        std::this_thread::sleep_for(std::chrono_literals::operator""ms(500));
        updateData();
        if (*enabled)
            nexus.connect(*address, *port, *endpoint);
        else
            nexus.disconnect();
    });
    reload.detach();
}

template <typename T> void rvarDataCallback(settings::VariableBase<T> &, T)
{
    std::thread reload([]() {
        std::this_thread::sleep_for(std::chrono_literals::operator""ms(500));
        updateData();
    });
    reload.detach();
}

static InitRoutine init([]() {
    updateData();
    enabled.installChangeCallback(rvarCallback<bool>);
    address.installChangeCallback(rvarCallback<std::string>);
    port.installChangeCallback(rvarCallback<std::string>);
    endpoint.installChangeCallback(rvarCallback<std::string>);

#if ENABLE_VISUALS
    colour.installChangeCallback(rvarDataCallback<rgba_t>);
#endif
    anon.installChangeCallback(rvarDataCallback<bool>);
    authenticate.installChangeCallback(rvarDataCallback<bool>);

    nexus.setHandlerChat(handlers::message);
    nexus.setHandlerAuthedplayers(handlers::authedplayers);
    if (*connect)
        nexus.connect(*address, *port, *endpoint);

    EC::Register(
        EC::Shutdown, []() { nexus.disconnect(); }, "shutdown_nullnexus");
    EC::Register(
        EC::FirstCM, []() { updateServer(); }, "firstcm_nullnexus");
    EC::Register(
        EC::LevelShutdown, []() { updateServer(); }, "firstcm_nullnexus");
});
static CatCommand nullnexus_send("nullnexus_send", "Send message to IRC", [](const CCommand &args) {
    std::string msg(args.ArgS());
    sendmsg(msg);
});
} // namespace nullnexus
#endif

#define DISCORDPP_IMPLEMENTATION
#include "discord.hpp"

#include <iostream>

void DC_Instance::InitDiscord(){
    discordClient = std::make_shared<discordpp::Client>();
    discordClient->SetApplicationId(APPLICATION_ID);

    discordpp::Activity activity;
    activity.SetName("Strata - C++ Sandbox");
    activity.SetType(discordpp::ActivityTypes::Playing);
    activity.SetDetails("Am Spielen bruh");
    activity.SetDetailsUrl("https://github.com/NexusioDEV/Strata");
    activity.SetState("Am Entwickeln");
    activity.SetStateUrl("https://github.com/NexusioDEV/Strata");


    discordpp::ActivityAssets assets;
    assets.SetLargeText("Strata");
    activity.SetAssets(assets);

    discordClient->UpdateRichPresence(activity, [](const discordpp::ClientResult &result) {
        if (result.Successful()) {
            std::cout << "Rich Presence updated";
        }
        else {
            std::cout << "Rich Presence not Updated: " << result.Error();
        }
    });
}

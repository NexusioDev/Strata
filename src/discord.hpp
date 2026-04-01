#ifndef STRATA_DISCORD_HPP
#define STRATA_DISCORD_HPP

#include "discordpp.h"
#include <memory>

class DC_Instance {
public:
    void InitDiscord();

    std::shared_ptr<discordpp::Client> discordClient;
    const uint64_t APPLICATION_ID = 1488543536788934706;

private:
};
#endif //STRATA_DISCORD_HPP
//
// Created by Me on 07/07/2025.
//

#include <iostream>
#include "connect.hpp"

int main () {
    auto settings = std::make_shared<FTPSettings>();

    FTPFetcher fetcher;
    settings->host = "localhost";
    settings->port = 21;
    settings->username = "user";
    settings->password = "password";
    settings->protocol = "ftp";
    settings->doUsePassive = true;
    settings->doUseTLS = true;
    fetcher.setSettings(settings);

    std::cout << fetcher.connect() << std::endl;

}

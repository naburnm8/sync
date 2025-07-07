#include <iostream>

#include "connect.hpp"

CURLcode FTPFetcher::curlPerform(const std::string &remoteResource, CURL *curl) {
    if (this->connectionSettings == nullptr) {
        throw std::runtime_error("No connection settings available");
    }
    const std::string url = constructUrl(remoteResource);

    if (this->connectionSettings->protocol != "ftp" or this->connectionSettings->protocol != "ftps") throw std::runtime_error("Unsupported protocol: expected ftp or ftps");

    if (!curl) throw std::runtime_error("Curl null reference");

    if (auto ftpSettings = dynamic_cast<FTPSettings*>(this->connectionSettings.get())) {
        if (ftpSettings->doUseTLS) {
            curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // turned off for debug
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // turned off for debug
        }
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERNAME, ftpSettings->username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, ftpSettings->password.c_str());
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        if (ftpSettings->doUsePassive) {
            curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 1L);
        } else {
            curl_easy_setopt(curl, CURLOPT_FTPPORT, "-");
        }
        return curl_easy_perform(curl);
    }
    throw std::runtime_error("Unsupported connection settings");
}

ConnectCode FTPFetcher::connect() {
    if (this->curl != nullptr) {
        return CURL_ALREADY_DEFINED;
    }
    CURL *curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0L);

    CURLcode res = this->curlPerform("", curl);

    if (res != CURLE_OK) {
        std::cerr << curl_easy_strerror(res) << std::endl;
        return REMOTE_CONNECTION_ERROR;
    }

    this->curl = curl;
    return CONNECT_OK;
}

ConnectCode FTPFetcher::fetch(const std::string &url, OperationStatus &buffer) {
    if (this->curl == nullptr) return CURL_NOT_DEFINED;

    curl_easy_reset(this->curl);

    std::vector<char> data;

    curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, writeToMemory);
    curl_easy_setopt(this->curl, CURLOPT_WRITEDATA, &data);

    CURLcode res = this->curlPerform(url, curl);

    if (res != CURLE_OK) {
        std::cerr << curl_easy_strerror(res) << std::endl;
        return REMOTE_FETCH_ERROR;
    }

    buffer.bytes = data.size();

    buffer.ptr = new char[buffer.bytes];

    std::ranges::copy(data, buffer.ptr);

    this->buffers.push_back(buffer);
    return CONNECT_OK;
}

FTPFetcher::~FTPFetcher() {
    delete this->connectionSettings;
    curl_easy_cleanup(this->curl);
    for (const auto &buffer : this->buffers) {
        delete &buffer;
    }
}

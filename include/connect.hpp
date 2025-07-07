#ifndef SYNC_CONNECT_HPP
#define SYNC_CONNECT_HPP

#include <memory>
#include <vector>
#include <tuple>

#include "curl/curl.h"

typedef std::vector<char> Buffer;

enum ConnectCode {
  CONNECT_OK = 0,
  REMOTE_CONNECTION_ERROR = -1,
  CURL_ALREADY_DEFINED = 1,
  CURL_NOT_DEFINED = -2,
  REMOTE_FETCH_ERROR = -3,

};

struct OperationStatus {
  char* ptr = nullptr;
  unsigned int bytes = 0;
  ~OperationStatus() {
    delete[] ptr;
  }
};

struct ConnectionSettings {
  std::string host;
  unsigned short port;
  std::string protocol;
  virtual ~ConnectionSettings() = default;
};

struct UserPasswordConnectionSettings: public ConnectionSettings {
  std::string username;
  std::string password;
};


class NetworkManipulator {
protected:
  std::shared_ptr<ConnectionSettings> connectionSettings;
  CURL* curl = nullptr;
  virtual CURLcode curlPerform(const std::string& remoteResource, CURL* curl) = 0;
  [[nodiscard]] std::string constructUrl(const std::string& remoteResource) const {
    if (this->connectionSettings == nullptr) {
      throw std::runtime_error("No connection settings available");
    }
    std::string url = this->connectionSettings->protocol + "://" + this->connectionSettings->host;
    if (this->connectionSettings->port != 0) {
      url += ":" + std::to_string(this->connectionSettings->port);
    }
    url += "/" + remoteResource;
    return url;
  };
public:
  void setSettings(std::shared_ptr<ConnectionSettings> connectionSettings) {
    this->connectionSettings = std::move(connectionSettings);
  }
  NetworkManipulator() = default;
  explicit NetworkManipulator(std::shared_ptr<ConnectionSettings> connectionSettings) : connectionSettings(std::move(connectionSettings)) {}
  virtual ~NetworkManipulator() = default;
  virtual ConnectCode connect() = 0;
};



class Fetcher : public NetworkManipulator {
protected:
  std::vector<OperationStatus> buffers;
  static size_t writeToMemory(void* buff, size_t size, size_t nmemb, void* data) {
    // expects a vector<char>
    if((size == 0) or (nmemb == 0) or (size*nmemb < 1) or (data == nullptr)) return 0;
    auto *vec = static_cast<Buffer*> (data);
    size_t ssize = size*nmemb;
    std::copy(static_cast<char *>(buff), static_cast<char *>(buff)+ssize, std::back_inserter(*vec));
    return ssize;
  };
public:
  virtual ConnectCode fetch(const std::string& url, OperationStatus& buffer) = 0;
};

class Sender : public NetworkManipulator {
protected:
  std::vector<OperationStatus> buffers;
public:
  virtual ConnectCode send(const std::string& url, OperationStatus& buffer) = 0;
};

class Transceiver {
protected:
  std::unique_ptr<Fetcher> fetcher;
  std::unique_ptr<Sender> sender;
  virtual void initClients(std::shared_ptr<ConnectionSettings> fetcherSettings, std::shared_ptr<ConnectionSettings> senderSettings) = 0;
public:
  [[nodiscard]] std::tuple<ConnectCode, ConnectCode> connect() const {
    ConnectCode fetcherCode = fetcher->connect();
    ConnectCode senderCode = sender->connect();
    return {fetcherCode, senderCode};
  };

  ConnectCode fetch(const std::string& url, OperationStatus& buffer) const {
    return fetcher->fetch(url, buffer);
  }

  ConnectCode send(const std::string& url, OperationStatus& buffer) const {
    return sender->send(url, buffer);
  }

  virtual ~Transceiver() = default;
};

struct FTPSettings final : public UserPasswordConnectionSettings {
  bool doUseTLS = false;
  bool doUsePassive = true;
};


class FTPFetcher final : public Fetcher {
protected:
  CURLcode curlPerform(const std::string& remoteResource, CURL* curl) override;

public:
  ConnectCode connect() override;

  ConnectCode fetch(const std::string &url, OperationStatus &buffer) override;

  ~FTPFetcher() override;
};

class FTPSender final : public Sender {
protected:
  CURLcode curlPerform(const std::string &remoteResource, CURL *curl) override;

public:
  ConnectCode connect() override;

  ConnectCode send(const std::string &url, OperationStatus &buffer) override;

  ~FTPSender() override;
};

class FTPTransceiver final : public Transceiver {
protected:
  void initClients(const std::shared_ptr<ConnectionSettings> fetcherSettings, const std::shared_ptr<ConnectionSettings> senderSettings) override {
    this->fetcher = std::make_unique<FTPFetcher>();
    this->fetcher->setSettings(fetcherSettings);
    this->sender = std::make_unique<FTPSender>();
    this->sender->setSettings(senderSettings);
  };
public:
  FTPTransceiver(const std::shared_ptr<ConnectionSettings> &fetcherSettings, const std::shared_ptr<ConnectionSettings> &senderSettings) {
    initClients(fetcherSettings, senderSettings);
  };
};

#endif
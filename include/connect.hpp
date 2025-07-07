#ifndef SYNC_CONNECT_HPP
#define SYNC_CONNECT_HPP

#include <memory>
#include <vector>
#include <tuple>

#include "curl/curl.h"

typedef std::vector<char> Buffer;

enum ConnectCode {
  CONNECT_OK = 0,
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
};

struct UserPasswordConnectionSettings: public ConnectionSettings {
  std::string username;
  std::string password;
};


class NetworkManipulator {
protected:
  std::unique_ptr<ConnectionSettings> connectionSettings;
  CURL* curl = nullptr;
  virtual CURLcode curlPerform() = 0;
  virtual std::string constructUrl(const std::string& pathToResource);
public:
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



#endif
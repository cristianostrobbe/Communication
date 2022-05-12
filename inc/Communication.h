#pragma once

#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include <stack>

#include <exception>
#include <functional>

#include <mutex>
#include <atomic>
#include <thread>

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <arpa/inet.h>

#include <unordered_map>

using namespace std;

struct communication_config
{
  std::string url;
  int port;
};

#define BIT(x) (1 << x)

typedef enum communication_errors_t{
  WRONG_CONFIGURATION=-1,
  COM_NONE,
  COM_OK,
  SEND_ERROR,
  RECEIVE_ERROR
}communication_errors_t;

typedef enum listener_errors_t{
  LISTEN_NOT_INITIALIZED=-1,
  LISTEN_NONE,
  LISTEN_OK,
  TOPIC_NOT_AVAILABLE,
  NEW_TOPIC_ADDED,
  TOPIC_REMOVED
}listener_errors_t;

namespace CO
{

  enum ConnectionMode
  {
    PUBLISHER = ZMQ_PUB,
    SUBSCRIBER = ZMQ_SUB
  };

  class Listener;

  class MessageStack;

  class Communication
  {
  public:
    Communication();

    // ~Communication();

    void Init(ConnectionMode, communication_config);

    void Pub(const std::string &topic, const std::string &payload) const;
    size_t Receive(const std::string &topic, std::string &message) const;

    int EnableTopicListener(const std::string &topic);
    int AddTopicToListener(const std::string &new_topic);
    int DisableTopicListener(const std::string &topic);
    // int RemoveAllTopics();
    int GetListenerMessages(const std::string &topic, std::vector<std::string> &);
    int SetListenerOnMessage(const std::string &topic, std::function<void(const std::string &topic, std::string message)>);
    int SetCallbackOnTopic(const std::string &topic, std::function<void(const std::string &, std::string)> callback);
    
    std::vector<double> GetAllMeanTimes();

    zmq::context_t *GetContext() { return context; };

    inline string GetURL() { return config.url; }
    inline int GetComPort() { return config.port; }

    std::vector<std::string> GetTopics();

  private:
    zmq::socket_t *socket;
    zmq::context_t *context;

    ConnectionMode connection_mode;
    communication_config config;

    std::unordered_map<std::string, Listener *> listeners;
    Listener *listener = nullptr;
  };

  class Listener
  {
  public:
    Listener(zmq::socket_t *socket, const std::string &topic);

    // ~Listener();

    void Start();
    void Stop();
    void GetMessages(std::vector<std::string> &);
    void SetOnMessage(std::function<void(const std::string &topic, std::string message)> callback);
    void SetOnTopic(const std::string &topic, std::function<void(const std::string &topic, std::string)> callback_);

    int AddTopic(const std::string &new_topic) {
      // Check if its already there
      socket->set(zmq::sockopt::subscribe, new_topic);
      topics.push_back(new_topic);
      #ifdef CONSOLE_LOG
      HEADERLOG("Listener") << "New topic: " << topic << " added" << FINI;
      #endif
      return NEW_TOPIC_ADDED;
    }

    int RemoveTopic(const std::string &topic) {
      if(IsTopic(topic)){
        socket->set(zmq::sockopt::unsubscribe, topic);
        topics.erase(std::find(topics.begin(), topics.end(), topic));
      }
      #ifdef CONSOLE_LOG
      HEADERLOG("Listener") << "Topic: " << topic << " removed" << FINI;
      #endif
      return TOPIC_REMOVED;
    }

    bool IsTopic(const std::string &topic){
      if (std::find(topics.begin(), topics.end(), topic) == topics.end())
        return false;
      else
        return true;
    };

    bool IsCallbackAvailable(const std::string &topic){
      if (callbacks.find(topic) != callbacks.end()) // If callback is available
        return true;
      else 
        return false;
    } 

    double GetMeanDeltaTime(const std::string &topic);

    std::vector<std::string> GetTopics() {
      return topics;
    }
    inline int GetNumOfTopics() {
      return topics.size();
    }

  private:
    bool enabled;
    std::string topic;
    std::vector<std::string> topics;
    std::queue<std::string> messages;

    std::function<void(const std::string &topic, std::string message)> callback;

    std::unordered_map<std::string, std::function<void(const std::string &topic, std::string message)>> callbacks;
    std::unordered_map<std::string, MessageStack *> topics_map;

    zmq::socket_t *socket;

    std::mutex mtx;
    std::thread *current_thread = nullptr;

  private:
    void Listen();
  };

class MessageStack {

public:
  MessageStack(int len=50); // Max number of messages

  int Push(const std::string &message){
    // Init t.o.a.
    if(!is_init) {
      last_toa = std::chrono::system_clock::now();
      is_init = true;
    }

    std::unique_lock<std::mutex> lck(mtx);
    if (msg_lifo.size() < (size_t)length){
      std::shared_ptr<stack_data_t> tmp_data = std::make_shared<stack_data_t>(message);
      msg_lifo.push(tmp_data);
    } else if (msg_lifo.size() >= (size_t)length) {
      msg_lifo.pop();
      std::shared_ptr<stack_data_t> tmp_data = std::make_shared<stack_data_t>(message);
      msg_lifo.push(tmp_data);
    }

    return 1;
  }

  inline int GetStackSize(){
    return (int)msg_lifo.size();
  }

  inline void SetStackSize(int len){
    std::unique_lock<std::mutex> lck(mtx);
    length = len;
  }

  double GetMean() {
    double sum=0.0;
    // {
      // std::unique_lock<std::mutex> lck(mtx);
      std::stack<std::shared_ptr<stack_data_t>> lifo_c(msg_lifo);
    // }
    int stck_len = (int)lifo_c.size();

    while(!lifo_c.empty()){
      auto val = lifo_c.top();
      double delta_time = chrono::duration_cast<chrono::nanoseconds>(val->toa - last_toa).count();
      last_toa = val->toa;
      sum += delta_time;
      lifo_c.pop();
    }

    return sum/stck_len;

  }

private:
  bool is_init = false;
  std::chrono::time_point<std::chrono::system_clock> last_toa;

  struct stack_data_t{
    std::chrono::time_point<std::chrono::system_clock> toa;
    const std::string message;

    stack_data_t(const std::string &message_) : message(message_){
      toa = std::chrono::system_clock::now();
    }
  };

  int length=50;
  std::mutex mtx;
  std::stack<std::shared_ptr<stack_data_t>> msg_lifo;
};

}; // end namespace
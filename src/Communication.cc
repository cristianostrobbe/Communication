#include "Communication.h"

using namespace CO;

Communication::Communication()
{
}

void Communication::Init(ConnectionMode mode, communication_config config)
{
  this->config = config;
  this->connection_mode = mode;
  context = new zmq::context_t(2, 10);

  if (mode == ConnectionMode::PUBLISHER)
  {
    try
    {
      std::cout << "Bind to: " << config.url << ":" << config.port << std::endl;
      socket = new zmq::socket_t(*context, ZMQ_PUB);
      socket->bind((config.url + ":" + to_string(config.port)));
    }
    catch (zmq::error_t e)
    {
      std::cout << e.what() << " err no: " << e.num() << std::endl;
    }
  }
  else if (mode == ConnectionMode::SUBSCRIBER)
  {
    try
    {
      std::cout << "Connect to: " << config.url << ":" << config.port << std::endl;
      socket = new zmq::socket_t(*context, zmq::socket_type::sub);
      socket->connect((config.url + ":" + to_string(config.port)));
    }
    catch (zmq::error_t e)
    {
      std::cout << e.what() << " err no: " << e.num() << std::endl;
    }
  }
  else
  {
    std::cout << "Wrong communication mode!" << std::endl;
  }
}

void Communication::Pub(const std::string &topic, const std::string &payload) const
{
  if (connection_mode != ConnectionMode::PUBLISHER)
    return;
  // If doesn't math PUBLISHER or PUBSUB then return

  zmq::message_t message(payload.size());
  std::memcpy(message.data(), payload.data(), payload.size());

  try {
    socket->send((void *)topic.c_str(), topic.size(), ZMQ_SNDMORE);
    socket->send(message);
  } catch (zmq::error_t e) {
    std::cout << e.what() << " err no: " << e.num() << std::endl;
  }
}

size_t Communication::Receive(const std::string &topic, std::string &message) const
{
  size_t ret = 0;
  socket->set(zmq::sockopt::subscribe, topic);

  std::vector<zmq::message_t> messages;
  zmq::recv_result_t result = zmq::recv_multipart(*socket, std::back_inserter(messages), zmq::recv_flags::dontwait);

  if (result.has_value())
    ret = result.value();

  if (messages.size() == 2)
    message = messages[1].to_string();

  return ret;
}

int Communication::EnableTopicListener(const std::string &topic)
{
  if (listener == nullptr) {
    listener = new Listener(socket, topic);
  }
  else if (listener->IsTopic(topic))
  {
    listener->Start();
    return COM_NONE;
  }

  return COM_OK;
}

int Communication::AddTopicToListener(const std::string &new_topic)
{
  if (listener == nullptr)
    return LISTEN_NOT_INITIALIZED;

  listener->AddTopic(new_topic);
  return COM_OK;
}

int Communication::DisableTopicListener(const std::string &topic)
{
  // if (listeners.find(topic) == listeners.end())
  //   return 0;

  if (listener == nullptr)
    return LISTEN_NOT_INITIALIZED;

  if(!listener->IsTopic(topic))
    return TOPIC_NOT_AVAILABLE;

  listener->RemoveTopic(topic);

  // listeners[topic]->Stop();
  return 1;
}

int Communication::GetListenerMessages(const std::string &topic, std::vector<std::string> &messages)
{
  if(!listener->IsTopic(topic))
    return TOPIC_NOT_AVAILABLE;   

  listener->GetMessages(messages);
  return messages.size();
}

int Communication::SetListenerOnMessage(const std::string &topic, std::function<void(const std::string &, std::string)> callback)
{
  if(!listener->IsTopic(topic))
    return TOPIC_NOT_AVAILABLE;

  listener->SetOnMessage(callback);

  return COM_OK;
}

int Communication::SetCallbackOnTopic(const std::string &topic, std::function<void(const std::string &, std::string)> callback)
{
  if(!listener->IsTopic(topic))
    return TOPIC_NOT_AVAILABLE;

  // listener->SetOnMessage(callback);
  listener->SetOnTopic(topic, callback);

  return COM_OK;
}

std::vector<std::string> Communication::GetTopics()
{
  std::vector<std::string> tpcs;
  if(listener == nullptr)
    return tpcs;
  else
    return listener->GetTopics();
}

std::vector<double> Communication::GetAllMeanTimes() {
  std::vector<double> mean_tss;
  if(listener != nullptr){
    std::vector<std::string> tpcs = GetTopics();
    for (std::string topic_ : tpcs) {
      double mean_ts = listener->GetMeanDeltaTime(topic_);
      mean_tss.push_back(mean_ts);
    }
  }
  return mean_tss;
}

Listener::Listener(zmq::socket_t *socket_, const std::string &topic_) : socket(socket_), topic(topic_)
{
  topics.push_back(topic);
  Start();
}

void Listener::GetMessages(std::vector<std::string> &ret)
{
  std::unique_lock<std::mutex> lck(mtx);
  while (!messages.empty())
  {
    ret.emplace_back(std::move(messages.front()));
    messages.pop();
  }
}

void Listener::Start()
{
  if (current_thread == nullptr)
    current_thread = new std::thread(&Listener::Listen, this);
  enabled = true;
}

void Listener::Stop()
{
  std::queue<std::string> empty;
  std::swap(messages, empty);
  topics.clear();
  current_thread = nullptr;
  enabled = false;
}

void Listener::SetOnMessage(std::function<void(const std::string &topic, std::string)> callback_)
{
  this->callback = callback_;
}

void Listener::SetOnTopic(const std::string &topic, std::function<void(const std::string &topic, std::string)> callback_)
{
  this->callbacks[topic] = callback_;
  this->topics_map[topic] = new MessageStack();
}

void Listener::Listen()
{
  socket->set(zmq::sockopt::subscribe, topics[0]); // Set first topic

  while (enabled)
  {
    std::vector<zmq::message_t> rcvd_messages;
    zmq::recv_result_t result = zmq::recv_multipart(*socket, std::back_inserter(rcvd_messages));
    if (result.has_value())
      if (result.value() <= 0)
        continue;

    // While waiting for message maybe is requested a stop
    if (enabled == false)
      break;

    std::unique_lock<std::mutex> lck(mtx);
    std::string message;
    std::string topic = "";
    for (size_t i = 0; i < result.value(); i++) {
      message = std::move(rcvd_messages[i].to_string());

      if(!IsTopic(message)) { // If is message
        messages.push(message);

        auto clbk_it = callbacks.find(topic);
        if (clbk_it != callbacks.end()) // If callback is available
        {
          clbk_it->second(topic, message);
          auto topic_stack_it = topics_map.find(topic); 
          if (topic_stack_it != topics_map.end()) // If topic stack is available
          {
            topic_stack_it->second->Push(message);
          }
        }
      } else {
        topic = message;
      }

    }

  }
}

double Listener::GetMeanDeltaTime(const std::string &topic) {
  auto topic_stack_it = topics_map.find(topic); 
  if (topic_stack_it != topics_map.end()) { // If topic stack is available
    return topic_stack_it->second->GetMean();
  } else  {
    return 0.0;
  }
}


MessageStack::MessageStack(int len) : length(len){}
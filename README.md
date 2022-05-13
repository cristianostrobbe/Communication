# __Communication__
Allows to establish __tcp/udp__ connection and send data with low latency (using cppzmq) in a __publisher/subsciber__ technology. 

# Dependecies
This library depends on:
- [ZMQ](https://github.com/zeromq/libzmq)
- [cppzmq](https://github.com/zeromq/cppzmq)
- [nlohman json](https://github.com/nlohmann/json)

## Install dependencies
### 1. __ZMQ__
~~~bash
git clone https://github.com/zeromq/libzmq.git
mkdir build
cd build
cmake ..
make
sudo make install
~~~

### 2. __CppZMQ__
~~~bash
git clone https://github.com/zeromq/cppzmq.git
mkdir build
cd build
cmake ..
make
sudo make install
~~~
### 3. __nlohmann json__
On __MacOS__:
~~~bash
brew install nlohmann-json
~~~
On __Ubuntu__
~~~bash
sudo apt install nlohmann-json3-dev
~~~

# __Build__
~~~bash
mkdir build
cd build && cmake ..
make
~~~
Then inside __bin__ folder you can find the compiled examples.

# __Usage__

### Initialization
~~~c++
// Loading configurations from file (url and port)
communication_config config;
bool ok_load = LoadJson(config, "config.json");

// This can only send data
CO::Communication sender;
sender.Init(CO::PUBLISHER, config);

// This can only receive data
CO::Communication receiver;
receiver.Init(CO::SUBSCRIBER, config);
~~~
**config.json** contains followig fields  
~~~json
{
  "url": "tcp://127.0.0.1",
  "port": 8080
}
~~~

### Receive/Send
~~~c++
// Sending a message to topic1
sender.Pub("topic1", "Hello World");

// Receiving a message from topic1
string msg = "";
size_t messages = receiver.Receive("topic1", msg);
~~~

### Advanced
Enable a listener on a topic, it will queue all the messages until  
someone retrieves the queue from it.  
***Make sure to clear the receiver queue*** by calling **receiver.GetListenerMessages**!!!.
~~~c++
// Enabling listener on topic1
receiver.EnableTopicListener("topic1");

// To disable
receiver.DisableTopicListener("topic1");

// To get all the messages from the listener
vector<string> messages;
receiver.GetListenerMessages("topic1", messages);

// Add another topic to the listener
listener.AddTopicToListener("config");
~~~

Callback functions can be also added, these called when the listener receives a message
~~~c++
void on_message(string topic, string message)
{
  cout << "[" << topic << "] Message: "<< message << endl;
  vector<string> messages;
  receiver.GetListenerMessages(topic, messages);
}

// Enabling listener on topic1
receiver.EnableTopicListener("topic1");
// Set callback
receiver.SetListenerOnMessage("topic1", on_message);
~~~

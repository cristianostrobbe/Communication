# Communication
Allows to establish tcp/udp connection and send data with low latency (using cppzmq).  

Contains python code generators to generate cpp files to serialize/deserialize structures from/to json,  
serialize/deserialize protocolbuffer messages.


## Installation
### ZMQ
~~~bash
git clone https://github.com/zeromq/libzmq.git
mkdir build
cd build
cmake ..
make
sudo make install
~~~

~~~bash
git clone https://github.com/zeromq/cppzmq.git
mkdir build
cd build
cmake ..
make
sudo make install
~~~


## Usage

### Initialization
~~~c++
// Loading configurations from file (url and port)
communication_config config;
LoadJson(config, "config.json");

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
  "port": "8080"
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

Also can be setted a callback function, called when the listener receives a message
~~~c++
void on_message(string topic, string message)
{
  cout << "[" << topic << "] Message: "<< message << endl;
  vector<string> messages;
  receiver.GetListenerMessages("topic1", messages);
}

// Enabling listener on topic1
receiver.EnableTopicListener("topic1");

receiver.SetListenerOnMessage("topic1", on_message);
~~~

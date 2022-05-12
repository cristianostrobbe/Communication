#include <future>
#include <iostream>
#include <string>

#include <unistd.h>
#include <math.h>
#include <chrono>

#include "Communication.h"

using namespace std;
using namespace Debug;


void on_message(string topic, string message)
{
    std::cout << topic << "***" << message << std::endl;
}

int main() {

    CO::Communication sender;
    CO::Communication receiver;
    CO::Communication listener;

    communication_config config;
    config.url = "tcp://127.0.0.1";
    config.port = 5656;

    sender.Init(CO::PUBLISHER, config);
    receiver.Init(CO::SUBSCRIBER, config);
    listener.Init(CO::SUBSCRIBER, config);
    usleep(10000);

    listener.EnableTopicListener("topic1");
    listener.SetListenerOnMessage("topic1", on_message);

    int count = 1;
    int listener_status = true;
    while(true)
    {
        sender.Pub("topic1", "[" + to_string(count) + "] message from library");

        if(count % 2 == 0)
        {
            string msg = "";
            size_t messages = receiver.Receive("topic1", msg);
            std::cout << messages << std::endl;
        }
        
        if(count % 10 == 0)
        {
            vector<string> msggs;
            listener.GetListenerMessages("topic1", msggs);
            for(auto message : msggs)
                std::cout << "- - - - " << message << std::endl;
            std::cout << "Actual count " << count << std::endl;
        }

        if(count % 100 == 0)
        {
            if(listener_status)
            {
                std::cout << "======== DISABLING =========" << std::endl;
                listener.DisableTopicListener("topic1");
                listener_status = false;
            }
            else
            {
                std::cout << "======== ENABLING  =========" << std::endl;
                listener.EnableTopicListener("topic1");
                listener_status = true;
            }
        }

        usleep(20000);
        count ++;
    }
}

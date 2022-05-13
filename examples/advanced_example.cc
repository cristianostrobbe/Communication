#include <future>
#include <iostream>
#include <string>
#include <fstream>

#include <unistd.h>
#include <math.h>
#include <chrono>

#include "Communication.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace std;

bool CheckJson(json j)
{
    bool ok_check = true;
	if(!j.contains("url")){
		throw std::runtime_error("JSON does not contain key [url] of type [std::string] in object [config]");
        ok_check = false;
    }
	if(!j.contains("port")){
		throw std::runtime_error("JSON does not contain key [port] of type [std::string] in object [config]");
        ok_check = false;
    }

    return ok_check;
}

bool Deserialize(communication_config& obj, json j)
{
	bool ok_check = CheckJson(j);
    if (ok_check) {
        obj.url = j["url"];
        obj.port = j["port"];
    }
    return ok_check;
}

bool LoadJson(communication_config& obj, std::string path)
{
	std::ifstream f(path);
    if(!f.is_open()){
		throw std::runtime_error("Error opening file!");
        return false;
    };
	json j;
	f >> j;
	f.close();
	bool ok_deser = Deserialize(obj, j);
	return ok_deser;
}

// Callback
void on_message(string topic, string message)
{
    std::cout << topic << "***" << message << std::endl;
}

int main() {

    CO::Communication sender;
    CO::Communication receiver;
    CO::Communication listener;

    // // Simple way
    // communication_config config;
    // config.url = "tcp://127.0.0.1";
    // config.port = 5656;

    // Using .json config
    communication_config config;
    bool ok_load = LoadJson(config, "../config/config.json");

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

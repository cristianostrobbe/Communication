#include <future>
#include <iostream>
#include <string>

#include <unistd.h>
#include <math.h>
#include <chrono>

using namespace std;

// #include "Profiler.h"
#include "Console.h"
#include "Communication.h"

// #include "json_models.h"
// #include "json_loader.h"

using namespace Debug;


int main() {

    CO::Communication sender;
    CO::Communication receiver;

    communication_config config;
    config.url = "tcp://127.0.0.1";
    config.port = 5656;

    sender.Init(CO::PUBLISHER, config);
    receiver.Init(CO::SUBSCRIBER, config);
    usleep(10000);

    int count = 1;
    int listener_status = true;
    while(true)
    {
        sender.Pub("topic1", "[" + to_string(count) + "] message from library");

        if(count % 10 == 0)
        {
            string msg = "EMPTY";
            size_t messages = receiver.Receive("topic1", msg);
            HEADERLOG("Receive") << "# msg: " << messages << " msg: " << msg << FINI;
        }

        usleep(10000);
        count ++;
    }
}

#include "influxdb_client.hpp"
#include "websocket_server.hpp"
#include "pcap_reader.hpp"
#include <iostream>
#include <time.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

int main() {
    printf("Relay\n");
    json j = convertPcapFileToJson("/home/kali/dev/Revolve/PCAP/2024-09-03-13-13-09.pcap");
    //std::cout << j.dump(4) << std::endl;

	//InfluxdbClient client("localhost", "8086", "30ffd1384cc64fb7", "MyInitialAdminToken0==");
	//client.writeJsonToInfluxdb(j, 5000);

    WebSocketServer server;
    server.startServer(8080);
    server.startHotspot("hotspot_name", "hotspot_password");

    while (server.isRunning()) {
	    if (!server.broadcastJson(j, 1000, 100)) {
	        server.stopServer();
	        break;
	    }
	}

    return 0;
}


// 	NTS functionality overview:
// 		car sends data on port XXXX.
// 		Relay recieves one message at a time and parses each message and stores it in a json objects. 
// 		There are three of these json objects so that while the parser writes to the json object, an other thread can read from one of the other two.
// 		One thread broadcasts the json object over websocket.
// 		Another thread parses the json file and stores it in PSN local influxdb.

// 		Frontend requests relay to store PCAP files over websocket. 
// 		Relay recieves PCAP files and parses each pcap file and stores it in the ChannelsManager class.
// 		Relay sends the data in ChannelsManager class to global Influx database

// 		Frontend requests relay to tune paramaters.
// 		Relay gets data for what to tune the Relay pars the data into CAN messages.
// 		then sends it to the car.

// 	Data must be converted in different ways:
// 		PCAP => UDP => CAN => ChannelManager => InfluxDB
// 		port in => UDP => CAN => ((JSON => websocket) and (ChannelsManager => InfluxDB))
// 		JSON => CAN => UDP => port out

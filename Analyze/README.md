# Analyze 
## As of 20 nov. 2024 the functionalities is:
 - Parsing PCAP files and storing the data in DataManager class
 - Visualizing the data stored in DataManager
 - Writing data stored in DataManager to InfluxDB
## Analyze.hpp 
Basically the frontend code. It depends on DataManager.hpp Deserialization.hpp PcapReader.hpp and InfluxDBClient.hpp
## DataManger.hpp 
Is for storing and handling the data efficiently. DataManager class stores an vector of Channels. a new channel is created for any unique log_id, measure or field. If the given log_id, measure or field allready exists in the vector Channel member inside the DataManager class it will just use the Channel that allready exists. The Channel class needs a type to be constructed. That type is based on what type the dsdl definitions says the field should be. The type is explcitly defined in the Deserialization.hpp file in all DataManager::AddDataPoint<[THE TYPE]> function.
DataManger.hpp only depends on InfluxDBClient.hpp. Otherwise files are usually dependent on DataManager.hpp. DataManger.hpp depends on InfluxDBClient.hpp becasue virtual CommonMembersChannel::writeToInfluxDB has to directly write to influxdb because that function has to be defined specifically based on which template typename the Channel class gets. The Channel class inherits from CommonMembersChannel where the virtual writeToInfluxDB is defined.
## Deserialization.hpp
Deserialization.hpp depends on DataManager.hpp as mentioned earlier. Deserialization.hpp is what directly writes data to DataManager class based on the given CAN data payload and revolve DSDL definitions.
## PcapReader.hpp
Depends on DataManager.hpp and Deserialization.hpp and ExtractUdpMsg.hpp.
## ExtractUdpMsg.hpp
Depends on DataManager.hpp and Deserialization.hpp.
## main.cpp
Depends on Analyze.hpp and temporarly for testing it depends on InfluxDBClient.hpp as well.

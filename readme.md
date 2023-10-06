# System Parameters

This document provides an overview of the parameters for the system. These parameters define various aspects of the system's behavior and configuration.

## Parameters

1. **nSpokes:** The size of nodes in the star.
   
2. **Number of Replica Servers:** The total number of replica servers used in the system.

3. **Number of Packets Sent by Each Client:** The number of packets sent by each client node to the load balancer or other nodes.

4. **Number of Clients Requesting Packets:** The total number of client nodes that request packets from the load balancer or other nodes.

5. **Error Rate of the Clients-LB Channel:** The error rate (or packet loss rate) of the communication channel between clients and the load balancer.

6. **Delay of Each Channel:** The delay (latency) of each communication channel in the system. This parameter defines the time it takes for packets to traverse a channel.

7. **Data Rate of Each Channel:** The data rate (bandwidth) of each communication channel in the system. This parameter defines the rate at which data can be transmitted over the channel.

8. **Dimension of the Sticky Cache:** The number of maximum entry of the cache for sticky sessione
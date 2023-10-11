#ifndef CUSTOM_TIME_H
#define CUSTOM_TIME_H


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/udp-echo-server.h"
#include "ns3/mobility-module.h"
#include "ns3/udp-socket.h"

using namespace ns3;

class CustomTime {
    public:
        CustomTime() {}


        static double getNow() {
        
            return (Simulator::Now().GetSeconds() + accumulatedDelay);
        }


        static void addDelay(double delayTime) {
            accumulatedDelay += delayTime;
        }

        
        static Time getNowInTime() {
            
            Time timeInSec = ns3::Seconds(accumulatedDelay);
            return Simulator::Now() + timeInSec;

        }

        static void SetAccumulatedDelay(double value); 

        
    private:

        static double accumulatedDelay;  

 

};


double CustomTime::accumulatedDelay = 0.0; 

#endif
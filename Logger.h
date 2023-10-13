#ifndef LOGGER_H
#define LOGGER_H


#include "CustomStarNode.h"

#include <vector>

using namespace std;
using namespace ns3;

class Logger {

    public:

        Logger() {}


        ~Logger() {}

        
        void addNode(Ptr<CustomStarNode> node) {
        
            this->clientVector.push_back(node);

        }


        void getStats() {
            Time currentTime = Simulator::Now();
            std::cout << "Simulation stopped at time " << currentTime.GetSeconds() << " seconds. Calculating stats" << std::endl;
            cout<<"Watching: "<<this->clientVector.size()<<" clients"<<endl;

            Simulator::Stop();
            for (const auto& node : this->clientVector) {
                std::unordered_map<uint, std::pair<Time, Time>>* clientStats = node->getClientStats();
                for (const auto& pair : *clientStats) {
                    cout<<pair<<endl;

                }
            }
        }


    private:

        std::vector<Ptr<CustomStarNode>> clientVector;
};

#endif
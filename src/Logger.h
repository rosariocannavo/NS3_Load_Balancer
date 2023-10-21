#ifndef LOGGER_H
#define LOGGER_H

#include "ns3/gnuplot.h"
#include "ns3/core-module.h"

#include "CustomStarNode.h"

#include <vector>
#include <fstream>

using namespace std;
using namespace ns3;


/**
 * 
 * This class Log the data created during the simulation and generate the plot
 * 
*/
class Logger {

    public:

        Logger(uint nClient, uint nReplicaServers, uint nPacketSendedByClient) {
            this->nClient = nClient;
            this->nPacketSendedByClient = nPacketSendedByClient;
            this->nReplicaServers = nReplicaServers;
        }


        ~Logger() {}

        
        void addNode(Ptr<CustomStarNode> node) {
            this->clientVector.push_back(node);
        }


        void getStats() {
            Time currentTime = Simulator::Now();
            std::cout << "Simulation stopped at time " << currentTime.GetSeconds() << " seconds. Calculating stats" << std::endl;
            cout<<"Watching: "<<this->clientVector.size()<<" clients"<<endl;

            Simulator::Stop();
            createPlot();
        }   


        void createPlot() {
            std::string fileNameWithNoExtension = "SimulationRTT";
            std::string graphicsFileName        = fileNameWithNoExtension + ".png";
            std::string plotFileName            = fileNameWithNoExtension + ".plt";

            Gnuplot2dDataset dataset[this->clientVector.size()+1];

            for(uint i=0; i < clientVector.size(); i++) {
                dataset[i].SetTitle("client "+to_string(i));
                dataset[i].SetStyle (Gnuplot2dDataset::LINES_POINTS);
            }
           
            dataset[clientVector.size()].SetTitle ("meanRTT");
            dataset[clientVector.size()].SetStyle (Gnuplot2dDataset::LINES);
            dataset[clientVector.size()].SetExtra("lc rgb \"red\"");


            int client = 0;
            Time totSum;
            Time partialSum;
            double maxTime = 0.0;
            for (const auto& node : this->clientVector) {
                
                partialSum = ns3::Seconds(0);
                int npacketsPerClient = 0;
                int i=0;
                std::unordered_map<uint, std::pair<Time, Time>>* clientStats = node->getClientStats();

                for (const auto& pair : *clientStats) {
                    if(pair.second.second > pair.second.first) {

                        partialSum += pair.second.second - pair.second.first;
                        npacketsPerClient++;

                        if(pair.second.second.GetSeconds() > maxTime) maxTime = pair.second.second.GetSeconds();
                        
                        dataset[client].Add(i, (pair.second.second.GetSeconds() - pair.second.first.GetSeconds())); //aggiunge tutti gli RTT
                        i++;

                    }
                }

                /*meanRTT for one client accumulated with the other*/
                totSum += partialSum/npacketsPerClient;
                               
                cout<<"meanRTT for client "<<client<<": "<<partialSum.GetSeconds()/npacketsPerClient<<" and npackets: "<<npacketsPerClient<<endl;

                //dataset[client].Add(client, partialSum.GetSeconds()/npacketsPerClient); //aggiunge solo le medie
                client++;
            }

            double totalMeanRTT = totSum.GetSeconds()/this->clientVector.size();

            cout<<"Total meanRTT with: "<<this->nReplicaServers<<" replicas: "<<totalMeanRTT<<endl;


            dataset[clientVector.size()].Add(0, totalMeanRTT);
            dataset[clientVector.size()].Add(this->nPacketSendedByClient,totalMeanRTT);
            
        
            Gnuplot plot ("simulationRTTFile");
            plot.SetTitle ("replicaserver: "+to_string(this->nReplicaServers));
            plot.SetTerminal ("png");
            plot.SetLegend ("packet id", "RTT");
            plot.AppendExtra ("set terminal pngcairo enhanced size 1200,1200"); 
            plot.AppendExtra ("set xrange [0:"+to_string(this->nPacketSendedByClient)+"]");     //xmax = npackets
            string ytick = "set ytics format \"%.3f\"";
            plot.AppendExtra(ytick);
            plot.AppendExtra ("set ytics 0.000, 1.500, "+to_string(maxTime));
            plot.AppendExtra ("set xtics 0, 1,"+to_string(this->nPacketSendedByClient));

            for(uint i=0; i < clientVector.size(); i++) {
                plot.AddDataset (dataset[i]);
            }
          
            plot.AddDataset(dataset[clientVector.size()]);
   
            std::ofstream plotFile(plotFileName.c_str());
            plot.GenerateOutput(plotFile);
            plotFile.close();
        }

   
    private:

        std::vector<Ptr<CustomStarNode>> clientVector;
        uint nClient;
        uint nPacketSendedByClient;
        uint nReplicaServers;

};

#endif

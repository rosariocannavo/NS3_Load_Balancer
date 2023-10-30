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

        Logger(uint nClient, uint nReplicaServers, uint nPacketSendedByClient, uint totalSimulationNumber) {
            this->nClient = nClient;
            this->nPacketSendedByClient = nPacketSendedByClient;
            this->nReplicaServers = nReplicaServers;
            this->totalSimulationNumber = totalSimulationNumber;
        }


        //copy constructor
        Logger(const Logger &other) {
            this->nClient = other.nClient;
            this->nReplicaServers = other.nReplicaServers;
            this->nPacketSendedByClient = other.nPacketSendedByClient;
            this->totalSimulationNumber = other.totalSimulationNumber;
        }


        ~Logger() {}

        
        void addNode(Ptr<CustomStarNode> node, int nOfSimulation) {
            this->simulationMap[nOfSimulation].push_back(node);
        }

        void simulateDisplay() {
            cout<<"I simulate display"<<endl;
            std::string fileNameWithNoExtension = "MARIOSARO";
            std::string graphicsFileName        = fileNameWithNoExtension + ".png";
            std::string plotFileName            = fileNameWithNoExtension + ".plt";

            Gnuplot2dDataset dataset[2];

            dataset[0].SetTitle("RTT");
            dataset[0].SetStyle (Gnuplot2dDataset::LINES_POINTS);

            dataset[1].SetTitle ("meanRTT");
            dataset[1].SetStyle (Gnuplot2dDataset::LINES);
            dataset[1].SetExtra("lc rgb \"red\"");
            
        
            double maxTime = 0.0;
       

            Time simulationRTT[this->totalSimulationNumber];
            for(uint nSim=0; nSim < this->totalSimulationNumber; nSim++) {
                
                Time meanRTTForSimulation;
                for (const auto& client : this->simulationMap[nSim]) {
                    std::unordered_map<uint, std::pair<Time, Time>>* clientStats = client->getClientStats();

                    Time clientMeanRTT;
                    for (const auto& timePair : *clientStats) {
                        clientMeanRTT += timePair.second.second - timePair.second.first;
                    }
                    meanRTTForSimulation += clientMeanRTT/this->nPacketSendedByClient;
                }   

                simulationRTT[nSim] = meanRTTForSimulation/this->nClient;
                if((meanRTTForSimulation/this->nClient).GetSeconds() > maxTime) maxTime = (meanRTTForSimulation/this->nClient).GetSeconds();

            }

            //asse x la i esima simulazione
            //asse y l'rtt medio dell'iesima simulazione
            uint i=0;
            double totalMeanRTT = 0.0;
            for(const auto& RTT: simulationRTT) {
                totalMeanRTT+= RTT.GetSeconds();
                dataset[0].Add(i, RTT.GetSeconds());
                i++;
            }
            totalMeanRTT = totalMeanRTT/this->totalSimulationNumber;


            Gnuplot plot ("MARIOSARO");
            plot.SetTitle ("replicaserver: "+to_string(this->nReplicaServers));
            plot.SetTerminal ("png");
            plot.SetLegend ("simulation number", "Simulation RTT");
            plot.AppendExtra ("set terminal pngcairo enhanced size 1200,1200"); 

            string ytick = "set ytics format \"%.2f\"";
            plot.AppendExtra(ytick);

            plot.AppendExtra ("set xrange [0:"+to_string(this->totalSimulationNumber)+"]");     //xmax = npackets
            plot.AppendExtra ("set yrange [0:"+to_string(maxTime*2)+"]");     //xmax = npackets

           

            plot.AppendExtra ("set ytics 0.00, 1.0, "+to_string(maxTime*2));
            //plot.AppendExtra ("set ytics 0.00, 3.00, "+to_string(maxTime));

            plot.AppendExtra ("set xtics 0, 5,"+to_string(this->totalSimulationNumber));



            dataset[1].Add(0, totalMeanRTT);
            dataset[1].Add(this->totalSimulationNumber, totalMeanRTT);
        
            plot.AddDataset(dataset[0]);
            plot.AddDataset(dataset[1]);

   
            std::ofstream plotFile(plotFileName.c_str());
            plot.GenerateOutput(plotFile);
            plotFile.close();

               
    
        }

        void simulateStats(uint nOfSimulation) {
            Time currentTime = Simulator::Now();
            cout << "Simulation "<<nOfSimulation<<" stopped at time " << currentTime.GetSeconds() << " seconds. Calculating stats" << std::endl;
            cout<<"number of client in simulation "<<nOfSimulation<<": "<<this->simulationMap[nOfSimulation].size()<<endl;

            Simulator::Stop();
            for (const auto& node : this->simulationMap[nOfSimulation]) {
               cout<<node->getClientStats()<<endl;
            }  

            cout<<"TOTAL SIM NUMBER "<<this->totalSimulationNumber<<endl;

            //if all the simulation runned
            if(nOfSimulation == this->totalSimulationNumber-1) {
                simulateDisplay();
            }

            
        }

       


        // void getStats() {
        //     Time currentTime = Simulator::Now();
        //     std::cout << "Simulation stopped at time " << currentTime.GetSeconds() << " seconds. Calculating stats" << std::endl;
        //     cout<<"Watching: "<<this->clientVector.size()<<" clients"<<endl;

        //     Simulator::Stop();
        //     createPlot();
        // }   


        // void createPlot() {
        //     std::string fileNameWithNoExtension = "SimulationRTT";
        //     std::string graphicsFileName        = fileNameWithNoExtension + ".png";
        //     std::string plotFileName            = fileNameWithNoExtension + ".plt";

        //     Gnuplot2dDataset dataset[this->clientVector.size()+1];

        //     for(uint i=0; i < clientVector.size(); i++) {
        //         dataset[i].SetTitle("client "+to_string(i));
        //         dataset[i].SetStyle (Gnuplot2dDataset::LINES_POINTS);
        //     }
           
        //     dataset[clientVector.size()].SetTitle ("meanRTT");
        //     dataset[clientVector.size()].SetStyle (Gnuplot2dDataset::LINES);
        //     dataset[clientVector.size()].SetExtra("lc rgb \"red\"");


        //     int client = 0;
        //     Time totSum;
        //     Time partialSum;
        //     double maxTime = 0.0;
        //     for (const auto& node : this->clientVector) {
                
        //         partialSum = ns3::Seconds(0);
        //         int npacketsPerClient = 0;
        //         int i=0;
        //         std::unordered_map<uint, std::pair<Time, Time>>* clientStats = node->getClientStats();

        //         for (const auto& pair : *clientStats) {
        //             if(pair.second.second > pair.second.first) {

        //                 partialSum += pair.second.second - pair.second.first;
        //                 npacketsPerClient++;

        //                 if(pair.second.second.GetSeconds() > maxTime) maxTime = pair.second.second.GetSeconds();
                        
        //                 dataset[client].Add(i, (pair.second.second.GetSeconds() - pair.second.first.GetSeconds())); //aggiunge tutti gli RTT
        //                 i++;

        //             }
        //         }

        //         /*meanRTT for one client accumulated with the other*/
        //         totSum += partialSum/npacketsPerClient;
                               
        //         cout<<"meanRTT for client "<<client<<": "<<partialSum.GetSeconds()/npacketsPerClient<<" and npackets: "<<npacketsPerClient<<endl;

        //         //dataset[client].Add(client, partialSum.GetSeconds()/npacketsPerClient); //aggiunge solo le medie
        //         client++;
        //     }

        //     double totalMeanRTT = totSum.GetSeconds()/this->clientVector.size();

        //     cout<<"Total meanRTT with: "<<this->nReplicaServers<<" replicas: "<<totalMeanRTT<<endl;


        //     dataset[clientVector.size()].Add(0, totalMeanRTT);
        //     dataset[clientVector.size()].Add(this->nPacketSendedByClient,totalMeanRTT);
            
        
        //     Gnuplot plot ("simulationRTTFile");
        //     plot.SetTitle ("replicaserver: "+to_string(this->nReplicaServers));
        //     plot.SetTerminal ("png");
        //     plot.SetLegend ("packet id", "RTT");
        //     plot.AppendExtra ("set terminal pngcairo enhanced size 1200,1200"); 
        //     plot.AppendExtra ("set xrange [0:"+to_string(this->nPacketSendedByClient)+"]");     //xmax = npackets
        //     string ytick = "set ytics format \"%.3f\"";
        //     plot.AppendExtra(ytick);
        //     plot.AppendExtra ("set ytics 0.000, 1.500, "+to_string(maxTime));
        //     plot.AppendExtra ("set xtics 0, 1,"+to_string(this->nPacketSendedByClient));

        //     for(uint i=0; i < clientVector.size(); i++) {
        //         plot.AddDataset (dataset[i]);
        //     }
          
        //     plot.AddDataset(dataset[clientVector.size()]);
   
        //     std::ofstream plotFile(plotFileName.c_str());
        //     plot.GenerateOutput(plotFile);
        //     plotFile.close();
        // }

   
    private:

        std::vector<Ptr<CustomStarNode>> clientVector;
        uint nClient;
        uint nPacketSendedByClient;
        uint nReplicaServers;
        uint totalSimulationNumber;
        std::map<int, std::vector<Ptr<CustomStarNode>>> simulationMap;


};

#endif

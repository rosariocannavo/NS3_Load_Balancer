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
            std::string fileNameWithNoExtension = "PLOT";
            std::string graphicsFileName        = fileNameWithNoExtension + ".png";
            std::string plotFileName            = fileNameWithNoExtension + ".plt";

            Gnuplot2dDataset dataset[2];

            string title = "% of client within the range";
            dataset[0].SetTitle(title);
            dataset[0].SetStyle (Gnuplot2dDataset::LINES_POINTS);

            dataset[1].SetTitle ("meanRTT");
            dataset[1].SetStyle (Gnuplot2dDataset::LINES);
            dataset[1].SetExtra("lc rgb \"red\"");
            
            std::vector<Time> MeanRTT;
           


            for(uint nSim=0; nSim < this->totalSimulationNumber; nSim++) {
                
                for (const auto& client : this->simulationMap[nSim]) {
                    std::unordered_map<uint, std::pair<Time, Time>>* clientStats = client->getClientStats();


                    Time clientMeanRTT;

                    for (const auto& timePair : *clientStats) {
                        clientMeanRTT += timePair.second.second - timePair.second.first;
                    }

                    MeanRTT.push_back(clientMeanRTT/this->nPacketSendedByClient);
                }  
            }

            Time sum;
            for (uint i = 0; i < MeanRTT.size(); ++i) {
                sum += MeanRTT[i];
            }

            double totalMeanRTT = sum.GetSeconds() / MeanRTT.size();

            cout<<"totalMeanRTT: "<<totalMeanRTT<<endl;

            std::sort(MeanRTT.begin(), MeanRTT.end());

            double minTime = 0; 
            double maxTime = MeanRTT[MeanRTT.size() - 1].GetSeconds();


            cout<<"min: "<<minTime<<endl;   
            cout<<"max: "<<maxTime<<endl;

            double delta = (maxTime - minTime)/100; 

            cout<<"delta: "<<delta<<endl;  

            vector<int> ElementsInRange;
            for(int i=0; i<100; i++){  
                ElementsInRange.push_back(countElementsInRange(MeanRTT, minTime, delta*i));
            }

            int k = 0;
            for(int elem : ElementsInRange) {
                cout << "Number of elements in range [" << minTime << ", " << minTime + (delta*k) << "): " << elem <<endl;
                k++;
            }

            normalizeToPercentage(ElementsInRange);

            //now is percentage
            for(int elem : ElementsInRange) {
                cout <<elem<<", ";
            }
            cout<<endl;
            

            //creating dataset 
            k = 0;
            for(int elem : ElementsInRange) {
                dataset[0].Add(minTime+ (k*delta), elem);
                k++;
            }
       

            Gnuplot plot ("PLOT");
            plot.SetTitle ("replicaserver: "+to_string(this->nReplicaServers));
            plot.SetTerminal ("png");
            plot.SetLegend ("MeanRTT + delta", title);
            plot.AppendExtra ("set terminal pngcairo enhanced size 1200,1200"); 


            plot.AddDataset(dataset[0]);

   
            std::ofstream plotFile(plotFileName.c_str());
            plot.GenerateOutput(plotFile);
            plotFile.close();

        }


        int countElementsInRange(const std::vector<Time>& vec, double x, double delta) {
            int count = 0;
            for (Time num : vec) {
                if (num.GetSeconds() >= x && num.GetSeconds() <= (x + delta)) {
                    count++;
                }
            }
            return count;
        }


        void normalizeToPercentage(std::vector<int>& numbers) {
            // Find minimum and maximum values in the vector
            int minVal = *std::min_element(numbers.begin(), numbers.end());
            int maxVal = *std::max_element(numbers.begin(), numbers.end());

            // Normalize each element in the vector
            for (int& num : numbers) {
                num = static_cast<int>((static_cast<double>(num - minVal) / (maxVal - minVal)) * 100.0);
            }
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


    private:

        std::vector<Ptr<CustomStarNode>> clientVector;
        uint nClient;
        uint nPacketSendedByClient;
        uint nReplicaServers;
        uint totalSimulationNumber;
        std::map<int, std::vector<Ptr<CustomStarNode>>> simulationMap;

};

#endif

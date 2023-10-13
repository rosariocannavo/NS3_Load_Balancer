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
 * TODO: Fix this class
 * 
*/
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
            createPlot();

        }   


        void createPlot() {
            std::string fileNameWithNoExtension = "DemoPlot";
            std::string graphicsFileName        = fileNameWithNoExtension + ".png";
            std::string plotFileName            = fileNameWithNoExtension + ".plt";

            Gnuplot2dDataset dataset[3];
            dataset[0].SetTitle ("client 0");
            dataset[1].SetTitle ("client 1");
            dataset[2].SetTitle ("meanRTT");

            dataset[0].SetStyle (Gnuplot2dDataset::LINES_POINTS);
            dataset[1].SetStyle (Gnuplot2dDataset::LINES_POINTS);
            dataset[2].SetStyle (Gnuplot2dDataset::LINES_POINTS);
            dataset[2].SetExtra("lc rgb \"red\"");

            int client = 0;
            int npackets = 0;
            Time sum = 0.0;

            for (const auto& node : this->clientVector) {

                int i=0;
                std::unordered_map<uint, std::pair<Time, Time>>* clientStats = node->getClientStats();

                for (const auto& pair : *clientStats) {

                    if(pair.second.second > pair.second.first) {

                        sum += pair.second.second - pair.second.first;
                        npackets++;
                        cout<<i<<" client "<<client<<" "<<pair.second.second.GetSeconds() - pair.second.first.GetSeconds()<<endl;
                        dataset[client].Add(i, (pair.second.second.GetSeconds() - pair.second.first.GetSeconds()));
                        i++;

                    }
                 
                }

                cout<<endl;
                client++;
            }


            dataset[2].Add(0, sum.GetSeconds()/npackets);
            dataset[2].Add(10,sum.GetSeconds()/npackets);
            
        
            Gnuplot plot ("testFile");
    
            plot.SetTitle ("replicaserver: 2");
            plot.SetTerminal ("png");
            plot.SetLegend ("packet id", "RTT");
            
            plot.AppendExtra ("set terminal pngcairo enhanced size 800,800"); 

            plot.AppendExtra ("set xrange [0:10]");     //xmax = npackets
            
            string ytick = "set ytics format \"%.3f\"";
            plot.AppendExtra(ytick);
            plot.AppendExtra ("set ytics 0.000, 0.100, 10.00");
          
            plot.AddDataset (dataset[0]);
            plot.AddDataset (dataset[1]);
          
            plot.AddDataset (dataset[2]);
            std::ofstream plotFile (plotFileName.c_str());
            plot.GenerateOutput (plotFile);
            plotFile.close ();
        }

   
    private:

        std::vector<Ptr<CustomStarNode>> clientVector;

};

#endif
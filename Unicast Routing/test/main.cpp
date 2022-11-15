#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <iostream>
#include <vector>
#include <utility>
#include <fstream>
#include <map> 

class LSA
{
	public:

	LSA(int globalMyID)
	{	
		myID=globalMyID;
		sequenceNumber=1;
	}
	LSA(){sequenceNumber=1;}

	int myID;
	int sequenceNumber;
	//neighbor id/cost;
	std::map<int,int> myNeighbor;
};

class AllLSA
{
	public:
	AllLSA()
	{
		myNeighborsize=0;
	}
	//neighborsize=0 if no neighbor;

	int myNeighborsize;
	std::vector<LSA*> neighborLSA;

};
//HELPER FUNCTION

void initialize(std::string filename, LSA* myLSA, AllLSA* myworld);
void initialize(std::string filename, LSA* myLSA, AllLSA* myworld)
{	

	//initialize worldLSA with head pointer to cur node.
    std::string line;
    std::ifstream costFile(filename);
	while(std::getline(costFile,line))
	{	

		std::string neighbor;
		std::string cost;
        int i=0;
        while(line[i]!=' ')
        {   neighbor.push_back(line[i]);
            i++;
        }
        while(i<line.size())
        {
            cost.push_back(line[i]);
            i++;
        }
		
		myLSA->myNeighbor[stoi(neighbor)]=stoi(cost);
		myLSA->sequenceNumber=1;
		myworld->myNeighborsize++;
	}
}
//output a msg used for sending LSA
std::string outputCost(LSA *myLSA,int globalMyID)
{	
	std::string buffer;
	buffer.append("LSAS"+std::to_string(myLSA->sequenceNumber)+"*");
	buffer.append("nodeID"+std::to_string(globalMyID)+"*");
	for(auto iter=myLSA->myNeighbor.begin();iter!=myLSA->myNeighbor.end();iter++)
	{	
		buffer.append(std::to_string(iter->first)+','+std::to_string(iter->second) + "/");
	}
	std::cout<<buffer<<std::endl;
	return buffer;
}
//PARSE INFO RECEIVED
//parse LSA from one neighbor and save info into neighborLSA
LSA* parseLSA(std::string buffer)
{
	LSA* neighborLSA=new LSA;
	int i=4;
	std::string sequence;
	while(buffer[i]!='*'&&i<buffer.size())
	{
		sequence.push_back(buffer[i]);
		i++;
	}
	int temp=std::stoi(sequence);
	neighborLSA->sequenceNumber=temp;
	i=i+7;
	std::string nodeID;
	while(buffer[i]!='*'&&i<buffer.size())
	{
		nodeID.push_back(buffer[i]);
		i++;
	}
	neighborLSA->myID=std::stoi(nodeID);

	i++;

	for(;i<buffer.size();i++)
	{
		std::string neighborID;
		std::string neighborCost;
		while(buffer[i]!=',')
		{
			neighborID.push_back(buffer[i]);
			i++;
		}
		i++;
		while(buffer[i]!='/')
		{
			neighborCost.push_back(buffer[i]);
			i++;
		}
		neighborLSA->myNeighbor[std::stoi(neighborID)]=std::stoi(neighborCost);
	}
	return neighborLSA;
}




int main()
{

    

    return 0;


}
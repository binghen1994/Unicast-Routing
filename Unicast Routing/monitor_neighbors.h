#pragma  once
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
#include <unordered_map>
#include <map> 
#include <cstring>
#include <sys/time.h>
#include <queue>
#include <mutex>
#include <algorithm>


extern int globalMyID;
//last time you heard from each node. TODO: you will want to monitor this
//in order to realize when a neighbor has gotten cut off from you.
extern struct timeval globalLastHeartbeat[256];

//our all-purpose UDP socket, to be bound to 10.1.1.globalMyID, port 7777
extern int globalSocketUDP;
//pre-filled for sending to 10.1.1.0 - 255, port 7777
extern struct sockaddr_in globalNodeAddrs[256];
// extern AllLSA* globalLSATicket;

//dijkstra result
extern std::unordered_map<int, std::pair<int, int>> destMap;
//key: destiation, value: <next node id, total cost>
extern FILE* logFile;

extern pthread_mutex_t mtx;

extern bool globalUpdate;

//Yes, this is terrible. It's also terrible that, in Linux, a socket
//can't receive broadcast packets unless it's bound to INADDR_ANY,
//which we can't do in this assignment.

class LSA
{
	public:
	LSA(int myneighbor, int mycost)
	{
		neighborID=myneighbor;
		cost=mycost;
		sequenceNumber=0;
		isUp=1;
	}
	LSA()
	{
		cost=0;
		sequenceNumber=0;
		isUp=1;
	}

	int neighborID;
	int cost;
	int sequenceNumber;
	bool isUp;
};

class LSATicket
{
	public:
	LSATicket(){};
	LSATicket(int myID)
	{
		fromID=myID;
	}
	int fromID;
	//LSAs
	std::vector<LSA*> aTicket; 
};

class AllLSA
{
	public:
	AllLSA()
	{
		myWorldsize=1;
	}

	int myWorldsize;
	//id, LSATicket;
	std::vector<LSATicket*> globalLSA;

};

extern AllLSA* globalLSATicket;
extern LSATicket* myLSATicket;

//dijkstra comparator class
class dCompare {
	public:
	bool operator() (std::vector<int> &a, std::vector<int> &b){
		return a[0] > b[0];
	}
};




//MAIN FUNCTION

//HELPER FUNCTION

//output a msg used for sending LSA
std::string outputCost(LSA *aLSA,int ticketId)
{	
	//LSAfromID,neighbor,cost,sequenceNumber,isUp
	std::string buffer;
	buffer.append("LSA"+std::to_string(ticketId)+","+std::to_string(aLSA->neighborID)+","+std::to_string(aLSA->cost)+","+std::to_string(aLSA->sequenceNumber)+","+std::to_string(aLSA->isUp));
	//std::cout<<buffer<<std::endl;
	return buffer;
}	

//print all LSAs stored in world
void printALLLSA(AllLSA* globalLSATicket)
{
	
	for(int i=0;i<globalLSATicket->globalLSA.size();i++)
	{	
		for(int j=0;j<globalLSATicket->globalLSA[i]->aTicket.size();j++)
		{
			std::string print=outputCost(globalLSATicket->globalLSA[i]->aTicket[j],globalLSATicket->globalLSA[i]->fromID);
			std::cout<<print<<std::endl;

		}
	}
}

void printMyLSA() {

	for(int j=0;j<myLSATicket->aTicket.size();j++) {
		std::string toPrint = outputCost(myLSATicket->aTicket[j], myLSATicket->fromID);
		std::cout<< toPrint <<std::endl;
	}

}



//find LSATickets from specific ID, return -1 if no such node exist;
LSATicket* findTicket(int findID)
{
	for(int i=0; i<globalLSATicket->globalLSA.size(); i++)
	{
		if(globalLSATicket->globalLSA[i]->fromID==findID)
		{
			return globalLSATicket->globalLSA[i];
		}
		
	}
	return nullptr;
}

//find a LSA by neighborID in a LSATicket
LSA* findLSA(int findID,LSATicket* aTicket)
{
	for(int i=0;i<aTicket->aTicket.size();i++)
	{
		if(aTicket->aTicket[i]->neighborID==findID)
		{
			return aTicket->aTicket[i];
		}
	}
	return nullptr;
}
//find position of a LSA by neighborID in a LSATicket
int findLSAposition(int findID,LSATicket* aTicket)
{
	for(int i=0;i<aTicket->aTicket.size();i++)
	{
		if(aTicket->aTicket[i]->neighborID==findID)
		{
			return i;
		}
	}
	return -1;
}

//update all LSAs when one node down
void updateLSA(int changeId, bool status)
{
	LSA* changeLSA=findLSA(changeId,myLSATicket);
	changeLSA->isUp=status;
	
}
//get all node cost info

void printNeighborID()
{
	for(int i=0; i<myLSATicket->aTicket.size();i++)
	{
		int destID=myLSATicket->aTicket[i]->neighborID;
		std::cout<<"MyNeighbor ID= "<<destID<<std::endl<<std::endl;
	}
}


void dijkstra()
{
	printALLLSA(globalLSATicket);
	destMap.clear();
	int nodeSize= globalLSATicket->globalLSA.size();
	std::priority_queue<std::vector<int>, std::vector<std::vector<int>>, dCompare> pq;
	//int[0]: current cost, int[1] current node, int[2] next neighbour

	std::vector<int> visited(256, (1 << 30));

	pq.push(std::vector<int>{0, globalMyID, -1});

	while(!pq.empty()){
		std::vector<int> cur = pq.top();

		std::cout << "Currently doing node " << cur[1] << " with cost of " << cur[0] << " with nexthop of " << cur[2] << std::endl;

		pq.pop();

		//visited but not the best route
		if(visited[cur[1]] < cur[0]){
			continue;
		}

		//get real first step
		int curNext = cur[2];
		if(curNext == -1 || curNext == globalMyID){
			curNext = cur[1];
		}

		//record to destmap
		if(cur[1] != globalMyID){
			if(destMap.count(cur[1])){
				//if destmap has recorded this
				if(destMap[cur[1]].second == cur[0]){
					//if we find same cost
					//std::cout << "Try to change node " << cur[1] << " with orig nexthop of " << destMap[cur[1]].first << " with new nexthop of " << curNext << " with same cost of " << cur[0] << std::endl;
					destMap[cur[1]].first = std::min(destMap[cur[1]].first, curNext);
				}
			} else {
				//if destmap does not have this
				//std::cout << "Recording node " << cur[1] << " with nexthop of " << curNext << " and cost of " << cur[0] << std::endl;
				destMap[cur[1]] = std::pair<int, int>(curNext, cur[0]);
			}
			//std::cout<<"aPathID= "<<cur[1]<<"aPathnext hop= "<<destMap[cur[1]].first<<std::endl;
		}
		
		//iterate neighbors
		LSATicket* curTicketP = findTicket(cur[1]);
		if(curTicketP!=nullptr){
			for(LSA* lsaP: curTicketP -> aTicket){
				//std::cout << "Trying neighbour " << lsaP -> neighborID << std::endl;
				//std::cout << "Neighbour " << lsaP -> neighborID << " visited " << visited[lsaP -> neighborID] << ", isUp: " << lsaP->isUp << std::endl;

				if(visited[lsaP -> neighborID] < (cur[0] + lsaP -> cost) || lsaP->isUp==0){
					// if(lsaP->isUp==0)
					// {	
					// 	std::cout<<"Dij down node excape"<<std::endl;
					// }
					continue;
				}

				std::cout << "Pushing neighbour " << lsaP -> neighborID << " with cost of " << (cur[0] + lsaP -> cost) << " with nexthop of " << curNext << std::endl;

				visited[lsaP -> neighborID] = (cur[0] + lsaP -> cost);

				pq.push(std::vector<int>{cur[0] + lsaP -> cost, lsaP -> neighborID, curNext});
			}
		}
	}

}

void printAllpath()
{
	std::cout<<"PrintAllPath: My ID is "<< globalMyID <<std::endl;

	// for(int i=0; i<globalLSATicket->globalLSA.size();i++)
	// {
	// 	int destID=globalLSATicket->globalLSA[i]->fromID;
	// 	if(destID!=globalMyID)
	// 	{
	// 		std::cout << "DestID = " << destID << ", nexthop is " << (destMap.find(destID) -> second).first << " with cost of " << (destMap.find(destID) -> second).second << std::endl;
	// 	}
			
	// }

	for(auto p: destMap) {
		int destID = p.first;
		if(destID!=globalMyID)
		{
			std::cout << "Dest ID = " << destID << ", nexthop is " << (destMap.find(destID) -> second).first << " with cost of " << (destMap.find(destID) -> second).second << std::endl;
		}
			
	}
}

void printAllKnown(){

	std::cout<<"PrintAllKnown: My ID is "<<globalMyID<<std::endl;
	std::cout << "Cur node recognizing: ";

	for(int i=0; i<globalLSATicket->globalLSA.size();i++)
	{
		int destID=globalLSATicket->globalLSA[i]->fromID;
		std::cout << destID << ", ";
	}

	std::cout << " total node count of " << globalLSATicket->globalLSA.size() << std::endl;
}


void initialize(std::string filename, LSATicket* myLSATicket)
{	
	//std::cout<<"initializing"<<std::endl;
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
		i++;
        while(i<line.size())
        {
            cost.push_back(line[i]);
            i++;
        }
		//std::cout<<neighbor<<std::endl;
		//std::cout<<cost<<std::endl;
		LSA* aLSA=new LSA(std::stoi(neighbor),std::stoi(cost));
		myLSATicket->aTicket.push_back(aLSA);
		
	}
	globalLSATicket->globalLSA.push_back(myLSATicket);
	globalUpdate=1;
}




//PARSE INFO RECEIVED
//parse LSA from one neighbor and save info into neighborLSA
void parseaLSA(std::string buffer,LSATicket* neighborLSATicket)
{
	if(buffer.size()<=4)
	{
		return;
	}
	int i=3;
	std::string fromID;
	LSA* aLSA= new LSA;
	while(i<buffer.size()&&buffer[i]!=',')
	{
		fromID.push_back(buffer[i]);
		i++;
	}
	neighborLSATicket->fromID=std::stoi(fromID);
	i++;
	std::string nodeID;
	while(i<buffer.size()&&buffer[i]!=',')
	{
		nodeID.push_back(buffer[i]);
		i++;
	}
	aLSA->neighborID=std::stoi(nodeID);
	i++;
	std::string cost;
	while(i<buffer.size()&&buffer[i]!=',')
	{
		cost.push_back(buffer[i]);
		i++;
	}
	aLSA->cost=std::stoi(cost);
	i++;
	std::string sequence;
	while(i<buffer.size()&&buffer[i]!=',')
	{
		sequence.push_back(buffer[i]);
		i++;
	}
	aLSA->sequenceNumber=std::stoi(sequence);
	i++;
	std::string isUp;
	while(i<buffer.size()&&buffer[i]!=',')
	{
		isUp.push_back(buffer[i]);
		i++;
	}
	aLSA->isUp=std::stoi(isUp);
	neighborLSATicket->aTicket.push_back(aLSA);
	
}

//BROADCAST INFOS
//broadcast to all IPs to find neighbors
void hackyBroadcast(const char* buf, int length)
{
	int i;
	for(i=0;i<256;i++)
		if(i != globalMyID) //(although with a real broadcast you would also get the packet yourself)
			sendto(globalSocketUDP, buf, length, 0,
				  (struct sockaddr*)&globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
}

//broadcast aLSATicket to all it's neighbors
void lsaBroadcast(LSATicket* aLSATicket)
{
	std::vector<int> neighborsID;
	for(int i=0; i<myLSATicket->aTicket.size();i++)
	{
		neighborsID.push_back(myLSATicket->aTicket[i]->neighborID);
		//std::cout<<"neighborsID"<<myLSATicket->aTicket[i]->neighborID<<std::endl;
	}
	for(int i=0;i<neighborsID.size();i++)
	{	
		for(int j=0;j<aLSATicket->aTicket.size();j++)
		{
			std::string buffer;
			buffer= outputCost(aLSATicket->aTicket[j],aLSATicket->fromID);
			//std::cout<<"broadcast buffer"<<buffer<<std::endl;
			//std::cout<<"broadcast neighborID"<<neighborsID[i]<<std::endl;
			const char* buf= buffer.c_str();
			sendto(globalSocketUDP, buf, strlen(buf), 0,
					(struct sockaddr*)&globalNodeAddrs[neighborsID[i]], sizeof(globalNodeAddrs[neighborsID[i]]));
		}
	}
}

//boradcast all LSATickets in world to only neighbors
void boradcastAllLSA(AllLSA* globalLSATicket)
{

	for(int i=0;i<globalLSATicket->globalLSA.size();i++)
	{
		LSATicket* aLSATicket= globalLSATicket->globalLSA[i];
		//std::cout<<"broadcast neighborID= "<<globalLSATicket->globalLSA[i]->fromID;
		lsaBroadcast(aLSATicket);
	}
}





//THREADS
//heartbeat send()
void* heartBeat(void* unusedParam)
{
	
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec= 300*1000*1000;
	char heartBeat[10];
	sprintf(heartBeat, "H%d", globalMyID);
	int sizeHeartBeat = strlen(heartBeat);
	while(1)
	{	
		hackyBroadcast(heartBeat,sizeHeartBeat);	
		nanosleep(&sleepFor,0);
	}
	
	

}

// global LSA send()
void* announceLSA(void* unusedParam)
{
	
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec= 300*1000*1000;
	while(1)
	{	
		boradcastAllLSA(globalLSATicket);
		nanosleep(&sleepFor,0);
	}
	
}



void* listenForNeighbors(void* logFile)
{

	
	char fromAddr[100];
	struct sockaddr_in theirAddr;
	socklen_t theirAddrLen;
	char recvBuf[1000];

	int bytesRecvd;
	
	while(1)
	{	
		

		
		
		theirAddrLen = sizeof(theirAddr);
		if ((bytesRecvd = recvfrom(globalSocketUDP, recvBuf, 1000 , 0, 
					(struct sockaddr*)&theirAddr, &theirAddrLen)) == -1)
		{
			//std::cout<<"recv failed"<<std::endl;
			perror("connectivity listener: recvfrom failed");
			exit(1);
		}
		recvBuf[bytesRecvd]= '\0';
		std::string test;
		test.append(recvBuf);
		
		//std::cout<<"recvBuf"<<test<<std::endl;
		inet_ntop(AF_INET, &theirAddr.sin_addr, fromAddr, 100);
		
		short int heardFrom = -1;
		
		if(strstr(fromAddr, "10.1.1."))
		{
			heardFrom = atoi(
					strchr(strchr(strchr(fromAddr,'.')+1,'.')+1,'.')+1);
			
			//TODO: this node can consider heardFrom to be directly connected to it; do any such logic now.
			
			//record that we heard from heardFrom just now.
			gettimeofday(&globalLastHeartbeat[heardFrom], 0);
		}

		//check neighbors
		struct timeval current;
		gettimeofday(&current,0);
		//std::cout<<"size"<<myLSATicket->aTicket.size()<<std::endl;
		//
		if(myLSATicket==nullptr)
		{
			//std::cout<<"nullptr"<<std::endl;
		}
		for(int i=0;i<myLSATicket->aTicket.size();i++)
		{
			//std::cout<<"check"<<std::endl;
			if((current.tv_sec-globalLastHeartbeat[myLSATicket->aTicket[i]->neighborID].tv_sec)*1000000+(current.tv_usec-globalLastHeartbeat[myLSATicket->aTicket[i]->neighborID].tv_usec)>700000)
			{
				//std::cout<<"nodeID= "<<myLSATicket->aTicket[i]->neighborID<<"time exceed limit"<<std::endl;
				pthread_mutex_lock(&mtx);
				if(myLSATicket->aTicket[i]->isUp!=0)
				{
					myLSATicket->aTicket[i]->sequenceNumber++;
				}
				myLSATicket->aTicket[i]->isUp=0;
				globalUpdate=1;
				pthread_mutex_unlock(&mtx);
				
			}

			else
			{
				if(myLSATicket->aTicket[i]->isUp==0)
				{
					pthread_mutex_lock(&mtx);
					myLSATicket->aTicket[i]->isUp=1;
					myLSATicket->aTicket[i]->sequenceNumber++;
					globalUpdate=1;
					pthread_mutex_unlock(&mtx);
				}
			}
		}

		//std::cout<<"checking complete"<<std::endl;
		//std::cout<<"recv complete"<<std::endl;

		//if heardfrom an unknown node
		if(!strncmp(recvBuf, "H", 1))
		{
			
			if(!findLSA(heardFrom,myLSATicket))
			{
				//std::cout<<"recvH"<<std::endl;
				//std::cout<<"recvBuf"<<test<<std::endl;
				//printALLLSA(globalLSATicket);
				
				LSA* aLSA= new LSA;
				aLSA->cost=1;
				aLSA->neighborID=heardFrom;
				aLSA->isUp=1;
				// need to update when one node has cost large than one and hasn't received LSA but receive heartbeat first
				aLSA->sequenceNumber=-1;
				pthread_mutex_lock(&mtx);
				myLSATicket->aTicket.push_back(aLSA);
				globalUpdate=1;
				globalLSATicket->myWorldsize++;
				pthread_mutex_unlock(&mtx);
				//printALLLSA(globalLSATicket);	
			}
		}
		

		//if from LSA 
		if(!strncmp(recvBuf, "LSA", 3))
		{
			//std::cout<<"RecvBuf"<<test<<std::endl<<std::endl;
			//std::cout<<"recvlsa"<<std::endl;
			LSATicket* newLSATicket= new LSATicket;
			std::string recv;
			recv.append(recvBuf);
			parseaLSA(recv,newLSATicket);
			//if it's the first time receive LSATicket from this node
			if(!findTicket(newLSATicket->fromID))
			{
				//std::cout<<"recvnewlsaTicket"<<std::endl;

				pthread_mutex_lock(&mtx);
				globalLSATicket->globalLSA.push_back(newLSATicket);
				globalLSATicket->myWorldsize++;
				globalUpdate=1;
				pthread_mutex_unlock(&mtx);
			}
			else
			{	//update existing LSAs if sequence number is newer
				LSATicket* old=findTicket(newLSATicket->fromID);
				//std::cout<<"recvnewlsa"<<std::endl;
				int findID=newLSATicket->aTicket[0]->neighborID;
				
				if(!findLSA(findID,old))
				{//std::cout<<"addnewlsa"<<std::endl;
					old->aTicket.push_back(newLSATicket->aTicket[0]);
					globalLSATicket->myWorldsize++;
					globalUpdate=1;
					
				}

				else
				{
					if(findLSA(findID,old)->sequenceNumber<(newLSATicket->aTicket[0]->sequenceNumber))
					{
						//std::cout<<"recvupdatelsa"<<std::endl;
						old->aTicket[findLSAposition(findID,old)]=newLSATicket->aTicket[0];
						globalUpdate=1;
					}
				}											
				//std::cout<<"recvnewlsa success"<<std::endl;
				
				

			}
		}

		//std::cout<<"updatecomplete"<<std::endl;
		//printALLLSA(globalLSATicket);
		//Is it a packet from the manager? (see mp2 specification for more details)
		//send format: 'send'<4 ASCII bytes>, destID<net order 2 byte signed>, <some ASCII message>
		if(!strncmp(recvBuf, "send", 4))
		{
			//std::cout<<"sendBuf= "<<test<<std::endl;

			int destId= ntohs((recvBuf[5]<<8| recvBuf[4]));
			//std::cout<<"send dest ID"<<destId<<std::endl;


			if(globalUpdate==1)
			{
				//printAllKnown();
				//printMyLSA();

				std::cout << std::endl << "Starting Dijkstra now!" << std::endl;
				pthread_mutex_lock(&mtx);
				dijkstra();
				pthread_mutex_unlock(&mtx);
			}
				printAllpath();

			if(destId==globalMyID)
			{
				char logLine1[150];
				sprintf(logLine1, "receive packet message %s\n", recvBuf + 6); 
				fprintf((FILE*)logFile, logLine1);
				fflush((FILE*)logFile);
				//std::cout<<"receive"<<std::endl;
			
			}
			else
			{
				if(destMap.find(destId)==destMap.end())
				{
					char logLine2[150];
					sprintf(logLine2, "unreachable dest %d\n", destId); 
					fprintf((FILE*)logFile, logLine2);
					fflush((FILE*)logFile);
					//std::cout<<"unreachable"<<std::endl;
				}
				else
				{
					char logLine3[150];
					int nexthop = destMap[destId].first;
					recvBuf[0]='m';
					sendto(globalSocketUDP, recvBuf, bytesRecvd, 0,	(struct sockaddr*)&globalNodeAddrs[nexthop],sizeof(globalNodeAddrs[nexthop]));
					sprintf(logLine3, "sending packet dest %d nexthop %d message %s\n", destId, nexthop, recvBuf + 6); 
					fprintf((FILE*)logFile, logLine3);
					fflush((FILE*)logFile);
					//std::cout<<"sending"<<std::endl;
				

				}
				

			}
			//TODO send the requested message to the requested destination node
			// ...
		}

		else if(!strncmp(recvBuf, "mend", 4))
		{
			//std::cout<<"forwardBuf= "<<test<<std::endl;

			int destId= ntohs((recvBuf[5]<<8| recvBuf[4]));
			//std::cout<<"forward dest ID"<<destId<<std::endl;
			
			if(globalUpdate==1)
			{
				pthread_mutex_lock(&mtx);
				dijkstra();
				pthread_mutex_unlock(&mtx);
				//printAllpath;
			}
			//printAllpath();
			
			if(destId==globalMyID)
			{
				char logLine4[150];
				sprintf(logLine4, "receive packet message %s\n", recvBuf + 6); 
				fprintf((FILE*)logFile, logLine4);
				fflush((FILE*)logFile);
				//std::cout<<"receive"<<std::endl;

			}
			else
			{
				if(destMap.find(destId)==destMap.end())
				{
					char logLine5[150];
					sprintf(logLine5, "unreachable dest %d\n", destId); 
					fprintf((FILE*)logFile, logLine5);
					fflush((FILE*)logFile);
					//std::cout<<"unreachable"<<std::endl;

				}
				else
				{
					char logLine6[150];
					int nexthop=destMap[destId].first;
					recvBuf[0]='m';
					sendto(globalSocketUDP, recvBuf, bytesRecvd, 0,	(struct sockaddr*)&globalNodeAddrs[nexthop],sizeof(globalNodeAddrs[nexthop]));
					sprintf(logLine6, "forward packet dest %d nexthop %d message %s\n", destId, nexthop, recvBuf + 6); 
					fprintf((FILE*)logFile, logLine6);
					fflush((FILE*)logFile);
					//std::cout<<"forwarding"<<std::endl;
					
				}
			}
		}
		//'cost'<4 ASCII bytes>, destID<net order 2 byte signed> newCost<net order 4 byte signed>
		// else if(!strncmp((char*)recvBuf, "cost", 4))
		// {
		// 	//TODO record the cost change (remember, the link might currently be down! in that case,
		// 	//this is the new cost you should treat it as having once it comes back up.)
		// 	// ...
		// }
		
		//TODO now check for the various types of packets you use in your own protocol
		//else if(!strncmp(recvBuf, "your other message types", ))
		// ... 
		//printALLLSA(globalLSATicket);
	}
	//(should never reach here)
	close(globalSocketUDP);
}

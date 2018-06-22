#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <bitset>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using namespace std;


struct setElem
{
    unsigned addr;
    unsigned tag;
    bool dirty;
    bool valid;
};

class cacheSet
{
    public:
        // variables
        unsigned setID;
        unsigned WaysNum;
        vector<setElem> tags; // if accessed get to top


        //Functions
        cacheSet(unsigned size, unsigned id){
            setID = id;
            WaysNum = size;
            tags.erase(tags.begin(),tags.end());
            tags.resize(size);
        }

        vector<setElem>::iterator findElembyTag(int tag)
        {
            vector<setElem>::iterator it;
            for(it = tags.begin(); it != tags.end(); it++ )    {
                setElem curr = (*it);
                if(curr.tag == tag)
                    return it;
            }
            return it; //returns tags.end() - tag doesnot exist
        }

        bool RemoveElembyTag(int tag)// return true if removed
        {
            vector<setElem>::iterator it;
            for(it = tags.begin(); it != tags.end(); it++ )    {
                setElem curr = (*it);
                if(curr.tag == tag)
                {
                    tags.erase(it);
                    return true;
                }
            }
            return false; //returns tags.end() - tag doesnot exist
        }

        setElem insertNewElem(unsigned newAddr, unsigned newTag, bool* isRemoved)
        {
            setElem newElem = {newAddr, newTag, false, true}, removedElem = {newAddr, newTag, false , true};
            *isRemoved =false;
            bool valid = (*tags.begin()).valid; //if first is invalid -> cache is not full
            if (valid)
            {// cache is full delete LRU
                removedElem = (*tags.begin()); // Removing the LRU
                *isRemoved = true;
                tags.erase(tags.begin());
                tags.push_back(newElem);
            }
            else //invalid
            { // cache is not full -> insert new element
                tags.erase(tags.begin());
                tags.push_back(newElem);
            }
            return removedElem;
        }

        void updateRecentlyUsed(vector<setElem>::iterator elem, bool isWriteCMD) {
            setElem UpdatedElem = (*elem);
            tags.erase(elem);
            if (isWriteCMD)
            {
                UpdatedElem.dirty=isWriteCMD;
            }
            tags.push_back(UpdatedElem); //Last in vector is LRU
        }

        void print()
        {
            cout << "SetID: " << setID <<endl;
            vector<setElem>::iterator it;
            for(it = tags.begin(); it != tags.end(); it++ )    {
                setElem curr = (*it);
                cout << curr.tag << " D: " << curr.dirty << " V: " << curr.valid <<endl;
            }
        }


};

//Global Variables
vector<cacheSet*> L1;
vector<cacheSet*> L2;
unsigned L1SetSize, L2SetSize, L1TotalEntriesNum, L2TotalEntriesNum, L1WayEntriesNum, L2WayEntriesNum;
unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0, L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;
unsigned cmdCounter = 0, totalCycles = 0, L1missCounter=0, L2missCounter= 0, L1AccessCounter = 0, L2AccessCounter = 0; // for calculations

void intiateDataStructures( unsigned innerL1SetSize , unsigned innerL2SetSize , unsigned  innerL1WayEntriesNum, unsigned innerL2WayEntriesNum)
{
    for (unsigned i=0 ; i != innerL1SetSize; i++) //insert number of sets in each Cache
    {
        cacheSet* pSet;
        pSet = new cacheSet(innerL1WayEntriesNum, i);
        L1.push_back(pSet);
    }

    for (unsigned i=0 ; i != innerL2SetSize; i++) //insert number of sets in each Cache
    {
        cacheSet* pSet;
        pSet = new cacheSet(innerL2WayEntriesNum, i);
        L2.push_back(pSet);
    }

    //Now we have two caches build from vectors sorted by setNum (if 1-Way than setSize=1). Each set has wayNum of tags it can insert
}

// Prints
void printCaches()
{
    cout << "::L1 Cache::" <<endl;
    for (int i=0 ; i!= L1SetSize; i++) //insert number of sets in each Cache
    {
        cacheSet* pSet = L1[i];
        pSet->print();
    }
    cout << endl <<"::L2 Cache::" <<endl;

    for (int i=0 ; i!= L2SetSize; i++) //insert number of sets in each Cache
    {
        cacheSet* pSet = L2[i];
        pSet->print();
    }

    cout << endl <<"::END OF CACHE::" <<endl <<endl;
};





void CacheLogic(char operation, unsigned addr)
{
    //extract set from addr

    unsigned tagL1 = (addr >> BSize) >> L1Assoc;
    cout << "tagL1: " << tagL1 ;
    unsigned tagL2 = (addr >> BSize) >> L2Assoc; //remove offset than remove tag
    cout << " tagL2: " << tagL2 <<endl;
    unsigned setL1 = (addr >> BSize)-(tagL1<<L1Assoc);
    cout << " setL1: " << setL1 <<endl;
    unsigned setL2 = (addr >> BSize)-(tagL2<<L2Assoc);
    cout << " setL2: " << setL2 <<endl << endl;
    //extract tag from addr


    vector<setElem>::iterator iterL1 = L1[setL1]->findElembyTag(tagL1);
    vector<setElem>::iterator iterL2 = L2[setL2]->findElembyTag(tagL2);
    setElem oldElem;
    bool* isRemoved;
    isRemoved = new bool;

    L1AccessCounter++; //in each cmd we are Accessing L1
    if (operation =='w')
    {


    }
    else if (operation =='r')
    {
        if (iterL2 != L2[setL2]->tags.end()) // tag already exists in L2
        {
            if (iterL1 != L1[setL1]->tags.end()) // tag data exists in L1
            { // data exists on L1 and L2
                //update the LRU only of L1, cause L1 hit
                L1[setL1]->updateRecentlyUsed(iterL1, false);
                totalCycles += L1Cyc;

            }
            else
            { // data exists only on L2

                L2[setL2]->updateRecentlyUsed(iterL2, false); //update tag's L2 LRU as

                // insert the data to L1
                oldElem = L1[setL1]->insertNewElem(addr, tagL1, isRemoved);
                // if dirty than update L2
                if (oldElem.dirty && (*isRemoved)) // if L1 entry removed -> update dirty bit on L2
                {
                    unsigned oldL2tag = (oldElem.addr >> BSize)-(tagL2<<L2Assoc);
                    vector<setElem>::iterator it = L2[setL2]->findElembyTag(oldL2tag);
                    (*it).dirty=oldElem.dirty; //update dirty bit of L2 from L1
                    L2[setL2]->updateRecentlyUsed(it, false);
                }

                L2AccessCounter++;
                L1missCounter++;
                totalCycles += L1Cyc+L2Cyc;
            }
        } else {
            //Data is only in MEM
            // insert data addr to L1, L2

            // Insert to L2 - snoop data if needed:
            oldElem = L2[setL2]->insertNewElem(addr, tagL2, isRemoved);
            unsigned oldElemL1Tag = (oldElem.addr >> BSize) >> L1Assoc;
            if (*isRemoved) //check out if L2 was full ->  snoop the removed L2 elem from L1
            {
                L1[setL1]->RemoveElembyTag(oldElemL1Tag); //snoop the element removed from L2
            }

            //Double code
            oldElem = L1[setL1]->insertNewElem(addr, tagL1, isRemoved);
            // if dirty than update L2
            if (oldElem.dirty && (*isRemoved)) // if L1 entry removed -> update dirty bit on L2
            {
                unsigned oldL2tag = (oldElem.addr >> BSize)-(tagL2<<L2Assoc);
                vector<setElem>::iterator it = L2[setL2]->findElembyTag(oldL2tag);
                (*it).dirty=oldElem.dirty; //update dirty bit of L2 from L1
                L2[setL2]->updateRecentlyUsed(it, false);
            }

            // L1 and L2 miss rate
            L2AccessCounter++;
            L1missCounter++;
            L2missCounter++;

            totalCycles += L1Cyc+L2Cyc+MemCyc;
        }
    }

free(isRemoved);
}


int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}



	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = (unsigned) atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = (unsigned)atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = (unsigned) atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = (unsigned) atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = (unsigned) atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = (unsigned) atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = (unsigned) atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = (unsigned) atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = (unsigned) atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	// Variables that will be easier to use:
    L1SetSize = 1<<L1Assoc;
    cout << "L1SetSize: " << L1SetSize << endl; //DEBUG
    L2SetSize = 1<<L2Assoc;
    cout << "L2SetSize: " <<  L2SetSize << endl; //DEBUG
    L1TotalEntriesNum = 1<<(L1Size-BSize); //Asuuming Bsize <= CacheSize
    cout << "L1TotalEntriesNum: "<< L1TotalEntriesNum << endl; //DEBUG
    L2TotalEntriesNum = 1<<(L2Size-BSize);
    cout << "L2TotalEntriesNum: " << L2TotalEntriesNum << endl; //DEBUG
    L1WayEntriesNum = L1TotalEntriesNum>>(L1Assoc); //(L1SetSize-1) set part from Address size
    cout << "L1WayEntriesNum: "<< L1WayEntriesNum  << endl; //DEBUG
    L2WayEntriesNum = L2TotalEntriesNum>>(L2Assoc); //(L2SetSize-1) set part from Address size
    cout << "L2WayEntriesNum: " << L2WayEntriesNum << endl; //DEBUG

    intiateDataStructures( L1SetSize , L2SetSize , L1WayEntriesNum, L2WayEntriesNum);

    printCaches();

	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

        cmdCounter++;
        CacheLogic(operation, num);
        printCaches(); //DEBUG
	}

	double L1MissRate;
	double L2MissRate;
	double avgAccTime;


    L1MissRate = L1missCounter /(double)L1AccessCounter; // TODO - round values as HW demands
    L2MissRate = L2missCounter / (double)L2AccessCounter; // TODO - round values as HW demands
    avgAccTime = totalCycles / (double)cmdCounter;       // TODO - round values as HW demands

	//DEBUG PART
	cout<< "DEBUG ANDWER" <<endl;
	cout << "L1missCounter: " <<L1missCounter <<  " L1AccessCounter: " << L1AccessCounter <<endl;
    cout << "L2missCounter: " <<L2missCounter <<  " L2AccessCounter: " << L2AccessCounter <<endl;
    cout << "totalCycles: " << totalCycles <<  " cmdCounter: " << cmdCounter <<endl;


    cout<< "L1MissRate full: " << L1MissRate <<endl;
    cout<< "L2MissRate full: " << L2MissRate <<endl <<endl << "The only needed answer:" <<endl <<endl;
    //END OF DEBUG PART

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}


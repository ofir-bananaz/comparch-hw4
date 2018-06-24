#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <math.h>

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
        vector<setElem> tags; // if accessed get to top


        //Functions
        cacheSet(unsigned size, unsigned id){
            setID = id;
            tags.erase(tags.begin(),tags.end());
            tags.resize(size);
        }

        vector<setElem>::iterator findElembyTag(unsigned tag)
        {
            vector<setElem>::iterator it;
            for(it = tags.begin(); it != tags.end(); it++ )    {
                setElem curr = (*it);
                //cout << "findElembyTAG CMP: " << curr.tag << " == " << tag <<endl;
                if((curr.tag == tag) && curr.valid)   //todo OHAD: added valid check, because returning invalid elements causes bugs
                    return it;

            }
            //cout<< "not found...."<<endl;//DEBUG
            return it; //returns tags.end() - tag doesnt exist
        }

        bool RemoveElembyTag(unsigned tag)// return true if removed
        {
            vector<setElem>::iterator it;
            for(it = tags.begin(); it != tags.end(); it++ )    {
                setElem curr = (*it);
                if(curr.tag == tag  && curr.valid)
                {
                    (*it).valid = 0;
                    /*
                    setElem blankElem;    ///todo : added by OHAD, for sets not the get empty while using this... causes seg fault
                    blankElem.addr= 0;
                    blankElem.dirty = false;
                    blankElem.tag = 0;
                    blankElem.valid = false;
                    tags.push_back(blankElem);
                     */
                    return true;
                }
            }
            return false; //returns tags.end() - tag doesnot exist
        }

        setElem insertNewElem(unsigned newAddr, unsigned newTag, bool* isRemoved, bool isWrite)
        {
            setElem newElem = {newAddr, newTag, isWrite, true}, removedElem = {newAddr, newTag, false , true};
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
            cout << (*elem).addr <<endl;
            tags.erase(elem);
            UpdatedElem.valid=true;
            if (isWriteCMD)
                UpdatedElem.dirty=true;

            tags.push_back(UpdatedElem); //Last in vector is LRU
        }

        void print() //used for debugging
        {
            cout << "SetID: " << setID <<endl;
            vector<setElem>::iterator it;
            for(it = tags.begin(); it != tags.end(); it++ )    {
                setElem curr = (*it);
             //   if(it->valid) {
                	cout <<"fullAddr: " <<curr.addr <<" tag: " <<curr.tag << " D: " << curr.dirty << " V: " << curr.valid <<endl;
               // }
            }
        }


};

//Global Variables
vector<cacheSet*> L1;
vector<cacheSet*> L2;
unsigned L1SetSize, L2SetSize, L1TotalEntriesNum, L2TotalEntriesNum, L1WayEntriesNum, L2WayEntriesNum;
unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0, L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;
unsigned cmdCounter = 0, totalCycles = 0, L1missCounter=0, L2missCounter= 0, L1AccessCounter = 0, L2AccessCounter = 0; // for calculations
//bool waysL1equalsL1Size = false;
bool waysL2equalsL2Size = false;

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


void insertToL1_ElemThatExistsInL2(setElem oldElem, unsigned setL1, unsigned addr, unsigned tagL1, unsigned tagL2, unsigned setL2, bool* isRemoved, bool isWrite)
{


    oldElem = L1[setL1]->insertNewElem(addr, tagL1, isRemoved, isWrite);
    // if dirty than update L2
    if (oldElem.dirty && (*isRemoved)) // if L1 entry removed -> update dirty bit on L2
    {

        unsigned oldL2tag = (oldElem.addr >> BSize) >> L2Assoc;
        vector<setElem>::iterator it = L2[setL2]->findElembyTag(oldL2tag);
        (*it).dirty=oldElem.dirty; //update dirty bit of L2 from L1
        if (L2[setL2]->tags.end() == it){ //OHAD: if iterator is last element on set, using "updateRecentlyUsed" will cause a segmentation fault ( erasing last element on vector does that)
        	it->valid = true;
			if(isWrite){
				it->dirty = true;
			}
        }
        else{
        L2[setL2]->updateRecentlyUsed(it, isWrite); // https://moodle.technion.ac.il/mod/forum/discuss.php?d=422420
        }
    }
}

// most Important function in this HW
void CacheLogic(char operation, unsigned addr)
{
    //extract set and tag from recieved addr

    unsigned tagL1 = (addr >> BSize) >> L1Assoc;            cout << "tagL1: " << tagL1 ; //DEBUG
    unsigned setL1 = (addr >> BSize)-(tagL1<<L1Assoc);    cout << " setL1: " << setL1 ; //DEBUG
	/*
    if(waysL1equalsL1Size){
		tagL1 = (addr >> BSize);
		setL1 = 0;
	}
	*/

    unsigned tagL2 = (addr >> BSize) >> L2Assoc;           cout << " tagL2: " << tagL2 ; //DEBUG
    unsigned setL2 = (addr >> BSize)-(tagL2<<L2Assoc);    cout << " setL2: " << setL2 <<endl << endl; //DEBUG
	if(waysL2equalsL2Size){
		tagL2 = (addr >> BSize);
		setL2 = 0;
	}


    vector<setElem>::iterator iterL1 = L1[setL1]->findElembyTag(tagL1);
    vector<setElem>::iterator iterL2 = L2[setL2]->findElembyTag(tagL2);
    //if (iterL2 != L2[setL2]->tags.end()) cout << "ok!!!!!!!!!!!!!!!" <<endl;
    setElem oldElem;
    bool* isRemoved;
    isRemoved = new bool;
    *isRemoved = false; // todo Ohad: added initializer..

    L1AccessCounter++; // Always Accessing L1 Cache..

    if (operation =='w')
    {
        if (iterL2 != L2[setL2]->tags.end() && iterL2->valid) // tag already exists in L2
        {

            if (iterL1 != L1[setL1]->tags.end() && iterL1->valid) // tag data exists in L1
            { // data exists on L1 and L2
                L1[setL1]->updateRecentlyUsed(iterL1, true); //update the LRU only of L1, see https://moodle.technion.ac.il/mod/forum/discuss.php?d=422323
                totalCycles += L1Cyc;
            }
            else { // data exists only on L2
                L2[setL2]->updateRecentlyUsed(iterL2, true);
                if (WrAlloc)
                {//Write Alloc ->As in tutorial bring to L1
                    insertToL1_ElemThatExistsInL2(oldElem, setL1, addr, tagL1, tagL2, setL2, isRemoved, true); // insert the data to L1
                }

                L2AccessCounter++;  L1missCounter++;   totalCycles += L1Cyc+L2Cyc;
            }
        } else {//Data is only in MEM - insert data addr to L1, L2

            // Insert new Elem to L2 - snoop datafrom L1 if needed:
            if (WrAlloc) {
                oldElem = L2[setL2]->insertNewElem(addr, tagL2, isRemoved, true);
                unsigned oldElemL1Tag = (oldElem.addr >> BSize) >> L1Assoc;
                if (*isRemoved) //check out if L2 was full ->  snoop the removed L2 elem from L1
                    L1[setL1]->RemoveElembyTag(oldElemL1Tag); //snoop the element removed from L2
                insertToL1_ElemThatExistsInL2(oldElem, setL1, addr, tagL1, tagL2, setL2, isRemoved, true);
            }
            L2AccessCounter++;  L1missCounter++;  L2missCounter++;  totalCycles += L1Cyc+L2Cyc+MemCyc;
        }

    }
    else if (operation =='r')
    {
        if (iterL2 != L2[setL2]->tags.end() && iterL2->valid) // tag already exists in L2
        {
            if (iterL1 != L1[setL1]->tags.end() && iterL1->valid) // tag data exists in L1
            { // data exists on L1 and L2
                L1[setL1]->updateRecentlyUsed(iterL1, false); //update the LRU only of L1, cause L1 hit
                totalCycles += L1Cyc;
            }
            else //L1 MISS , L2 HIT
            {
                L2[setL2]->updateRecentlyUsed(iterL2, false); //update tag's L2 LRU as
                insertToL1_ElemThatExistsInL2(oldElem, setL1, addr, tagL1, tagL2, setL2, isRemoved, false); // insert the data to L1
                L2AccessCounter++;  L1missCounter++;   totalCycles += L1Cyc+L2Cyc;
            }
        } else {//Data is only in MEM - insert data addr to L1, L2
            // Insert new Elem to L2 because of read cmd - snoop datafrom L1 if needed:
            oldElem = L2[setL2]->insertNewElem(addr, tagL2, isRemoved, false);
            unsigned oldElemL1Tag = (oldElem.addr >> BSize) >> L1Assoc;

            if (*isRemoved){ //check out if L2 was full ->  snoop the removed L2 elem from L1
                L1[setL1]->RemoveElembyTag(oldElemL1Tag); //snoop the element removed from L2
            	//todo OHAD: this causes L1 to decrease in size... solving by changin valid bit to 0.
            	//cout << "oldElement tag is "<< oldElemL1Tag<< " and valid num is " << (bool)(L1[setL1]->findElembyTag(oldElemL1Tag))->valid << endl;
                //cout << "BEFORE:" <<endl;
                //printCaches();
            	//(L1[setL1]->findElembyTag(oldElemL1Tag))->valid = false;
            	//cout << "AFTER:" <<endl;
                //printCaches();
            //todo check behaviour!
            }

            //cout << "gotere3 set 1 is " << setL1 <<"addr is " << addr << "tagL1 is "<<  tagL1 << "removed variable is: "<< isRemoved <<  endl;
            insertToL1_ElemThatExistsInL2(oldElem, setL1, addr, tagL1, tagL2, setL2, isRemoved, false);
            L2AccessCounter++;  L1missCounter++;  L2missCounter++;  totalCycles += L1Cyc+L2Cyc+MemCyc;
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
    L1SetSize = 1<<L1Assoc; cout << "L1WayNum: " << L1SetSize << endl; //DEBUG
    L2SetSize = 1<<L2Assoc; cout << "L2WayNum: " <<  L2SetSize << endl; //DEBUG
    L1TotalEntriesNum = 1<<(L1Size-BSize); /*Asuuming Bsize <= CacheSize */ cout << "L1TotalEntriesNum: "<< L1TotalEntriesNum << endl; //DEBUG
    L2TotalEntriesNum = 1<<(L2Size-BSize); cout << "L2TotalEntriesNum: " << L2TotalEntriesNum << endl; //DEBUG
    L1WayEntriesNum = L1TotalEntriesNum>>(L1Assoc); /*(L1SetSize-1) set part from Address size */ cout << "L1WayEntriesNum: "<< L1WayEntriesNum  << endl; //DEBUG
    L2WayEntriesNum = L2TotalEntriesNum>>(L2Assoc); /*(L2SetSize-1) set part from Address size*/ cout << "L2WayEntriesNum: " << L2WayEntriesNum << endl; //DEBUG

    /*
    if(L1Assoc == L1Size-BSize){
        cout << " let my people sleep! L1"  <<endl;
        L1WayEntriesNum= L1TotalEntriesNum;
        L1SetSize = 1;
        waysL1equalsL1Size = true;
    }
     */

    if(L2Assoc == L2Size-BSize){
        cout << " let my people sleep! L2"  <<endl;
        L2WayEntriesNum= L2TotalEntriesNum;
        L2SetSize = 1;
        waysL2equalsL2Size = true;
    }



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
	cout<< "DEBUG ANSWER" <<endl;
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


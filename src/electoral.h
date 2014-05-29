/* 
 * File:   electoral.h
 * Author: C. Klopfenstein
 *
 * Created on May 16, 2014, 12:11 AM
 */

#ifndef ELECTORAL_H
#define	ELECTORAL_H

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <string>
#include <cstring>
#include <iostream>
#include <vector>
#include <array>
#include <exception>
#include <stdexcept>
#include <sqlite3.h>

using std::string;
using std::vector;
using std::array;
        
class electoral {
public:
    electoral();                    // default ctor
    electoral(string filename);     // ctor from dB filename
    electoral(const electoral& orig);   // copy ctor
    virtual ~electoral();
    
    int getInputData(string file);
    
    int putOutputData(string file);
       
    bool setThreshold(double thr) { threshold = thr; }
    bool filterStates();
    
    bool calcStateProb();           // calc stateProb and logP for 
                                    // all states "in play"
    bool calcResultProb();          // calculate P(result) for all combinations
                                    // of states 'in play'
    
    bool findPWin();                // compute prob(A has > 1/2 total el. votes)
    double pWin;                    // the answer
    
private:
    sqlite3* dB;                // SQLite dB
    uint64_t nStates = 0;
    uint64_t nInPlay = 0;       // needs to be 64-bit to shift > 32 positions
    uint64_t nPossibleOutcomes = 0;  // 2^(nInPlay)
    int totalElectoralVotes = 0;
    int votesToWin = 0;
    int baseA = 0;              // A's decided states electoral votes
    int baseB = 0;              // same for B
    int votes = 0;
    double spread = 0.0;
    double error = 0.0;
    double threshold = 0.0;

    vector<string> states;     // state ids
    vector<int> electoralVotes; // electoral votes for each state
    vector<double> margins;     // vote(A) - vote(B), in %
    vector<double> sigmas;      // uncertainty in margins in %
//note that there is a long double version of erf(), but log() is only double)
    vector <double> stateProb;  // P(A wins state)
                                // 1/2(1 + erf(margin/2 sigma))
    vector<double> logPA;        // log of stateProb
    vector<double> logPB;        // log of (1 - stateProb)
    
    vector<string> filteredStates;     // state ids
    vector<int> filterElectoralVotes; // electoral votes for each state
    vector<double> filterMargins;     // vote(A) - vote(B), in %
    vector<double> filterSigmas;      // uncertainty in margins in %
    vector <double> filterStateProb;    // P(A wins state)
                                        // = 1/2(1 + erf(margin/sigma))
    vector<double> filterLogPA;        // log of stateProb
    vector<double> filterLogPB;        // log of (1 - stateProb)

    vector<double> pVotes;      // P(A wins certain number of electoral votes)
    
    
    sqlite3* openInputDB(string file);
    int getDataFromInputDB(sqlite3* base);
    int closeInputDB(sqlite3* base);

    sqlite3* openOutputDB(string file);
    int writeDataToOutputDB(sqlite3* base);
    int closeOutputDB(sqlite3* base);
        
    int execSQLiteStatement(sqlite3* base, string cmd); // execute one SQLite 
                                                        // statement -
                                                        // prepare(), step(), finalize()

    // for output, need to bind parameters to sql statements, so make
    // functions for each output table
    int writeControlTable(sqlite3* base);
    int writeVote_DataTable(sqlite3* base);
    int writeAll_State_DataTable(sqlite3* base);
    int writeFiltered_State_DataTable(sqlite3* base);
    
};

#endif	/* ELECTORAL_H */


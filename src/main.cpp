/* 
 * File:   main.cpp
 * Author: C. Klopfenstein
 * 
 * Calculate probability of candidate A winning election, (and of
 * all possible electoral vote totals)
 * given an SQLite input DB file of state polling data.
 *
 * Created on May 13, 2014, 8:08 PM
 */

#include <cstdlib>
//#include <stdio.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <vector>
#include <sqlite3.h>

//#include "electoral.h"
#include "electoralSQLite.h"

using namespace std;
using namespace chris;

/*
 * 
 */
int main(int argc, char** argv) {
    
    int status = 0;
    sqlite3* dB;

// get filename from console, convert it to a c-string
    
    string filestring;
    cout << "Enter input dB filename: ";
    bool success = getline(cin, filestring);
    if (!success) cout << endl << "Did not get filename";
    cout << endl;

// create electoral object - ctor opens dB file, reads the data, closes the file
    
    electoralSQLite * elect = new electoralSQLite();
    status = elect->getInputData(filestring);
    
// debug
//    uint64_t temp = 0;
//    cout << "sizeof(uint64_t): " << sizeof(temp) << endl;
    
// get the threshold - N(sigma) to ignore.
    cout << "Enter threshold - N(sigma) for states to ignore: ";
    double thresh = 0.0;
    cin >> thresh;
    if (!success) cout << endl << "Did not get threshold";
    cout << endl;
    success = elect->setThreshold(thresh);

// get filename for output dB, but don't open dB until results available
    string outFilestring;
    cout << "Enter output dB filename: ";
    cin >> outFilestring;
// why this doesn't work?
//    success = getline(cin, outFilestring);
//    if (!success) cout << endl << "Did not get filename";
    cout << endl;
    
    char* outfile = new char[outFilestring.length() + 1];
    strcpy(outfile, outFilestring.c_str());
    
// Now do the calculation
    // btw -should be checking return codes here - but I do get tired of 
    // surrounding every trivial thing with a try-catch block...
    
    // put whole thing in try-catch block
    
    try {
    
// get probabilities for candidate A to win each state    
    success = elect->calcStateProb();
// exclude states with spread > threshold    
    success = elect->filterStates();
// calculate probabilities for each electoal vote total    
    success = elect->calcResultProb();
// write results to output SQLITE dB    
    status = elect->putOutputData(outFilestring);
    
// add to get one-line answer
    success = elect->findPWin();
    
    } catch (exception x) {
        cout << "Exception thrown during  Probability calculation" << endl;
        throw x;
    }
    
// write to console
    
    cout << "P(win) = " << elect->pWin << endl;;
    
    return status;
}


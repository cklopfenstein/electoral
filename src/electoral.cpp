/* 
 * File:   electoral.cpp
 * Author: Owner
 * 
 * Created on May 16, 2014, 12:11 AM
 */

#include "electoral.h"   

using std::cerr;
using std::cout;
using std::cin;
using std::endl;
using std::exception;
using std::runtime_error;

    electoral::electoral() : states(0), filteredStates(0),
            electoralVotes(0), margins(0), sigmas(0),
            filterElectoralVotes(0), filterMargins(0), filterSigmas(0),
            filterLogPA(0), filterLogPB(0),
            stateProb(0), filterStateProb(0),
            logPA(0), logPB(0), pVotes(0) {
    }

    electoral::electoral(const electoral& orig) {
    }

    electoral::electoral(const string filename) : states(0), filteredStates(0),
            electoralVotes(0), margins(0), sigmas(0),
            filterElectoralVotes(0), filterMargins(0), filterSigmas(0),
            filterLogPA(0), filterLogPB(0),
            stateProb(0), filterStateProb(0), 
            logPA(0), logPB(0), pVotes(0)  {
    }

    electoral::~electoral() {
    }
    
    int electoral::getInputData(string file) {
        int status = 0;
 
        try {
            dB = openInputDB(file);
        } catch (exception x) {
            cerr << "SQLException thrown in openInputDB" << endl;
            throw x;
        }

        try {
            int status = getDataFromInputDB(dB);
        } catch (exception x) {
            cerr << "SQLException thrown in getDataFromInputDB" << endl;
            throw x;
        }

        try {
            int status = closeInputDB(dB);
        } catch (exception x) {
            cerr << "SQLException thrown in closeInputDB" << endl;
            throw x;
        }
        return status;
    }
    
    int electoral::putOutputData(string file) {
        
        int status = 0;
        
        try {
            dB = openOutputDB(file);
        } catch (exception x) {
            cerr << "SQLException thrown in openOutputDB" << endl;
            throw x;
        }

        try {
            int status = writeDataToOutputDB(dB);
        } catch (exception x) {
            cerr << "SQLException thrown in writeDataToOutputDB" << endl;
            throw x;
        }

        try {
            int status = closeOutputDB(dB);
        } catch (exception x) {
            cerr << "SQLException thrown in closeOutputDB" << endl;
            throw x;
        }
        
        return status;
    }

    sqlite3 * electoral::openInputDB(string filestring) {
        
        char * filename = new char[filestring.length() + 1];
        strcpy(filename, filestring.c_str());
    
// sqlite3_open() by default opens file in RW mode, and creates new 
// file if it doesn't exist. Should really use sqlite3_open_v2() here to
// specify behavior.
        
        int open_status = sqlite3_open(filename, &dB);
    
        if (0 != open_status) {
            throw new runtime_error("openInputDB: failed to open file " + filestring);
        }
        return dB;
        
    }
    
    sqlite3* electoral::openOutputDB(string filestring) {
        
        char * filename = new char[filestring.length() + 1];
        strcpy(filename, filestring.c_str());
    
// sqlite3_open() by default opens file in RW mode, and creates new 
// file if it doesn't exist. Should really use sqlite3_open_v2() here to
// specify behavior.
        
        int open_status = sqlite3_open(filename, &dB);
    
        if (0 != open_status) {
            throw new runtime_error("openOutputDB: failed to open file " + filestring);
        }
        return dB;
                
    }
    
    int electoral::closeInputDB(sqlite3* base) {
        
        int exitCode = sqlite3_close(dB);       // close the dB
        if (0 != exitCode) {
            throw new runtime_error("closeInputDB: failed to close DB");
        }
        return exitCode;
    }
    
    int electoral::closeOutputDB(sqlite3* base) {
        
        int exitCode = sqlite3_close(dB);       // close the dB
        if (0 != exitCode) {
            throw new runtime_error("closeOutputDB: failed to close DB");
        }
        return exitCode;
    }
    
    int electoral::getDataFromInputDB(sqlite3* base) {
        
        sqlite3_stmt * statement;
        const char * tail;
 
// prepare an SQL statement for querying the dB
        int status = 
            sqlite3_prepare_v2(dB, "SELECT * from ev", -1, &statement, &tail);
        if (0 != status) 
            throw new runtime_error("getDataFromDB: sqlite_prepare failure");
    
// now step thru the query results, one row at a time 
    
        const unsigned char * state;
        const char * cState;
        string sState;
        int votes = 0;
        double spread = 0.0;
        double sigma = 0.0;
        //debug
        double prob = 0.0;
        //
    
        nStates = 0;
    
        status = sqlite3_step(statement);   // do this first
    
        while (SQLITE_ROW == status) {       // did it return a row?
                state = sqlite3_column_text(statement, 0);  // extract data
                cState = (const char*) state;   // cast to c-string
                sState = string(cState);        // and create a string object
                votes = sqlite3_column_int(statement, 1);
                spread = sqlite3_column_double(statement, 2);
                sigma = sqlite3_column_double(statement, 3);
                
                states.push_back(sState);       // store input data
                electoralVotes.push_back(votes);
                margins.push_back(spread);
                sigmas.push_back(sigma);
        
                nStates++;
                totalElectoralVotes += votes;
                
            status = sqlite3_step(statement);   // get next row        
        }         
        if (status != SQLITE_DONE) {            // should have hit EOF
            throw new runtime_error("getDataFromDB: sqlite3_step failure");
        }
   
        status = sqlite3_finalize(statement);   // need to finalize after each sqlite3-prepare
        if (status != 0) {
            throw new runtime_error("getDataFromDB: sqlite3_finalize failure");
        }
        
        votesToWin = totalElectoralVotes/2 + 1;
        return status;
    }
    
    int electoral::writeDataToOutputDB(sqlite3* base) {
        
        int status = 0;
        status = writeControlTable(base);
        status = writeAll_State_DataTable(base);
        status = writeFiltered_State_DataTable(base);
        status = writeVote_DataTable(base);
        
        return status;    
    }
    
    int electoral::execSQLiteStatement(sqlite3* base, string cmd) {    
// doesn't work with bind()... - only for literals
        
        sqlite3_stmt * statement;
        const char * tail;
        int status = 0;
        
        status = 
            sqlite3_prepare_v2(base, cmd.c_str(), -1, &statement, &tail);
        if (SQLITE_OK != status) 
            throw new runtime_error("writeDataToOutputDB: sqlite_prepare failure");
        
        status = sqlite3_step(statement); // execute the statement
        if (SQLITE_DONE != status) 
            throw new runtime_error("writeDataToOutputDB: sqlite_step failure");

        status = sqlite3_finalize(statement);   // need to finalize after each sqlite3-prepare
        if (status != SQLITE_OK) {
            throw new runtime_error("writeDataToOutputDB: sqlite3_finalize failure");
        }
        
        return status;
    }
    
    int electoral::writeControlTable(sqlite3* base) {
        
// Prepare an SQL statement for writing to the dB
// In this case a 1-row table of control info
        string cmd = 
          "Create Table Control "
                "(Nstates int, "
                "Threshold double, "
                "NStatesInPlay int, "
                "BaseA int,"
                "BaseB int)";
        int status = execSQLiteStatement(base, cmd);
        
// now write 1 row of data
        cmd = "Insert Into Control Values(?, ?, ?, ?, ?)";  // 5 parameters to bind: 
                                                            // int, double, int, int, int
        sqlite3_stmt * statement;
        const char * tail;
        status =                                        // prepare statement
            sqlite3_prepare_v2(dB, cmd.c_str(), -1, &statement, &tail);
        if (SQLITE_OK != status) 
            throw new runtime_error("writeControlTable: sqlite_prepare failure");
        
        int n1 = nStates;   // convert from uint64_t to int
        int n2 = nInPlay;

        status = sqlite3_bind_int(statement, 1, n1);    // bind parameters
        status = (sqlite3_bind_double(statement, 2, threshold) | status);
        status = (sqlite3_bind_int(statement, 3, n2) | status);
        status = (sqlite3_bind_int(statement, 4, baseA) | status);
        status = (sqlite3_bind_int(statement, 5, baseB) | status);
        if (SQLITE_OK != status) // status = 0 if all sqlite_bind ops succeeded
            throw new runtime_error("writeControlTable: sqlite_bind failure");
        
        status = sqlite3_step(statement);           // execute the statement
        if (SQLITE_DONE != status) 
            throw new runtime_error("writeControlTable: sqlite_step failure");

        status = sqlite3_finalize(statement);       // need to finalize after each sqlite3-prepare
        if (SQLITE_OK != status) 
            throw new runtime_error("writeControlTable: sqlite_finalize failure");

        return status;    
    }
    
    int electoral::writeAll_State_DataTable(sqlite3* base) {
        int status = 0;
        
        auto iterVotes = electoralVotes.begin();
        auto iterMargins = margins.begin();
        auto iterSigmas = sigmas.begin();
        auto iterPA = stateProb.begin();
        auto iterLogPA = logPA.begin();
        auto iterLogPB = logPB.begin();
        
        string cmd =
          "Create Table All_State_Data "
                "(States text, "
                "Votes int, "
                "Margins double, "
                "Sigmas double, "
                "ProbA double, "
                "LogProbA double, "
                "LogProbB double)";
        status = execSQLiteStatement(base, cmd);
        
// prepare a statement       
        cmd = "Insert Into All_State_Data Values(?, ?, ?, ?, ?, ?, ?)";    // 7 parameters to bind: 
                                                        
        sqlite3_stmt * statement;
        const char * tail;
        status =                                        // prepare statement
            sqlite3_prepare_v2(dB, cmd.c_str(), -1, &statement, &tail);
        if (SQLITE_OK != status) 
            throw new runtime_error("writeAll_State_DataTable: sqlite_prepare failure");
        
// bind values to parameters, execute the statement, reset, repeat...      
        for (auto iterStates = states.begin();
                iterStates != states.end();
                iterStates++) {
            status = sqlite3_bind_text(statement, 1,    // bind a (const char*)
                    (*iterStates).c_str(), -1, SQLITE_STATIC);
            status = (sqlite3_bind_int(statement, 2, *iterVotes) | status);
            status = (sqlite3_bind_double(statement, 3, *iterMargins) | status);
            status = (sqlite3_bind_double(statement, 4, *iterSigmas) | status);
            status = (sqlite3_bind_double(statement, 5, *iterPA) | status);
            status = (sqlite3_bind_double(statement, 6, *iterLogPA) | status);
            status = (sqlite3_bind_double(statement, 7, *iterLogPB) | status);
            if (SQLITE_OK != status)    // status = 0 if all sqlite_bind ops succeeded 
                throw new runtime_error("writeAll_State_DataTable: sqlite_bind failure");
        
            status = sqlite3_step(statement);           // execute the statement
            if (SQLITE_DONE != status) 
                throw new runtime_error("writeAll_State_DataTable: sqlite_step failure");
            
            status = sqlite3_reset(statement);           // reset the statement
            if (SQLITE_OK != status) 
                throw new runtime_error("writeAll_State_DataTable: sqlite_reset failure");
            
            iterVotes++;        // increment iterators
            iterMargins++;
            iterSigmas++;
            iterPA++;
            iterLogPA++;
            iterLogPB++;
        }
        
        status = sqlite3_finalize(statement);       // need to finalize after each sqlite3-prepare
        if (SQLITE_OK != status) 
            throw new runtime_error("writeAll_State_DataTable: sqlite_finalize failure");
        
        return status;
    }
    int electoral::writeFiltered_State_DataTable(sqlite3* base) {
        int status = 0;
        
        auto iterVotes = filterElectoralVotes.begin();
        auto iterMargins = filterMargins.begin();
        auto iterSigmas = filterSigmas.begin();
        auto iterPA = filterStateProb.begin();
        auto iterLogPA = filterLogPA.begin();
        auto iterLogPB = filterLogPB.begin();
        
        string cmd =
          "Create Table Filtered_State_Data "
                "(States text, "
                "Votes int, "
                "Margins double, "
                "Sigmas double, "
                "ProbA double, "
                "LogProbA double, "
                "LogProbB double)";
        status = execSQLiteStatement(base, cmd);
        
// prepare a statement       
        cmd = "Insert Into Filtered_State_Data Values(?, ?, ?, ?, ?, ?, ?)";    // 7 parameters to bind: 
                                                        // int, double, int
        sqlite3_stmt * statement;
        const char * tail;
        status =                                        // prepare statement
            sqlite3_prepare_v2(dB, cmd.c_str(), -1, &statement, &tail);
        if (SQLITE_OK != status) 
            throw new runtime_error("writeFiltered_State_DataTable: sqlite_prepare failure");
        
// bind values to parameters, execute the statement, reset, repeat...       
        for (auto iterStates = filteredStates.begin();
                iterStates != filteredStates.end();
                iterStates++) {
            status = sqlite3_bind_text(statement, 1,    // bind a (const char*) 
                    (*iterStates).c_str(), -1, SQLITE_STATIC);
            status = (sqlite3_bind_int(statement, 2, *iterVotes) | status);
            status = (sqlite3_bind_double(statement, 3, *iterMargins) | status);
            status = (sqlite3_bind_double(statement, 4, *iterSigmas) | status);
            status = (sqlite3_bind_double(statement, 5, *iterPA) | status);
            status = (sqlite3_bind_double(statement, 6, *iterLogPA) | status);
            status = (sqlite3_bind_double(statement, 7, *iterLogPB) | status);
            if (SQLITE_OK != status)    // status = 0 if all sqlite_bind ops succeeded  
                throw new runtime_error("writeFiltered_State_DataTable: sqlite_bind failure");
        
            status = sqlite3_step(statement);           // execute the statement
            if (SQLITE_DONE != status) 
                throw new runtime_error("writeFiltered_State_DataTable: sqlite_step failure");
            
            status = sqlite3_reset(statement);           // reset the statement
            if (SQLITE_OK != status) 
                throw new runtime_error("writeFiltered_State_DataTable: sqlite_reset failure");
            
            iterVotes++;        // increment iterators
            iterMargins++;
            iterSigmas++;
            iterPA++;
            iterLogPA++;
            iterLogPB++;
        }
        
        status = sqlite3_finalize(statement);       // need to finalize after each sqlite3-prepare
        if (SQLITE_OK != status) 
            throw new runtime_error("writeFiltered_State_DataTable: sqlite_finalize failure");
                
        return status;
    }
    
    int electoral::writeVote_DataTable(sqlite3* base) {
        int status = 0;
        
        string cmd =
          "Create Table Vote_Data "
                "(Votes int, "
                "ProbA double)";
        status = execSQLiteStatement(base, cmd);
        
// prepare a statement       
        cmd = "Insert Into Vote_Data Values(?, ?)";    // 2 parameters to bind: 
                                                        // int, double
        sqlite3_stmt * statement;
        const char * tail;
        status =                                        // prepare statement
            sqlite3_prepare_v2(dB, cmd.c_str(), -1, &statement, &tail);
        if (SQLITE_OK != status) 
            throw new runtime_error("writeVote_DataTable: sqlite_prepare failure");
        
        int nVotes = 0;
        
        for (auto iterVotes = pVotes.begin();
                iterVotes != pVotes.end();
                iterVotes++) {
            status = sqlite3_bind_int(statement, 1, nVotes);
            status = (sqlite3_bind_double(statement, 2, *iterVotes) | status);
            
            if (SQLITE_OK != status)    // status = 0 if all sqlite_bind ops succeeded  
                throw new runtime_error("writeVote_DataTable: sqlite_bind failure");
        
            status = sqlite3_step(statement);           // execute the statement
            if (SQLITE_DONE != status) 
                throw new runtime_error("writeVote_DataTable: sqlite_step failure");
            
            status = sqlite3_reset(statement);           // reset the statement
            if (SQLITE_OK != status) 
                throw new runtime_error("writeVote_DataTable: sqlite_reset failure");
            
            nVotes++;
        }
        
        status = sqlite3_finalize(statement);       // need to finalize after each sqlite3-prepare
        if (SQLITE_OK != status) 
            throw new runtime_error("writeVote_DataTable: sqlite_finalize failure");
        
        return status;
    }

    bool electoral::filterStates() {    // find states where the spread exceeds
                                        // a given threshold (e.g. 4 sigma).
                                        // Add the electoral votes for those 
                                        // states to the appropriate base vote.
                                        // Construct new lists of states 
                                        // 'in play'.
        bool status = true;
                
        double nSigma = 0.0;
        nInPlay = 0;
        
        auto iterStates = states.begin();
        auto iterMargins = margins.begin();
        auto iterSigmas = sigmas.begin();
        auto iterProb = stateProb.begin();
        auto iterLogPA = logPA.begin();
        auto iterLogPB = logPB.begin();
        for(auto iterVotes = electoralVotes.begin(); 
                iterVotes != electoralVotes.end(); 
                iterVotes++) {
            nSigma = ( *iterMargins )/( *iterSigmas );
            if (abs(nSigma) >= threshold) {
                if (*iterMargins > 0.0) {
                    baseA += *iterVotes;
                    } else {
                    baseB += *iterVotes;
                    } 
            } else {                    // add state to filtered lists
                filteredStates.push_back(*iterStates);
                filterElectoralVotes.push_back(*iterVotes);
                filterMargins.push_back(*iterMargins);
                filterSigmas.push_back(*iterSigmas);
                filterStateProb.push_back(*iterProb);
                filterLogPA.push_back(*iterLogPA);
                filterLogPB.push_back(*iterLogPB);
                nInPlay++;
            }
            iterStates++;
            iterMargins++;
            iterSigmas++;
            iterProb++;
            iterLogPA++;
            iterLogPB++;
        }
        
        
        return status;
    }
    
    bool electoral::calcStateProb() {   // calculate P(A wins) for each state,
                                        // along with log(PA) and log(PB).
        bool success = true;
        double p = 0.0;
        
        auto iterSigmas = sigmas.begin();
        for (auto iterMargins = margins.begin();
                iterMargins != margins.end();
                iterMargins++) {
            
// this appears to be correct
            p = 0.5 * (1.0 + erf((*iterMargins) / (2.0 * (*iterSigmas))));

            stateProb.push_back(p);
            logPA.push_back(log(p));
            logPB.push_back(log(1 - p));
            iterSigmas++;
        }
        
        return success;
    }

    bool electoral::calcResultProb() {  // calculate P(any electoral vote total)
                                        // This is the loop over the 2^N
                                        // possible results.
        // Maybe use threads here?
        bool success = true;
        
        int votes = 0;
        double logP = 0.0;
        uint64_t mask = 0;
        
        auto iterLogPA = filterLogPA.begin();
        auto iterLogPB = filterLogPB.begin();
        auto iterVotes = electoralVotes.begin();
        
        //init pVotes here
        pVotes.resize(totalElectoralVotes + 1);
        
        uint64_t one = 1;
        nPossibleOutcomes = one << nInPlay;   // equiv to 2^N
// print progress periodically
        uint64_t interval = 1000000;    // million
        for (uint64_t index = 0; index < nPossibleOutcomes; index++) {  // inner loop
            // to monitor progress
            if (0 == index % interval) cout << "index = " << index << endl;
            //
            votes = 0;
            logP = 0.0;
            mask = 1;
            iterLogPA = filterLogPA.begin();
            iterLogPB = filterLogPB.begin();
 
            for (iterVotes = filterElectoralVotes.begin();    // iterate over 
                    iterVotes != filterElectoralVotes.end();  // filtered states
                    iterVotes++) {
                
                if (0 != (index & mask)) {  // if corresponding bit set,
                    logP += *iterLogPA;     // mult by P(A) - add logP that is
                    votes += *iterVotes;    // and add corresponding votes
                } else {
                    logP += *iterLogPB;     // else mult P(B), no votes
                }
                
                iterLogPA++;
                iterLogPB++;
                mask = mask << 1;
            }
            
            pVotes[votes + baseA] += exp(logP); // add probability of this 
                                                // result to 
                                                // P(corresponding vote total)
        }
        
        return success;
    }
    
    bool electoral::findPWin() {            // add it up
        bool status = true;
        
        pWin = 0.0;
        for (int i = votesToWin; i < (totalElectoralVotes + 1); i++) {
            pWin += pVotes[i];
        }
        
        return status;
    }
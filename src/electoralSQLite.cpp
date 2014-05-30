/* 
 * File:   electoralSQLite.cpp
 * Author: C. Klopfenstein
 * 
 * Extend Electoral base class by adding I/O functions, in this
 * case using SQLite as input and output files.
 * 
 * Created on May 29, 2014, 8:15 PM
 */

#include "electoralSQLite.h"

using std::cerr;
using std::cout;
using std::cin;
using std::endl;
using std::exception;
using std::runtime_error;

using namespace chris;

electoralSQLite::electoralSQLite() {
}

electoralSQLite::electoralSQLite(const electoralSQLite& orig) {
}

electoralSQLite::~electoralSQLite() {
}

int electoralSQLite::getInputData(string file) {
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
    
    int electoralSQLite::putOutputData(string file) {
        
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

    sqlite3 * electoralSQLite::openInputDB(string filestring) {
        
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
    
    sqlite3* electoralSQLite::openOutputDB(string filestring) {
        
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
    
    int electoralSQLite::closeInputDB(sqlite3* base) {
        
        int exitCode = sqlite3_close(dB);       // close the dB
        if (0 != exitCode) {
            throw new runtime_error("closeInputDB: failed to close DB");
        }
        return exitCode;
    }
    
    int electoralSQLite::closeOutputDB(sqlite3* base) {
        
        int exitCode = sqlite3_close(dB);       // close the dB
        if (0 != exitCode) {
            throw new runtime_error("closeOutputDB: failed to close DB");
        }
        return exitCode;
    }
    
    int electoralSQLite::getDataFromInputDB(sqlite3* base) {
        
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
    
    int electoralSQLite::writeDataToOutputDB(sqlite3* base) {
        
        int status = 0;
        status = writeControlTable(base);
        status = writeAll_State_DataTable(base);
        status = writeFiltered_State_DataTable(base);
        status = writeVote_DataTable(base);
        
        return status;    
    }
    
    int electoralSQLite::execSQLiteStatement(sqlite3* base, string cmd) {    
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
    
    int electoralSQLite::writeControlTable(sqlite3* base) {
        
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
    
    int electoralSQLite::writeAll_State_DataTable(sqlite3* base) {
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
    int electoralSQLite::writeFiltered_State_DataTable(sqlite3* base) {
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
    
    int electoralSQLite::writeVote_DataTable(sqlite3* base) {
        int status = 0;
        double pB = 0.0;    //trivial to calculate from pA
        
        string cmd =
          "Create Table Vote_Data "
                "(Votes int, "
                "ProbA double,"
                "ProbB double)";
        status = execSQLiteStatement(base, cmd);
        
// prepare a statement       
        cmd = "Insert Into Vote_Data Values(?, ?, ?)";    // 2 parameters to bind: 
                                                        // int, double
        sqlite3_stmt * statement;
        const char * tail;
        status =                                        // prepare statement
            sqlite3_prepare_v2(dB, cmd.c_str(), -1, &statement, &tail);
        if (SQLITE_OK != status) 
            throw new runtime_error("writeVote_DataTable: sqlite_prepare failure");
        
        int nVotes = 0;
                                        //PB(votes) =PA(totalElectoralVotes - votes)
        auto pBIter = pVotes.rbegin();  // use a reverse iterator
        
        for (auto iterVotes = pVotes.begin();
                iterVotes != pVotes.end();
                iterVotes++) {
            status = sqlite3_bind_int(statement, 1, nVotes);
            status = (sqlite3_bind_double(statement, 2, *iterVotes) | status);
            status = (sqlite3_bind_double(statement, 3, *pBIter) | status);
 
            
            if (SQLITE_OK != status)    // status = 0 if all sqlite_bind ops succeeded  
                throw new runtime_error("writeVote_DataTable: sqlite_bind failure");
        
            status = sqlite3_step(statement);           // execute the statement
            if (SQLITE_DONE != status) 
                throw new runtime_error("writeVote_DataTable: sqlite_step failure");
            
            status = sqlite3_reset(statement);           // reset the statement
            if (SQLITE_OK != status) 
                throw new runtime_error("writeVote_DataTable: sqlite_reset failure");
            
            nVotes++;
            pBIter++;
        }
        
        status = sqlite3_finalize(statement);       // need to finalize after each sqlite3-prepare
        if (SQLITE_OK != status) 
            throw new runtime_error("writeVote_DataTable: sqlite_finalize failure");
        
        return status;
    }
    
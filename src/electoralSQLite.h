/* 
 * File:   electoralSQLite.h
 * Author: C. Klopfenstein
 * 
 * Extend Electoral base class by adding I/O functions, in this
 * case using SQLite as input and output files.
 *
 * Created on May 29, 2014, 8:14 PM
 */

#ifndef ELECTORALSQLITE_H
#define	ELECTORALSQLITE_H

#include "electoral.h"
#include <sqlite3.h>

namespace chris {
    class electoralSQLite : public electoral {
    public:
        electoralSQLite();
        electoralSQLite(const electoralSQLite& orig);
        virtual ~electoralSQLite();
        
        int getInputData(string file);
        int putOutputData(string file);
        
    private:
        sqlite3* dB;                // SQLite dB

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

    };  // class electoralSQLite
}       // namespace chris
#endif	/* ELECTORALSQLITE_H */


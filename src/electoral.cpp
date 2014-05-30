/* 
 * File:   electoral.cpp
 * Author: Owner
 * 
 * Find probabilities for all posssible electoral vote totals
 * for 2 candidates in a US presidential election, given individual
 * state poll results. 
 * 
 * For each state, we have a spread and margin of error (sigma), in percent.
 * (positive spread mans candidate A leads).
 * Use erf() to calculate probabiity for A wining each individual state. Then 
 * consider each possible outcome - compute the probabaility for each 
 * possibility - and combine (add) probabilites for each possible electoral
 * vote total.
 * 
 * Since the number of possible outcomes is very large (2^nState), apply a 
 * filter - if the spread in a given state exceeds some threshold in number of 
 * sigma (specified at runtime), assign p(A) = 1 (or 0). Remove those states
 * from the calculation - and add their electoral votes to a base vote for each
 * candidate.
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

using namespace chris;

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
                                                                        // over possible outcomes
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
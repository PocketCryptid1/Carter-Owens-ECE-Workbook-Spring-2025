#ifndef MYPREDICTOR_H
#define MYPREDICTOR_H

#include <vector>
#include <deque>
#include <map>
#include <list>
#include <string>
#include <strings.h>
#include <iostream>
using namespace std;

// ============================================================================
// Prediction Mode Toggle
// ============================================================================
// Set PRED_MODE to select which predictor to use:
//   0 = Last Value Prediction
//   1 = Stride Prediction
// Recompile after changing this value.
// ============================================================================
#define PRED_LAST_VALUE 0
#define PRED_STRIDE     1

#define PRED_MODE PRED_STRIDE

// ============================================================================
// Prediction Table Parameters
// ============================================================================
#define TABLE_SIZE 256   // Number of entries in the direct-mapped table

// ============================================================================
// Table Entry
// ============================================================================
// Each entry in the direct-mapped prediction table stores:
//   tag       - Full PC used for tag matching to detect aliasing
//   lastValue - The most recently retired value for this PC
//   stride    - The difference between the two most recent values (stride mode)
//   valid     - Whether this entry has been initialized with at least one value
// ============================================================================
struct TableEntry {
    uint64_t tag;        // Full PC for tag comparison
    uint64_t lastValue;  // Most recent value produced by this PC
    int64_t  stride;     // Stride = (current value - previous value)
    bool     valid;      // True once we have stored at least one value
};

// ============================================================================
// In-Flight Instruction Info
// ============================================================================
// Since updatePredictor() only receives seq_no (not PC or instruction type),
// we must remember the PC and eligibility for each in-flight instruction.
// This info is recorded during speculativeUpdate() and consumed at retire.
// ============================================================================
struct InFlightInfo {
    uint64_t pc;            // PC of the instruction
    bool     isPredictable; // True if this instruction type should be predicted
    bool     specUpdated;   // True if the table was speculatively updated (correct prediction)
};

#endif // MYPREDICTOR_H


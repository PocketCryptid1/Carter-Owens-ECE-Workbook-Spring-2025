#include <cinttypes>
#include "cvp.h"
#include "mypredictor.h"
#include <cassert>
#include <cstdio>
#include <cstring>


// ============================================================================
// Global State
// ============================================================================

// Global branch and path history (kept for potential future use)
static uint64_t ghr = 0, phist = 0;

// Load/Store address history
static uint64_t addrHist = 0;

// Direct-mapped prediction table indexed by PC
static TableEntry predTable[TABLE_SIZE];

// Map from seq_no -> in-flight info (PC and predictability)
// Needed because updatePredictor() only provides seq_no, not PC.
static map<uint64_t, InFlightInfo> inflightMap;

// Temporary state passed from getPrediction() to speculativeUpdate().
// These are called consecutively for the same instruction, so a simple
// global variable is safe for handing off the predicted value.
static uint64_t g_lastPredValue  = 0;
static bool     g_lastDidPredict = false;


// ============================================================================
// Helper: compute table index from PC
// ============================================================================
static inline uint32_t getIndex(uint64_t pc) {
    // Use folded XOR hash for better distribution across the table.
    // This reduces aliasing between PCs that only differ in high bits.
    return (uint32_t)((pc ^ (pc >> 8)) % TABLE_SIZE);
}


// ============================================================================
// getPrediction  (called at Fetch stage)
// ============================================================================
// We look up the direct-mapped table using the PC.  If the entry is valid
// and the tag matches, we issue a prediction:
//   - Last Value mode:  predict the last retired value
//   - Stride mode:      predict last retired value + stride
// We only predict for PCs that previously executed a predictable instruction
// (ALU, LOAD, or SLOWALU), which is naturally enforced because only those
// instruction types have their values stored in the table.
// ============================================================================
bool getPrediction(uint64_t seq_no, uint64_t pc, uint8_t piece, uint64_t& predicted_value) {

    uint32_t idx = getIndex(pc);

    // Only predict if the table entry is valid and the tag matches this PC
    if (predTable[idx].valid && predTable[idx].tag == pc) {

#if PRED_MODE == PRED_LAST_VALUE
        // ---- Last Value Prediction ----
        // Predict that the instruction will produce the same value as last time.
        predicted_value = predTable[idx].lastValue;
#elif PRED_MODE == PRED_STRIDE
        // ---- Stride Prediction ----
        // Predict last value + stride.  The stride captures repeating
        // increments such as loop index updates from the same PC.
        predicted_value = predTable[idx].lastValue + predTable[idx].stride;
#endif
        // Save predicted value so speculativeUpdate() can use it for
        // speculative table updates when the prediction is confirmed correct.
        g_lastPredValue  = predicted_value;
        g_lastDidPredict = true;
        return true;
    }

    // No valid entry for this PC – do not predict
    g_lastDidPredict = false;
    return false;
}


// ============================================================================
// speculativeUpdate  (called at Execute stage, immediately after getPrediction)
// ============================================================================
// Here we learn the instruction class and eligibility.  We record the PC
// and predictability for later use in updatePredictor().
// Branch-related histories are also maintained here.
// ============================================================================
void speculativeUpdate(uint64_t seq_no,
                       bool eligible,
                       uint8_t prediction_result,
                       uint64_t pc,
                       uint64_t next_pc,
                       InstClass insn,
                       uint8_t piece,
                       uint64_t src1,
                       uint64_t src2,
                       uint64_t src3,
                       uint64_t dst) {

    // Determine whether this instruction type is one we want to predict.
    // We focus on ALU, LOAD, and SLOWALU instructions.
    bool isPredictable = eligible &&
                         (insn == aluInstClass ||
                          insn == loadInstClass ||
                          insn == slowAluInstClass);

    // ---- Speculative table update on confirmed-correct predictions ----
    // When the prediction was correct (prediction_result == 1), update the
    // table immediately so that subsequent fetches of the same PC (e.g., the
    // next loop iteration) see the latest value.  This is essential for
    // stride prediction in tight loops where many instances of the same PC
    // are in flight simultaneously.
    bool specUpdated = false;
    if (isPredictable && g_lastDidPredict && prediction_result == 1) {
        uint32_t idx = getIndex(pc);
        if (predTable[idx].valid && predTable[idx].tag == pc) {
            // Update lastValue to the (correct) predicted value.
            // Stride remains unchanged – the pattern continues.
            predTable[idx].lastValue = g_lastPredValue;
            specUpdated = true;
        }
    }
    g_lastDidPredict = false;

    // Store mapping so updatePredictor() can look up the PC later
    inflightMap[seq_no] = {pc, isPredictable, specUpdated};

    // ---- Branch history maintenance (architectural at this point) ----
    bool isCondBr  = (insn == condBranchInstClass);
    bool isIndBr   = (insn == uncondIndirectBranchInstClass);

    if (isCondBr)
        ghr = (ghr << 1) | (pc + 4 != next_pc);

    if (isIndBr)
        phist = (phist << 4) | (next_pc & 0x3);
}


// ============================================================================
// updatePredictor  (called at Retire stage)
// ============================================================================
// The actual value is now known.  We update the prediction table:
//   - Last Value mode:  store the actual value
//   - Stride mode:      compute stride = actual_value - lastValue,
//                        then store both the new value and new stride
// ============================================================================
void updatePredictor(uint64_t seq_no,
                     uint64_t actual_addr,
                     uint64_t actual_value,
                     uint64_t actual_latency) {

    // Update address history register (kept for potential future use)
    if (actual_addr != 0xdeadbeef)
        addrHist = (addrHist << 4) | actual_addr;

    // Look up the in-flight info we recorded during speculativeUpdate
    auto it = inflightMap.find(seq_no);
    if (it == inflightMap.end())
        return;  // should not happen

    uint64_t pc            = it->second.pc;
    bool     isPredictable = it->second.isPredictable;
    bool     specUpdated   = it->second.specUpdated;
    inflightMap.erase(it);

    // Only update the table for predictable instruction types with a real value
    if (!isPredictable || actual_value == 0xdeadbeef)
        return;

    // If the table was already speculatively updated (prediction was correct),
    // skip the retire-time update to avoid overwriting the stride with 0.
    if (specUpdated)
        return;

    uint32_t idx = getIndex(pc);

    if (predTable[idx].valid && predTable[idx].tag == pc) {
        // Entry already exists for this PC – update stride and value
        predTable[idx].stride    = (int64_t)(actual_value - predTable[idx].lastValue);
        predTable[idx].lastValue = actual_value;
    } else {
        // First time seeing this PC (or previous entry was evicted)
        // Initialize entry; stride starts at 0 (no history yet)
        predTable[idx].tag       = pc;
        predTable[idx].lastValue = actual_value;
        predTable[idx].stride    = 0;
        predTable[idx].valid     = true;
    }
}


// ============================================================================
// beginPredictor  (called once before simulation starts)
// ============================================================================
void beginPredictor(int argc_other, char **argv_other) {
    if (argc_other > 0)
        printf("CONTESTANT ARGUMENTS:\n");

    for (int i = 0; i < argc_other; i++)
        printf("\targv_other[%d] = %s\n", i, argv_other[i]);

    // Zero-initialize the prediction table
    memset(predTable, 0, sizeof(predTable));

#if PRED_MODE == PRED_LAST_VALUE
    printf("Prediction Mode: LAST VALUE\n");
#elif PRED_MODE == PRED_STRIDE
    printf("Prediction Mode: STRIDE\n");
#endif
}


// ============================================================================
// endPredictor  (called once after simulation ends)
// ============================================================================
void endPredictor() {
    printf("CONTESTANT OUTPUT--------------------------\n");
#if PRED_MODE == PRED_LAST_VALUE
    printf("Mode: Last Value Prediction\n");
#elif PRED_MODE == PRED_STRIDE
    printf("Mode: Stride Prediction\n");
#endif
    printf("--------------------------\n");
}

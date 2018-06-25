#include "L4HLTTask_simu.h"
#include <rtsLog.h>
#include <stdlib.h>
#include <bitset>

#ifdef USE_COI // cover all

// -----------------------------------------------------------------------------
#define CHECK_RESULT(_COIFUNC) \
{ \
    COIRESULT result = _COIFUNC; \
    if (result != COI_SUCCESS) \
    { \
        printf("%s returned %s\n", #_COIFUNC, COIResultGetName(result));\
        return -1; \
    } \
}

#define CHECK_RESULT_VOID(_COIFUNC) \
{ \
    COIRESULT result = _COIFUNC; \
    if (result != COI_SUCCESS) \
    { \
        printf("%s returned %s\n", #_COIFUNC, COIResultGetName(result));\
    } \
}

// -----------------------------------------------------------------------------
L4HLTTask_simu::~L4HLTTask_simu()
{
    // Destroy the pipeline
    CHECK_RESULT_VOID(COIPipelineDestroy(pipeline));
    LOG(INFO, "Destroyed pipeline");

    int8_t   sink_return;
    uint32_t exit_reason;

    // Destroy the process
    COIRESULT result = COIProcessDestroy(
        proc,           // Process handle to be destroyed
        -1,             // Wait indefinitely until main() (on sink side) returns
        false,          // Don't force to exit. Let it finish executing
                        // functions enqueued and exit gracefully
        &sink_return,   // Don't care about the exit result.
        &exit_reason
    );

    if (result != COI_SUCCESS) {
        LOG(INFO, "COIProcessDestroy result %s", COIResultGetName(result));
    }

    LOG(INFO, "Sink process returned %d", sink_return);
    LOG(INFO, "Sink exit reason ");
    switch (exit_reason) {
    case 0:
        LOG(INFO, "SHUTDOWN OK");
        break;
    default:
        LOG(INFO, "Exit reason %d - %s", exit_reason, strsignal(exit_reason));
    }
}

// -----------------------------------------------------------------------------
void L4HLTTask_simu::init()
{
    LOG(INFO, "Running L4HLTTask_simu::init");
    
    // Make sure there is an Intel(r) Xeon Phi(tm) device available
    //
    CHECK_RESULT_VOID(COIEngineGetCount(COI_ISA_MIC, &num_engines));

    LOG("THLT", "%u engines available", num_engines);

    // If there isn't at least one engine, there is something wrong
    if (num_engines < 1) {
        LOG("ERR", "ERROR: Need at least 1 engine");
        return;
    }

    int engine_id = 0;
    if (num_engines == 2) {
        engine_id = getIdx() % 2;
    }

    // Get a handle to the Intel(r) Xeon Phi(tm) engine
    CHECK_RESULT_VOID(COIEngineGetHandle(COI_ISA_MIC, engine_id, &engine));
    LOG("THLT", "Got engine handle");

    // The following call creates a process on the sink.
    // Intel(r) Coprocessor Offload Infrastructure (Intel(r) COI)
    // will automatically load any dependent libraries and run the "main"
    // function in the binary.
    COIRESULT result = COIProcessCreateFromFile(
        engine,         // The engine to create the process on.
        SINK_NAME,      // The local path to the sink side binary to launch.
        0, NULL,        // argc and argv for the sink process.
        false, NULL,    // Environment variables to set for the sink process.
        true, NULL,     // Enable the proxy but don't specify a proxy root path.
        0,              // The amount of memory to pre-allocate
                        // and register for use with COIBUFFERs.
        NULL,           // Path to search for dependencies
        &proc           // The resulting process handle.
    );

    if (result != COI_SUCCESS)
    {
        LOG("THLT", "COIProcessCreateFromFile result %s",
            COIResultGetName(result));
        return;
    }
    LOG("THLT", "Sink process created");

    int core_id = getIdx() / num_engines % 60;
    COI_CPU_MASK cpumask = {0ul};
    std::bitset<244>* maskp = new (cpumask) std::bitset<244>;
    maskp->set(core_id*4+1);
    maskp->set(core_id*4+2);
    maskp->set(core_id*4+3);
    maskp->set(core_id*4+4);

    CHECK_RESULT_VOID(
    COIPipelineCreate(proc,      // Process to associate the pipeline with
                      cpumask,   // Do not set any sink thread affinity for the pipeline
                      0,         // Use the default stack size for the pipeline thread
                      &pipeline  // Handle to the new pipeline
                      ));
    LOG("THLT", "COI Created pipeline");

    const char* func_name = "Foo";
    CHECK_RESULT_VOID(
    COIProcessGetFunctionHandles(proc,       // Process to query for the function
                                 1,          // The number of functions to look up
                                 &func_name, // The name of the function to look up
                                 func        // A handle to the function
                                 ));
    LOG("THLT", "Got handle to sink function %s", func_name);
}

// -----------------------------------------------------------------------------
int L4HLTTask_simu::sendConfig(SimpleXmlDoc* cfg)
{
    return 0;
}

// -----------------------------------------------------------------------------
int L4HLTTask_simu::startRun()
{

    return 0;
}

// -----------------------------------------------------------------------------
int L4HLTTask_simu::stopRun()
{
    return 0;
}

// -----------------------------------------------------------------------------
void L4HLTTask_simu::process(int task_id, L4EventData* evt)
{
    // we assume we could have a few different tasks that can run simultaneous
    LOG(NOTE, "Processing %d", getIdx());
    if (task_id) return;      // only one task for now
    char* taskOutputBuff = evt->simu_result[task_id];

    uint16_t strlength = 1<<15;
    void* misc_data = malloc(strlength);
    memset(misc_data, 1, strlength);

    // Enough to hold the return value
    char* return_value = (char*) malloc(strlength);

    // Enqueue the function for execution
    // Pass in misc_data and return value pointer to run function
    // Get an event to wait on until the run function completion
    CHECK_RESULT_VOID(
    COIPipelineRunFunction(
                           pipeline, func[0],         // Pipeline handle and function handle
                           0, NULL, NULL,             // Buffers and access flags
                           0, NULL,                   // Input dependencies
                           misc_data,   strlength,    // Misc Data to pass to the function
                           return_value, strlength,   // Return values that will be passed back
                           &completion_event          // Event to signal when it completes
                           ));
    // LOG(INFO, "Called sink function %s(\"%s\" [%d bytes])",
    //        func_name, misc_data, strlength);

    // Now wait indefinitely for the function to complete
    CHECK_RESULT_VOID(
    COIEventWait(
                 1,                         // Number of events to wait for
                 &completion_event,         // Event handles
                 -1,                        // Wait indefinitely
                 true,                      // Wait for all events
                 NULL, NULL                 // Number of events signaled
                                            // and their indices
                 ));

    free(misc_data);
    free(return_value);
}

#endif // USE_COI

#include "execution_data.h"

using namespace std;

ExecutionData::ExecutionData(const string& name, int exec, int numTries) {
    sampleName = name;
    executions = exec;
    tries = numTries;

    contradictions = vector<int>(executions, 0);
    durations = vector<chrono::duration<double, milli>>(executions);
}

void ExecutionData::SetContradictions(int c, int execID) {
    contradictions[execID] = c;
}

void ExecutionData::SetDuration(chrono::duration<double, milli> d, int execID) {
    durations[execID] = d;
}

void ExecutionData::PrintResults() {
    cout << "Results for -" << sampleName << "- sample (" << tries << " tries per execution, " << executions << " executions):" << endl << endl;

    int fails = 0;
    double contradictionsMean = 0;
    auto now = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration = now - now;

    for (int exec = 0; exec < executions; exec++)
    {
        contradictions[exec] == tries ? ++fails : contradictionsMean += contradictions[exec];
        duration += durations[exec];
    }

    cout << "* " << fails << " executions didn't find a solution in " << tries << " tries." << endl;

    if (executions - fails != 0)
    {
        cout << "* The other " << executions - fails << " executions found a solution and a total of " << contradictionsMean << " contradictions." << endl;

        contradictionsMean /= double(executions - fails);
        cout << "* This makes for a mean of " << contradictionsMean << " contradictions per successful execution." << endl << endl;
    }
    else cout << endl;

    double durationMean = duration.count();
    cout << "* In total, all the execution's duration was " << durationMean << " miliseconds." << endl;

    durationMean /= double(executions);
    cout << "* This makes for a mean of " << durationMean << " miliseconds per execution." << endl << endl;
}

#ifndef EXECUTION_DATA_H
#define EXECUTION_DATA_H

#include <vector>
#include <chrono>
#include <iostream>

using namespace std;

class ExecutionData {

private:
	string sampleName;
	int executions;
	int tries;

	vector<int> contradictions;
	vector<chrono::duration<double, milli>> durations;

public:
	ExecutionData(const string& name, int exec, int numTries);

	void SetContradictions(int c, int execID);
	void SetDuration(chrono::duration<double, milli> d, int execID);

	void PrintResults();
};

#endif
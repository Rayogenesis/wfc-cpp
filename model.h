#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <functional>
#include <cmath>
#include <random>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <stdexcept>

using namespace std;

class Model {

private:
	int NextUnobservedNode(mt19937& random);
	void Observe(int node, mt19937& random);
	bool Propagate();
	void Ban(int i, int t);
	void Clear();

public:
	enum Heuristic { Entropy, MRV, Scanline };
	Heuristic heuristic;

	Model(int width, int height, int N, bool periodic, Heuristic heuristic);

	void Init();
	bool Run(int seed, int limit);
	virtual void Save(const string& filename) = 0;

protected:
	vector<vector<bool>> wave;
	vector<vector<vector<int>>> propagator;
	vector<vector<vector<int>>> compatible;
	vector<int> observed;

	vector<pair<int, int>> stack;
	int stacksize, observedSoFar;

	int MX, MY, T, N;
	bool periodic, ground;

	vector<double> weights;
	vector<double> weightLogWeights, distribution;

	vector<int> sumsOfOnes;
	double sumOfWeights, sumOfWeightLogWeights, startingEntropy;
	vector<double> sumsOfWeights, sumsOfWeightLogWeights, entropies;

	static const vector<int> dx;
	static const vector<int> dy;
	static const vector<int> opposite;
};

#endif
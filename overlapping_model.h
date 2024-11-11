#ifndef OVERLAPPING_MODEL_H
#define OVERLAPPING_MODEL_H

#include <unordered_map>

#include "model.h"
#include "bitmap_helper.h"

using namespace std;

class OverlappingModel : public Model {
	
private:
	vector<vector<uint8_t>> patterns;
	vector<int> colors;

	static vector<uint8_t> pattern(function<uint8_t(int, int)> f, int N);
	static vector<uint8_t> rotate(const vector<uint8_t>& p, int N);
	static vector<uint8_t> reflect(const vector<uint8_t>& p, int N);
	static int64_t hash(const vector<uint8_t>& p, int C);

	static bool agrees(const vector<uint8_t>& p1, const vector<uint8_t>& p2, int dx, int dy, int N);

public:
	OverlappingModel(const string& name, int N, int width, int height, bool periodicInput, bool periodic, int symmetry, bool ground, Heuristic heuristic);
	void Save(const string& filename) override;
};

#endif
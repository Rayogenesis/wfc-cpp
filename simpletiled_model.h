#ifndef SIMPLETILED_MODEL_H
#define SIMPLETILED_MODEL_H

#include <string>
#include <map>
#include <unordered_map>
#include <utility>

#include <fstream>
#include <sstream>
#include <iostream>

#include "model.h"
#include "bitmap_helper.h"
#include "helper.h"
#include "tinyxml2.h"

using namespace std;

class SimpleTiledModel : public Model {

private:
	vector<vector<int>> tiles;
	vector<string> tilenames;
	int tilesize;
	bool blackBackground;

	map<string, int> firstOccurrence;
	vector<vector<int>> action;

	static vector<int> tile(function<int(int, int)> f, int size);
	static vector<int> rotate(const vector<int>& array, int size);
	static vector<int> reflect(const vector<int>& array, int size);
	
public:
	SimpleTiledModel(const string& name, const string& subsetName, int width, int height, bool periodic, bool blackBackground, Heuristic heuristic);

	void Save(const string& filename) override;
	string TextOutput() const;
};

#endif
#ifndef BITMAP_HELPER_H
#define BITMAP_HELPER_H

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <tuple>

using namespace std;

tuple<vector<int>, int, int> LoadBitmap(const string& filename);

void SaveBitmap(const vector<int>& data, int width, int height, const string& filename);

#endif
#include "simpletiled_model.h"

using namespace std;

// Constructor del modelo de mosaico simple
SimpleTiledModel::SimpleTiledModel(const string& name, const string& subsetName, int width, int height, bool periodic, bool blackBackground, Heuristic heuristic)
    : Model(width, height, 1, periodic, heuristic), blackBackground(blackBackground) {

    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(("tilesets/" + name + ".xml").c_str()) != tinyxml2::XML_SUCCESS) {
        cerr << "Error al cargar el archivo XML." << endl;
        return;
    }

    tinyxml2::XMLElement* root = doc.RootElement();
    bool unique = root->BoolAttribute("unique", false);

    vector<string> subset;
    if (!subsetName.empty()) {
        tinyxml2::XMLElement* subsets = root->FirstChildElement("subsets");
        tinyxml2::XMLElement* subsetElem = subsets->FirstChildElement("subset");
        while (subsetElem != nullptr) {
            if (subsetElem->Attribute("name") == subsetName) {
                tinyxml2::XMLElement* tileElem = subsetElem->FirstChildElement("tile");
                while (tileElem != nullptr) {
                    subset.push_back(tileElem->Attribute("name"));
                    tileElem = tileElem->NextSiblingElement("tile");
                }
                break;
            }
            subsetElem = subsetElem->NextSiblingElement("subset");
        }
    }

    // Inicializamos listas y acciones
    vector<double> weightList;
    vector<vector<int>> action;
    unordered_map<string, int> firstOccurrence;

    // Iterar sobre los elementos <tile> en el XML
    tinyxml2::XMLElement* tilesElement = root->FirstChildElement("tiles");
    tinyxml2::XMLElement* xtile = tilesElement->FirstChildElement("tile");

    while (xtile != nullptr) {
        string tilename = xtile->Attribute("name");
        if (!subset.empty() && find(subset.begin(), subset.end(), tilename) == subset.end()) {
            xtile = xtile->NextSiblingElement("tile");
            continue;
        }

        char sym = xtile->Attribute("symmetry")[0];
        int cardinality;
        function<int(int)> a, b;

        // Determinar simetría
        if (sym == 'L') {
            cardinality = 4;
            a = [](int i) { return (i + 1) % 4; };
            b = [](int i) { return i % 2 == 0 ? i + 1 : i - 1; };
        }
        else if (sym == 'T') {
            cardinality = 4;
            a = [](int i) { return (i + 1) % 4; };
            b = [](int i) { return i % 2 == 0 ? i : 4 - i; };
        }
        else if (sym == 'I') {
            cardinality = 2;
            a = [](int i) { return 1 - i; };
            b = [](int i) { return i; };
        }
        else if (sym == '\\') {
            cardinality = 2;
            a = [](int i) { return 1 - i; };
            b = [](int i) { return 1 - i; };
        }
        else if (sym == 'F') {
            cardinality = 8;
            a = [](int i) { return i < 4 ? (i + 1) % 4 : 4 + (i - 1) % 4; };
            b = [](int i) { return i < 4 ? i + 4 : i - 4; };
        }
        else {
            cardinality = 1;
            a = [](int i) { return i; };
            b = [](int i) { return i; };
        }

        patternsTotal = action.size();
        firstOccurrence[tilename] = patternsTotal;

        vector<vector<int>> map(cardinality, vector<int>(8));
        for (int t = 0; t < cardinality; ++t) {
            map[t][0] = t;
            map[t][1] = a(t);
            map[t][2] = a(a(t));
            map[t][3] = a(a(a(t)));
            map[t][4] = b(t);
            map[t][5] = b(a(t));
            map[t][6] = b(a(a(t)));
            map[t][7] = b(a(a(a(t))));

            for (int s = 0; s < 8; ++s) {
                map[t][s] += patternsTotal;
            }
            action.push_back(map[t]);
        }
        
        // Cargar los bitmaps
        if (unique) {
            for (int t = 0; t < cardinality; ++t) {
                tuple<vector<int>, int, int> bitmap_tupple = LoadBitmap("tilesets/" + name + "/" + tilename + " " + to_string(t) + ".png");
                vector<int> bitmap = get<0>(bitmap_tupple);
                tilesize = get<1>(bitmap_tupple);

                tiles.push_back(bitmap);
                tilenames.push_back(tilename + " " + to_string(t));
            }
        }
        else {
            tuple<vector<int>, int, int> bitmap_tupple = LoadBitmap("tilesets/" + name + "/" + tilename + ".png");
            vector<int> bitmap = get<0>(bitmap_tupple);
            tilesize = get<1>(bitmap_tupple);

            tiles.push_back(bitmap);
            tilenames.push_back(tilename + " 0");

            for (int t = 1; t < cardinality; ++t) {
                if (t <= 3) {
                    tiles.push_back(rotate(tiles[patternsTotal + t - 1], tilesize));
                }
                if (t >= 4) {
                    tiles.push_back(reflect(tiles[patternsTotal + t - 4], tilesize));
                }
                tilenames.push_back(tilename + " " + to_string(t));
            }
        }

        // Peso de las fichas
        for (int t = 0; t < cardinality; ++t)
            weightList.push_back(xtile->DoubleAttribute("weight", 1.0));
        
        xtile = xtile->NextSiblingElement("tile");
    }

    // Inicializar propagadores
    patternsTotal = action.size();
    weights = weightList;

    propagator = vector<vector<vector<int>>>(4, vector<vector<int>>(patternsTotal));
    vector<vector<vector<bool>>> densePropagator(4, vector<vector<bool>>(patternsTotal, vector<bool>(patternsTotal)));

    tinyxml2::XMLElement* xneighbor = root->FirstChildElement("neighbors")->FirstChildElement("neighbor");
    while (xneighbor) {
        string left_str = xneighbor->Attribute("left");
        string right_str = xneighbor->Attribute("right");

        vector<string> left = SplitString(left_str, ' ');
        vector<string> right = SplitString(right_str, ' ');

        if (!subset.empty() && (find(subset.begin(), subset.end(), left[0]) == subset.end() ||
            find(subset.begin(), subset.end(), right[0]) == subset.end())) {
            xneighbor = xneighbor->NextSiblingElement("neighbor");
            continue;
        }

        int L = action[firstOccurrence[left[0]]][left.size() == 1 ? 0 : stoi(left[1])];
        int D = action[L][1];
        int R = action[firstOccurrence[right[0]]][right.size() == 1 ? 0 : stoi(right[1])];
        int U = action[R][1];

        densePropagator[0][R][L] = true;
        densePropagator[0][action[R][6]][action[L][6]] = true;
        densePropagator[0][action[L][4]][action[R][4]] = true;
        densePropagator[0][action[L][2]][action[R][2]] = true;

        densePropagator[1][U][D] = true;
        densePropagator[1][action[D][6]][action[U][6]] = true;
        densePropagator[1][action[U][4]][action[D][4]] = true;
        densePropagator[1][action[D][2]][action[U][2]] = true;

        xneighbor = xneighbor->NextSiblingElement("neighbor");
    }

    // Inicializar el resto de propagadores
    for (int t2 = 0; t2 < patternsTotal; ++t2) {
        for (int t1 = 0; t1 < patternsTotal; ++t1) {
            densePropagator[2][t2][t1] = densePropagator[0][t1][t2];
            densePropagator[3][t2][t1] = densePropagator[1][t1][t2];
        }
    }

    vector<vector<vector<int>>> sparsePropagator(4, vector<vector<int>>(patternsTotal));
    for (int d = 0; d < 4; ++d) {
        for (int t1 = 0; t1 < patternsTotal; ++t1) {
            vector<int>& sp = sparsePropagator[d][t1];
            const vector<bool>& tp = densePropagator[d][t1];
            for (int t2 = 0; t2 < patternsTotal; ++t2) {
                if (tp[t2]) {
                    sp.push_back(t2);
                }
            }

            int ST = static_cast<int>(sp.size());
            if (ST == 0) {
                cerr << "ERROR: tile " << tilenames[t1] << " has no neighbors in direction " << d << endl;
            }

            propagator[d][t1] = sp;
        }
    }
}

// Guardar el resultado como imagen
void SimpleTiledModel::Save(const string& filename) {
    vector<int> bitmapData(outputWidth * outputHeight * tilesize * tilesize);
    if (observed[0] >= 0) {
        for (int x = 0; x < outputWidth; ++x) {
            for (int y = 0; y < outputHeight; ++y) {
                const vector<int>& tile = tiles[observed[x + y * outputWidth]];
                for (int dy = 0; dy < tilesize; ++dy) {
                    for (int dx = 0; dx < tilesize; ++dx) {
                        bitmapData[x * tilesize + dx + (y * tilesize + dy) * outputWidth * tilesize] = tile[dx + dy * tilesize];
                    }
                }
            }
        }
    }
    else {
        for (int i = 0; i < wave.size(); ++i) {
            int x = i % outputWidth, y = i / outputWidth;
            if (blackBackground && sumsOfOnes[i] == patternsTotal) {
                for (int yt = 0; yt < tilesize; ++yt) {
                    for (int xt = 0; xt < tilesize; ++xt) {
                        bitmapData[x * tilesize + xt + (y * tilesize + yt) * outputWidth * tilesize] = 255 << 24;
                    }
                }
            }
            else {
                const vector<bool>& w = wave[i];
                double normalization = 1.0 / sumsOfWeights[i];
                for (int yt = 0; yt < tilesize; ++yt) {
                    for (int xt = 0; xt < tilesize; ++xt) {
                        int idi = x * tilesize + xt + (y * tilesize + yt) * outputWidth * tilesize;
                        double r = 0, g = 0, b = 0;
                        for (int t = 0; t < patternsTotal; ++t) {
                            if (w[t]) {
                                int argb = tiles[t][xt + yt * tilesize];
                                r += ((argb & 0xff0000) >> 16) * weights[t] * normalization;
                                g += ((argb & 0xff00) >> 8) * weights[t] * normalization;
                                b += (argb & 0xff) * weights[t] * normalization;
                            }
                        }
                        bitmapData[idi] = 0xff000000 | (static_cast<int>(r) << 16) | (static_cast<int>(g) << 8) | static_cast<int>(b);
                    }
                }
            }
        }
    }

    SaveBitmap(bitmapData, outputWidth * tilesize, outputHeight * tilesize, filename);
}

// Recibe una función desde rotate o reflect con la que procesará un tile dado para devolverlo rotado o reflejado
vector<int> SimpleTiledModel::tile(function<int(int, int)> f, int size) {
    vector<int> result(size * size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            result[x + y * size] = f(x, y);
        }
    }
    return result;
}

// Rota
vector<int> SimpleTiledModel::rotate(const vector<int>& array, int size) {
    return tile([&array, size](int x, int y) { return array[size - 1 - y + x * size]; }, size);
}

// Refleja
vector<int> SimpleTiledModel::reflect(const vector<int>& array, int size) {
    return tile([&array, size](int x, int y) { return array[size - 1 - x + y * size]; }, size);
}

// Generar salida en texto
string SimpleTiledModel::TextOutput() const {
    stringstream result;
    for (int y = 0; y < outputHeight; ++y) {
        for (int x = 0; x < outputWidth; ++x) {
            result << tilenames[observed[x + y * outputWidth]] << ", ";
        }
        result << "\n";
    }
    return result.str();
}
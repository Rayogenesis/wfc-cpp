#include "model.h"

using namespace std;

const vector<int> Model::directionX = { -1, 0, 1, 0 };
const vector<int> Model::directionY = { 0, 1, 0, -1 };
const vector<int> Model::opposite = { 2, 3, 0, 1 };

Model::Model(int width, int height, int N, bool periodic, Heuristic heuristic)
    : outputWidth(width), outputHeight(height), patternSize(N), periodic(periodic), heuristic(heuristic),
    stacksize(0), observedSoFar(0) {}

// Inicializamos varias estructuras de datos que almacenan el estado actual del colapso de la función de onda
// Calculamos los pesos y las entropías iniciales de cada celda
void Model::Init() {
    wave.resize(outputWidth * outputHeight, vector<bool>(patternsTotal, true));
    compatible.resize(outputWidth * outputHeight, vector<vector<int>>(patternsTotal, vector<int>(4, 0)));
    distribution.resize(patternsTotal);
    observed.resize(outputWidth * outputHeight, -1);

    weightLogWeights.resize(patternsTotal);
    sumOfWeights = 0;
    sumOfWeightLogWeights = 0;

    for (int t = 0; t < patternsTotal; ++t) {
        weightLogWeights[t] = weights[t] * log(weights[t]);
        sumOfWeights += weights[t];
        sumOfWeightLogWeights += weightLogWeights[t];
    }

    startingEntropy = log(sumOfWeights) - sumOfWeightLogWeights / sumOfWeights;

    sumsOfOnes.resize(outputWidth * outputHeight);
    sumsOfWeights.resize(outputWidth * outputHeight);
    sumsOfWeightLogWeights.resize(outputWidth * outputHeight);
    entropies.resize(outputWidth * outputHeight);

    stack.resize(wave.size() * patternsTotal);
    stacksize = 0;
}

bool Model::Run(int seed, int limit) {
    // Inicializamos el modelo si no lo está ya
    if (wave.empty()) Init();

    // Reiniciamos el tablero (no tiene backtracking)
    Clear();
    mt19937 random(seed);

    // Definimos un número limitado de pasos y procedemos al funcionamiento estándar
    for (int l = 0; l < limit || limit < 0; ++l) {
        int node = NextUnobservedNode(random);
        if (node >= 0) {
            Observe(node, random);
            bool success = Propagate();
            if (!success) return false;
        }
        else {
            for (int i = 0; i < wave.size(); ++i)
                for (int t = 0; t < patternsTotal; ++t)
                    if (wave[i][t]) {
                        observed[i] = t;
                        break;
                    }
            return true;
        }
    }

    return true;
}

int Model::NextUnobservedNode(mt19937& random) {
    if (heuristic == Heuristic::Scanline) {
        for (int i = observedSoFar; i < wave.size(); ++i) {
            if (!periodic && (i % outputWidth + patternSize > outputWidth || i / outputWidth + patternSize > outputHeight)) continue;
            if (sumsOfOnes[i] > 1) {
                observedSoFar = i + 1;
                return i;
            }
        }
        return -1;
    }

    double min = 1E+4;
    int argmin = -1;
    for (int i = 0; i < wave.size(); ++i) {
        if (!periodic && (i % outputWidth + patternSize > outputWidth || i / outputWidth + patternSize > outputHeight)) continue;
        int remainingValues = sumsOfOnes[i];
        double entropy = heuristic == Heuristic::Entropy ? entropies[i] : remainingValues;
        if (remainingValues > 1 && entropy <= min) {
            double noise = 1E-6 * random();
            if (entropy + noise < min) {
                min = entropy + noise;
                argmin = i;
            }
        }
    }
    return argmin;
}

void Model::Observe(int node, mt19937& random) {
    vector<bool>& w = wave[node];
    for (int t = 0; t < patternsTotal; ++t) distribution[t] = w[t] ? weights[t] : 0.0;
    discrete_distribution<int> dist(distribution.begin(), distribution.end());
    int r = dist(random);
    for (int t = 0; t < patternsTotal; ++t)
        if (w[t] != (t == r)) Ban(node, t);
}

bool Model::Propagate() {
    while (stacksize > 0) {
        auto [i1, t1] = stack[--stacksize];
        int x1 = i1 % outputWidth;
        int y1 = i1 / outputWidth;

        for (int d = 0; d < 4; ++d) {
            int x2 = x1 + directionX[d];
            int y2 = y1 + directionY[d];
            if (!periodic && (x2 < 0 || y2 < 0 || x2 + patternSize > outputWidth || y2 + patternSize > outputHeight)) continue;

            if (x2 < 0) x2 += outputWidth;
            else if (x2 >= outputWidth) x2 -= outputWidth;
            if (y2 < 0) y2 += outputHeight;
            else if (y2 >= outputHeight) y2 -= outputHeight;

            int i2 = x2 + y2 * outputWidth;
            const vector<int>& p = propagator[d][t1];
            vector<vector<int>>& compat = compatible[i2];

            for (int l = 0; l < p.size(); ++l) {
                int t2 = p[l];
                vector<int>& comp = compat[t2];
                comp[d]--;
                if (comp[d] == 0) Ban(i2, t2);
            }
        }
    }

    return sumsOfOnes[0] > 0;
}

// Marca un patrón como imposible en una determinada celda
void Model::Ban(int i, int t) {
    wave[i][t] = false;

    vector<int>& comp = compatible[i][t];
    for (int d = 0; d < 4; ++d) comp[d] = 0;
    stack[stacksize++] = { i, t };

    sumsOfOnes[i] -= 1;
    sumsOfWeights[i] -= weights[t];
    sumsOfWeightLogWeights[i] -= weightLogWeights[t];

    double sum = sumsOfWeights[i];
    entropies[i] = log(sum) - sumsOfWeightLogWeights[i] / sum;
}

// Marca todas las celdas como posibles para todos los patrones y restablece los contadores
void Model::Clear() {
    for (int i = 0; i < wave.size(); ++i) {
        fill(wave[i].begin(), wave[i].end(), true);
        for (int t = 0; t < patternsTotal; ++t) {
            for (int d = 0; d < 4; ++d)
                compatible[i][t][d] = propagator[opposite[d]][t].size();
        }

        sumsOfOnes[i] = weights.size();
        sumsOfWeights[i] = sumOfWeights;
        sumsOfWeightLogWeights[i] = sumOfWeightLogWeights;
        entropies[i] = startingEntropy;
        observed[i] = -1;
    }
    observedSoFar = 0;

    if (ground) {
        for (int x = 0; x < outputWidth; ++x) {
            for (int t = 0; t < patternsTotal - 1; ++t) Ban(x + (outputHeight - 1) * outputWidth, t);
            for (int y = 0; y < outputHeight - 1; ++y) Ban(x + y * outputWidth, patternsTotal - 1);
        }
        Propagate();
    }
}
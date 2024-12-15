#include "overlapping_model.h"

using namespace std;

OverlappingModel::OverlappingModel(const string& name, int N, int width, int height, bool periodicInput, bool periodic, int symmetry, bool ground, Heuristic heuristic)
    : Model(width, height, N, periodic, heuristic)
{
    // Cargamos el bitmap
    tuple<vector<int>, int, int> bitmap_tupple = LoadBitmap("samples/" + name + ".png");
    vector<int> bitmap = get<0>(bitmap_tupple);
    int SX = get<1>(bitmap_tupple);
    int SY = get<2>(bitmap_tupple);

    vector<uint8_t> sample(bitmap.size());
    colors.clear();

    // Convertimos todos los colores encontrados a índices
    for (size_t i = 0; i < sample.size(); i++) {
        int color = bitmap[i];

        auto it = find(colors.begin(), colors.end(), color);
        if (it == colors.end()) {
            colors.push_back(color);
            sample[i] = colors.size() - 1;
        }
        else {
            sample[i] = distance(colors.begin(), it);
        }
    }

    // Inicializamos las listas con los patrones, sus índices y sus pesos
    patterns.clear();
    unordered_map<int64_t, int> patternIndices;
    vector<double> weightList;

    // Asignamos a C los colores únicos encontrados y definimos las posiciones de las que se extraerán los patrones
    int C = colors.size();
    int xmax = periodicInput ? SX : SX - patternSize + 1;
    int ymax = periodicInput ? SY : SY - patternSize + 1;

    for (int y = 0; y < ymax; y++) {
        for (int x = 0; x < xmax; x++) {
            // Creamos un vector con 8 patrones, 4 son para el original y sus rotaciones, otros 4 para cada reflejo
            vector<vector<uint8_t>> ps(8);

            ps[0] = pattern([&](int dx, int dy) { return sample[(x + dx) % SX + (y + dy) % SY * SX]; }, patternSize);
            ps[1] = reflect(ps[0], patternSize);
            ps[2] = rotate(ps[0], patternSize);
            ps[3] = reflect(ps[2], patternSize);
            ps[4] = rotate(ps[2], patternSize);
            ps[5] = reflect(ps[4], patternSize);
            ps[6] = rotate(ps[4], patternSize);
            ps[7] = reflect(ps[6], patternSize);

            // Recorremos los niveles de simetría definidos
            for (int k = 0; k < symmetry; k++) {
                // Escogemos un patrón y creamos un hash
                vector<uint8_t> p = ps[k];
                int64_t h = hash(p, C);

                // Buscamos su índice para aumentar su peso o añadirlo
                auto it = patternIndices.find(h);
                if (it != patternIndices.end()) {
                    weightList[it->second] += 1.0;
                }
                else {
                    patternIndices[h] = weightList.size();
                    weightList.push_back(1.0);
                    patterns.push_back(p);
                }
            }
        }
    }

    // Asignamos la lista de pesos y a T la cantidad de patrones únicos encontrados
    weights = weightList;
    patternsTotal = weights.size();
    this->ground = ground;

    // Inicializamos el propagador con 4 direcciones
    propagator.resize(4);
    for (int d = 0; d < 4; d++) {
        // Cada dirección tiene un vector de tamaño T (patrones únicos)
        propagator[d].resize(patternsTotal);
        for (int t = 0; t < patternsTotal; t++) {
            // Cada patrón contiene la lista de patrones compatibles gracias a la función agrees
            vector<int> list;
            for (int t2 = 0; t2 < patternsTotal; t2++) {
                if (agrees(patterns[t], patterns[t2], directionX[d], directionY[d], patternSize)) {
                    list.push_back(t2);
                }
            }
            propagator[d][t] = list;
        }
    }
}

// Recibe una función desde rotate o reflect con la que procesará un patrón dado para devolverlo rotado o reflejado
vector<uint8_t> OverlappingModel::pattern(function<uint8_t(int, int)> f, int N) {
    vector<uint8_t> result(N * N);
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            result[x + y * N] = f(x, y);
        }
    }
    return result;
}

// Rota
vector<uint8_t> OverlappingModel::rotate(const vector<uint8_t>& p, int N) {
    return pattern([&](int x, int y) { return p[N - 1 - y + x * N]; }, N);
}

// Refleja
vector<uint8_t> OverlappingModel::reflect(const vector<uint8_t>& p, int N) {
    return pattern([&](int x, int y) { return p[N - 1 - x + y * N]; }, N);
}

// Crea un hash para el patrón dado en base a la cantidad de colores únicos existentes
int64_t OverlappingModel::hash(const vector<uint8_t>& p, int C) {
    int64_t result = 0;
    int64_t power = 1;
    for (size_t i = 0; i < p.size(); i++) {
        result += p[p.size() - 1 - i] * power;
        power *= C;
    }
    return result;
}

// Comprueba si dos patrones son compatibles uno al lado del otro superponiéndose
bool OverlappingModel::agrees(const vector<uint8_t>& p1, const vector<uint8_t>& p2, int dx, int dy, int N) {
    int xmin = dx < 0 ? 0 : dx;
    int xmax = dx < 0 ? dx + N : N;
    int ymin = dy < 0 ? 0 : dy;
    int ymax = dy < 0 ? dy + N : N;
    for (int y = ymin; y < ymax; y++) {
        for (int x = xmin; x < xmax; x++) {
            if (p1[x + N * y] != p2[x - dx + N * (y - dy)]) {
                return false;
            }
        }
    }
    return true;
}

void OverlappingModel::Save(const string& filename) {
    vector<int> bitmap(outputWidth * outputHeight, 0);

    if (observed[0] >= 0) {
        for (int y = 0; y < outputHeight; y++) {
            int dy = (y < outputHeight - patternSize + 1) ? 0 : patternSize - 1;
            for (int x = 0; x < outputWidth; x++) {
                int dx = (x < outputWidth - patternSize + 1) ? 0 : patternSize - 1;
                bitmap[x + y * outputWidth] = colors[patterns[observed[x - dx + (y - dy) * outputWidth]][dx + dy * patternSize]];
            }
        }
    }
    else {
        for (size_t i = 0; i < wave.size(); i++) {
            int contributors = 0, r = 0, g = 0, b = 0;
            int x = i % outputWidth;
            int y = i / outputWidth;
            for (int dy = 0; dy < patternSize; dy++) {
                for (int dx = 0; dx < patternSize; dx++) {
                    int sx = (x - dx + outputWidth) % outputWidth;
                    int sy = (y - dy + outputHeight) % outputHeight;
                    int s = sx + sy * outputWidth;
                    if (!periodic && (sx + patternSize > outputWidth || sy + patternSize > outputHeight || sx < 0 || sy < 0)) continue;
                    for (int t = 0; t < patternsTotal; t++) {
                        if (wave[s][t]) {
                            contributors++;
                            int argb = colors[patterns[t][dx + dy * patternSize]];
                            r += (argb & 0xff0000) >> 16;
                            g += (argb & 0xff00) >> 8;
                            b += argb & 0xff;
                        }
                    }
                }
            }
            bitmap[i] = (0xff000000 | ((r / contributors) << 16) | ((g / contributors) << 8) | (b / contributors));
        }
    }

    SaveBitmap(bitmap, outputWidth, outputHeight, filename);
}
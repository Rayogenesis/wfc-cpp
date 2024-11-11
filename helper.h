#include "tinyxml2.h"  // Para trabajar con XML

#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <iostream>

using namespace std;

namespace txml = tinyxml2;

// Funci�n para obtener un valor basado en pesos (equivalente a Random en C#)
static int Random(const vector<double>& weights, double r) {
    double sum = 0;
    for (double weight : weights) sum += weight;
    double threshold = r * sum;

    double partialSum = 0;
    for (size_t i = 0; i < weights.size(); i++) {
        partialSum += weights[i];
        if (partialSum >= threshold) return static_cast<int>(i);
    }
    return 0;
}

// Funci�n para elevar un n�mero a la potencia n (equivalente a ToPower en C#)
static long ToPower(int a, int n) {
    long product = 1;
    for (int i = 0; i < n; i++) product *= a;
    return product;
}

// Plantilla gen�rica para obtener el valor de un atributo XML (equivalente a Get en C#)
template <typename T>
static T Get(txml::XMLElement* elem, const string& attribute, T defaultT) {
    const char* attr = elem->Attribute(attribute.c_str());
    if (!attr) return defaultT;

    istringstream iss(attr);
    T value;
    iss >> value;
    return value;
}

// Especializaci�n de la plantilla Get para cadenas (string)
template <>
static string Get<string>(txml::XMLElement* elem, const string& attribute, string defaultT) {
    const char* attr = elem->Attribute(attribute.c_str());
    return attr ? string(attr) : defaultT;
}

// Funci�n para seleccionar m�ltiples elementos de nombres espec�ficos
static vector<txml::XMLElement*> Elements(txml::XMLElement* parent, const vector<string>& names) {
    vector<txml::XMLElement*> elements;
    for (txml::XMLElement* elem = parent->FirstChildElement(); elem != nullptr; elem = elem->NextSiblingElement()) {
        string elemName = elem->Name();
        if (find(names.begin(), names.end(), elemName) != names.end()) {
            elements.push_back(elem);
        }
    }
    return elements;
}

static vector<string> SplitString(const string& input, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(input);

    // Usamos getline para separar el string en base al delimitador
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}
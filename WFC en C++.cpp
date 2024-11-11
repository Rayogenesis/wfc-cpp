// WFC en C++.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//

#include "tinyxml2.h"  // Para trabajar con XML
#include "model.h"
#include "overlapping_model.h"
#include "simpletiled_model.h"
#include "execution_data.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <random>

using namespace std;

namespace txml = tinyxml2;

int main()
{
    // Inicialización del cronómetro
    auto start = chrono::high_resolution_clock::now();

    // Crear carpeta de salida y limpiar archivos existentes
    filesystem::create_directory("output");
    for (const auto& entry : filesystem::directory_iterator("output"))
    {
        filesystem::remove(entry.path());
    }

    // Inicialización de la semilla para random
    random_device rd;
    mt19937 random(rd());

    // Cargar y leer el archivo XML
    txml::XMLDocument doc;
    doc.LoadFile("samples2.xml");
    txml::XMLElement* root = doc.RootElement();

    txml::XMLElement* execution = root->FirstChildElement("execution");
    if (string(execution->Attribute("type")) == "Classic")
    {
        for (txml::XMLElement* xelem = execution->NextSiblingElement(); xelem != nullptr; xelem = xelem->NextSiblingElement())
        {
            Model* model = nullptr;
            string name = string(xelem->Attribute("name"));
            cout << "< " << name << endl;

            bool isOverlapping = string(xelem->Name()) == "overlapping";
            int size = xelem->IntAttribute("size", isOverlapping ? 48 : 24);
            int width = xelem->IntAttribute("width", size);
            int height = xelem->IntAttribute("height", size);
            bool periodic = xelem->BoolAttribute("periodic", false);

            const char* heuristicAux = xelem->Attribute("heuristic");
            string heuristicString = (heuristicAux != nullptr) ? string(heuristicAux) : "Entropy";
            Model::Heuristic heuristic = (heuristicString == "Scanline") ? Model::Heuristic::Scanline :
                (heuristicString == "MRV") ? Model::Heuristic::MRV :
                Model::Heuristic::Entropy;

            if (isOverlapping)
            {
                int N = xelem->IntAttribute("N", 3);
                bool periodicInput = xelem->BoolAttribute("periodicInput", true);
                int symmetry = xelem->IntAttribute("symmetry", 8);
                bool ground = xelem->BoolAttribute("ground", false);

                model = new OverlappingModel(name, N, width, height, periodicInput, periodic, symmetry, ground, heuristic);
            }
            else
            {
                const char* subsetAux = xelem->Attribute("subset");
                string subset = (subsetAux != nullptr) ? string(subsetAux) : "";
                bool blackBackground = xelem->BoolAttribute("blackBackground", false);

                model = new SimpleTiledModel(name, subset, width, height, periodic, blackBackground, heuristic);
            }

            int screenshots = xelem->IntAttribute("screenshots", 2);
            int tries = 10;
            for (int s = 0; s < screenshots; s++)
            {
                for (int t = 0; t < tries; t++)
                {
                    cout << "> ";
                    int seed = random();
                    bool success = model->Run(seed, xelem->IntAttribute("limit", -1));
                    if (success)
                    {
                        cout << "DONE" << endl;
                        model->Save("output/" + name + " " + to_string(seed) + ".png");

                        // Si es un modelo SimpleTiledModel, generar salida en texto si se requiere
                        if (!isOverlapping && xelem->BoolAttribute("textOutput", false))
                        {
                            SimpleTiledModel* stmodel = dynamic_cast<SimpleTiledModel*>(model);
                            ofstream textOut("output/" + name + " " + to_string(seed) + ".txt");
                            textOut << stmodel->TextOutput();
                        }
                        break;
                    }
                    else
                    {
                        cout << "CONTRADICTION" << endl;
                    }
                }
            }

            delete model; // Liberar la memoria al final de cada iteración
        }
    }
    else // string(execution->Attribute("type")) == "Analysis"
    {
        for (txml::XMLElement* xelem = execution->NextSiblingElement(); xelem != nullptr; xelem = xelem->NextSiblingElement())
        {
            Model* model = nullptr;
            string name = string(xelem->Attribute("name"));
            
            bool isOverlapping = string(xelem->Name()) == "overlapping";
            int size = xelem->IntAttribute("size", isOverlapping ? 48 : 24);
            int width = xelem->IntAttribute("width", size);
            int height = xelem->IntAttribute("height", size);
            bool periodic = xelem->BoolAttribute("periodic", false);

            const char* heuristicAux = xelem->Attribute("heuristic");
            string heuristicString = (heuristicAux != nullptr) ? string(heuristicAux) : "Entropy";
            Model::Heuristic heuristic = (heuristicString == "Scanline") ? Model::Heuristic::Scanline :
                (heuristicString == "MRV") ? Model::Heuristic::MRV :
                Model::Heuristic::Entropy;

            if (isOverlapping)
            {
                int N = xelem->IntAttribute("N", 3);
                bool periodicInput = xelem->BoolAttribute("periodicInput", true);
                int symmetry = xelem->IntAttribute("symmetry", 8);
                bool ground = xelem->BoolAttribute("ground", false);

                model = new OverlappingModel(name, N, width, height, periodicInput, periodic, symmetry, ground, heuristic);
            }
            else
            {
                const char* subsetAux = xelem->Attribute("subset");
                string subset = (subsetAux != nullptr) ? string(subsetAux) : "";
                bool blackBackground = xelem->BoolAttribute("blackBackground", false);

                model = new SimpleTiledModel(name, subset, width, height, periodic, blackBackground, heuristic);
            }

            int screenshots = xelem->IntAttribute("screenshots", 10);
            int tries = 10;
            ExecutionData* data = new ExecutionData(name, screenshots, tries);
            for (int s = 0; s < screenshots; s++)
            {
                int contradictions = 0;
                auto execStart = chrono::high_resolution_clock::now();
                
                for (int t = 0; t < tries; t++)
                {
                    int seed = random();
                    bool success = model->Run(seed, xelem->IntAttribute("limit", -1));
                    if (success)
                    {
                        model->Save("output/" + name + " " + to_string(seed) + ".png");

                        // Si es un modelo SimpleTiledModel, generar salida en texto si se requiere
                        if (!isOverlapping && xelem->BoolAttribute("textOutput", false))
                        {
                            SimpleTiledModel* stmodel = dynamic_cast<SimpleTiledModel*>(model);
                            ofstream textOut("output/" + name + " " + to_string(seed) + ".txt");
                            textOut << stmodel->TextOutput();
                        }
                        break;
                    }
                    else
                    {
                        ++contradictions;
                    }
                }
                data->SetContradictions(contradictions, s);

                auto execFinish = chrono::high_resolution_clock::now();
                chrono::duration<double, milli> execElapsed = execFinish - execStart;
                data->SetDuration(execElapsed, s);
            }
            data->PrintResults();

            delete model; delete data; // Liberar la memoria al final de cada iteración
        }
    }

    

    // Finalización del cronómetro
    auto finish = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> elapsed = finish - start;
    cout << "time = " << elapsed.count() << " ms" << endl;

    return 0;
}

// Ejecutar programa: Ctrl + F5 o menú Depurar > Iniciar sin depurar
// Depurar programa: F5 o menú Depurar > Iniciar depuración

// Sugerencias para primeros pasos: 1. Use la ventana del Explorador de soluciones para agregar y administrar archivos
//   2. Use la ventana de Team Explorer para conectar con el control de código fuente
//   3. Use la ventana de salida para ver la salida de compilación y otros mensajes
//   4. Use la ventana Lista de errores para ver los errores
//   5. Vaya a Proyecto > Agregar nuevo elemento para crear nuevos archivos de código, o a Proyecto > Agregar elemento existente para agregar archivos de código existentes al proyecto
//   6. En el futuro, para volver a abrir este proyecto, vaya a Archivo > Abrir > Proyecto y seleccione el archivo .sln

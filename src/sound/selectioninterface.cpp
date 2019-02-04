#include "selectioninterface.h"
CSelectionInterface::~CSelectionInterface()
{
}

void CSelectionInterface::Enumerate(vector<deviceprop>& devs, const int* desiredsamplerates)
{
    vector<string> names;
    vector<string> descriptions;
    Enumerate(names, descriptions);
    deviceprop dp;
    for (const int* dsr=desiredsamplerates; *dsr; dsr++)
        dp.samplerates[abs(*dsr)] = true;
    devs.clear();
    for (size_t i=0; i<names.size(); i++)
    {
        dp.name = names.at(i);
        dp.desc = i<descriptions.size() ? descriptions.at(i) : "";
        devs.push_back(dp);
    }
}

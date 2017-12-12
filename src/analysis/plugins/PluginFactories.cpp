#include "PluginFactories.h"

// Maps for registering the plugins
map<string, unique_ptr<IFDSTabulationProblemPlugin> (*)(
                LLVMBasedICFG &I, vector<string> EntryPoints)>
    IFDSTabulationProblemPluginFactory;

map<string, unique_ptr<IDETabulationProblemPlugin> (*)(
                LLVMBasedICFG &I, vector<string> EntryPoints)>
    IDETabulationProblemPluginFactory;

map<string, unique_ptr<IntraMonotoneProblemPlugin> (*)()>
    IntraMonotoneProblemPluginFactory;

map<string, unique_ptr<InterMonotoneProblemPlugin> (*)()>
    InterMonotoneProblemPluginFactory;

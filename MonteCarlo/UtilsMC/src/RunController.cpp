#include "RunController.h"
#include "TabularStream.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <VModule.h>
#include "colorText.h"

using namespace std;
using namespace utl;
namespace pt = boost::property_tree;

namespace fwk {

    RunController::RunController() :
            fTiming(false),
            fModuleTracing(false) {
        fCurrentEvent = new evt::Event();
    }


    VModule &
    RunController::GetModule(const string &moduleName)
    const {
        auto m = VModuleFactory::Create<VModule>(moduleName);

        if (find(fUsedModuleNames.begin(), fUsedModuleNames.end(), moduleName) == fUsedModuleNames.end()) {
            fUsedModuleNames.push_back(moduleName);
        }


        if (!m) {
            ostringstream emsg;
            emsg << "No module creator found for module with name : '" << moduleName << "' "
                                                                                        "Most likely reasons:\n"
                                                                                        "1) You misspelled the module name, or the module doesn't exist.\n"
                                                                                        "2) You declared a default constructor for your module\n"
                                                                                        "   but did not provide an implementation.\n"
                                                                                        "3) You linked with static libraries (currently not supported).\n"
                                                                                        "Note : the following modules are registered : \n"
                 << GetRegisteredModuleNames();
            throw std::runtime_error(emsg.str());
        }

        auto rawModule = m.release();
        return *rawModule;
    }


//    bool
//    RunController::HasModule(const string& moduleName)
//    const
//    {
//        return VModuleFactory::Create(moduleName);
//    }


    unsigned int
    RunController::GetNumberOfRegisteredModules() {
        return VModuleFactory::GetNumberOfCreators();
    }


    string
    RunController::GetRegisteredModuleNames() {
        ostringstream modules;
        for (const auto &p: fwk::VModuleFactory::GetRegiseredIdentifiers()) {
            modules << KBLU << p << RST "\n";
        }
        return modules.str();
    }


    void
    RunController::Init(const boost::property_tree::ptree &config) {
        BuildModules(config.get_child("ModuleSequence"));
        InitModules(config.get_child("ModuleConfiguration"));
    }


    void
    RunController::Run() {

    }


    void
    RunController::Finish() {
        fStopwatch.Stop();
        const double totalUTime = fStopwatch.GetCPUTime(Stopwatch::eUser) / second;
        const double totalSTime = fStopwatch.GetCPUTime(Stopwatch::eSystem) / second;
        const double totalTime = fStopwatch.GetCPUTime() / second;
        double moduleUTimeSum = 0;
        double moduleSTimeSum = 0;
        double moduleRTimeSum = 0;

        ostringstream failureMessage;

        TabularStream tab("r:  .  .  .  .");

        tab << "Module" << endc << "USR" << endc << "SYS" << endc << "REAL" << endc << "%" << endr
            << hline;

        // NB call finish in reverse order as init, for reasons of compatibility
        // with legacy clients
        for (auto it = fUsedModuleNames.rbegin();
             it != fUsedModuleNames.rend(); ++it) {

            VModule &module = GetModule(*it);

            if (fTiming) {
                auto stopwatch = module.GetStopwatch();
                auto realTimeStopwatch = module.GetRealTimeStopwatch();
                const double moduleUTime = stopwatch.GetCPUTime(Stopwatch::eUser) / second;
                const double moduleSTime = stopwatch.GetCPUTime(Stopwatch::eSystem) / second;
                const double moduleRTime = realTimeStopwatch.GetTime() / second;
                moduleUTimeSum += moduleUTime;
                moduleSTimeSum += moduleSTime;
                moduleRTimeSum += moduleRTime;
                const double frac = int(1000 * (moduleUTime + moduleSTime) / totalTime + 0.5) / 10.;

                tab << *it << endc << moduleUTime << endc << moduleSTime << endc << moduleRTime << endc << frac << endr;
            }

            const VModule::EResultFlag flag = module.Finish();
            if (flag == VModule::eFailure)
                failureMessage << (failureMessage.str().empty() ? "" : "\n")
                               << "Received Failure message from Finish method of module: " << *it;
        }

        if (fTiming) {
            const double frac = int(1000 * (moduleUTimeSum + moduleSTimeSum) / totalTime) / 10.;
            tab << hline
                << "All modules" << endc << moduleUTimeSum << endc
                << moduleSTimeSum << endc << moduleRTimeSum << endc << frac << endr
                << "Total" << endc << totalUTime << endc << totalSTime << endc << "" << endc << 100;
            ostringstream info;
            info << "\n\nCPU user and system time in Module::Run()\n"
                 << tab;
            std::cout << info.str() << std::endl;
        }
        const double time = fRealTimeStopwatch.Stop();
        ostringstream info;
        info << "Total real time of the run: " << time / second << " sec.";
        std::cout << info.str() << std::endl;

        if (!failureMessage.str().empty()) {
            std::cerr << failureMessage.str() << std::endl;
            throw std::runtime_error(failureMessage.str());
        }
    }


    // Sequence the modules (call their Run methods)
    // according to the sequence specified in the XML file.
//    void
//    RunController::DoRunSequence(const Branch& currentB)
//    {
//        Branch modControl = currentB.GetChild("moduleControl");
//
//        if (!modControl) {
//            INFO("No <moduleControl> tag was found in the module sequencing file. No sequencing will be done.");
//            return;
//        }
//
//        DoNextInSequence(modControl.GetFirstChild());
//    }


    RunController::~RunController() {
        delete fCurrentEvent;
    }

    void RunController::BuildModules(const boost::property_tree::ptree &moduleSequence) {
        //std::cout<<moduleSequence<<std::endl;
        for (const auto &m: moduleSequence) {
            auto moduleName = std::string(m.second.data());
            auto mod = VModuleFactory::Create<VModule>(moduleName);
            if (!mod) {
                ostringstream emsg;
                emsg << "No module creator found for module with name : '" << moduleName << "' "
                                                                                            "Most likely reasons:\n"
                                                                                            "1) You misspelled the module name, or the module doesn't exist.\n"
                                                                                            "2) You declared a default constructor for your module\n"
                                                                                            "   but did not provide an implementation.\n"
                                                                                            "Note : the following modules are registered : \n"
                     << GetRegisteredModuleNames();
                throw std::runtime_error(emsg.str());
            }
            std::cout << "Module: " << mod->GetName() << std::endl;
            mod->Finish();
            fModules[moduleName] = std::move(mod);
        }
    }

    void RunController::InitModules(const boost::property_tree::ptree &moduleConfig) {
        for(const auto& m :fModules){
            auto modCfg = moduleConfig.get_child_optional(m.first);
            if(!modCfg) {
                ostringstream emsg;
                emsg<<"No configuration for module with name: '"<<m.first<<"'!\n";
                throw std::runtime_error(emsg.str());
            }
            m.second->Init(*modCfg);
        }
    }

}

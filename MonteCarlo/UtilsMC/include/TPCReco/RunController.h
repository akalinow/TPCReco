#ifndef TPCSOFT_RUNCONTROLLER_H
#define TPCSOFT_RUNCONTROLLER_H

#include <string>
#include <list>
#include <map>
#include "VModule.h"
#include "boost/property_tree/ptree.hpp"
#include "ModuleExchangeSpace.h"

namespace fwk {

    class RunController {

    public:
        enum EBreakStatus {
            eNoBreak, eBreak
        };

        RunController();
        virtual ~RunController();
        RunController(const RunController &) = delete;
        RunController &operator=(const RunController &) = delete;

        /// Return number of registered modules
        static unsigned int GetNumberOfRegisteredModules();

        /// Get list of all module builder names and module versions in the registry
        // Should this be removed?  Note that it will create modules that don't
        // already exist.
        static std::string GetRegisteredModuleNames();

        /// Get a list of the names of all modules accessed thus far in the run
        std::list<std::string> GetUsedModuleNames() const { return fUsedModuleNames; }

        ModuleExchangeSpace & GetCurrentEvent() { return *fCurrentEvent; }

        PEventTPC getCurrentPEventTPC() {
            return fCurrentEvent->tpcPEvt;
        }

        /// Is timing enabled?
        bool IsTiming() const { return fTiming; }

        virtual void Init(const boost::property_tree::ptree &config);
        virtual EBreakStatus RunSingle();
        virtual void RunFull();
        virtual void Finish();


    private:
        void BuildModules(const boost::property_tree::ptree &moduleConfig);
        void InitModules(const boost::property_tree::ptree &moduleConfig, const std::shared_ptr<GeometryTPC>& geom);
        mutable std::list<std::string> fUsedModuleNames;
        std::map<std::string, std::unique_ptr<VModule>> fModules;
        std::vector<std::string> fModuleSequence;

        ModuleExchangeSpace *fCurrentEvent;

        bool fTiming;
        utl::Stopwatch fStopwatch;
        utl::RealTimeStopwatch fRealTimeStopwatch;


    };

}

#endif //TPCSOFT_RUNCONTROLLER_H

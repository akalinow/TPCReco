#ifndef TPCSOFT_RUNCONTROLLER_H
#define TPCSOFT_RUNCONTROLLER_H

#include <string>
#include <list>
#include <map>
#include <VModule.h>
#include "boost/property_tree/ptree.hpp"


namespace evt {
    class Event {
    };
}


namespace fwk {

    class RunController {

    public:
        enum EBreakStatus {
            eNoBreak, eBreak, eContinue
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

        /// Get module by name
        VModule &GetModule(const std::string &moduleName) const;
        bool HasModule(const std::string &moduleName) const;

        /// Get Event currently being processed
        evt::Event &GetCurrentEvent() const { return *fCurrentEvent; }

        /// Is timing enabled?
        bool IsTiming() const { return fTiming; }

        /// Is module tracing enabled?
        bool IsModuleTracing() const { return fModuleTracing; }

        virtual void Init(const boost::property_tree::ptree &config);
        virtual void Run();
        virtual void Finish();

    protected:


//        void InitBranch(utl::Branch& currentB);
//        void RunBranch(utl::Branch& currentB);
//        void FinishBranch(utl::Branch& currentB);

    private:


//        void DoRunSequence(const utl::Branch& currentB);
//        EBreakStatus DoNextInSequence(const utl::Branch& currentB);
//        void GetNextModuleName(const utl::Branch& currentB);

        void BuildModules(const boost::property_tree::ptree &moduleConfig);
        void InitModules(const boost::property_tree::ptree &moduleConfig);
        mutable std::list<std::string> fUsedModuleNames;
        std::map<std::string,std::unique_ptr<VModule>> fModules;

        evt::Event *fCurrentEvent;

        bool fTiming;
        bool fModuleTracing;
        utl::Stopwatch fStopwatch;
        utl::RealTimeStopwatch fRealTimeStopwatch;


    };

}

#endif //TPCSOFT_RUNCONTROLLER_H

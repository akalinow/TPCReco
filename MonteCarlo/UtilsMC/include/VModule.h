#ifndef TPCSOFT_VMODULE_H
#define TPCSOFT_VMODULE_H

#include <string>
#include <utility>
#include <vector>

#include <ObjectRegistrator.h>
#include <ObjectFactory.h>
#include "Stopwatch.h"
#include "RealTimeStopwatch.h"
#include "ModuleExchangeSpace.h"
#include "GeometryTPC.h"

#include "boost/property_tree/ptree.hpp"


namespace fwk {

    class VModule;

    typedef utl::ObjectFactory<VModule *, std::string> VModuleFactory;


#define REGISTER_MODULE(_moduleName_)                                    \
public:                                                                  \
  static std::string GetRegistrationId()                                 \
  { return #_moduleName_; }                                              \
                                                                         \
  static                                                                 \
  VModule*                                                               \
  Create()                                                               \
  {                                                                      \
    return new _moduleName_;                                             \
  }                                                                      \
                                                                         \
 std::string GetName() const override                                    \
  {                                                                      \
    return #_moduleName_;                                                \
  }                                                                      \
                                                                         \
private:                                                                 \
  utl::ObjectRegistrator<_moduleName_, fwk::VModuleFactory> fAutoModuleReg{};


    /**
      \class VModule VModule.h "fwk/VModule.h"

      \brief Module interface

      Module interface defining standard methods to be
      implemented in user module.  User modules inherit
      from this class.  RunController sequences the methods
      methods in modules via this interface.

      The include file VModule.h also contains the macro
      which registers user modules with the framework.  The
      syntax for use is of this macro:
      \code
        REGISTER_MODULE("module", type);
      \endcode
      where
      <ul>
        <li> module = the name by which the module is known to the RunController
        <li> type = the class name of the module
      </ul>
      This macro should be placed at the very end of your class definition.

      \author T. Paul
      \author D. Veberic
      \date Jan 30, 2003
      Modified by P. Podlaski, 02.2023
    */

    class VModule {

    public:
        VModule() = default;

        // prevent copying
        VModule(const VModule &) = delete;

        VModule &operator=(const VModule &) = delete;

        virtual ~VModule() = default;

        /// Flag returned by module methods to the RunController
        enum EResultFlag {
            /// Report success to RunController.
            eSuccess,
            /// Report failure to RunController, causing RunController to terminate execution.
            eFailure,
            /// Break current loop.  It works for nested loops too!
            eBreakLoop,
            /// Skip remaining modules in the current loop and continue with next iteration of the loop
            eContinueLoop
        };

        static std::string GetResultFlagByName(EResultFlag flag);

        /// Initialize: invoked at beginning of run (NOT beginning of event)
        /** This method is for things that should be done once
            at the beginning of a run (for example, booking histograms,
            performing calculations that need to be done only once,
            initializing parameters) {\em You must override this method in
            your concrete module}
        */
        virtual EResultFlag Init(boost::property_tree::ptree config) = 0;

        /// Process: invoked once per event
        /** This method is for things that should be done once per event
            {\em You must override this method in your concrete module}
        */
        virtual EResultFlag Process(ModuleExchangeSpace &event) = 0;

        /// Finish: invoked at end of the run (NOT end of the event)
        /** This method is for things that should be done at the end of the run (for
            example, closing files or writing out histograms)
            {\em You must override this method in your concrete module}
        */
        virtual EResultFlag Finish() = 0;

        void InitTiming() {
            fStopwatch.Reset();
            fRealTimeStopwatch.Reset();
        }

        EResultFlag
        ProcessWithTiming(ModuleExchangeSpace &event) {
            fRealTimeStopwatch.Start();
            fStopwatch.Start();
            auto flag = Process(event);
            fStopwatch.Stop();
            fRealTimeStopwatch.Stop();
            return flag;
        }

        utl::Stopwatch &GetStopwatch() { return fStopwatch; }

        const utl::Stopwatch &GetStopwatch() const { return fStopwatch; }

        utl::RealTimeStopwatch &GetRealTimeStopwatch() { return fRealTimeStopwatch; }

        const utl::RealTimeStopwatch &GetRealTimeStopwatch() const { return fRealTimeStopwatch; }

        virtual std::string GetName() const = 0;

        void SetGeometry(std::shared_ptr<GeometryTPC> geom) { geometry = std::move(geom); }


    protected:
        std::shared_ptr<GeometryTPC> geometry;
    private:
        utl::Stopwatch fStopwatch;
        utl::RealTimeStopwatch fRealTimeStopwatch;

    };

    //A dummy module that does nothing:
    //Interesting bug that I (PP) was not able to solve:
    //Without including at least one .h file with the usage of REGISTER_MODULE macro the registration does not work,
    //so this dummy module is sort of a workaround this issue.
//    class Dummy: public VModule{
//        EResultFlag Init(boost::property_tree::ptree config) override { return eSuccess; }
//        EResultFlag Process(evt::Event& event) override { return eSuccess; }
//        EResultFlag Finish() override { return eSuccess; }
//
//    REGISTER_MODULE(Dummy);
//    };

}


#endif //TPCSOFT_VMODULE_H

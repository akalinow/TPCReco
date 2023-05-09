// $Id$

#include "TPCReco/VModule.h"

using namespace std;


namespace fwk {

    string
    VModule::GetResultFlagByName(const VModule::EResultFlag flag)
    {
        switch (flag) {
            case eSuccess:
                return "eSuccess";
            case eFailure:
                return "eFailure";
            case eBreakLoop:
                return "eBreakLoop";
            case eContinueLoop:
                return "eContinueLoop";
        }
        return "eFailure";

    }

}

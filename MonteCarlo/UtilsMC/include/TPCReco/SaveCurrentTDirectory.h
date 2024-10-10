#ifndef TPCSOFT_SAVECURRENTTDIRECTORY_H
#define TPCSOFT_SAVECURRENTTDIRECTORY_H

#include <TDirectory.h>


namespace utl {

    class SaveCurrentTDirectory {
    public:
        SaveCurrentTDirectory() : fDirectory(gDirectory) { }
        SaveCurrentTDirectory(const SaveCurrentTDirectory&) = delete;
        SaveCurrentTDirectory& operator=(const SaveCurrentTDirectory&)=delete;

        ~SaveCurrentTDirectory() { fDirectory->cd(); }

    private:

        TDirectory* const fDirectory;
    };

}

#endif //TPCSOFT_SAVECURRENTTDIRECTORY_H

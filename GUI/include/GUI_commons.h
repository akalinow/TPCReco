#ifndef GUI_Commons_H
#define GUI_Commons_H

enum CommandIdentifiers {
   M_FILE_OPEN,
   M_FILE_SAVE,
   M_FILE_SAVEAS,
   M_FILE_PRINT,
   M_FILE_PRINTSETUP,
   M_FILE_EXIT,

   M_TOGGLE_LOGSCALE,
   M_DIR_WATCH,

   M_HELP_CONTENTS,
   M_HELP_SEARCH,
   M_HELP_ABOUT,

   M_NEXT_EVENT,
   M_PREVIOUS_EVENT,
   M_GOTO_EVENT,
   M_GOTO_ENTRY,

   M_ADD_SEGMENT,
   M_FIT,
};

enum Messages {
	       M_DATA_FILE_UPDATED
	       
};

enum Modes {
	    M_ONLINE_MODE,
	    M_OFFLINE_GRAW_MODE,
	    M_OFFLINE_ROOT_MODE	       
};

#endif

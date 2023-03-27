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
   M_TOGGLE_AUTOZOOM,
   M_TOGGLE_RECOMODE,
   M_TOGGLE_RATE,
   M_DIR_WATCH,

   M_HELP_CONTENTS,
   M_HELP_SEARCH,
   M_HELP_ABOUT,

   M_NEXT_EVENT,
   M_PREVIOUS_EVENT,
   M_RESET_EVENT,
   M_GOTO_EVENT,
   M_GOTO_ENTRY,
   M_RESET_RATE,

   M_CLEAR_TRACKS,
   M_ADD_VERTEX,
   M_ADD_SEGMENT,
   M_FIT_SEGMENT,
   M_WRITE_SEGMENT,

   M_SET_V_DRIFT,
   M_SET_SAMLING_RATE,
   M_SET_TRG_DELAY,
   M_SET_CONDITIONS

};

enum Messages {
	       M_DATA_FILE_UPDATED
	       
};

enum Modes {
	    M_ONLINE_GRAW_MODE,
	    M_ONLINE_NGRAW_MODE,
	    M_OFFLINE_GRAW_MODE,
	    M_OFFLINE_NGRAW_MODE,
	    M_OFFLINE_ROOT_MODE,
	    M_OFFLINE_MC_MODE,	       
};

#endif
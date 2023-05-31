#ifndef _dataEventTPC_H_
#define _dataEventTPC_H_

#include <string>
#include <vector>

std::map<filter_type, std::string> FilterTypes = {
      {filter_type::none,"filter_type::none"},
      {filter_type::threshold,"filter_type::threshold"}
};
std::map<scale_type, std::string> ScaleTypes = {
      {scale_type::raw,"scale_type::raw"},
      {scale_type::mm,"scale_type::mm"}
};
std::map < projection_type, std::string> Projections1D = {
      {projection_type::DIR_V, "projection_type::DIR_V"},
      {projection_type::DIR_TIME, "projection_type::DIR_TIME"}
};

std::map<projection_type, std::string> ProjectionTypes1D = {
      {projection_type::DIR_U       ,     "projection_type::DIR_U"},
      {projection_type::DIR_V       ,     "projection_type::DIR_V"},
      {projection_type::DIR_W       ,     "projection_type::DIR_W"},
      {projection_type::DIR_TIME    ,     "projection_type::DIR_TIME"},
      {projection_type::DIR_TIME_U  ,     "projection_type::DIR_TIME_U"},
      {projection_type::DIR_TIME_V  ,     "projection_type::DIR_TIME_V"},
      {projection_type::DIR_TIME_W  ,     "projection_type::DIR_TIME_W"}
};

std::map<projection_type, std::string> ProjectionTypes2D = {
      {projection_type::DIR_TIME_U  , "projection_type::DIR_TIME_U"},
      {projection_type::DIR_TIME_V  , "projection_type::DIR_TIME_V"},
      {projection_type::DIR_TIME_W  , "projection_type::DIR_TIME_W"}
 };

std::map<std::tuple<double, double, double, double>, std::string> Test_GetTotalCharge = {
      {std::make_tuple(-1, -1, -1, -1)    , "(-1, -1, -1, -1"},
      {std::make_tuple(DIR_U, -1, -1, -1) , "(DIR_U, -1, -1, -1"},
      {std::make_tuple(DIR_U, -1, 1, -1)  , "(DIR_U, -1, 1, -1"},
      {std::make_tuple(DIR_U, 1, 58, -1)  , "(DIR_U, 1, 58, -1"},
      {std::make_tuple(-1, -1, -1, 128)   , "(-1, -1, -1, 128"},
      {std::make_tuple(DIR_U, -1, -1, 128), "(DIR_U, -1, -1, 128"},
      {std::make_tuple(DIR_U, 1, -1, 128) , "(DIR_U, 1, -1, 128"}
};

std::map<std::tuple<double, double, double>, std::string> Test_GetMaxCharge = {
      {std::make_tuple(-1, -1, -1)    , "(-1, -1, -1"},
      {std::make_tuple(DIR_U, -1, -1) , "(DIR_U, -1, -1"},
      {std::make_tuple(DIR_U, -1, 1)  , "(DIR_U, -1, 1"},
      {std::make_tuple(DIR_U, 1, 58)  , "(DIR_U, 1, 58"}
};

std::map<double, std::string> Test_GetMaxChargePos = { {-1,"-1"}, {DIR_U,"DIR_U"}};

int maxTime = 0, maxStrip = 0, minTime = 0, minStrip = 0;

std::map<std::tuple<double, filter_type>, std::tuple<double, double, double, double, std::string, std::string, std::string, std::string>> Test_GetSignalRange = {};

std::map<std::tuple<bool, double, double, double>,  std::string> Test_GetMultiplicity = {
      {std::make_tuple(false, -1, -1, -1),    "(false, -1, -1, -1"},    {std::make_tuple(true, -1, -1, -1),      "(true, -1, -1, -1"},
      {std::make_tuple(false, DIR_U, -1, -1), "(false, DIR_U, -1, -1"}, {std::make_tuple(true, DIR_U, -1, -1),"(true, DIR_U, -1, -1"},
      {std::make_tuple(false, DIR_V, -1, -1), "(false, DIR_V, -1, -1"}, {std::make_tuple(true, DIR_V, -1, -1),"(true, DIR_V, -1, -1"},
      {std::make_tuple(false, DIR_W, -1, -1), "(false, DIR_W, -1, -1"}, {std::make_tuple(true, DIR_W, -1, -1),"(true, DIR_W, -1, -1"},
      {std::make_tuple(false, DIR_U, 0, -1) , "(false, DIR_U, 0, -1"},  {std::make_tuple(true, DIR_U, 0, -1),  "(true, DIR_U, 0, -1"},
      {std::make_tuple(false, DIR_V, 0, -1) , "(false, DIR_V, 0, -1"},  {std::make_tuple(true, DIR_U, 0, 70),  "(true, DIR_U, 0, 70"},
      {std::make_tuple(false, DIR_W, 0, -1) , "(false, DIR_W, 0, -1"},  {std::make_tuple(true, DIR_U, -1, 70), "(true, DIR_U, -1, 70"}  
};

std::string geometryFileName = "geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat";

std::string dataFileName = "/scratch/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/CoBo0_AsAd0_2022-04-12T08:03:44.531_0000.graw,/scratch/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/CoBo0_AsAd1_2022-04-12T08:03:44.533_0000.graw,/scratch/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/CoBo0_AsAd2_2022-04-12T08:03:44.536_0000.graw,/scratch/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/CoBo0_AsAd3_2022-04-12T08:03:44.540_0000.graw";

std::string referenceDataFileName = "";

 std::map<std::string, std::string> Test_Reference_Titles = {
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::raw)->GetTitle()"                  , "Event-89 selected by raw from V integrated over time"       },
      {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::raw)->GetTitle()"             , "Event-89 selected by Threshold from V integrated over time" },
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::raw)->GetXaxis()->GetTitle()"      , "V [strip]"                                                  },
      {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::raw)->GetXaxis()->GetTitle()" , "V [strip]"                                                  },
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::raw)->GetYaxis()->GetTitle()"      , "Charge/strip [arb.u.]"                                      },
      {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::raw)->GetYaxis()->GetTitle()" , "Charge/strip [arb.u.]"                                      },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->GetTitle()"                  , "Event-89 selected by raw from time "                     },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::raw)->GetTitle()"             , "Event-89 selected by Threshold from time "               },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->GetXaxis()->GetTitle()"      , "Time [bin]"                                              },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::raw)->GetXaxis()->GetTitle()" , "Time [bin]"                                              },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->GetYaxis()->GetTitle()"      , "Charge/time bin [arb.u.]"                                },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::raw)->GetYaxis()->GetTitle()" , "Charge/time bin [arb.u.]"                                },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetTitle()"                  , "Event-89 selected by raw from V"                       },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetTitle()"             , "Event-89 selected by Threshold from V"                 },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetXaxis()->GetTitle()"      , "Time [bin]"                                            },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetXaxis()->GetTitle()" , "Time [bin]"                                            },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetYaxis()->GetTitle()"      , "V [strip]"                                             },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetYaxis()->GetTitle()" , "V [strip]"                                             },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetTitle()"                   , "Event-89 selected by raw from V"                       },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetTitle()"              , "Event-89 selected by Threshold from V"                 },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetXaxis()->GetTitle()"       , "Time [mm]"                                             },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetXaxis()->GetTitle()"  , "Time [mm]"                                             },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetYaxis()->GetTitle()"       , "V [mm]"                                                },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetYaxis()->GetTitle()"  , "V [mm]"                                                },
      
      {"get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::raw)->GetName()"           , "hraw_Upro_evt89"               },
      {"get1DProjection(projection_type::DIR_U, filter_type::threshold, scale_type::raw)->GetName()"      , "hThreshold_Upro_evt89"         },
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::raw)->GetName()"           , "hraw_Vpro_evt89"               },
      {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::raw)->GetName()"      , "hThreshold_Vpro_evt89"         },
      {"get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::raw)->GetName()"           , "hraw_Wpro_evt89"               },
      {"get1DProjection(projection_type::DIR_W, filter_type::threshold, scale_type::raw)->GetName()"      , "hThreshold_Wpro_evt89"         },
      {"get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::mm)->GetName()"            , "hraw_Upro_mm_evt89"            },
      {"get1DProjection(projection_type::DIR_U, filter_type::threshold, scale_type::mm)->GetName()"       , "hThreshold_Upro_mm_evt89"      },
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::mm)->GetName()"            , "hraw_Vpro_mm_evt89"            },
      {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::mm)->GetName()"       , "hThreshold_Vpro_mm_evt89"      },
      {"get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::mm)->GetName()"            , "hraw_Wpro_mm_evt89"            },
      {"get1DProjection(projection_type::DIR_W, filter_type::threshold, scale_type::mm)->GetName()"       , "hThreshold_Wpro_mm_evt89"      },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->GetName()"        , "hraw_timetime_evt89"           },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::raw)->GetName()"   , "hThreshold_timetime_evt89"     },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::raw)->GetName()"      , "hraw_Utime_evt89"              },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::raw)->GetName()" , "hThreshold_Utime_evt89"        },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetName()"      , "hraw_Vtime_evt89"              },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetName()" , "hThreshold_Vtime_evt89"        },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::raw)->GetName()"      , "hraw_Wtime_evt89"              },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::raw)->GetName()" , "hThreshold_Wtime_evt89"        },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::mm)->GetName()"         , "hraw_timetime_mm_evt89"        },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::mm)->GetName()"    , "hThreshold_timetime_mm_evt89"  },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::mm)->GetName()"       , "hraw_Utime_mm_evt89"           },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::mm)->GetName()"  , "hThreshold_Utime_mm_evt89"     },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetName()"       , "hraw_Vtime_mm_evt89"           },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetName()"  , "hThreshold_Vtime_mm_evt89"     },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::mm)->GetName()"       , "hraw_Wtime_mm_evt89"           },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::mm)->GetName()"  , "hThreshold_Wtime_mm_evt89"     },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::raw)->GetName()"      , "hraw_U_vs_time_evt89"          },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::raw)->GetName()" , "hThreshold_U_vs_time_evt89"    },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetName()"      , "hraw_V_vs_time_evt89"          },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetName()" , "hThreshold_V_vs_time_evt89"    },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::raw)->GetName()"      , "hraw_W_vs_time_evt89"          },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::raw)->GetName()" , "hThreshold_W_vs_time_evt89"    },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::mm)->GetName()"       , "hraw_U_vs_time_mm_evt89"       },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::mm)->GetName()"  , "hThreshold_U_vs_time_mm_evt89" },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetName()"       , "hraw_V_vs_time_mm_evt89"       },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetName()"  , "hThreshold_V_vs_time_mm_evt89" },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::mm)->GetName()"       , "hraw_W_vs_time_mm_evt89"       },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::mm)->GetName()"  , "hThreshold_W_vs_time_mm_evt89" },
      {"GetChannels(0,0)->GetName()"      , "hraw_cobo0_asad0_signal_evt89"     },
      {"GetChannels_raw(0,0)->GetName()"  , "hraw_cobo0_asad0_signal_fpn_evt89" }
  };

  std::map<std::string, double> Test_Reference = {
      {"get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::raw)->GetEntries()"                , 155908          },
      {"get1DProjection(projection_type::DIR_U, filter_type::threshold, scale_type::raw)->GetEntries()"           , 226979          },
      {"get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::raw)->GetSumOfWeights()"           , 155908.76829307 },
      {"get1DProjection(projection_type::DIR_U, filter_type::threshold, scale_type::raw)->GetSumOfWeights()"      , 226979.29878    },
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::raw)->GetEntries()"                , 176881          },
      {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::raw)->GetEntries()"           , 285676          },
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::raw)->GetSumOfWeights()"           , 176881.810976   },
      {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::raw)->GetSumOfWeights()"      , 285676.20122    },
      {"get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::raw)->GetEntries()"                , 188406          },
      {"get1DProjection(projection_type::DIR_W, filter_type::threshold, scale_type::raw)->GetEntries()"           , 282279          },
      {"get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::raw)->GetSumOfWeights()"           , 188406.182927   },
      {"get1DProjection(projection_type::DIR_W, filter_type::threshold, scale_type::raw)->GetSumOfWeights()"      , 282279.72561    },
      {"get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::mm)->GetEntries()"                 , 155908          },
      {"get1DProjection(projection_type::DIR_U, filter_type::threshold, scale_type::mm)->GetEntries()"            , 226979          },
      {"get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::mm)->GetSumOfWeights()"            , 155908.768293   },
      {"get1DProjection(projection_type::DIR_U, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"       , 226979.29878    },
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::mm)->GetEntries()"                 , 176881          },
      {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::mm)->GetEntries()"            , 285676          },
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::mm)->GetSumOfWeights()"            , 176881.810976   },
      {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"       , 285676.20122    },
      {"get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::mm)->GetEntries()"                 , 188406          },
      {"get1DProjection(projection_type::DIR_W, filter_type::threshold, scale_type::mm)->GetEntries()"            , 282279          },
      {"get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::mm)->GetSumOfWeights()"            , 188406.182927   },
      {"get1DProjection(projection_type::DIR_W, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"       , 282279.72561    },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->GetEntries()"             , 520098          },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::raw)->GetEntries()"        , 794935          },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->GetSumOfWeights()"        , 520098.542683   },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::raw)->GetSumOfWeights()"   , 794935.22561    },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::raw)->GetEntries()"           , 155908          },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::raw)->GetEntries()"      , 226979          },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::raw)->GetSumOfWeights()"      , 155908.768293   },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::raw)->GetSumOfWeights()" , 226979.29878    },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetEntries()"           , 176881          },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetEntries()"      , 285676          },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetSumOfWeights()"      , 176881.81097561 },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetSumOfWeights()" , 285676.20122    },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::raw)->GetEntries()"           , 187307          },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::raw)->GetEntries()"      , 282279          },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::raw)->GetSumOfWeights()"      , 187307.96341464 },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::raw)->GetSumOfWeights()" , 282279.72561    },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::mm)->GetEntries()"              , 520098          },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::mm)->GetEntries()"         , 794935          },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::mm)->GetSumOfWeights()"         , 520098.542683   },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"    , 794935.22561    },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::mm)->GetEntries()"            , 155908          },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::mm)->GetEntries()"       , 226979          },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::mm)->GetSumOfWeights()"       , 155908.768293   },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"  , 226979.29878    },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetEntries()"            , 176881          },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetEntries()"       , 285676          },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetSumOfWeights()"       , 176881.810976   },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"  , 285676.20122    },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::mm)->GetEntries()"            , 187307          },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::mm)->GetEntries()"       , 282279          },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::mm)->GetSumOfWeights()"       , 187307.9634146  },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"  , 282279.72561    },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::raw)->GetEntries()"           , 155908          },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::raw)->GetEntries()"      , 226979          },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::raw)->GetSumOfWeights()"      , 155908.768293   },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::raw)->GetSumOfWeights()" , 226979.29878    },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetEntries()"           , 176881          },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetEntries()"      , 285676          },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetSumOfWeights()"      , 176881.81097563 },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetSumOfWeights()" , 285676.20122    },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::raw)->GetEntries()"           , 187307          },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::raw)->GetEntries()"      , 282279          },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::raw)->GetSumOfWeights()"      , 187307.9634146  },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::raw)->GetSumOfWeights()" , 282279.72561    },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::mm)->GetEntries()"            , 224650          },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::mm)->GetEntries()"       , 295721          },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::mm)->GetSumOfWeights()"       , 155908.768293   },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"  , 226979.29878    },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetEntries()"            , 176881          },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetEntries()"       , 285676          },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetSumOfWeights()"       , 176881.810976   },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"  , 285676.20122    },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::mm)->GetEntries()"            , 304271          },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::mm)->GetEntries()"       , 399243          },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::mm)->GetSumOfWeights()"       , 187307.9634145  },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"  , 282279.72561    } ,
      {"GetChannels(0,0)->GetEntries()"          , 131072           },
      {"GetChannels(0,0)->GetSumOfWeights()"     , -5029.57317073   },
      {"GetChannels_raw(0,0)->GetEntries()"      , 139264           },
      {"GetChannels_raw(0,0)->GetSumOfWeights()" , -5029.57317073   },


      {"GetTotalCharge(-1, -1, -1, -1, filter_type::none)"       , 520098.542683  } , {"GetTotalCharge(-1, -1, -1, -1, filter_type::threshold)"       , 794935.22561   } ,
      {"GetTotalCharge(DIR_U, -1, -1, -1, filter_type::none)"    , 155908.768293  } , {"GetTotalCharge(DIR_U, -1, -1, -1, filter_type::threshold)"    , 226979.29878   } ,
      {"GetTotalCharge(DIR_U, -1, 1, -1, filter_type::none)"     , 1407.329268293 } , {"GetTotalCharge(DIR_U, -1, 1, -1, filter_type::threshold)"     , 0              } ,
      {"GetTotalCharge(DIR_U, 1, 58, -1, filter_type::none)"     , 367.042682927  } , {"GetTotalCharge(DIR_U, 1, 58, -1, filter_type::threshold)"     , 0              } ,
      {"GetTotalCharge(-1, -1, -1, 128, filter_type::none)"      , 2890.25609756  } , {"GetTotalCharge(-1, -1, -1, 128, filter_type::threshold)"      , 3425.77439024  } ,
      {"GetTotalCharge(DIR_U, -1, -1, 128, filter_type::none)"   , 950.109756098  } , {"GetTotalCharge(DIR_U, -1, -1, 128, filter_type::threshold)"   , 1032.90243902  } ,
      {"GetTotalCharge(DIR_U, 1,  -1, 128, filter_type::none)"   , -79.378        } , {"GetTotalCharge(DIR_U, 1,  -1, 128, filter_type::threshold)"   , 0              } ,
      {"GetMaxCharge(-1, -1, -1, filter_type::none)"             , 1735.09146341  } , {"GetMaxCharge(-1, -1, -1, filter_type::threshold)"             , 1732.16463415  } ,
      {"GetMaxCharge(DIR_U, -1, -1, filter_type::none)"          , 1392.21341463  } , {"GetMaxCharge(DIR_U, -1, -1, filter_type::threshold)"          , 1385.27439024  } ,
      {"GetMaxCharge(DIR_U, -1, 1, filter_type::none)"           , 23.7256        } , {"GetMaxCharge(DIR_U, -1, 1, filter_type::threshold)"           , 0              } ,
      {"GetMaxCharge(DIR_U, 1, 58, filter_type::none)"           , 22.2561        } , {"GetMaxCharge(DIR_U, 1, 58, filter_type::threshold)"           , 0              } ,
      {"GetMaxChargePos(-1, filter_type::none)->maxTime"         , 58             } , {"GetMaxChargePos(-1, filter_type::threshold)->maxTime"         , 58             } ,
      {"GetMaxChargePos(-1, filter_type::none)->maxStrip"        , 65             } , {"GetMaxChargePos(-1, filter_type::threshold)->maxStrip"        , 65             } ,
      {"GetMaxChargePos(DIR_U, filter_type::none)->maxTime"      , 58             } , {"GetMaxChargePos(DIR_U, filter_type::threshold)->maxTime"      , 58             } ,
      {"GetMaxChargePos(DIR_U, filter_type::none)->maxStrip"     , 65             } , {"GetMaxChargePos(DIR_U, filter_type::threshold)->maxStrip"     , 65             } ,
      {"GetSignalRange(-1, filter_type::none)->minTime"          , 2              } , {"GetSignalRange(-1, filter_type::threshold)->minTime"          , 33             } ,
      {"GetSignalRange(-1, filter_type::none)->maxTime"          , 500            } , {"GetSignalRange(-1, filter_type::threshold)->maxTime"          , 251            } ,
      {"GetSignalRange(-1, filter_type::none)->minStrip"         , 1              } , {"GetSignalRange(-1, filter_type::threshold)->minStrip"         , 46             } ,
      {"GetSignalRange(-1, filter_type::none)->maxStrip"         , 226            } , {"GetSignalRange(-1, filter_type::threshold)->maxStrip"         , 216            } ,
      {"GetSignalRange(DIR_U, filter_type::none)->minTime"       , 3              } , {"GetSignalRange(DIR_U, filter_type::threshold)->minTime"       , 37             } ,
      {"GetSignalRange(DIR_U, filter_type::none)->maxTime"       , 501            } , {"GetSignalRange(DIR_U, filter_type::threshold)->maxTime"       , 250            } ,
      {"GetSignalRange(DIR_U, filter_type::none)->minStrip"      , 1              } , {"GetSignalRange(DIR_U, filter_type::threshold)->minStrip"      , 46             } ,
      {"GetSignalRange(DIR_U, filter_type::none)->maxStrip"      , 132            } , {"GetSignalRange(DIR_U, filter_type::threshold)->maxStrip"      , 81             } ,
      {"GetMultiplicity(false, -1, -1, -1, filter_type::none)"   , 1018           } , {"GetMultiplicity(false, -1, -1, -1, filter_type::threshold)"   , 112            } ,
      {"GetMultiplicity(false, DIR_U, -1, -1, filter_type::none)", 131            } , {"GetMultiplicity(false, DIR_U, -1, -1, filter_type::threshold)", 25             } ,
      {"GetMultiplicity(false, DIR_V, -1, -1, filter_type::none)", 224            } , {"GetMultiplicity(false, DIR_V, -1, -1, filter_type::threshold)", 43             } ,
      {"GetMultiplicity(false, DIR_W, -1, -1, filter_type::none)", 225            } , {"GetMultiplicity(false, DIR_W, -1, -1, filter_type::threshold)", 29             } ,
      {"GetMultiplicity(false, DIR_U, 0, -1, filter_type::none)" , 0              } , {"GetMultiplicity(false, DIR_U, 0, -1, filter_type::threshold)" , 0              } ,
      {"GetMultiplicity(false, DIR_V, 0, -1, filter_type::none)" , 74             } , {"GetMultiplicity(false, DIR_V, 0, -1, filter_type::threshold)" , 28             } ,
      {"GetMultiplicity(false, DIR_W, 0, -1, filter_type::none)" , 74             } , {"GetMultiplicity(false, DIR_W, 0, -1, filter_type::threshold)" , 15             } ,
      {"GetMultiplicity(true, -1, -1, -1, filter_type::none)"    , 290917         } , {"GetMultiplicity(true, -1, -1, -1, filter_type::threshold)"    , 5874           } ,
      {"GetMultiplicity(true, DIR_U, -1, -1, filter_type::none)" , 65868          } , {"GetMultiplicity(true, DIR_U, -1, -1, filter_type::threshold)" , 1747           } ,
      {"GetMultiplicity(true, DIR_V, -1, -1, filter_type::none)" , 112275         } , {"GetMultiplicity(true, DIR_V, -1, -1, filter_type::threshold)" , 2266           } ,
      {"GetMultiplicity(true, DIR_W, -1, -1, filter_type::none)" , 112774         } , {"GetMultiplicity(true, DIR_W, -1, -1, filter_type::threshold)" , 1861           } ,
      {"GetMultiplicity(true, DIR_U, 0, -1, filter_type::none)"  , 0              } , {"GetMultiplicity(true, DIR_U, 0, -1, filter_type::threshold)"  , 0              } ,
      {"GetMultiplicity(true, DIR_U, 0, 70, filter_type::none)"  , 0              } , {"GetMultiplicity(true, DIR_U, 0, 70, filter_type::threshold)"  , 0              } ,
      {"GetMultiplicity(true, DIR_U, -1, 70, filter_type::none)" , 499            } , {"GetMultiplicity(true, DIR_U, -1, 70, filter_type::threshold)" , 104            }  

  };

#endif

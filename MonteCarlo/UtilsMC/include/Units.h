#ifndef TPCSOFT_UNITS_H
#define TPCSOFT_UNITS_H

namespace utl {

    /**
      \file ShineUnits.h

      \brief Defines units in terms of NA61/Shine standard units

      You should use the units defined in this file whenever you
      have a dimensional quantity in your code. For example,
      write:
      \code
       double s = 1.5*m;
      \endcode
      instead of:
      \code
        double s = 1.5;   // don't forget this is in meters!
      \endcode
      The conversion factors defined in this file
      convert your data into Shine base units, so that
      all dimensional quantities in the code are in a
      single system of units! You can also
      use the conversions defined here to, for example,
      display data with the unit of your choice.  For example:
      \code
        cout << "s = " << s/mm << " mm";
      \endcode

      The base units are:
        - meter                   (meter)
        - nanosecond              (nanosecond)
        - electron Volt           (eV)
        - positron charge         (eplus)
        - degree Kelvin           (kelvin)
        - the amount of substance (mol)
        - luminous intensity      (candela)
        - radian                  (radian)
        - steradian               (steradian)

      The SI numerical value of the positron charge is defined here,
      as it is needed for conversion factor : positron charge = eSI (coulomb)

      This is a slightly modified version of the units definitions written by
      the Geant4 collaboration

      \version $Id$

      \author M. Maire
      \author S. Giani
      \author D. Veberic (modifications for Shine)
      \ingroup units
    */

    // Prefixes
    const double yocto = 1e-24;
    const double zepto = 1e-21;
    const double atto  = 1e-18;
    const double femto = 1e-15;
    const double pico  = 1e-12;
    const double nano  = 1e-9;
    const double micro = 1e-6;
    const double milli = 1e-3;
    const double centi = 1e-2;
    const double deci  = 1e-1;
    const double deka  = 1e+1;
    const double hecto = 1e+2;
    const double kilo  = 1e+3;
    const double mega  = 1e+6;
    const double giga  = 1e+9;
    const double tera  = 1e+12;
    const double peta  = 1e+15;
    const double exa   = 1e+18;
    const double zetta = 1e+21;
    const double yotta = 1e+24;

      // Time [T]
    const double nanosecond  = 1;
    const double nanosecond2 = nanosecond*nanosecond;
    const double second      = giga*nanosecond;
    const double second2     = second*second;
    const double millisecond = milli*second;
    const double microsecond = micro*second;
    const double picosecond  = pico*second;
    const double minute      = 60*second;
    const double hour        = 60*minute;
    const double day         = 24*hour;

    const double hertz = 1/second;
    const double kilohertz = kilo*hertz;
    const double megahertz = mega*hertz;

    const double Hz = hertz;
    const double kHz = kilohertz;
    const double MHz = megahertz;

    // symbols
    const double ns = nanosecond;
    const double s  = second;
    const double s2 = second2;
    const double ms = millisecond;

}

#endif //TPCSOFT_UNITS_H

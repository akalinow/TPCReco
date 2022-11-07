#ifndef __MULTIKEY_H__

#include<functional>
#include<cstdint>

#include "PEventTPC.h"

typedef std::tuple<int, int> MultiKey2;

typedef std::tuple<int, int, int> MultiKey3;

typedef std::tuple<uint8_t, uint8_t, uint8_t> MultiKey3_uint8;

typedef PEventTPC::chargeMapType::key_type MultiKey4;

typedef std::tuple<int, int, int, int , int> MultiKey5;

#define __MULTIKEY_H__
#endif


#ifndef __MULTIKEY_H__
#define __MULTIKEY_H__

#include <functional>

// Allows to define multi-keyword maps.
// VERSION: 11 Feb 2018

/* ================ 3-key version ================= */ 
//
// Example of use:
//
// std::map<MultiKey3, int, multikey3_less> mymap3;
//
class MultiKey3 {
 public:
  int  key[3];
    
  MultiKey3(int k1=-1, int k2=-1, int k3=-1)
      : key{k1,k2,k3} {}

  bool operator<(const MultiKey3 &right) const 
  {
    if ( key[0] == right.key[0] ) {
      if ( key[1] == right.key[1] ) {
	return key[2] < right.key[2];
      }
      else {
	return key[1] < right.key[1];
      }
    }
    else {
      return key[0] < right.key[0];
    }
  }    
  bool operator()(const MultiKey3 &right) const 
  {
    if ( key[0] == right.key[0] && 
	 key[1] == right.key[1] &&
	 key[2] == right.key[2]    ) return true;
    return false;
  }    
};

struct multikey3_less : public std::binary_function<MultiKey3, MultiKey3, bool>
{
  bool operator()(const MultiKey3 &mkey1, const MultiKey3 &mkey2 ) const 
  {
    return mkey1.operator<(mkey2); // mkey1 < mkey2;
  }
};



#endif

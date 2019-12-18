
#ifndef __MULTIKEY_H__
#define __MULTIKEY_H__

#include <functional>

// Allows to define multi-keyword maps.
// VERSION: 11 Feb 2018

/* ================ 2-key version ================= */ 
//
// Example of use:
//
// std::map<MultiKey2, int, multikey2_less> mymap2;
//
class MultiKey2 {
 public:
  int  key1;
  int  key2;
    
  MultiKey2(int k1=-1, int k2=-1)
    : key1(k1), key2(k2) {};

  bool operator<(const MultiKey2 &right) const 
  {
    if ( key1 == right.key1 ) {
      return key2 < right.key2;
    }
    else {
      return key1 < right.key1;
    }
  }    
  bool operator()(const MultiKey2 &right) const 
  {
    if ( key1 == right.key1 && 
	 key2 == right.key2    ) return true;
    return false;
  }    
};

struct multikey2_less : public std::binary_function<MultiKey2, MultiKey2, bool>
{
  bool operator()(const MultiKey2 &mkey1, const MultiKey2 &mkey2 ) const 
  {
    return mkey1.operator<(mkey2); // mkey1 < mkey2;
  }
};


/* ================ 3-key version ================= */ 
//
// Example of use:
//
// std::map<MultiKey3, int, multikey3_less> mymap3;
//
class MultiKey3 {
 public:
  int  key1;
  int  key2;
  int  key3;
    
  MultiKey3(int k1=-1, int k2=-1, int k3=-1)
    : key1(k1), key2(k2), key3(k3) {}

  bool operator<(const MultiKey3 &right) const 
  {
    if ( key1 == right.key1 ) {
      if ( key2 == right.key2 ) {
	return key3 < right.key3;
      }
      else {
	return key2 < right.key2;
      }
    }
    else {
      return key1 < right.key1;
    }
  }    
  bool operator()(const MultiKey3 &right) const 
  {
    if ( key1 == right.key1 && 
	 key2 == right.key2 &&
	 key3 == right.key3    ) return true;
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


/* ================ 4-key version ================= */ 
//
// Example of use:
//
// std::map<MultiKey4, int, multikey4_less> mymap4;
//
class MultiKey4 {
 public:
  int  key1;
  int  key2;
  int  key3;
  int  key4;
    
  MultiKey4(int k1=-1, int k2=-1, int k3=-1, int k4=-1)
    : key1(k1), key2(k2), key3(k3), key4(k4) {}  

  bool operator<(const MultiKey4 &right) const 
  {
    if ( key1 == right.key1 ) {
      if ( key2 == right.key2 ) {
	if ( key3 == right.key3 ) {
	  return key4 < right.key4;
	}
	else {
	  return key3 < right.key3;
	}
      }
      else {
	return key2 < right.key2;
      }
    }
    else {
      return key1 < right.key1;
    }
  }    
  bool operator()(const MultiKey4 &right) const 
  {
    if ( key1 == right.key1 && 
	 key2 == right.key2 &&
	 key3 == right.key3 &&
	 key4 == right.key4    ) return true;
    return false;
  }    
};

struct multikey4_less : public std::binary_function<MultiKey4, MultiKey4, bool>
{
  bool operator()(const MultiKey4 &mkey1, const MultiKey4 &mkey2 ) const 
  {
    return mkey1.operator<(mkey2); // mkey1 < mkey2;
  }
};

/* ================ 5-key version ================= */ 
//
// Example of use:
//
// std::map<MultiKey5, int, multikey5_less> mymap5;
//
class MultiKey5 {
 public:
  int  key1;
  int  key2;
  int  key3;
  int  key4;
  int  key5;
    
 MultiKey5(int k1=-1, int k2=-1, int k3=-1, int k4=-1, int k5=-1)
   : key1(k1), key2(k2), key3(k3), key4(k4), key5(k5) {}  

  bool operator<(const MultiKey5 &right) const 
  {
    if ( key1 == right.key1 ) {
      if ( key2 == right.key2 ) {
	if ( key3 == right.key3 ) {
	  if ( key4 == right.key4 ) {
	    return key5 < right.key5;
	  }
	  else {
	    return key4 < right.key4;
	  }
	}
	else {
	  return key3 < right.key3;
	}
      }
      else {
	return key2 < right.key2;
      }
    }
    else {
      return key1 < right.key1;
    }
  }    
  bool operator()(const MultiKey5 &right) const 
  {
    if ( key1 == right.key1 && 
	 key2 == right.key2 &&
	 key3 == right.key3 &&
	 key4 == right.key4 &&
	 key5 == right.key5    ) return true;
    return false;
  }    
};

struct multikey5_less : public std::binary_function<MultiKey5, MultiKey5, bool>
{
  bool operator()(const MultiKey5 &mkey1, const MultiKey5 &mkey2 ) const 
  {
    return mkey1.operator<(mkey2); // mkey1 < mkey2;
  }
};

#endif

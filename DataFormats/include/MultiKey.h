
#ifndef __MULTIKEY_H__
#define __MULTIKEY_H__

#include <functional>
#include <initializer_list>

template<std::size_t key_size, typename T = uint16_t>
class MultiKey {
private:
    union {
        uint_fast64_t unified_key;
        T keys[key_size] = { T() };
    };
    bool state = false;
public:
    MultiKey(std::initializer_list<T> list) : keys{ list } {
        state = true;
    };
    MultiKey() = default;
    ~MultiKey() = default;

    inline bool operator<(const MultiKey<key_size, T>& right) {
        return unified_key < right.unified_key;
    }

    inline bool operator()(const MultiKey<key_size, T>& right) const {
        return unified_key == right.unified_key;
    }

    inline MultiKey<key_size, T>& operator=(std::initializer_list<T> init_list) {
        keys = init_list;
        state = true;
        return *this;
    }

    inline MultiKey<key_size, T>& operator=(MultiKey<key_size, T>& key) {
        unified_key = key.unified_key;
    }

    inline T& operator[](int index) {
        return keys[index];
    }

    inline uint_fast64_t unified() {
        return unified_key;
    }
    inline operator bool() {
        return state;
    }
};

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
  int  key[2];
    
  MultiKey2(int k1=-1, int k2=-1)
      : key{ k1,k2 } {};

  bool operator<(const MultiKey2 &right) const 
  {
    if ( key[0] == right.key[0] ) {
      return key[1] < right.key[1];
    }
    else {
      return key[0] < right.key[0];
    }
  }    
  bool operator()(const MultiKey2 &right) const 
  {
    if ( key[0] == right.key[0] && 
	 key[1] == right.key[1]    ) return true;
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


/* ================ 4-key version ================= */ 
//
// Example of use:
//
// std::map<MultiKey4, int, multikey4_less> mymap4;
//
class MultiKey4 {
 public:
  int  key[4];
    
  MultiKey4(int k1=-1, int k2=-1, int k3=-1, int k4=-1)
      : key{k1,k2,k3,k4} {}

  bool operator<(const MultiKey4 &right) const 
  {
    if ( key[0] == right.key[0] ) {
      if ( key[1] == right.key[1] ) {
	if ( key[2] == right.key[2] ) {
	  return key[3] < right.key[3];
	}
	else {
	  return key[2] < right.key[2];
	}
      }
      else {
	return key[1] < right.key[1];
      }
    }
    else {
      return key[0] < right.key[0];
    }
  }    
  bool operator()(const MultiKey4 &right) const 
  {
    if ( key[0] == right.key[0] && 
	 key[1] == right.key[1] &&
	 key[2] == right.key[2] &&
	 key[3] == right.key[3]    ) return true;
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

#endif

/**
   \file
   test object facotry

   \author Lukas Nellen
   \version $Id$
   \date 10 Mar 2004

   Modified by Piotr.Podlaski@fuw.edu.pl
   01.2023

   \ingroup testing
*/

// header for unit test ObjectFactoryTest

#include <ObjectFactory.h>
#include <string>
#include "gtest/gtest.h"


using namespace utl;
using namespace std;

namespace {
    // Test factory polymorphic class with a default constructor
    class IdObjectBase {
    public:
        virtual ~IdObjectBase() = default;
        virtual int GetId() = 0;
    };

    // Template to create concrete implementations of IdObjectBase
    template<int id>
    class IdObject : public IdObjectBase {
    public:
        int GetId() override { return id; }
        static IdObjectBase* Create()
        { return new IdObject; }
    };

    // Convenience typedef for factory
    typedef ObjectFactory<IdObjectBase*, int> IdObjectFactory;

}


/**
  \ingroup testing
*/

TEST(ObjectFactory, creation)
{

    ASSERT_TRUE(IdObjectFactory::Register(1, IdObject<1>::Create));

    auto p1 = IdObjectFactory::Create<IdObjectBase>(1);
    ASSERT_NE(p1.get(), nullptr);
    ASSERT_EQ(p1->GetId(), 1);
    p1.reset();

    auto p2 = IdObjectFactory::Create<IdObjectBase>(2);
    ASSERT_EQ(p2.get(), nullptr);

    ASSERT_TRUE(IdObjectFactory::Register(2, IdObject<2>::Create));

    p2 = IdObjectFactory::Create<IdObjectBase>(2);
    ASSERT_NE(p2.get(), nullptr);
    ASSERT_EQ(p2->GetId(), 2);
    p2.reset();

    auto iter = IdObjectFactory::Begin();
    ASSERT_EQ(iter->first, 1);
    ++iter;
    ASSERT_EQ(iter->first, 2);
    ++iter;
    ASSERT_TRUE(iter == IdObjectFactory::End());
}
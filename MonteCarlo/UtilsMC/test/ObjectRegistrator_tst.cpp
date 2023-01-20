/**
  \file
  Test automatic registration in ObjectFactory

  \author Lukas Nellen
  \version $Id$
  \date 13 Mar 2004

  Modified by Piotr.Podlaski@fuw.edu.pl
  01.2023

  \ingroup testing
*/

#include <ObjectRegistrator.h>
#include <ObjectFactory.h>
#include "gtest/gtest.h"

using namespace utl;

namespace {

    class InheritObjectBase {
    public:
        virtual ~InheritObjectBase() = default;
        virtual int GetId() = 0;
    };

    typedef ObjectFactory<InheritObjectBase*, int> InheritObjectFactory;


    template<int id>
    class InheritObject : public InheritObjectBase,
                          private ObjectRegistrator<InheritObject<id>, InheritObjectFactory> {
    public:
        int GetId() override { return GetRegistrationId(); }

        static typename InheritObjectFactory::IdentifierType
        GetRegistrationId()  { return id; }

        static InheritObjectBase* Create()
        { return new InheritObject; }
    };

    template class InheritObject<1>;
    template class InheritObject<3>;


    class MemberObjectBase {
    public:
        virtual ~MemberObjectBase() = default;
        virtual int GetId() = 0;
    };

    typedef ObjectFactory<MemberObjectBase*, int> MemberObjectFactory;


    template<int id>
    class MemberObject : public MemberObjectBase {
    public:
        int GetId() override { return GetRegistrationId(); }

        static typename MemberObjectFactory::IdentifierType
        GetRegistrationId()  { return id; }

        static MemberObjectBase* Create()
        { return new MemberObject; }
    private:
        ObjectRegistrator<MemberObject<id>, MemberObjectFactory> fAutoReg;
    };

    template class MemberObject<1>;
    template class MemberObject<3>;

    class ExternalObjectBase
    {
    public:
        virtual ~ExternalObjectBase() = default;
        virtual int GetId() = 0;
    };

    typedef utl::ObjectFactory<ExternalObjectBase*, int> ExternalObjectFactory;

    template<int id>
    class ExternalObject
            : public ExternalObjectBase
    {
    public:
        int GetId() override { return GetRegistrationId(); }

        static typename ExternalObjectFactory::IdentifierType
        GetRegistrationId()  { return id; }

        static ExternalObjectBase* Create()
        { return new ExternalObject; }
    private:
        utl::ObjectRegistrator<ExternalObject<id>, ExternalObjectFactory> fAutoReg;
    };

    template class ExternalObject<1>;
    template class ExternalObject<3>;


}


/**
  \ingroup testing
*/

TEST(ObjectRegistrator, CreationInherit)
{
    ASSERT_EQ(InheritObjectFactory::GetNumberOfCreators(), 2u);

    auto p = InheritObjectFactory::Create<InheritObjectBase>(1);
    ASSERT_NE(p.get(), nullptr);
}


TEST(ObjectRegistrator, CreationMember)
{
    ASSERT_EQ(MemberObjectFactory::GetNumberOfCreators(), 2u);
    auto p = MemberObjectFactory::Create<MemberObjectBase>(1);
    ASSERT_NE(p.get(), nullptr);
}

TEST(ObjectRegistrator, CreationExternal)
{
    ASSERT_EQ(ExternalObjectFactory::GetNumberOfCreators(), 2u);

    auto p = ExternalObjectFactory::Create<ExternalObjectBase>(1);
    ASSERT_NE(p.get(), nullptr);
}
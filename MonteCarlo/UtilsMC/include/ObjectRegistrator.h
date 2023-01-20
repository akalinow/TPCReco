#ifndef TPCSOFT_OBJECTREGISTRATOR_H
#define TPCSOFT_OBJECTREGISTRATOR_H
/**
  \file
  Automatic registration of objects

  \author Lukas Nellen
  \version $Id$
  \date 12 Mar 2004
*/

#include "ObjectFactory.h"


namespace utl {

    /**
      \class StandardCreator ObjectRegistrator.h "utl/ObjectRegistrator.h"

      \brief Class for the automatic creation of an object.

      This class provide the creator function for a class that provides
      a \c Create functions to create itself.

      \author Lukas Nellen
      \date 18 Mar 2004
      \ingroup stl
    */

    template<class ObjectType, class ObjectFactory>
    class StandardCreator {

    public:
        static typename ObjectFactory::CreatorType GetCreator()
        { return ObjectType::Create; }

    };


    /**
       \class ObjectRegistrator ObjectRegistrator.h "utl/ObjectRegistrator.h"

       \brief Class for the automatic registration of an object.

       This class can be used as a mix-in with private inheritance or by
       instantiating it in a (private) member variable.

       The object is registerd in an ObjectFactory of the appropriate type.

       For a class to be registrable, it has to provide the following static
       function:

       * \c GetRegisteredId() to return the identifier under which the
       type is registered in the \c ObjectFactory.

       When using the default \c CPolicy, the class also has to provide
       a static \c Create() function that creates the class with the
       default constructor.

       \author Lukas Nellen
       \date 12 Mar 2004
       \ingroup stl
    */

    template<class ObjectType,
            class ObjFactoryType,
            class CreatorPolicy = StandardCreator<ObjectType, ObjFactoryType> >
    class ObjectRegistrator {

    public:
        /// The type of the ObjectFactory we register with
        typedef ObjFactoryType ObjectFactoryType;
        typedef typename ObjFactoryType::ObjectPtrType ObjectPtrType;

        // The default constructor is required to force the instantiation
        // of the static member variable fgAutoRegistrar whose
        // initialization triggers the registration with the factory
        ObjectRegistrator()
        {
            if (!fgAutoRegistrar)
                fgAutoRegistrar = new ObjectRegistrator(true);
        }

    private:
        // This constructor in needed to avoid recursion during
        // initialization of fgAutoRegistrar. The bool argument is ignored
        explicit ObjectRegistrator(bool) { }

        static
        ObjectRegistrator*
        AutoRegister()
        {
            ObjectFactoryType::Register(ObjectType::GetRegistrationId(),
                                        CreatorPolicy::GetCreator());
            return new ObjectRegistrator(true);
        }

        static const ObjectRegistrator* fgAutoRegistrar;

    }; // ObjectRegistrator


    template<class Object, class ObjectFactory, class CreatorPolicy>
    const ObjectRegistrator<Object, ObjectFactory, CreatorPolicy>*
            ObjectRegistrator<Object, ObjectFactory, CreatorPolicy>::fgAutoRegistrar =
            ObjectRegistrator<Object, ObjectFactory, CreatorPolicy>::AutoRegister();

}


#endif //TPCSOFT_OBJECTREGISTRATOR_H

#ifndef TPCSOFT_OBJECTFACTORY_H
#define TPCSOFT_OBJECTFACTORY_H

/**
  \file
  Object Factory template.

  \author Lukas Nellen
  \version $Id$
  \date 10 Mar 2004
  Modified by Piotr.Podlaski@fuw.edu.pl
  01.2023
*/

#include <map>
#include <memory>
#include <vector>
#include <algorithm>


namespace utl {

    /**
      \class FactoryErrorIgnore ObjectFactory.h "utl/ObjectFactory.h"

      \brief Default error policy for ObjectFactory: return 0

      The policy is defined by the implementation of the static
      function \c Unregistered.

      In alternative inmplementations, this functions could throw an
      exception or try to load a library dynamically.
    */

    template<typename IdentifierType, class ObjectPtrType>
    class FactoryErrorIgnore {

    public:
        static ObjectPtrType Unregistered(const IdentifierType &) { return 0; }

        template<typename ArgumentType>
        static ObjectPtrType Unregistered(const IdentifierType & /*theId*/,
                                          const ArgumentType & /*theArg*/) { return 0; }

        template<typename Argument1Type, typename Argument2Type>
        static ObjectPtrType Unregistered(const IdentifierType & /*theId*/,
                                          const Argument1Type & /*theArg1*/,
                                          const Argument2Type & /*theArg2*/) { return 0; }

    };


    /**
      \class ObjectFactory ObjectFactory.h "utl/ObjectFactory.h"

      \brief Template for object factory

      The implementation relies on static objects to implement a
      mono-state like structure without having to instantiate the factory.

      The template allows for factory functions or functors with 0, 1,
      and 2 arguments. It is trivial to add more creation functions to
      allow for more arguments. It is checked at compile time that the
      CreatorType and the form of the Create call match.

      The design is inspired by the object factory in the book Modern
      C++ Design by Andrei Alexandrescu.

    */

    template<class ObjPtrType,
            typename IdentType,
            typename CreatType = ObjPtrType (*)(),
            class FactoryErrorPolicy = FactoryErrorIgnore<IdentType, ObjPtrType> >
    class ObjectFactory : public FactoryErrorPolicy {

    private:
        typedef std::map<IdentType, CreatType> RegistryType;

    public:
        typedef ObjPtrType ObjectPtrType;
        typedef IdentType IdentifierType;
        typedef CreatType CreatorType;
        typedef typename RegistryType::const_iterator Iterator;

    public:
        ObjectFactory() = default;
        ~ObjectFactory() = default;

        /// Register a factory function with an ID
        static
        bool
        Register(const IdentifierType &theId,
                 const CreatorType &theCreator) {
            if (!fgRegistry)
                fgRegistry = new RegistryType;
            return fgRegistry->insert(std::make_pair(theId, theCreator)).second;
        }

        /// Get number of object types know to this factory
        static unsigned int GetNumberOfCreators() { return fgRegistry ? fgRegistry->size() : 0; }

        /// Create an object (0-argument constructor)

        template<typename DerivedClassType>
        static
        std::unique_ptr<DerivedClassType>
        Create(const IdentifierType &theId) {
            if (!fgRegistry)
            {
                auto p = dynamic_cast<DerivedClassType *>(ObjectFactory::Unregistered(theId));
                return std::unique_ptr<DerivedClassType>(p);
            }

            const Iterator it = fgRegistry->find(theId);
            if (it != fgRegistry->end())
            {
                auto p = dynamic_cast<DerivedClassType *>((it->second)());
                return std::unique_ptr<DerivedClassType>(p);
            }
            else
            {
                auto p = dynamic_cast<DerivedClassType *>(ObjectFactory::Unregistered(theId));
                return std::unique_ptr<DerivedClassType>(p);
            }
        }

        /// Begin iterator over the internal map (read only)
        static
        Iterator
        Begin() {
            if (!fgRegistry)
                fgRegistry = new RegistryType;
            return fgRegistry->begin();
        }

        /// End iterator over the internal map (read only)
        static
        Iterator
        End() {
            if (!fgRegistry)
                fgRegistry = new RegistryType;
            return fgRegistry->end();
        }

        static
        std::vector<IdentType>
        GetRegiseredIdentifiers() {
            if (!fgRegistry)
                fgRegistry = new RegistryType;
            std::vector<IdentType> v;
            std::transform(fgRegistry->begin(), fgRegistry->end(),
                           std::back_inserter(v),
                           [](auto &p) { return p.first; });

            return v;
        }

    private:
        ObjectFactory(const ObjectFactory &theObjectFactory);
        ObjectFactory &operator=(const ObjectFactory &theObjectFactory);

        static RegistryType *fgRegistry;

    }; // ObjectFactory


    // the definition of the registry, which cannot be done in the class defn.
    template<class ObjectPtrType,
            typename IdentifierType,
            typename CreatorType,
            class FactoryErrorPolicy>
    typename ObjectFactory<ObjectPtrType, IdentifierType,
            CreatorType, FactoryErrorPolicy>::RegistryType *
            ObjectFactory<ObjectPtrType, IdentifierType,
                    CreatorType, FactoryErrorPolicy>::fgRegistry = 0;

} // utl


#endif //TPCSOFT_OBJECTFACTORY_H

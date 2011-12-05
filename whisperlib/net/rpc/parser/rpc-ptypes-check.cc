// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Cosmin Tudorache

#include "net/rpc/parser/rpc-ptypes-check.h"
#include <algorithm>
#include "net/rpc/parser/export/rpc-base-types.h"
#include "common/base/types.h"
#include "common/base/log.h"


#define LOG_LOCATION(fileInfo) \
  (fileInfo).filename_.c_str() << ":" << (fileInfo).lineno_ << " "

bool Contains(const vector<string>& array,
              const string& item) {
  return find(array.begin(), array.end(), item) != array.end();
}

bool Contains(const map<string, const PCustomType*>& stringMap,
              const string& name) {
  return stringMap.find(name) != stringMap.end();
}

bool Contains(const Keywords& keywords, const string& item) {
  return keywords.ContainsKeyword(item);
}

bool IsKnownType(const PType& type,
                 const vector<string>  rpcBaseTypes,
                 const map<string, const PCustomType*>& customTypes) {
  if ( type.name_ == RPC_ARRAY ) {
    CHECK_NOT_NULL(type.subtype1_);
    return IsKnownType(*type.subtype1_, rpcBaseTypes, customTypes);
  }
  if ( type.name_ == RPC_MAP ) {
    CHECK_NOT_NULL(type.subtype1_);
    CHECK_NOT_NULL(type.subtype2_);
    return IsKnownType(*type.subtype1_, rpcBaseTypes, customTypes) &&
           IsKnownType(*type.subtype2_, rpcBaseTypes, customTypes);
  }
  return Contains(rpcBaseTypes, type.name_) ||  // RPC base type.
         Contains(customTypes, type.name_);     // previously defined customType
}

bool VerifyPTypesCorrectenss(const PCustomTypeArray customTypes,
                             const PServiceArray services,
                             const Keywords& restrictedKeywords) {
  // make a vector of base types.
  const vector<string>& rpcBaseTypes = rpc::BaseTypeArray();

  bool bErrorFound = false;

  // verify types
  //
  // a list of found types:
  map<string, const PCustomType*> definedCustomTypes;
  for ( PCustomTypeArray::const_iterator it = customTypes.begin();
        it != customTypes.end(); ++it ) {
    const PCustomType& customType = *it;
    const string& customTypeName = customType.name_;

    // check not an rpcBaseType
    //
    if ( Contains(rpcBaseTypes, customTypeName) ) {
      LOG_ERROR << LOG_LOCATION(customType)
                << "You cannot redefine a base type";
      bErrorFound = true;
    }

    // check not a restricted keyword
    //
    if ( Contains(restrictedKeywords, customTypeName) ) {
      LOG_ERROR << LOG_LOCATION(customType)
                << "Type '" << customTypeName << "'"
                << " is a restricted keyword in "
                << restrictedKeywords.LanguageName() << ".";
      bErrorFound = true;
    }

    // check not a duplicate
    //
    map<string, const PCustomType*>::iterator
      itDup = definedCustomTypes.find(customType.name_);
    if ( itDup != definedCustomTypes.end() ) {
      const PCustomType& previousType = *itDup->second;
      LOG_ERROR << LOG_LOCATION(customType)
                << "Duplicate type " << customType.name_
                << " already defined at "
                << LOG_LOCATION(previousType);
      bErrorFound = true;
    }

    // check all attributes are known types
    //
    const PAttributeArray& attributes = customType.attributes_;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      const PType& attributeType = attribute.type_;

      //
      // attributeType must be a base type or a previous defined custom type
      //

      if ( !IsKnownType(attributeType, rpcBaseTypes, definedCustomTypes) ) {
        // undefied attribute type
        LOG_WARNING << LOG_LOCATION(attribute)
                    << "Undefined type " << attributeType;
        //bErrorFound = true;
      }
    }
    definedCustomTypes[customTypeName] = &customType;
  }

  vector<string> foundServiceNames;

  // verify services
  //
  for ( PServiceArray::const_iterator it = services.begin();
        it != services.end(); ++it) {
    const PService& service = *it;
    const string& serviceName = service.name_;
    const PFunctionArray& functions = service.functions_;

    //
    // check service name is not a known type
    //
    if ( Contains(rpcBaseTypes, serviceName) ) {
      LOG_ERROR << LOG_LOCATION(service)
                << "Illegal service name. " << serviceName
                << " is a RPC base type.";
      bErrorFound = true;
    }
    if ( Contains(restrictedKeywords, serviceName) ) {
      LOG_ERROR << LOG_LOCATION(service)
                << "Illegal service name. '" << serviceName << "'"
                << " is a restricted keyword in "
                << restrictedKeywords.LanguageName() << ".";
      bErrorFound = true;
    }

    map<string, const PCustomType*>::iterator
      itDup = definedCustomTypes.find(serviceName);
    if ( itDup != definedCustomTypes.end() ) {
      const PCustomType& previousType = *itDup->second;
      LOG_ERROR << LOG_LOCATION(service)
                << "Illegal service name. " << serviceName
                << " is already defined as typename at "
                << LOG_LOCATION(previousType);
      bErrorFound = true;
    }

    // check service name is not duplicate
    //
    PServiceArray::const_iterator itPreviousSameNameService = services.begin();
    for ( ; itPreviousSameNameService != it &&
            itPreviousSameNameService->name_ != serviceName;
          ++itPreviousSameNameService) {
    }
    if ( itPreviousSameNameService != it ) {
      LOG_ERROR << LOG_LOCATION(service)
                << "Duplicate service name \"" << serviceName
                << "\". First defined at: "
                << LOG_LOCATION(*itPreviousSameNameService);
      bErrorFound = true;
    }

    // check functions
    //
    for ( PFunctionArray::const_iterator it = functions.begin();
          it != functions.end(); ++it ) {
      const PFunction& function = *it;
      const string& functionName = function.name_;
      const PParamArray& args = function.input_;
      const PType& ret = function.output_;

      //
      // check function name is not a known type
      //
      if ( Contains(rpcBaseTypes, functionName) ) {
        LOG_ERROR << LOG_LOCATION(function)
                  << "Illegal function name. " << functionName
                  << " is a RPC base type.";
        bErrorFound = true;
      }
      if ( Contains(restrictedKeywords, functionName) ) {
        LOG_ERROR << LOG_LOCATION(function)
                  << "Illegal function name. '" << functionName << "'"
                  << " is a restricted keyword in "
                  << restrictedKeywords.LanguageName() << ".";
        bErrorFound = true;
      }
      map<string, const PCustomType*>::iterator
        itDup = definedCustomTypes.find(functionName);
      if ( itDup != definedCustomTypes.end() ) {
        const PCustomType& previousType = *itDup->second;
        LOG_ERROR << LOG_LOCATION(function)
                  << "Illegal function name. " << functionName
                  << " is already defined as typename at "
                  << LOG_LOCATION(previousType);
      }

      // check function name is not duplicate
      //
      PFunctionArray::const_iterator
        itPreviousSameNameFunction = functions.begin();
      for ( ; itPreviousSameNameFunction != it &&
              itPreviousSameNameFunction->name_ != functionName;
            ++itPreviousSameNameFunction) {
      }
      if ( itPreviousSameNameFunction != it ) {
        LOG_ERROR << LOG_LOCATION(function)
                  << "Duplicate function name \"" << functionName
                  << "\". First defined at: "
                  << LOG_LOCATION(*itPreviousSameNameFunction);
        bErrorFound = true;
      }

      // check function arguments
      //
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ++it ) {
        // every argument should be a known type
        //
        const PParam& arg = *it;
        if ( !IsKnownType(arg.type_, rpcBaseTypes, definedCustomTypes) ) {
          // undefined type
          LOG_WARNING << LOG_LOCATION(function)
                      << "Undefined type " << arg.type_;
          //bErrorFound = true;
        }

        //
        // check argument name is not identical with a type
        //
        if ( Contains(rpcBaseTypes, arg.name_) ) {
          LOG_ERROR << LOG_LOCATION(function)
                    << "Illegal argument name. " << arg.name_
                    << " is a RPC base type.";
          bErrorFound = true;
        }
        if ( Contains(restrictedKeywords, arg.name_) ) {
          LOG_ERROR << LOG_LOCATION(function)
                    << "Illegal argument name. '" << arg.name_ << "'"
                    << " is a restricted keyword in "
                    << restrictedKeywords.LanguageName() << ".";
          bErrorFound = true;
        }
        map<string, const PCustomType*>::iterator
          itDup = definedCustomTypes.find(arg.name_);
        if ( itDup != definedCustomTypes.end() ) {
          const PCustomType& previousType = *itDup->second;
          LOG_ERROR << LOG_LOCATION(function)
                    << "Illegal argument name. " << arg.name_
                    << " is already defined as typename at "
                    << LOG_LOCATION(previousType);
        }

        // check argument name is not duplicate
        //
        PParamArray::const_iterator itPreviousSameNameArgument = args.begin();
        for ( ; itPreviousSameNameArgument != it &&
                  itPreviousSameNameArgument->name_ != arg.name_;
              ++itPreviousSameNameArgument) {
        }
        if ( itPreviousSameNameArgument != it ) {
          LOG_ERROR << LOG_LOCATION(function)
                    << "Duplicate argument name \"" << arg.name_
                    << "\".";
          bErrorFound = true;
        }
      }

      // check function return is a known type
      //
      if ( !IsKnownType(ret, rpcBaseTypes, definedCustomTypes) ) {
        // undefied type
        LOG_WARNING << LOG_LOCATION(function) << "Undefined type " << ret;
        // bErrorFound = true;
      }
    }
  }

  return !bErrorFound;
}

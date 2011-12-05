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

#ifndef __NET_RPC_PARSER_RPC_PTYPES_H__
#define __NET_RPC_PARSER_RPC_PTYPES_H__

#include <string>
#include <vector>
#include <list>
#include <iostream>

#include <whisperlib/common/base/types.h>
struct PFileInfo {
  string filename_;
  unsigned lineno_;
};

// A type name = a tree of simple types. Every simple type is a string.
//
// e.g. "int" -> "int"
//              |     |
//             NULL  NULL
//
//      "array<T>"  ->    "array"
//                         |   |
//                         T  NULL
//
//      "map<K,V>"  ->     "map"
//                         |   |
//                         K   V
//
//      "array<map<float, array<int>>>"  ->      "array"
//                                               |     |
//                                             "map"   NULL
//                                             |       |
//                                         "float"  "array"
//                                          |   |    |     |
//                                       NULL  NULL "int"  NULL
//                                                  |   |
//                                                NULL  NULL
//
struct PType {
  string name_;
  PType* parent_;    // parent branh
  PType* subtype1_;  // left branch
  PType* subtype2_;  // right branch

  // if typename_ == "map" then [subtype1_, subtype2_] is the mapped pair
  // if typename_ == "array" then [subtype1_] is the element type.
  //                             (and subtype2_ should be NULL)
  // else both subtype1_ and subtype2_ should be NULL

  PType()
      : name_(),
        parent_(NULL),
        subtype1_(NULL),
        subtype2_(NULL) {
  }
  PType(const PType& t)
      : name_(),
        parent_(NULL),
        subtype1_(NULL),
        subtype2_(NULL) {
    *this = t;
  }
  virtual ~PType() {
    Clear();
  }
  PType& operator=(const PType& t) {
    name_ = t.name_;
    if ( t.subtype1_ ) {
      delete subtype1_;
      subtype1_ = new PType();
      subtype1_->parent_ = this;
      *subtype1_ = *(t.subtype1_);
    }
    if ( t.subtype2_ ) {
      delete subtype2_;
      subtype2_ = new PType();
      subtype2_->parent_ = this;
      *subtype2_ = *(t.subtype2_);
    }
    return *this;
  }
  string ToString() const;
  void Clear() {
    name_.clear();
    parent_ = NULL;
    delete subtype1_;
    subtype1_ = NULL;
    delete subtype2_;
    subtype2_ = NULL;
  }
};

ostream& operator<<(ostream& out, const PType& p);
typedef vector<PType> PTypeArray;

// A custom type definition.
struct PCustomType : public PFileInfo {
  struct Attribute : public PFileInfo {
    PType type_;        // attribute's type name
    string name_;       // attribute's name
    bool isOptional_;   // true = OPTIONAL, false = REQUIRED

    Attribute()
        : type_(),
          name_(),
          isOptional_(false) {
    }
    void Clear() {
      type_.Clear();
      name_.clear();
      isOptional_ = false;
    }
  };
  typedef vector<Attribute> AttributeArray;

  string name_;                 // the name of this custom type
  AttributeArray attributes_;   // attributes of this custom type

  PCustomType()
      : name_() {
  }
  void Clear() {
    name_.clear();
    attributes_.clear();
  }
};
typedef PCustomType::Attribute PAttribute;
typedef PCustomType::AttributeArray PAttributeArray;
typedef list<PCustomType> PCustomTypeArray;

// a function parameter
struct PParam {
  PType type_;
  string name_;

  PParam()
      : type_(),
        name_() {
  }
  void Clear() {
    type_.Clear();
    name_.clear();
  }
};
typedef vector<PParam> PParamArray;

// a function definition
struct PFunction : public PFileInfo {
  string name_;                // function name
  PParamArray input_;          // input parameters types and names
  PType output_;               // output type name

  void Clear() {
    name_.clear();
    input_.clear();
    output_.Clear();
  }
};
typedef vector<PFunction> PFunctionArray;

// a service definition
struct PService : public PFileInfo {
  string name_;                 // service name
  PFunctionArray functions_;    // service functions

  void Clear() {
    name_.clear();
    functions_.clear();
  }
};
typedef list<PService> PServiceArray;


// some verbatim text
struct PVerbatim : public PFileInfo {
  string language_;
  string verbatim_;
  void Clear() {
    language_.clear();
    verbatim_.clear();
  }
};
typedef list<PVerbatim> PVerbatimArray;

#endif   // __NET_RPC_PARSER_RPC_PTYPES_H__

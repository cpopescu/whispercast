#ifndef TESTRPCCOMMON_H_
#define TESTRPCCOMMON_H_

#include "common/base/types.h"
#include "net/rpc/lib/types/rpc_all_types.h"

//#ifdef OUT_OF_COMPILE
/*********************************************************************/
/*               Data types that can be exchanged on RPC             */
/*********************************************************************/

class Person : public rpc::Custom
{
public:
  Attribute<rpc::String> name_;  
  Attribute<rpc::Float>  height_;
  Attribute<rpc::Int>  age_;
  Attribute<rpc::Bool>   married_;
  
public:
  //static inline RPC_OBJECT_TYPE Type() { return 19001; } // ID for this type 
  static inline const char *    Name() { return "Person"; }// the name is also the ID
  
public:
  Person();
  Person(rpc::String _name_, rpc::Float _height_, rpc::Int _age_, rpc::Bool _married_);
  Person(const Person & p);
  virtual ~Person();
    
  void Fill(rpc::String _name_, rpc::Float _height_, rpc::Int _age_, rpc::Bool _married_);
  void FillRef(const rpc::String & _name_, const rpc::Float & _height_, const rpc::Int & _age_, const rpc::Bool & _married_);
  
  static string GetHTMLFormFunction();
  
public:
  /*************************************************************/
  /*                         operators                         */
  /*************************************************************/
  Person & operator=(const Person & p);
  bool operator==(const Person & p) const;

public:
  /*************************************************************/
  /*                      Serialization                        */
  /*************************************************************/
  /**
   *  Load data using the given decoder.
   *  The decoder is already linked with the input stream.
   */
  rpc::DECODE_RESULT SerializeLoad(rpc::Decoder & in);
  
  /**
   *  Save data using the given encoder.
   *  The encoder is already linked with the output stream.
   */
  void SerializeSave(rpc::Encoder & out) const;

  /***********************************************************************/
  /*                     rpc::Custom interface methods                   */
  /***********************************************************************/
  const char * GetName() const;

  /***********************************************************************/
  /*                     rpc::Object interface methods                   */
  /***********************************************************************/
  rpc::Object * Clone() const;

  /****************************************************************/
  /*             rpc::Loggable interface methods                  */
  /****************************************************************/
  std::string ToString() const;
};

//typedef rpc::SimpleTypeFactoryStd<Person> rpc::TypeFactoryPerson;

//#endif

//#ifdef OUT_OF_COMPILE

class Family : public rpc::Custom
{
public:
  Attribute<Person> father_;
  Attribute<Person> mother_;
  Attribute<rpc::Array<Person> > children_;
  
public:
  //static inline RPC_OBJECT_TYPE Type() { return 19002; } // Family type ID
  static inline const char *    Name() { return "Family"; }// the name is also the ID
  
public:
  Family();
  Family(Person _father_, Person _mother_, rpc::Array< Person > _children_);
  Family(const Family & p);
  virtual ~Family();
    
  void Fill(Person _father_, Person _mother_, rpc::Array< Person > _children_);
  void FillRef(const Person & _father_, const Person & _mother_, const rpc::Array< Person > & _children_);
  
  static string GetHTMLFormFunction();
  
  /*************************************************************/
  /*                         operators                         */
  /*************************************************************/
  Family & operator=(const Family & p);
  bool operator==(const Family & p) const;
  
public:
  /*************************************************************/
  /*                      Serialization                        */
  /*************************************************************/
  /**
   *  Load data using the given decoder.
   *  The decoder is already linked with the input stream.
   */
  rpc::DECODE_RESULT SerializeLoad(rpc::Decoder & in);
  
  /**
   *  Save data using the given encoder.
   *  The encoder is already linked with the output stream.
   */
  void SerializeSave(rpc::Encoder & out) const;
  
  /***********************************************************************/
  /*                     rpc::Custom interface methods                   */
  /***********************************************************************/
  const char * GetName() const;
  
  /***********************************************************************/
  /*                     rpc::Object interface methods                   */
  /***********************************************************************/
  rpc::Object * Clone() const;
  
  /****************************************************************/
  /*             rpc::Loggable interface methods                  */
  /****************************************************************/
  std::string ToString() const;
};

//typedef rpc::SimpleTypeFactoryStd<Family> rpc::TypeFactoryFamily;

//#endif

#endif /*TESTRPCCOMMON_H_*/


#include "common/base/errno.h"
#include "common/base/log.h"
#include "net/rpc/lib/codec/rpc_encoder.h"
#include "net/rpc/lib/codec/rpc_decoder.h"

#include "types.h"

/******************************************************************************/
/*                                 Person                                     */
/******************************************************************************/

Person::Person()
    : rpc::Custom(),
      name_(),
      height_(),
      age_(),
      married_()
{
}
Person::Person(rpc::String _name_, rpc::Float _height_, rpc::Int _age_, rpc::Bool _married_)
    : rpc::Custom(),
      name_(_name_),
      height_(_height_),
      age_(_age_),
      married_(_married_)
{
}
Person::Person(const Person & p)
    : rpc::Custom(),
      name_(p.name_),
      height_(p.height_),
      age_(p.age_),
      married_(p.married_)
{
}
Person::~Person()
{
}

void Person::Fill(rpc::String _name_, rpc::Float _height_, rpc::Int _age_, rpc::Bool _married_)
{
  name_ = _name_;
  height_ = _height_;
  age_ = _age_;
  married_ = _married_;
}
void Person::FillRef(const rpc::String & _name_, const rpc::Float & _height_, const rpc::Int & _age_, const rpc::Bool & _married_)
{
  name_ = _name_;
  height_ = _height_;
  age_ = _age_;
  married_ = _married_;
}
string  Person::GetHTMLFormFunction() {
  string s;
  s += "function add_Person_control(label, name, is_optional, f, elem) {\n";
  s += " if ( is_optional ) {\n";
  s += "   add_optional_element(f, elem);\n";
  s += " }\n";
  s += " var ul = document.createElement('ul');\n";
  s += " add_begin_struct(name, ul, null);\n";
  s += " if ( label != '' ) {\n";
  s += "   ul.appendChild(document.createTextNode(label + ': '));\n";
  s += " }\n";
  s += "  add_text_control('name_', 'name_', false, ul, null);\n";
  s += "  add_number_control('height_', 'height_', true, ul, null);\n";
  s += "  add_number_control('age_', 'age_', false, ul, null);\n";
  s += "  add_boolean_control('married_', 'married_', true, ul, null);\n";
  s += "  add_end_struct(name, ul, null);\n";
  s += "  ul.appendChild(document.createElement('hr'));\n";
  s += "  if ( elem == null ) {\n";
  s += "    f.appendChild(ul);\n";
  s += "  } else {\n";
  s += "    f.insertBefore(ul, elem)\n";
  s += "  }\n";
  s += "}\n";
  return s;
}

/*************************************************************/
/*                         operators                         */
/*************************************************************/
Person & Person::operator=(const Person & p)
{
  name_ = p.name_;
  height_ = p.height_;
  age_ = p.age_;
  married_ = p.married_;
  return *this;
}
bool Person::operator==(const Person & p) const
{
  return name_ == p.name_ &&
         height_ == p.height_ &&
         age_ == p.age_ &&
         married_ == p.married_;
}

/*************************************************************/
/*                      Serialization                        */
/*************************************************************/
/**
 *  Load data using the given decoder.
 *  The decoder is already linked with the input stream.
 */
rpc::DECODE_RESULT Person::SerializeLoad(rpc::Decoder & in)
{
  // unset all fields
  //
  name_.Reset();
  height_.Reset();
  age_.Reset();
  married_.Reset();
  
  // read the number of serialized fields
  //
  uint32 nFields;
  rpc::DECODE_RESULT result = in.DecodeStructStart(nFields);
  if(result != rpc::DECODE_RESULT_SUCCESS)
  {
    return result; // bad or not enough data
  }
  
  uint32 i = 0;
  bool hasMoreFields = false;
  for(i = 0; true; i++)
  {
    DECODE_VERIFY(in.DecodeStructContinue(hasMoreFields));
    if(!hasMoreFields)
    {
      break;
    }
    
    DECODE_VERIFY(in.DecodeStructAttribStart());
    
    // read field identifier
    //
    rpc::String fieldname;
    result = in.Decode(fieldname);
    if(result != rpc::DECODE_RESULT_SUCCESS)
    {
      return result; // bad or not enough data
    }
    LOG_DEBUG << "found field: " << fieldname;
    
    DECODE_VERIFY(in.DecodeStructAttribMiddle());
    
    // read field size in stream (useful if you want to skip over field)
    //
    /*
    rpc::Int fieldsize;
    result = in.Decode(fieldsize);
    if(result != rpc::DECODE_RESULT_SUCCESS)
    {
      return result; // bad or not enough data
    }
    */
    
    if(fieldname == "name")
    {
      if(name_.IsSet())
      {
        LOG_ERROR << "Duplicate field \"name\".";
        return rpc::DECODE_RESULT_ERROR;
      }
      result = in.Decode(name_.Ref());
      if(result != rpc::DECODE_RESULT_SUCCESS)
      {
        LOG_ERROR << "Failed to decode field \"name\", result = " << result;
        return result;
      }
    }
    else if (fieldname == "height")
    {
      if(height_.IsSet())
      {
        LOG_ERROR << "Duplicate field \"height\".";
        return rpc::DECODE_RESULT_ERROR;
      }
      result = in.Decode(height_.Ref());
      if(result != rpc::DECODE_RESULT_SUCCESS)
      {
        LOG_ERROR << "Failed to decode field \"height\", result = " << result;
        return result;
      }
    }
    else if (fieldname == "age")
    {
      if(age_.IsSet())
      {
        LOG_ERROR << "Duplicate field \"age\".";
        return rpc::DECODE_RESULT_ERROR;
      }
      result = in.Decode(age_.Ref());
      if(result != rpc::DECODE_RESULT_SUCCESS)
      {
        LOG_ERROR << "Failed to decode field \"age\", result = " << result;
        return result;
      }
    }
    else if (fieldname == "married")
    {
      if(married_.IsSet())
      {
        LOG_ERROR << "Duplicate field \"married\".";
        return rpc::DECODE_RESULT_ERROR;
      }
      result = in.Decode(married_.Ref());
      if(result != rpc::DECODE_RESULT_SUCCESS)
      {
        LOG_ERROR << "Failed to decode field \"married\", result = " << result;
        return result;
      }
    }
    else
    {
      LOG_ERROR << "Unknown field name: '" << fieldname << "', at position " << i << ".";
      //in.SkipBytes(fieldsize);
      return rpc::DECODE_RESULT_ERROR;
    };
    
    DECODE_VERIFY(in.DecodeStructAttribEnd());
  }
  // bug-trap
  if(nFields != 0xffffffff)
  {
    CHECK_EQ(nFields, i);
  }
  
  // verify required fields
  //
  if(!name_.IsSet())
  {
    LOG_ERROR << "Deserialization cannot find required field \"name\"";
    return rpc::DECODE_RESULT_ERROR;
  }
  if(!age_.IsSet())
  {
    LOG_ERROR << "Deserialization cannot find required field \"age\"";
    return rpc::DECODE_RESULT_ERROR;
  }
  
  // success
  return rpc::DECODE_RESULT_SUCCESS;
}

/**
  * returns: success status.
  */
void Person::SerializeSave(rpc::Encoder & out) const
{
  // verify required fields
  //
  CHECK(name_.IsSet()) << "Unset required field \"name\".";
  CHECK(age_.IsSet()) << "Unset required field \"age\".";
  
  // proceed to serialization
  uint32 fields = 2 + (height_.IsSet()  ? 1 : 0) +  // 2 = required fields which were already verified
                      (married_.IsSet() ? 1 : 0);
  out.EncodeStructStart(fields);
  
  uint32 i = 0;
  //if(name_.IsSet()) //REQUIRED field existence already verified
  {
    out.EncodeStructAttribStart();
    out.Encode(rpc::String("name"));
    out.EncodeStructAttribMiddle();
    //out.Encode(rpc::Int(out.EstimateEncodingSize(name_.Get())));
    out.Encode(name_.Get());
    out.EncodeStructAttribEnd();
    if(++i < fields) { out.EncodeStructContinue(); }
  }
  if(height_.IsSet())
  {
    out.EncodeStructAttribStart();
    out.Encode(rpc::String("height"));
    out.EncodeStructAttribMiddle();
    //out.Encode(rpc::Int(out.EstimateEncodingSize(height_.Get())));
    out.Encode(height_.Get());
    out.EncodeStructAttribEnd();
    if(++i < fields) { out.EncodeStructContinue(); }
  }
  //if(age_.IsSet()) //REQUIRED field existence already verified
  {
    out.EncodeStructAttribStart();
    out.Encode(rpc::String("age"));
    out.EncodeStructAttribMiddle();
    //out.Encode(rpc::Int(out.EstimateEncodingSize(age_.Get())));
    out.Encode(age_.Get());
    out.EncodeStructAttribEnd();
    if(++i < fields) { out.EncodeStructContinue(); }
  }
  if(married_.IsSet())
  {
    out.EncodeStructAttribStart();
    out.Encode(rpc::String("married"));
    out.EncodeStructAttribMiddle();
    //out.Encode(rpc::Int(out.EstimateEncodingSize(married_.Get())));
    out.Encode(married_.Get());
    out.EncodeStructAttribEnd();
    if(++i < fields) { out.EncodeStructContinue(); }
  }
  out.EncodeStructEnd();
}
/***********************************************************************/
/*                     rpc::Custom interface methods                   */
/***********************************************************************/
const char * Person::GetName() const
{
  return Name();
}

/***********************************************************************/
/*                     rpc::Object interface methods                   */
/***********************************************************************/
rpc::Object * Person::Clone() const
{
  return new Person(*this);
}

/***********************************************************************/
/*                     Loggable interface methods                      */
/***********************************************************************/
std::string Person::ToString() const
{
  std::ostringstream oss;
  oss << "Person{" //<< std::endl
      << "name=" << name_ << ", " //std::endl
      << "height=" << height_ << ", " //std::endl
      << "age=" << age_ << ", " //std::endl
      << "married=" << married_ << "}";
  return oss.str();
}

/******************************************************************************/
/*                                   Family                                   */
/******************************************************************************/

Family::Family()
    : rpc::Custom(),
      father_(),
      mother_(),
      children_()
{
}
Family::Family(Person _father_, Person _mother_, rpc::Array< Person > _children_)
    : rpc::Custom(),
      father_(_father_),
      mother_(_mother_),
      children_(_children_)
{
}
Family::Family(const Family & p)
    : rpc::Custom(),
      father_(p.father_),
      mother_(p.mother_),
      children_(p.children_)
{
}
Family::~Family()
{
}

void Family::Fill(Person _father_, Person _mother_, rpc::Array< Person > _children_)
{
  father_ = _father_;
  mother_ = _mother_;
  children_ = _children_;
}
void Family::FillRef(const Person & _father_, const Person & _mother_, const rpc::Array< Person > & _children_)
{
  father_ = _father_;
  mother_ = _mother_;
  children_ = _children_;
}

string  Family::GetHTMLFormFunction() {
  string s;  
  s += Person::GetHTMLFormFunction();
  s += "\n";
  s += "function add_Family_control(label, name, is_optional, f, elem) {\n";
  s += " if ( is_optional ) {\n";
  s += "   add_optional_element(f, elem);\n";
  s += " }\n";
  s += " var ul = document.createElement('ul');\n";
  s += " add_begin_struct(name, ul, null);\n";
  s += " if ( label != '' ) {\n";
  s += "   ul.appendChild(document.createTextNode(label + ': '));\n";
  s += " }\n";
  s += "  add_Person_control('father_', 'father_', false, ul, null);\n";
  s += "  add_Person_control('mother_', 'mother_', false, ul, null);\n";
  s += "  add_array_control('children_', 'children_', true, ul, null, [ 'Person' ]);\n";
  s += "  add_end_struct(name, ul, null);\n";
  s += "  ul.appendChild(document.createElement('hr'));\n";
  s += "  if ( elem == null ) {\n";
  s += "    f.appendChild(ul);\n";
  s += "  } else {\n";
  s += "    f.insertBefore(ul, elem)\n";
  s += "  }\n";
  s += "}\n";
  return s;
}

/*************************************************************/
/*                         operators                         */
/*************************************************************/
Family & Family::operator=(const Family & p)
{
  father_ = p.father_;
  mother_ = p.mother_;
  children_ = p.children_;
  return *this;
}
bool Family::operator==(const Family & f) const
{    
  return father_ == f.father_ &&
         mother_ == f.mother_ &&
         children_ == f.children_;
  
}

/*************************************************************/
/*                      Serialization                        */
/*************************************************************/
rpc::DECODE_RESULT Family::SerializeLoad(rpc::Decoder & in)
{
  // unset all fields
  //
  mother_.Reset();
  father_.Reset();
  children_.Reset();
  
  // read the number of serialized fields
  //
  uint32 fields;
  rpc::DECODE_RESULT result = in.DecodeStructStart(fields);
  if(result != rpc::DECODE_RESULT_SUCCESS)
  {
    return result; // bad or not enough data
  }
  
  uint32 i = 0;
  bool hasMoreFields = false;
  for(i = 0; true; i++)
  {
    DECODE_VERIFY(in.DecodeStructContinue(hasMoreFields));
    if(!hasMoreFields)
    {
      break;
    }
    
    DECODE_VERIFY(in.DecodeStructAttribStart());
    
    // read field identifier
    //
    rpc::String fieldname;
    result = in.Decode(fieldname);
    if(result != rpc::DECODE_RESULT_SUCCESS)
    {
      return result; // bad or not enough data
    }
    
    DECODE_VERIFY(in.DecodeStructAttribMiddle());
    
    // read field size in stream (useful if you want to skip over field)
    //
    /*
    rpc::Int fieldsize;
    result = in.Decode(fieldsize);
    if(result != rpc::DECODE_RESULT_SUCCESS)
    {
      return result; // bad or not enough data
    }
    */
    
    if (fieldname == "father")
    {
      if(father_.IsSet())
      {
        LOG_ERROR << "Duplicate field \"father\".";
        return rpc::DECODE_RESULT_ERROR;
      }
      result = in.Decode(father_.Ref());
      if(result != rpc::DECODE_RESULT_SUCCESS)
      {
        LOG_ERROR << "Failed to decode field \"father\", result = " << result;
        return result;
      }
    }
    else if (fieldname == "mother")
    {
      if(mother_.IsSet())
      {
        LOG_ERROR << "Duplicate field \"mother\".";
        return rpc::DECODE_RESULT_ERROR;
      }
      result = in.Decode(mother_.Ref());
      if(result != rpc::DECODE_RESULT_SUCCESS)
      {
        LOG_ERROR << "Failed to decode field \"mother\", result = " << result;
        return result;
      }
    }
    else if (fieldname == "children")
    {
      if(children_.IsSet())
      {
        LOG_ERROR << "Duplicate field \"children\".";
        return rpc::DECODE_RESULT_ERROR;
      }
      result = in.Decode(children_.Ref());
      if(result != rpc::DECODE_RESULT_SUCCESS)
      {
        LOG_ERROR << "Failed to decode field \"children\", result = " << result;
        return result;
      }
    }
    else 
    {
      LOG_ERROR << "Unknown field name: '" << fieldname << "', at position " << i << ".";
      //in.SkipBytes(fieldsize);
      return rpc::DECODE_RESULT_ERROR;
    };
    
    DECODE_VERIFY(in.DecodeStructAttribEnd());
  }
  if(fields != 0xffffffff)
  {
    CHECK_EQ(fields, i);
  }
  
  // verify required fields
  //
  if(!father_.IsSet())
  {
    LOG_ERROR << "Deserialization cannot find required field \"father\"";
    return rpc::DECODE_RESULT_ERROR;
  }
  if(!mother_.IsSet())
  {
    LOG_ERROR << "Deserialization cannot find required field \"mother\"";
    return rpc::DECODE_RESULT_ERROR;
  }
  
  // success
  return rpc::DECODE_RESULT_SUCCESS;
}

void Family::SerializeSave(rpc::Encoder & out) const
{
  // verify required fields are present
  //
  CHECK(father_.IsSet()) << "Required field unset: \"father\"";
  CHECK(mother_.IsSet()) << "Required field unset: \"mother\"";
  
  // proceed to serialization
  //
  
  // write the number of fields in this structure
  const uint32 fields = 2 + (children_.IsSet() ? 1 : 0);
  out.EncodeStructStart(fields);
  
  uint32 i = 0;
  //if(father_.IsSet())
  {
    out.EncodeStructAttribStart();
    out.Encode(rpc::String("father"));
    out.EncodeStructAttribMiddle();
    //out.Encode(rpc::Int(out.EstimateEncodingSize(father_.Get())));
    out.Encode(father_.Get());
    out.EncodeStructAttribEnd();
    if(++i < fields) { out.EncodeStructContinue(); }
  }
  //if(mother_.IsSet())
  {
    out.EncodeStructAttribStart();
    out.Encode(rpc::String("mother"));
    out.EncodeStructAttribMiddle();
    //out.Encode(rpc::Int(out.EstimateEncodingSize(mother_.Get())));
    out.Encode(mother_.Get());
    if(++i < fields) { out.EncodeStructContinue(); }
  }
  if(children_.IsSet())
  {
    out.EncodeStructAttribStart();
    out.Encode(rpc::String("children"));
    out.EncodeStructAttribMiddle();
    //out.Encode(rpc::Int(out.EstimateEncodingSize(children_.Get())));
    out.Encode(children_.Get());
    if(++i < fields) { out.EncodeStructContinue(); }
  }
  out.EncodeStructEnd();
}

/***********************************************************************/
/*                     rpc::Custom interface methods                   */
/***********************************************************************/
const char * Family::GetName() const
{
  return Name();
}

/***********************************************************************/
/*                     rpc::Object interface methods                   */
/***********************************************************************/
rpc::Object * Family::Clone() const
{
  return new Family(*this);
}

/***********************************************************************/
/*                     Loggable interface methods                      */
/***********************************************************************/
std::string Family::ToString() const
{
  std::ostringstream oss;
  oss << "Family{" //<< std::endl
      << "father=" << father_ << ", " //std::endl
      << "mother=" << mother_ << ", " //std::endl
      << "children=" << children_ << "}";
  return oss.str();
}

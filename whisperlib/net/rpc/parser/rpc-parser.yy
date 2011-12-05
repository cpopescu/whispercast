
   /**********************************************************************/
   /**********************************************************************/
   /*                           DEFINITIONS                              */
   /**********************************************************************/
   /**********************************************************************/

%{
#define RPC_PARSER_YY_INCLUDED

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include "net/rpc/parser/rpc-parser.h"
#include "net/rpc/parser/rpc-ptypes.h"
#include "net/rpc/parser/export/rpc-base-types.h"
#include "common/base/log.h"
%}

%option noyywrap
%option yylineno
%option yyclass="rpc::Parser"

 // The same definition order must be used in function StateName(..)
%x S_COMMENT
%x S_SERVICE_HEAD
%x S_SERVICE_BODY
%x S_FUNCTION_RETURN
%x S_FUNCTION_NAME
%x S_FUNCTION_PARAM_TYPE
%x S_FUNCTION_PARAM_NAME
%x S_CUSTOMTYPE_HEAD
%x S_CUSTOMTYPE_BODY
%x S_CUSTOMTYPE_ATTR
%x S_CUSTOMTYPE_ATTR_CODE
%x S_TYPE
%x S_VERBATIM_HEAD
%x S_VERBATIM_BODY

 // ! All C/C++ code must be indented by at least 1 space
 // or enclosed between %{}
%{

// Another name for start condition INITIAL
#define S_INITIAL 0 // INITIAL == 0

// Returns: name of the given start condition.
const char * StateName(int sc);

// Change start condition. This macro does not alter execution (like REJECT does).
// Current state is lost; the lexer goes in the newState state.
#define ChangeState(newState) {\
  PRINT_INFO << "StateChange: " << StateName(YY_START) << " -> " << StateName(newState) << " at line " << lineno() << PRINT_END;\
  BEGIN(newState);\
}

// Change start condition. This macro does not alter execution (like REJECT does).
// Current state is saved; the lexer goes to newState state; the old state can be restored with PopState.
#define PushState(newState) {\
  PRINT_INFO << "StateChange: " << StateName(YY_START) << " -> " << StateName(newState) << " at line " << lineno() << PRINT_END;\
  yy_push_state(newState); /* save current state and BEGIN(newState) */\
}

// Change back start condition to the one before the last PushState call.
#define PopState() {\
  int old_sc = YY_START;\
  yy_pop_state(); /* restore last saved state */\
  int new_sc = YY_START;\
  PRINT_INFO << "StateChange: " << StateName(old_sc) << " -> " << StateName(new_sc) << " at line " << lineno() << PRINT_END;\
}

#define SET_CRT_LOCATION(pfileinfo) { pfileinfo.filename_ = strFilename_; pfileinfo.lineno_ = lineno(); }

// parser log. This log uses lexer's current state information.
//
#define PRINT_ERROR   bErrorsFound_ = true; std::cerr << strFilename_ << ":" << lineno() << " error: "
#define PRINT_WARNING std::cerr << strFilename_ << ":" << lineno() << " warning: "
#define PRINT_INFO    if(!bPrintInfo_); else std::cout << strFilename_ << ":" << lineno() << " info: "
#define PRINT_END     std::endl;


%}

DIGIT    [0-9]
ALPHA    [a-zA-Z_]
NAME     [a-zA-Z_][a-zA-Z_0-9]*
INTEGER  {DIGIT}+
WS       [ \t]+
TOKEN    [^ \t\n]+

 //SIMPLE_TYPENAME NAME
 //ARRAY_TYPENAME  "array<"{NAME}">"
 //MAP_TYPENAME    "map<"{NAME}

 //TYPENAME  SIMPLE_TYPENAME | ARRAY_TYPENAME | MAP_TYPENAME

    /********************************************************************/
    /********************************************************************/
%%  /*                            RULES                                 */
    /********************************************************************/
    /********************************************************************/

 // ! All C/C++ code must be indented by at least 1 space or enclosed between %{}
 //   All code in this section is local to the yylex() routine and executed
 //   every time yylex() is called.

 PService crtService;           // the current service.
 PFunction crtFunction;         // the current function.
 PParam crtParam;               // the current function parameter.
 PCustomType crtCustomType;     // the current custom type.
 PAttribute crtAttribute;       // the current custom type's attribute.
 bool crtAttributeRequirementSet = false; // true is "required" or "optional" was found. False otherwise.
 PType crtType;                 // the current type.
 PVerbatim crtVerbatim;         // current verbatim stuff
 int verbatim_brackets = 0;

 // the current subtype (of the current type). Points to a node under crtType tree.
 // Used only by S_TYPE.
 PType * pcrtSubtype = &crtType;

 /*************************************/
 /*          verbatim            */
 /*************************************/
<S_VERBATIM_HEAD>{NAME} {
  if (crtVerbatim.language_ != "") {
    REJECT;
  }
  crtVerbatim.language_ = yytext;
}
<S_VERBATIM_HEAD>"{" {
  PopState();
  PushState(S_VERBATIM_BODY);
  verbatim_brackets = 1;
}

<S_VERBATIM_BODY>"{" {
  crtVerbatim.verbatim_ += "{";
  verbatim_brackets++;
}
<S_VERBATIM_BODY>"}" {
  verbatim_brackets--;
  if ( verbatim_brackets <= 0 ) {
    verbatim_.push_back(crtVerbatim);
    crtVerbatim.Clear();
    PopState();
  } else {
    crtVerbatim.verbatim_ += "}";
  }
}
<S_VERBATIM_BODY>"\n" {
  crtVerbatim.verbatim_  += "\n";
}
<S_VERBATIM_BODY>. {
  crtVerbatim.verbatim_ += yytext;
}
<S_VERBATIM_BODY>{NAME} {
  crtVerbatim.verbatim_ += yytext;
}


<*>"/""*"+             PushState(S_COMMENT); // save current state and BEGIN(S_COMMENT)
<S_COMMENT>[^*]*       /* eat anything that's not a '*' */
<S_COMMENT>"*"+[^/]    /* eat up '*'s not followed by '/'s */
<S_COMMENT>"*"+"/"     PopState(); // restore last saved state

<*>"//".*              { /* eat up one-line comments, without the \n terminator */
  //yytext[yyleng-1]=0; // cut newline terminator
  //PRINT_INFO << "one line comment, at line " << lieno() << ": \"" << yytext << "\"" << PRINT_END;
}

<*>{WS}                 /* eat up whitespace */

<*>"*/"                 PRINT_WARNING << "Bad Comment Terminator" << PRINT_END;


 /*************************************/
 /*             initial               */
 /*************************************/
<INITIAL>"Service" {
  crtService.Clear();
  SET_CRT_LOCATION(crtService);
  PushState(S_SERVICE_HEAD);
}
<INITIAL>"Type" {
  crtCustomType.Clear();
  SET_CRT_LOCATION(crtCustomType);
  PushState(S_CUSTOMTYPE_HEAD);
}
<INITIAL>"Verbatim" {
  crtVerbatim.Clear();
  SET_CRT_LOCATION(crtVerbatim);
  PushState(S_VERBATIM_HEAD);
}

 /*************************************/
 /*          serviceheader            */
 /*************************************/
<S_SERVICE_HEAD>{NAME} {
  if(crtService.name_ != "")
  {
    REJECT; // bad token after service name
  }
  crtService.name_ = yytext;
  PRINT_INFO << "Service: '" << crtService.name_ << "'" << PRINT_END;
}

<S_SERVICE_HEAD>"{" {
  PopState();                // exit S_SERVICE state
  PushState(S_SERVICE_BODY); // upon returning from S_SERVICE_BODY
}

 /*************************************/
 /*           servicebody             */
 /*************************************/
<S_SERVICE_BODY>""/{NAME} {
  // found a function return type
  PushState(S_FUNCTION_RETURN); // enter S_FUNCTION_RETURN
}
<S_SERVICE_BODY>"}" {
  services_.push_back(crtService);
  crtService.Clear();
  PopState(); // end of S_SERVICE_BODY
}


 /*************************************/
 /*              function             */
 /*************************************/
<S_FUNCTION_RETURN>""/{NAME} {
  CHECK(crtFunction.output_.name_.empty());
  CHECK(crtType.name_.empty());

  // this is function return type
  PopState();                 // exit  S_FUNCTION_RETURN now
  PushState(S_FUNCTION_NAME); // enter S_FUNCTION_NAME
  PushState(S_TYPE);          // enter S_TYPE (it will return to S_FUNCTION_NAME)
}

<S_FUNCTION_NAME>{NAME} {
  if(!crtType.name_.empty())
  {
    CHECK(crtFunction.output_.name_.empty());

    crtFunction.output_ = crtType;
    crtType.Clear();
  }

  if(crtFunction.name_.empty())
  {
    // this is function name
    crtFunction.name_ = yytext;
    SET_CRT_LOCATION(crtFunction);
    PRINT_INFO << "Function: '" << crtFunction.name_ << "'" << PRINT_END;
  }
  else
  {
    PRINT_ERROR << "in function '" << crtFunction.name_ << "': unexpected identifier '" << yytext << "' after function name" << PRINT_END;
  }
}

<S_FUNCTION_NAME>"(" {
  if(!crtFunction.input_.empty())
  {
    // this function already has params. e.g. "void Foo (int) (float)"
    PRINT_ERROR << "in function '" << crtFunction.name_ << "': unexpected '(' after parameters list" << PRINT_END;
    yyterminate(); // fatal. Parsing these parameters would add up. Ignoring will screw up the following text.
  }
  PushState(S_FUNCTION_PARAM_TYPE);
}

<S_FUNCTION_NAME>";" {
  PRINT_INFO << "Function '" << crtFunction.name_ << "' declaration end" << PRINT_END;
  crtService.functions_.push_back(crtFunction);
  crtFunction.Clear();

  // function declaration ended
  PopState();
}

<S_FUNCTION_PARAM_TYPE>""/{NAME} {
  ChangeState(S_FUNCTION_PARAM_NAME); // return to S_FUNCTION_PARAM_NAME
  PushState(S_TYPE);                  // after reading type
}
<S_FUNCTION_PARAM_TYPE>")" {
  CHECK(crtType.name_.empty());
  if(crtFunction.input_.empty())
  {    // no parameter at all. This is legal: e.g. "void Foo()"
  }
  else // no type after comma. This is bad: e.g. "void Foo(int,)"
  {
    PRINT_ERROR << "in function '" << crtFunction.name_ << "': expected type before ')'" << PRINT_END;
  }
  PopState();
}
<S_FUNCTION_PARAM_TYPE>"," { // e.g. "void Foo(,int)"
  PRINT_ERROR << "in function '" << crtFunction.name_ << "': expected type before ','" << PRINT_END;
}

<S_FUNCTION_PARAM_NAME>{NAME} {
  CHECK(!crtType.name_.empty()); // you cannot reach S_FUNCTION_PARAM_NAME without reading a type first
  if(!crtParam.name_.empty())
  {
    // e.g. "void Foo(int x y);" // the 'y' was matched here.
    PRINT_ERROR << "multiple parameter names. First was '" << crtParam.name_ << "' and now '" << yytext << "'." << PRINT_END;
    break;
  }
  crtParam.name_ = yytext;
  PRINT_INFO << "Param name: " << crtParam.name_ << PRINT_END;
}

<S_FUNCTION_PARAM_NAME>"," |
<S_FUNCTION_PARAM_NAME>")" {
  CHECK(!crtType.name_.empty());

  crtParam.type_ = crtType;
  crtType.Clear();

  // if not specified, auto-generate parameter name
  if(crtParam.name_.empty())
  {
    char szName[8] = {0,};
#if __WORDSIZE == 64
    snprintf(szName, sizeof(szName), "arg%u", 
             static_cast<unsigned int>(crtFunction.input_.size()));
#else
#ifdef __APPLE__
    snprintf(szName, sizeof(szName), "arg%lu", 
             crtFunction.input_.size());
#else
    snprintf(szName, sizeof(szName), "arg%u", 
             crtFunction.input_.size());
#endif
#endif
    crtParam.name_ = szName;
  }
  PRINT_INFO << "Add function parameter: '" << crtParam.type_ << " " << crtParam.name_ << "' on current symbol: '" << yytext << "'" << PRINT_END;
  crtFunction.input_.push_back(crtParam);
  crtParam.Clear();

  if(yytext[0] == ',')
  {
    ChangeState(S_FUNCTION_PARAM_TYPE); // type expected after ','
  }
  else // yytext[0] == ')'
  {
    PopState(); // parameters list ended
  }
}

 /*************************************/
 /*         custom type name          */
 /*************************************/
<S_CUSTOMTYPE_HEAD>{NAME} {
  if(crtCustomType.name_.empty())
  {
    crtCustomType.name_ = yytext;
    PRINT_INFO << "CustomType: " << crtCustomType.name_ << PRINT_END;
  }
  else
  { // e.g. "Type Person XXX {' // 'XXX' is matched here
    PRINT_ERROR << "unexpected identifier '" << yytext << "' after custom type name" << PRINT_END;
  }
}

<S_CUSTOMTYPE_HEAD>"{" {
  CHECK(crtCustomType.attributes_.empty());
  PopState();                   // exit current state
  PushState(S_CUSTOMTYPE_BODY); // on return from body
}

 /*************************************/
 /*         custom type body          */
 /*************************************/
<S_CUSTOMTYPE_BODY>""/{NAME} {
  crtAttribute.Clear();
  crtAttributeRequirementSet = false;
  SET_CRT_LOCATION(crtAttribute);
  PushState(S_CUSTOMTYPE_ATTR); // a new attribute begins
}
<S_CUSTOMTYPE_BODY>"}" {
  customTypes_.push_back(crtCustomType);
  crtCustomType.Clear();
  PopState();
}

<S_CUSTOMTYPE_ATTR>"optional" |
<S_CUSTOMTYPE_ATTR>"required" {
  if(crtAttributeRequirementSet)
  {
    // e.g. "optional required int x;" // matches the "required" keyword
    PRINT_ERROR << "duplicate required/optional keyword" << PRINT_END;
    break;
  }
  crtAttribute.isOptional_ = (yytext[0] == 'o');
  crtAttributeRequirementSet = true;
}
<S_CUSTOMTYPE_ATTR>""/{NAME} {
  if(!crtType.name_.empty() || !crtAttribute.type_.name_.empty())
  {
    REJECT; // type already found
  }
  PushState(S_TYPE);
}
<S_CUSTOMTYPE_ATTR>{NAME} {
  if(!crtType.name_.empty()) // copy crtType to crtAttribute
  {
    CHECK(crtAttribute.type_.name_.empty());
    crtAttribute.type_ = crtType;
    crtType.Clear();
  }

  if(crtAttribute.name_.empty())
  {
    crtAttribute.name_ = yytext;
    PRINT_INFO << "Attribute name: " << crtAttribute.name_ << PRINT_END;
  }
  else
  {
    // e.g. "int x y = 7" // 'y' is matched here
    PRINT_ERROR << "missing ';' after \"" << crtAttribute.name_ << "\"" << PRINT_END;
    PRINT_ERROR << "unexpected identifier '" << yytext << "' after attribute name '" << crtAttribute.name_ << "'" << PRINT_END;
  }
}
<S_CUSTOMTYPE_ATTR>";" {
  bool bError = false;
  if(!crtType.name_.empty()) // copy crtType to crtAttribute
  {
    CHECK(crtAttribute.type_.name_.empty());
    crtAttribute.type_ = crtType;
    crtType.Clear();
  }
  if(crtAttribute.type_.name_.empty())
  {
    PRINT_ERROR << "missing attribute type before ';'" << PRINT_END;
    bError = true;
  }
  if(crtAttribute.name_.empty())
  {
    PRINT_ERROR << "missing attribute name before ';'" << PRINT_END;
    bError = true;
  }

  if(!bError)
  {
    crtCustomType.attributes_.push_back(crtAttribute);
  }

  crtAttribute.Clear();
  crtAttributeRequirementSet = false;
  PopState();
}
<S_CUSTOMTYPE_ATTR>"}" {
  if(!crtType.name_.empty()) // copy crtType to crtAttribute
  {
    CHECK(crtAttribute.type_.name_.empty());
    crtAttribute.type_ = crtType;
    crtType.Clear();
  }
  CHECK(!crtAttribute.type_.name_.empty()); // we are inside an attribute, so at least the typename should have been matched
  if(crtAttribute.name_.empty())
  {
    PRINT_ERROR << "missing identifier after type \"" << crtAttribute.type_.name_ << "\"" << PRINT_END;
  }
  PRINT_ERROR << "missing ';'" << PRINT_END;

  crtAttribute.Clear();
  crtAttributeRequirementSet = false;
  PopState(); // exit S_CUSTOMTYPE_ATTR to S_CUSTOMTYPE_BODY
  crtCustomType.Clear();
  PopState(); // exit S_CUSTOMTYPE_BODY to INITIAL
}

 /*************************************/
 /*               type                */
 /*************************************/
<S_TYPE>{NAME} {
  if(crtType.name_.empty())
  {
    pcrtSubtype = &crtType; // begin from top level
  }
  if(!pcrtSubtype->name_.empty())
  {
    PRINT_ERROR << "unexpected identifier '" << yytext << "' while waiting for terminator '>'" << PRINT_END;
    yyterminate(); // fatal. Forgeting a '>' => the rest of the text is interpreted as type.
  }
  pcrtSubtype->name_ = yytext;
  PRINT_INFO << "Simple type name: " << pcrtSubtype->name_ << PRINT_END;
  PopState();
}
<S_TYPE>"array<" { // cannot use RPC_ARRAY instead of "array". Flex analyzer does not expand macros.
  pcrtSubtype->name_ = RPC_ARRAY;
  pcrtSubtype->subtype1_ = new PType();
  pcrtSubtype->subtype1_->parent_ = pcrtSubtype;
  pcrtSubtype = pcrtSubtype->subtype1_;
  PushState(S_TYPE);
}
<S_TYPE>"map<" {
  pcrtSubtype->name_ = RPC_MAP;
  pcrtSubtype->subtype1_ = new PType();
  pcrtSubtype->subtype1_->parent_ = pcrtSubtype;
  pcrtSubtype->subtype2_ = new PType();
  pcrtSubtype->subtype2_->parent_ = pcrtSubtype;
  pcrtSubtype = pcrtSubtype->subtype1_;
  PushState(S_TYPE);
}
<S_TYPE>"," {
  CHECK(pcrtSubtype->parent_);
  if(pcrtSubtype->name_.empty())
  {
    PRINT_ERROR << "missing template parameter before ','" << PRINT_END;
    PopState();
  }
  if(pcrtSubtype == pcrtSubtype->parent_->subtype2_)
  {
    PRINT_ERROR << "too many template parameters for type map" << PRINT_END;
    yyterminate(); //PopState();
  }
  if(pcrtSubtype->parent_->subtype2_ == NULL)
  {
    PRINT_ERROR << "too many template parameters for type array" << PRINT_END;
    yyterminate(); //PopState();
  }
  pcrtSubtype = pcrtSubtype->parent_;
  pcrtSubtype = pcrtSubtype->subtype2_;
  PushState(S_TYPE);
}

<S_TYPE>">" {
  CHECK(pcrtSubtype->parent_);
  if(pcrtSubtype->name_.empty())
  {
    PRINT_ERROR << "missing template parameter before '>'" << PRINT_END;
    PopState();
  }
  pcrtSubtype = pcrtSubtype->parent_;
  if(pcrtSubtype->subtype2_ && pcrtSubtype->subtype2_->name_.empty())
  {
    PRINT_ERROR << "type map needs 2 template parameters" << PRINT_END;
    PopState();
  }
  PRINT_INFO << "Complex type name: " << (*pcrtSubtype) << PRINT_END;
  PopState();
}

 /*************************************/
 /*  global rules with low priority   */
 /*************************************/

<*>\n /* eat up line feeds */

 /**
  * Print an error for unmatched characters.
  *
  * NOTE: this MUST be the LAST rule!
  * Because you may want to match correctly some simple characters like: + - = .
  * using one of the above rules. The rule here takes action only if all the above
  * rules failed.
  */
<*>{NAME} PRINT_ERROR << "Bad syntax: unrecognized token \"" << yytext << "\"" << PRINT_END;
<*>.      PRINT_ERROR << "Bad syntax: unrecognized symbol \"" << yytext << "\"" << PRINT_END;

   /********************************************************************/
   /********************************************************************/
%% /*                          USER CODE                               */
   /********************************************************************/
   /********************************************************************/

const char * StateName(int sc)
{
  switch(sc)
  {
  case S_INITIAL:              return "Initial";
  case S_COMMENT:              return "Comment";
  case S_SERVICE_HEAD:         return "ServiceHead";
  case S_SERVICE_BODY:         return "ServiceBody";
  case S_FUNCTION_RETURN:      return "FunctionReturn";
  case S_FUNCTION_NAME:        return "FunctionName";
  case S_FUNCTION_PARAM_TYPE:  return "FunctionParamType";
  case S_FUNCTION_PARAM_NAME:  return "FunctionParamName";
  case S_CUSTOMTYPE_HEAD:      return "CustomTypeHead";
  case S_CUSTOMTYPE_BODY:      return "CustomTypeBody";
  case S_CUSTOMTYPE_ATTR:      return "CustomTypeAttr";
  case S_CUSTOMTYPE_ATTR_CODE: return "CustomTypeAttrCode";
  case S_TYPE:                 return "Type";
  default:                     return "Unrecognized state";
  };
}

rpc::Parser::Parser(std::istream & input,
                     std::ostream & output,
                     PCustomTypeArray & customTypes,
                     PServiceArray & services,
                     PVerbatimArray & verbatim,
                     const char * filename,
                     bool bPrintInfo)
  : yyFlexLexer(&input, &output),
    customTypes_(customTypes),
    services_(services),
    verbatim_(verbatim),
    strFilename_(filename),
    bErrorsFound_(false),
    bPrintInfo_(bPrintInfo)
{
}
rpc::Parser::~Parser()
{
}

bool rpc::Parser::Run()
{
  while(yylex() != 0)
    ;
  return !bErrorsFound_;
}

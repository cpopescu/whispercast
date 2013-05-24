
#include "common/base/log.h"
#include "common/base/errno.h"
#include "net/rpc/lib/types/rpc_all_types.h"
#include "net/rpc/lib/server/rpc_services_manager.h"

#include "invokers.h"

/********************************************************************/
/*                       serverInvokers.cc                          */
/********************************************************************/
ServiceInvokerGigel::ServiceInvokerGigel() : rpc::ServiceInvoker("Gigel") {}
ServiceInvokerGigel::~ServiceInvokerGigel() {}

bool ServiceInvokerGigel::Call(rpc::Query* query) {
  CHECK_EQ(query->service(), GetName()); // service should have already been verified
  LOG_DEBUG << "Call: method=" << query->method();

  if ( !query->InitDecodeParams() ) {
    LOG_ERROR << "Failed to initialize parameters decoding";
    query->Complete(rpc::RPC_GARBAGE_ARGS);
    return true;
  }

  if(query->method() == "DoSomething") {
    // decode expected parameters
    //
    rpc::Int* const __tmp_a = new rpc::Int();
    query->AddParam(__tmp_a);
    if ( !query->DecodeParam(*__tmp_a) ) {
      LOG_WARNING << "Invoke 'DoSomething' with garbage arguments: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }
    rpc::Float* const __tmp_b = new rpc::Float();
    query->AddParam(__tmp_b);
    if ( !query->DecodeParam(*__tmp_b) ) {
      LOG_WARNING << "Invoke 'DoSomething' with garbage arguments: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }
    if ( query->HasMoreParams() ) {
      LOG_WARNING << "Invoke 'DoSomething' with too many args: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }

    DoSomething(new rpc::CallContext<rpc::Int>(query), __tmp_a, __tmp_b);
    return true;
  }
  if(query->method() == "DoSomethingElse") {
    // decode expected parameters
    //
    if ( query->HasMoreParams() ) {
      LOG_WARNING << "Invoke 'DoSomethingElse' with too many args: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }
    DoSomethingElse(new rpc::CallContext<rpc::Void>(query));
    return true;
  }

  // default
  LOG_WARNING << "No such method '" << query->method() << "' in service Gigel";
  query->Complete(rpc::RPC_PROC_UNAVAIL);
  return true;
};

string ServiceInvokerGigel::GetTurntablePage(const string& base_path, const string& url_path) const {
  if ( url_path == "__form_DoSomething" ) return GetDoSomethingForm(base_path);
  if ( url_path == "__form_DoSomethingElse" ) return GetDoSomethingElseForm(base_path);
  if ( url_path != "__forms" ) return "";
  string s = "<ul>\n";
  s += "<a href=\"" + base_path + "/" + GetName() + "/__form_DoSomething\">DoSomething</a><br>\n";
  s += "<a href=\"" + base_path + "/" + GetName() + "/__form_DoSomethingElse\">DoSomethingElse</a><br>\n";
  s += "</ul>\n";
  return s;
}
string ServiceInvokerGigel::GetDoSomethingForm(const string& command_base_path) const {
  string s;
  string aux;
  s += "<h1>Gigel :: DoSomething</h1>\n";
  s += "<a href=\"" + command_base_path + "/" + GetName() + "/__forms\">all commands...</a><br>\n";
  s += "<p><form>\n";
  s += "<input type='hidden' name='bottom_page'>\n";
  s += "<script language=\"JavaScript1.1\">\n";
  s += "  f = document.getElementsByTagName('form')[0];\n";
  s += "  elem = document.getElementsByTagName('hidden')[0];\n";
  s += "  add_begin_array('', f, elem);\n";
  s += "  add_number_control('a', '', false, f, elem);\n";
  s += "  add_number_control('b', '', false, f, elem);\n";
 s += "  add_end_array('', f, elem);\n";
 s += "</script>\
";
 s += "<input type='button' name='__send__' value='Send Command'  onclick='var x = extract_form_data(parentNode, 0, parentNode.elements.length, false)[0];  parentNode.appendChild(document.createElement(\"br\"));  parentNode.appendChild(document.createTextNode(\"Sending: [\" + x + \"]\"));  rpc_raw_query(\"" + command_base_path + "\", \"Gigel\", \"DoSomething\", x,  display_rpc_return)'>\n";
 s += "<br>\
";
 s += "</form>\
";
  aux += "<script language=\"JavaScript1.1\">\n";
 aux += "</script>\
";
  return aux + s;
}
string ServiceInvokerGigel::GetDoSomethingElseForm(const string& command_base_path) const {
  string s;
  string aux;
  s += "<h1>Gigel :: DoSomethingElse</h1>\n";
  s += "<a href=\"" + command_base_path + "/" + GetName() + "/__forms\">all commands...</a><br>\n";
  s += "<p><form>\n";
  s += "<input type='hidden' name='bottom_page'>\n";
  s += "<script language=\"JavaScript1.1\">\n";
  s += "  f = document.getElementsByTagName('form')[0];\n";
  s += "  elem = document.getElementsByTagName('hidden')[0];\n";
  s += "  add_begin_array('', f, elem);\n";
 s += "  add_end_array('', f, elem);\n";
 s += "</script>\
";
 s += "<input type='button' name='__send__' value='Send Command'  onclick='var x = extract_form_data(parentNode, 0, parentNode.elements.length, false)[0];  parentNode.appendChild(document.createElement(\"br\"));  parentNode.appendChild(document.createTextNode(\"Sending: [\" + x + \"]\"));  rpc_raw_query(\"" + command_base_path + "\", \"Gigel\", \"DoSomethingElse\", x,  display_rpc_return)'>\n";
 s += "<br>\
";
 s += "</form>\
";
  aux += "<script language=\"JavaScript1.1\">\n";
 aux += "</script>\
";
  return aux + s;
}

ServiceInvokerMitica::ServiceInvokerMitica() : rpc::ServiceInvoker("Mitica") {}
ServiceInvokerMitica::~ServiceInvokerMitica() {}

bool ServiceInvokerMitica::Call(rpc::Query * query) {
  if(query->method() == "Initialize")
  {
    // decode expected parameters
    //
    rpc::String* const __tmp_address = new rpc::String();
    query->AddParam(__tmp_address);
    if ( !query->DecodeParam(*__tmp_address) ) {
      LOG_WARNING << "Invoke 'Initialize' with garbage arguments: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }
    rpc::Int* const __tmp_age = new rpc::Int();
    query->AddParam(__tmp_age);
    if ( !query->DecodeParam(*__tmp_age) ) {
      LOG_WARNING << "Invoke 'Initialize' with garbage arguments: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }
    if ( query->HasMoreParams() ) {
      LOG_WARNING << "Invoke 'Initialize' with too many args: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }

    Initialize(new rpc::CallContext<rpc::Bool>(query), __tmp_address, __tmp_age);
    return true;
  }

  if(query->method() == "Exit")
  {
    // decode expected parameters
    //
    if ( query->HasMoreParams() ) {
      LOG_WARNING << "Invoke 'Exit' with too many args: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }

    Exit(new rpc::CallContext<rpc::Void>(query));
    return true;
  }

  if(query->method() == "TestMe")
  {
    // decode expected parameters
    //
    rpc::Int* const __tmp_a = new rpc::Int();
    query->AddParam(__tmp_a);
    if ( !query->DecodeParam(*__tmp_a) )
    {
      LOG_WARNING << "Invoke 'TestMe' with garbage arguments: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }
    rpc::Float* const __tmp_b = new rpc::Float();
    query->AddParam(__tmp_b);
    if ( !query->DecodeParam(*__tmp_b) ) {
      LOG_WARNING << "Invoke 'TestMe' with garbage arguments: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }
    rpc::String* const __tmp_c = new rpc::String();
    query->AddParam(__tmp_c);
    if ( !query->DecodeParam(*__tmp_c) ) {
      LOG_WARNING << "Invoke 'TestMe' with garbage arguments: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }
    if ( query->HasMoreParams() ) {
      LOG_WARNING << "Invoke 'TestMe' with too many args: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }

    TestMe(new rpc::CallContext<rpc::Int>(query), __tmp_a, __tmp_b, __tmp_c);
    return true;
  }

  if(query->method() == "Foo")
  {
    // decode expected parameters
    //
    rpc::Date* const __tmp_d = new rpc::Date();
    query->AddParam(__tmp_d);
    if ( !query->DecodeParam(*__tmp_d) ) {
      LOG_WARNING << "Invoke 'Foo' with garbage arguments: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }
    if ( query->HasMoreParams() ) {
      LOG_WARNING << "Invoke 'Foo' with too many args: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }

    Foo(new rpc::CallContext<rpc::String>(query), __tmp_d);
    return true;
  }

  if(query->method() == "SetPerson")
  {
    // decode expected parameters
    //
    Person* const __tmp_arg0 = new Person();
    query->AddParam(__tmp_arg0);
    if ( !query->DecodeParam(*__tmp_arg0) ) {
      LOG_WARNING << "Invoke 'SetPerson' with garbage arguments: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }
    if ( query->HasMoreParams() ) {
      LOG_WARNING << "Invoke 'SetPerson' with too many args: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }

    SetPerson(new rpc::CallContext<rpc::Void>(query), __tmp_arg0);
    return true;
  }

  if(query->method() == "GetPerson")
  {
    // decode expected parameters
    //
    if ( query->HasMoreParams() ) {
      LOG_WARNING << "Invoke 'GetPerson' with too many args: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }

    GetPerson(new rpc::CallContext<Person>(query));
    return true;
  }

  if(query->method() == "SetFamily")
  {
    // decode expected parameters
    //
    Person* const __tmp_mother = new Person();
    query->AddParam(__tmp_mother);
    if ( !query->DecodeParam(*__tmp_mother) ) {
      LOG_WARNING << "Invoke 'SetFamily' with garbage arguments: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }
    Person* const __tmp_father = new Person();
    query->AddParam(__tmp_father);
    if ( !query->DecodeParam(*__tmp_father) ) {
      LOG_WARNING << "Invoke 'SetFamily' with garbage arguments: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }
    rpc::Array< Person >* const __tmp_children = new rpc::Array< Person >();
    query->AddParam(__tmp_children);
    if ( !query->DecodeParam(*__tmp_children) ) {
      LOG_WARNING << "Invoke 'SetFamily' with garbage arguments: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }
    if ( query->HasMoreParams() ) {
      LOG_WARNING << "Invoke 'SetFamily' with too many args: " << query->RewindParams();
      query->Complete(rpc::RPC_GARBAGE_ARGS);
      return true;
    }

    SetFamily(new rpc::CallContext<Family>(query), __tmp_mother, __tmp_father, __tmp_children);
    return true;
  }

  // default
  LOG_WARNING << "No such method '" << query->method() << "' in service Mitica";
  query->Complete(rpc::RPC_PROC_UNAVAIL);
  return true;
}

string ServiceInvokerMitica::GetTurntablePage(const string& base_path, const string& url_path) const {
  if ( url_path == "__form_Initialize" ) return GetInitializeForm(base_path);
  if ( url_path == "__form_Exit" ) return GetExitForm(base_path);
  if ( url_path == "__form_TestMe" ) return GetTestMeForm(base_path);
  if ( url_path == "__form_Foo" ) return GetFooForm(base_path);
  if ( url_path == "__form_SetPerson" ) return GetSetPersonForm(base_path);
  if ( url_path == "__form_GetPerson" ) return GetGetPersonForm(base_path);
  if ( url_path == "__form_SetFamily" ) return GetSetFamilyForm(base_path);
  if ( url_path == "__form_GetChildren" ) return GetGetChildrenForm(base_path);
  if ( url_path != "__forms" ) return "";
  string s = "<ul>\n";
  s += "<a href=\"" + base_path + "/" + GetName() + "/__form_Initialize\">Initialize</a><br>\n";
  s += "<a href=\"" + base_path + "/" + GetName() + "/__form_Exit\">Exit</a><br>\n";
  s += "<a href=\"" + base_path + "/" + GetName() + "/__form_TestMe\">TestMe</a><br>\n";
  s += "<a href=\"" + base_path + "/" + GetName() + "/__form_Foo\">Foo</a><br>\n";
  s += "<a href=\"" + base_path + "/" + GetName() + "/__form_SetPerson\">SetPerson</a><br>\n";
  s += "<a href=\"" + base_path + "/" + GetName() + "/__form_GetPerson\">GetPerson</a><br>\n";
  s += "<a href=\"" + base_path + "/" + GetName() + "/__form_SetFamily\">SetFamily</a><br>\n";
  s += "<a href=\"" + base_path + "/" + GetName() + "/__form_GetChildren\">GetChildren</a><br>\n";
  s += "</ul>\n";
  return s;
}
string ServiceInvokerMitica::GetInitializeForm(const string& command_base_path) const {
  string s;
  string aux;
  s += "<h1>Mitica :: Initialize</h1>\n";
  s += "<a href=\"" + command_base_path + "/" + GetName() + "/__forms\">all commands...</a><br>\n";
  s += "<p><form>\n";
  s += "<input type='hidden' name='bottom_page'>\n";
  s += "<script language=\"JavaScript1.1\">\n";
  s += "  f = document.getElementsByTagName('form')[0];\n";
  s += "  elem = document.getElementsByTagName('hidden')[0];\n";
  s += "  add_begin_array('', f, elem);\n";
  s += "  add_text_control('address', 'address', false, f, elem);\n";
  s += "  add_number_control('age', '', false, f, elem);\n";
 s += "  add_end_array('', f, elem);\n";
 s += "</script>\
";
 s += "<input type='button' name='__send__' value='Send Command'  onclick='var x = extract_form_data(parentNode, 0, parentNode.elements.length, false)[0];  parentNode.appendChild(document.createElement(\"br\"));  parentNode.appendChild(document.createTextNode(\"Sending: [\" + x + \"]\"));  rpc_raw_query(\"" + command_base_path + "\", \"Mitica\", \"Initialize\", x,  display_rpc_return)'>\n";
 s += "<br>\
";
 s += "</form>\
";
  aux += "<script language=\"JavaScript1.1\">\n";
 aux += "</script>\
";
  return aux + s;
}
string ServiceInvokerMitica::GetExitForm(const string& command_base_path) const {
  string s;
  string aux;
  s += "<h1>Mitica :: Exit</h1>\n";
  s += "<a href=\"" + command_base_path + "/" + GetName() + "/__forms\">all commands...</a><br>\n";
  s += "<p><form>\n";
  s += "<input type='hidden' name='bottom_page'>\n";
  s += "<script language=\"JavaScript1.1\">\n";
  s += "  f = document.getElementsByTagName('form')[0];\n";
  s += "  elem = document.getElementsByTagName('hidden')[0];\n";
  s += "  add_begin_array('', f, elem);\n";
 s += "  add_end_array('', f, elem);\n";
 s += "</script>\
";
 s += "<input type='button' name='__send__' value='Send Command'  onclick='var x = extract_form_data(parentNode, 0, parentNode.elements.length, false)[0];  parentNode.appendChild(document.createElement(\"br\"));  parentNode.appendChild(document.createTextNode(\"Sending: [\" + x + \"]\"));  rpc_raw_query(\"" + command_base_path + "\", \"Mitica\", \"Exit\", x,  display_rpc_return)'>\n";
 s += "<br>\
";
 s += "</form>\
";
  aux += "<script language=\"JavaScript1.1\">\n";
 aux += "</script>\
";
  return aux + s;
}
string ServiceInvokerMitica::GetTestMeForm(const string& command_base_path) const {
  string s;
  string aux;
  s += "<h1>Mitica :: TestMe</h1>\n";
  s += "<a href=\"" + command_base_path + "/" + GetName() + "/__forms\">all commands...</a><br>\n";
  s += "<p><form>\n";
  s += "<input type='hidden' name='bottom_page'>\n";
  s += "<script language=\"JavaScript1.1\">\n";
  s += "  f = document.getElementsByTagName('form')[0];\n";
  s += "  elem = document.getElementsByTagName('hidden')[0];\n";
  s += "  add_begin_array('', f, elem);\n";
  s += "  add_number_control('a', '', false, f, elem);\n";
  s += "  add_number_control('b', '', false, f, elem);\n";
  s += "  add_text_control('c', 'c', false, f, elem);\n";
 s += "  add_end_array('', f, elem);\n";
 s += "</script>\
";
 s += "<input type='button' name='__send__' value='Send Command'  onclick='var x = extract_form_data(parentNode, 0, parentNode.elements.length, false)[0];  parentNode.appendChild(document.createElement(\"br\"));  parentNode.appendChild(document.createTextNode(\"Sending: [\" + x + \"]\"));  rpc_raw_query(\"" + command_base_path + "\", \"Mitica\", \"TestMe\", x,  display_rpc_return)'>\n";
 s += "<br>\
";
 s += "</form>\
";
  aux += "<script language=\"JavaScript1.1\">\n";
 aux += "</script>\
";
  return aux + s;
}
string ServiceInvokerMitica::GetFooForm(const string& command_base_path) const {
  string s;
  string aux;
  s += "<h1>Mitica :: Foo</h1>\n";
  s += "<a href=\"" + command_base_path + "/" + GetName() + "/__forms\">all commands...</a><br>\n";
  s += "<p><form>\n";
  s += "<input type='hidden' name='bottom_page'>\n";
  s += "<script language=\"JavaScript1.1\">\n";
  s += "  f = document.getElementsByTagName('form')[0];\n";
  s += "  elem = document.getElementsByTagName('hidden')[0];\n";
  s += "  add_begin_array('', f, elem);\n";
  s += "  add_text_control('d', 'd', false, f, elem);\n";
 s += "  add_end_array('', f, elem);\n";
 s += "</script>\
";
 s += "<input type='button' name='__send__' value='Send Command'  onclick='var x = extract_form_data(parentNode, 0, parentNode.elements.length, false)[0];  parentNode.appendChild(document.createElement(\"br\"));  parentNode.appendChild(document.createTextNode(\"Sending: [\" + x + \"]\"));  rpc_raw_query(\"" + command_base_path + "\", \"Mitica\", \"Foo\", x,  display_rpc_return)'>\n";
 s += "<br>\
";
 s += "</form>\
";
  aux += "<script language=\"JavaScript1.1\">\n";
 aux += "</script>\
";
  return aux + s;
}
string ServiceInvokerMitica::GetSetPersonForm(const string& command_base_path) const {
  string s;
  string aux;
  s += "<h1>Mitica :: SetPerson</h1>\n";
  s += "<a href=\"" + command_base_path + "/" + GetName() + "/__forms\">all commands...</a><br>\n";
  s += "<p><form>\n";
  s += "<input type='hidden' name='bottom_page'>\n";
  s += "<script language=\"JavaScript1.1\">\n";
  s += "  f = document.getElementsByTagName('form')[0];\n";
  s += "  elem = document.getElementsByTagName('hidden')[0];\n";
  s += "  add_begin_array('', f, elem);\n";
  s += "  add_Person_control('arg0', '', false, f, elem);\n";
 s += "  add_end_array('', f, elem);\n";
 s += "</script>\
";
 s += "<input type='button' name='__send__' value='Send Command'  onclick='var x = extract_form_data(parentNode, 0, parentNode.elements.length, false)[0];  parentNode.appendChild(document.createElement(\"br\"));  parentNode.appendChild(document.createTextNode(\"Sending: [\" + x + \"]\"));  rpc_raw_query(\"" + command_base_path + "\", \"Mitica\", \"SetPerson\", x,  display_rpc_return)'>\n";
 s += "<br>\
";
 s += "</form>\
";
  aux += "<script language=\"JavaScript1.1\">\n";
 aux += "\n" + Person::GetHTMLFormFunction();
 aux += "</script>\
";
  return aux + s;
}
string ServiceInvokerMitica::GetGetPersonForm(const string& command_base_path) const {
  string s;
  string aux;
  s += "<h1>Mitica :: GetPerson</h1>\n";
  s += "<a href=\"" + command_base_path + "/" + GetName() + "/__forms\">all commands...</a><br>\n";
  s += "<p><form>\n";
  s += "<input type='hidden' name='bottom_page'>\n";
  s += "<script language=\"JavaScript1.1\">\n";
  s += "  f = document.getElementsByTagName('form')[0];\n";
  s += "  elem = document.getElementsByTagName('hidden')[0];\n";
  s += "  add_begin_array('', f, elem);\n";
 s += "  add_end_array('', f, elem);\n";
 s += "</script>\
";
 s += "<input type='button' name='__send__' value='Send Command'  onclick='var x = extract_form_data(parentNode, 0, parentNode.elements.length, false)[0];  parentNode.appendChild(document.createElement(\"br\"));  parentNode.appendChild(document.createTextNode(\"Sending: [\" + x + \"]\"));  rpc_raw_query(\"" + command_base_path + "\", \"Mitica\", \"GetPerson\", x,  display_rpc_return)'>\n";
 s += "<br>\
";
 s += "</form>\
";
  aux += "<script language=\"JavaScript1.1\">\n";
 aux += "</script>\
";
  return aux + s;
}
string ServiceInvokerMitica::GetSetFamilyForm(const string& command_base_path) const {
  string s;
  string aux;
  s += "<h1>Mitica :: SetFamily</h1>\n";
  s += "<a href=\"" + command_base_path + "/" + GetName() + "/__forms\">all commands...</a><br>\n";
  s += "<p><form>\n";
  s += "<input type='hidden' name='bottom_page'>\n";
  s += "<script language=\"JavaScript1.1\">\n";
  s += "  f = document.getElementsByTagName('form')[0];\n";
  s += "  elem = document.getElementsByTagName('hidden')[0];\n";
  s += "  add_begin_array('', f, elem);\n";
  s += "  add_Person_control('mother', '', false, f, elem);\n";
  s += "  add_Person_control('father', '', false, f, elem);\n";
  s += "  add_array_control('children', '', false, f, elem, [ 'Person' ]);\n";
 s += "  add_end_array('', f, elem);\n";
 s += "</script>\
";
 s += "<input type='button' name='__send__' value='Send Command'  onclick='var x = extract_form_data(parentNode, 0, parentNode.elements.length, false)[0];  parentNode.appendChild(document.createElement(\"br\"));  parentNode.appendChild(document.createTextNode(\"Sending: [\" + x + \"]\"));  rpc_raw_query(\"" + command_base_path + "\", \"Mitica\", \"SetFamily\", x,  display_rpc_return)'>\n";
 s += "<br>\
";
 s += "</form>\
";
  aux += "<script language=\"JavaScript1.1\">\n";
 aux += "\n" + Person::GetHTMLFormFunction();
 aux += "</script>\
";
  return aux + s;
}
string ServiceInvokerMitica::GetGetChildrenForm(const string& command_base_path) const {
  string s;
  string aux;
  s += "<h1>Mitica :: GetChildren</h1>\n";
  s += "<a href=\"" + command_base_path + "/" + GetName() + "/__forms\">all commands...</a><br>\n";
  s += "<p><form>\n";
  s += "<input type='hidden' name='bottom_page'>\n";
  s += "<script language=\"JavaScript1.1\">\n";
  s += "  f = document.getElementsByTagName('form')[0];\n";
  s += "  elem = document.getElementsByTagName('hidden')[0];\n";
  s += "  add_begin_array('', f, elem);\n";
 s += "  add_end_array('', f, elem);\n";
 s += "</script>\
";
 s += "<input type='button' name='__send__' value='Send Command'  onclick='var x = extract_form_data(parentNode, 0, parentNode.elements.length, false)[0];  parentNode.appendChild(document.createElement(\"br\"));  parentNode.appendChild(document.createTextNode(\"Sending: [\" + x + \"]\"));  rpc_raw_query(\"" + command_base_path + "\", \"Mitica\", \"GetChildren\", x,  display_rpc_return)'>\n";
 s += "<br>\
";
 s += "</form>\
";
  aux += "<script language=\"JavaScript1.1\">\n";
 aux += "</script>\
";
  return aux + s;
}

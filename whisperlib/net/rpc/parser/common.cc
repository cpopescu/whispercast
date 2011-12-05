
#include "net/rpc/parser/common.h"
#include "common/base/log.h"
#include "common/base/strutil.h"

Keywords::Keywords(const string& language_name,
                   const set<string>& keywords,
                   bool is_case_sensitive)
  : language_name_(language_name),
    keywords_(keywords),
    is_case_sensitive_(is_case_sensitive) {
}
Keywords::Keywords(const Keywords& other)
  : language_name_(other.language_name_),
    keywords_(other.keywords_),
    is_case_sensitive_(other.is_case_sensitive_) {
}
Keywords::~Keywords() {
}

const string& Keywords::LanguageName() const {
  return language_name_;
}

bool Keywords::ContainsKeyword(const string& word) const {
  if ( is_case_sensitive_ ) {
    return keywords_.find(word) != keywords_.end();
  }
  string lword(word);
  std::transform(lword.begin(), lword.end(), lword.begin(), ::tolower);
  return keywords_.find(lword) != keywords_.end();
}

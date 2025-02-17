/* SPDX-License-Identifier: MPL-2.0 */
/* Copyright © 2019 Fragcolor Pte. Ltd. */
/* Copyright (C) 2015 Joel Martin <github@martintribe.org> */

#include "MAL.h"
#include "Types.h"

#include <iostream>
#include <regex>

typedef std::regex Regex;

static const Regex intRegex("^[-+]?\\d+$");
static const Regex hexRegex("^0x[0-9a-fA-F]+$");
static const Regex floatRegex("^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$");
static const Regex closeRegex("[\\)\\]}]");

static const Regex newlineRegex("(\\r\\n|\\r|\\n)");
static const Regex whitespaceRegex("[\\s,]+|;.*");
static const Regex tokenRegexes[] = {
    Regex("~@"),
    Regex("#\\("),
    Regex("[\\[\\]{}()'`~^@]"),
    Regex("\"(?:\\\\.|[^\\\\\"])*\""),
    Regex("#\"(?:\\\\.|[^\\\\\"])*\""),
    Regex("[^\\s\\[\\]{}('\"`,;)]+"),
};

class Tokeniser {
public:
  Tokeniser(const String &input);

  String peek() const {
    ASSERT(!eof(), "Tokeniser reading past EOF in peek\n");
    return m_token;
  }

  String next() {
    ASSERT(!eof(), "Tokeniser reading past EOF in next\n");
    String ret = peek();
    nextToken();
    return ret;
  }

  bool eof() const { return m_iter == m_end; }

  size_t line() const { return m_currentLine; }

private:
  void skipWhitespace();
  void nextToken();

  bool matchRegex(const Regex &regex);

  typedef String::const_iterator StringIter;

  String m_token;
  StringIter m_iter;
  StringIter m_begin;
  StringIter m_end;
  size_t m_currentLine{1};
};

Tokeniser::Tokeniser(const String &input)
    : m_iter(input.begin()), m_begin(input.begin()), m_end(input.end()) {
  nextToken();
}

bool Tokeniser::matchRegex(const Regex &regex) {
  if (eof()) {
    return false;
  }

  std::smatch match;
  auto flags = std::regex_constants::match_continuous;
  if (!std::regex_search(m_iter, m_end, match, regex, flags)) {
    return false;
  }

  ASSERT(match.size() == 1, "Should only have one submatch, not %llu\n",
         match.size());
  ASSERT(match.position(0) == 0, "Need to match first character\n");
  ASSERT(match.length(0) > 0, "Need to match a non-empty string\n");

  // Don't advance  m_iter now, do it after we've consumed the token in
  // next().  If we do it now, we hit eof() when there's still one token left.
  m_token = match.str(0);

  return true;
}

void Tokeniser::nextToken() {
  m_iter += m_token.size();

  skipWhitespace();
  if (eof()) {
    return;
  }

  m_currentLine +=
      size_t(std::distance(std::sregex_iterator(m_begin, m_iter, newlineRegex),
                           std::sregex_iterator()));
  m_begin = m_iter;

  for (auto &it : tokenRegexes) {
    if (matchRegex(it)) {
      return;
    }
  }

  String mismatch(m_iter, m_end);
  if (mismatch[0] == '"') {
    MAL_CHECK(false, "expected '\"', got EOF, line: %i", line());
  } else {
    MAL_CHECK(false, "unexpected '%s', line: %i", mismatch.c_str(), line());
  }
}

void Tokeniser::skipWhitespace() {
  while (matchRegex(whitespaceRegex)) {
    m_iter += m_token.size();
  }
}

#define VALUE_WITH_LINE(__val__)                                               \
  [&]() {                                                                      \
    auto val = __val__;                                                        \
    val->line = tokeniser.line();                                              \
    return val;                                                                \
  }()

static malValuePtr readAtom(Tokeniser &tokeniser);
static malValuePtr readForm(Tokeniser &tokeniser);
static void readList(Tokeniser &tokeniser, malValueVec *items,
                     const String &end);
static malValuePtr processMacro(Tokeniser &tokeniser, const String &symbol);

malValuePtr readStr(const String &input) {
  Tokeniser tokeniser(input);
  if (tokeniser.eof()) {
    throw malEmptyInputException();
  }
  return readForm(tokeniser);
}

static malValuePtr readForm(Tokeniser &tokeniser) {
  MAL_CHECK(!tokeniser.eof(), "expected form, got EOF, line: %i",
            tokeniser.line());
  String token = tokeniser.peek();

  MAL_CHECK(!std::regex_match(token, closeRegex), "unexpected '%s', line: %i",
            token.c_str(), tokeniser.line());

  if (token == "(") {
    tokeniser.next();
    std::unique_ptr<malValueVec> items(new malValueVec);
    readList(tokeniser, items.get(), ")");
    return VALUE_WITH_LINE(mal::list(items.release()));
  } else if (token == "[") {
    tokeniser.next();
    std::unique_ptr<malValueVec> items(new malValueVec);
    readList(tokeniser, items.get(), "]");
    return VALUE_WITH_LINE(mal::vector(items.release()));
  } else if (token == "#(") {
    tokeniser.next();
    std::unique_ptr<malValueVec> items(new malValueVec);
    readList(tokeniser, items.get(), ")");
    return VALUE_WITH_LINE(
        mal::list(mal::symbol("chainify"), mal::vector(items.release())));
  } else if (token == "{") {
    tokeniser.next();
    malValueVec items;
    readList(tokeniser, &items, "}");
    return VALUE_WITH_LINE(mal::hash(items.begin(), items.end(), false));
  } else {
    return VALUE_WITH_LINE(readAtom(tokeniser));
  }
}

static malValuePtr readAtom(Tokeniser &tokeniser) {
  struct ReaderMacro {
    const char *token;
    const char *symbol;
  };
  ReaderMacro macroTable[] = {{"@", "deref"},
                              {"`", "quasiquote"},
                              {"'", "quote"},
                              {"~@", "splice-unquote"},
                              {"~", "unquote"}};

  struct Constant {
    const char *token;
    malValuePtr value;
  };
  Constant constantTable[] = {
      {"false", mal::falseValue()},
      {"nil", mal::nilValue()},
      {"true", mal::trueValue()},
  };

  String token = tokeniser.next();

  if (token[0] == '"') {
    return mal::string(unescape(token));
  }

  if (token[0] == '#' && token[1] == '"') {
    return mal::string(token.substr(2, token.length() - 3));
  }

  if (token[0] == ':') {
    return mal::keyword(token);
  }

  if (token[0] == '.') {
    auto str = token.substr(1);
    return mal::contextVar(str);
  }

  if (token == "^") {
    malValuePtr meta = readForm(tokeniser);
    malValuePtr value = readForm(tokeniser);
    // Note that meta and value switch places
    return mal::list(mal::symbol("with-meta"), value, meta);
  }

  for (auto &constant : constantTable) {
    if (token == constant.token) {
      return constant.value;
    }
  }
  for (auto &macro : macroTable) {
    if (token == macro.token) {
      return processMacro(tokeniser, macro.symbol);
    }
  }

  if (std::regex_match(token, intRegex)) {
    return mal::number(token, true);
  }

  if (std::regex_match(token, floatRegex)) {
    return mal::number(token, false);
  }

  if (std::regex_match(token, hexRegex)) {
    return mal::numberHex(token);
  }

  return mal::symbol(token);
}

static void readList(Tokeniser &tokeniser, malValueVec *items,
                     const String &end) {
  while (1) {
    MAL_CHECK(!tokeniser.eof(), "expected '%s', got EOF, line: %i", end.c_str(),
              tokeniser.line());
    if (tokeniser.peek() == end) {
      tokeniser.next();
      return;
    }
    items->push_back(readForm(tokeniser));
  }
}

static malValuePtr processMacro(Tokeniser &tokeniser, const String &symbol) {
  return mal::list(mal::symbol(symbol), readForm(tokeniser));
}

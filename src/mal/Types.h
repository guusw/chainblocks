/* SPDX-License-Identifier: MPL-2.0 */
/* Copyright © 2019 Fragcolor Pte. Ltd. */
/* Copyright (C) 2015 Joel Martin <github@martintribe.org> */

#ifndef INCLUDE_TYPES_H
#define INCLUDE_TYPES_H

#include "MAL.h"

#include <exception>
#include <map>

class malEmptyInputException : public std::exception {};

class malValue : public RefCounted {
public:
  malValue() { TRACE_OBJECT("Creating malValue %p\n", this); }
  malValue(malValuePtr meta) : m_meta(meta) {
    TRACE_OBJECT("Creating malValue %p\n", this);
  }
  virtual ~malValue() { TRACE_OBJECT("Destroying malValue %p\n", this); }

  malValuePtr withMeta(malValuePtr meta) const;
  virtual malValuePtr doWithMeta(malValuePtr meta) const = 0;
  malValuePtr meta() const;

  bool isTrue() const;

  bool isEqualTo(const malValue *rhs) const;

  virtual malValuePtr eval(malEnvPtr env);

  virtual String print(bool readably) const = 0;

  size_t line{0};

protected:
  virtual bool doIsEqualTo(const malValue *rhs) const = 0;

  malValuePtr m_meta;
};

template <class T> T *value_cast(malValuePtr obj, const char *typeName) {
  T *dest = dynamic_cast<T *>(obj.ptr());
  MAL_CHECK(dest != NULL, "%s is not a %s, line: %u", obj->print(true).c_str(),
            typeName, obj->line);
  return dest;
}

#define VALUE_CAST(Type, Value) value_cast<Type>(Value, #Type)
#define DYNAMIC_CAST(Type, Value) (dynamic_cast<Type *>((Value).ptr()))
#define STATIC_CAST(Type, Value) (static_cast<Type *>((Value).ptr()))

#define WITH_META(Type)                                                        \
  virtual malValuePtr doWithMeta(malValuePtr meta) const {                     \
    return new Type(*this, meta);                                              \
  }

class malConstant : public malValue {
public:
  malConstant(String name) : m_name(name) {}
  malConstant(const malConstant &that, malValuePtr meta)
      : malValue(meta), m_name(that.m_name) {}

  virtual String print(bool readably) const { return m_name; }

  virtual bool doIsEqualTo(const malValue *rhs) const {
    return this == rhs; // these are singletons
  }

  WITH_META(malConstant);

private:
  const String m_name;
};

class malNumber : public malValue {
public:
  malNumber(double value, bool isInteger)
      : m_value(value), m_integer(isInteger) {}
  malNumber(const malNumber &that, malValuePtr meta)
      : malValue(meta), m_value(that.m_value), m_integer(that.m_integer) {}

  virtual String print(bool readably) const {
    if (m_integer)
      return std::to_string(static_cast<int64_t>(m_value));
    else
      return std::to_string(m_value);
  }

  double value() const { return m_value; }
  bool isInteger() const { return m_integer; }

  virtual bool doIsEqualTo(const malValue *rhs) const {
    return m_value == static_cast<const malNumber *>(rhs)->m_value;
  }

  WITH_META(malNumber);

private:
  const double m_value;
  const bool m_integer;
};

class malStringBase : public malValue {
public:
  malStringBase(const String &token) : m_value(token) {}
  malStringBase(const malStringBase &that, malValuePtr meta)
      : malValue(meta), m_value(that.value()) {}

  virtual String print(bool readably) const { return m_value; }

  String value() const { return m_value; }

  const String &ref() const { return m_value; }

private:
  const String m_value;
};

class malString : public malStringBase {
public:
  malString(const String &token) : malStringBase(token) {}
  malString(const malString &that, malValuePtr meta)
      : malStringBase(that, meta) {}

  virtual String print(bool readably) const;

  String escapedValue() const;

  virtual bool doIsEqualTo(const malValue *rhs) const {
    return value() == static_cast<const malString *>(rhs)->value();
  }

  WITH_META(malString);
};

class malKeyword : public malStringBase {
public:
  malKeyword(const String &token) : malStringBase(token) {}
  malKeyword(const malKeyword &that, malValuePtr meta)
      : malStringBase(that, meta) {}

  virtual bool doIsEqualTo(const malValue *rhs) const {
    return value() == static_cast<const malKeyword *>(rhs)->value();
  }

  WITH_META(malKeyword);
};

class malSymbol : public malStringBase {
public:
  malSymbol(const String &token) : malStringBase(token) {}
  malSymbol(const malSymbol &that, malValuePtr meta)
      : malStringBase(that, meta) {}

  virtual malValuePtr eval(malEnvPtr env);

  virtual bool doIsEqualTo(const malValue *rhs) const {
    return value() == static_cast<const malSymbol *>(rhs)->value();
  }

  WITH_META(malSymbol);
};

class malSequence : public malValue {
public:
  malSequence(malValueVec *items);
  malSequence(malValueIter begin, malValueIter end);
  malSequence(const malSequence &that, malValuePtr meta);
  virtual ~malSequence();

  virtual String print(bool readably) const;

  malValueVec *evalItems(malEnvPtr env) const;
  int count() const { return m_items->size(); }
  bool isEmpty() const { return m_items->empty(); }
  malValuePtr item(int index) const { return (*m_items)[index]; }

  malValueIter begin() const { return m_items->begin(); }
  malValueIter end() const { return m_items->end(); }

  virtual bool doIsEqualTo(const malValue *rhs) const;

  virtual malValuePtr conj(malValueIter argsBegin,
                           malValueIter argsEnd) const = 0;

  malValuePtr first() const;
  virtual malValuePtr rest() const;

private:
  malValueVec *const m_items;
};

class malList : public malSequence {
public:
  malList(malValueVec *items) : malSequence(items) {}
  malList(malValueIter begin, malValueIter end) : malSequence(begin, end) {}
  malList(const malList &that, malValuePtr meta) : malSequence(that, meta) {}

  virtual String print(bool readably) const;
  virtual malValuePtr eval(malEnvPtr env);

  virtual malValuePtr conj(malValueIter argsBegin, malValueIter argsEnd) const;

  WITH_META(malList);
};

class malVector : public malSequence {
public:
  malVector(malValueVec *items) : malSequence(items) {}
  malVector(malValueIter begin, malValueIter end) : malSequence(begin, end) {}
  malVector(const malVector &that, malValuePtr meta)
      : malSequence(that, meta) {}

  virtual malValuePtr eval(malEnvPtr env);
  virtual String print(bool readably) const;

  virtual malValuePtr conj(malValueIter argsBegin, malValueIter argsEnd) const;

  WITH_META(malVector);
};

class malApplicable : public malValue {
public:
  malApplicable() {}
  malApplicable(malValuePtr meta) : malValue(meta) {}

  virtual malValuePtr apply(malValueIter argsBegin,
                            malValueIter argsEnd) const = 0;
};

class malHash : public malValue {
public:
  typedef std::map<String, malValuePtr> Map;

  malHash(malValueIter argsBegin, malValueIter argsEnd, bool isEvaluated);
  malHash(const malHash::Map &map);
  malHash(const malHash &that, malValuePtr meta)
      : malValue(meta), m_map(that.m_map), m_isEvaluated(that.m_isEvaluated) {}

  malValuePtr assoc(malValueIter argsBegin, malValueIter argsEnd) const;
  malValuePtr dissoc(malValueIter argsBegin, malValueIter argsEnd) const;
  bool contains(malValuePtr key) const;
  malValuePtr eval(malEnvPtr env);
  malValuePtr get(malValuePtr key) const;
  malValuePtr keys() const;
  malValuePtr values() const;

  virtual String print(bool readably) const;

  virtual bool doIsEqualTo(const malValue *rhs) const;

  WITH_META(malHash);

  const Map m_map;

private:
  const bool m_isEvaluated;
};

class malBuiltIn : public malApplicable {
public:
  typedef malValuePtr(ApplyFunc)(const String &name, malValueIter argsBegin,
                                 malValueIter argsEnd);

  malBuiltIn(const String &name, ApplyFunc *handler)
      : m_name(name), m_handler(handler) {}

  malBuiltIn(const malBuiltIn &that, malValuePtr meta)
      : malApplicable(meta), m_name(that.m_name), m_handler(that.m_handler) {}

  virtual malValuePtr apply(malValueIter argsBegin, malValueIter argsEnd) const;

  virtual String print(bool readably) const {
    return STRF("#builtin-function(%s)", m_name.c_str());
  }

  virtual bool doIsEqualTo(const malValue *rhs) const {
    return this == rhs; // these are singletons
  }

  String name() const { return m_name; }

  WITH_META(malBuiltIn);

private:
  const String m_name;
  ApplyFunc *m_handler;
};

class malLambda : public malApplicable {
public:
  malLambda(const StringVec &bindings, malValuePtr body, malEnvPtr env);
  malLambda(const malLambda &that, malValuePtr meta);
  malLambda(const malLambda &that, bool isMacro);

  virtual malValuePtr apply(malValueIter argsBegin, malValueIter argsEnd) const;

  malValuePtr getBody() const { return m_body; }
  malEnvPtr makeEnv(malValueIter argsBegin, malValueIter argsEnd) const;

  virtual bool doIsEqualTo(const malValue *rhs) const {
    return this == rhs; // do we need to do a deep inspection?
  }

  virtual String print(bool readably) const {
    return STRF("#user-%s(%p)", m_isMacro ? "macro" : "function", this);
  }

  bool isMacro() const { return m_isMacro; }

  virtual malValuePtr doWithMeta(malValuePtr meta) const;

private:
  const StringVec m_bindings;
  const malValuePtr m_body;
  const malEnvPtr m_env;
  const bool m_isMacro;
};

class malAtom : public malValue {
public:
  malAtom(malValuePtr value) : m_value(value) {}
  malAtom(const malAtom &that, malValuePtr meta)
      : malValue(meta), m_value(that.m_value) {}

  virtual bool doIsEqualTo(const malValue *rhs) const {
    return this->m_value->isEqualTo(rhs);
  }

  virtual String print(bool readably) const {
    return "(atom " + m_value->print(readably) + ")";
  };

  malValuePtr deref() const { return m_value; }

  malValuePtr reset(malValuePtr value) { return m_value = value; }

  WITH_META(malAtom);

private:
  malValuePtr m_value;
};

namespace mal {
malValuePtr atom(malValuePtr value);
malValuePtr boolean(bool value);
malValuePtr builtin(const String &name, malBuiltIn::ApplyFunc handler);
malValuePtr falseValue();
malValuePtr hash(malValueIter argsBegin, malValueIter argsEnd,
                 bool isEvaluated);
malValuePtr hash(const malHash::Map &map);
malValuePtr number(double value, bool isInteger);
malValuePtr number(const String &token, bool isInteger);
malValuePtr numberHex(const String &token);
malValuePtr keyword(const String &token);
malValuePtr lambda(const StringVec &, malValuePtr, malEnvPtr);
malValuePtr list(malValueVec *items);
malValuePtr list(malValueIter begin, malValueIter end);
malValuePtr list(malValuePtr a);
malValuePtr list(malValuePtr a, malValuePtr b);
malValuePtr list(malValuePtr a, malValuePtr b, malValuePtr c);
malValuePtr macro(const malLambda &lambda);
malValuePtr nilValue();
malValuePtr string(const String &token);
malValuePtr symbol(const String &token);
malValuePtr trueValue();
malValuePtr vector(malValueVec *items);
malValuePtr vector(malValueIter begin, malValueIter end);
malValuePtr contextVar(const String &name);
}; // namespace mal

#endif // INCLUDE_TYPES_H

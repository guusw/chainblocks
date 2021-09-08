/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2020 Fragcolor Pte. Ltd. */

#include <random>

#include "../../include/ops.hpp"
#include "../../include/utility.hpp"
#include "../core/runtime.hpp"
#include <linalg_shim.hpp>

#undef CHECK

#define CATCH_CONFIG_RUNNER

#include <catch2/catch_all.hpp>

int main(int argc, char *argv[]) {
  chainblocks::GetGlobals().RootPath = "./";
  registerCoreBlocks();
  int result = Catch::Session().run(argc, argv);
  return result;
}

using namespace chainblocks;

struct Writer {
  std::vector<uint8_t> &_buffer;
  Writer(std::vector<uint8_t> &stream) : _buffer(stream) {}
  void operator()(const uint8_t *buf, size_t size) {
    _buffer.insert(_buffer.end(), buf, buf + size);
  }
};

struct Reader {
  const CBVar &_bytesVar;
  size_t _offset;
  Reader(const CBVar &var) : _bytesVar(var), _offset(0) {}
  void operator()(uint8_t *buf, size_t size) {
    if (_bytesVar.payload.bytesSize < _offset + size) {
      throw ActivationError("FromBytes buffer underrun");
    }

    memcpy(buf, _bytesVar.payload.bytesValue + _offset, size);
    _offset += size;
  }
};

#define TEST_SERIALIZATION(_source_)                                           \
  Serialization ws;                                                            \
  std::vector<uint8_t> buffer;                                                 \
  Writer w{buffer};                                                            \
  ws.serialize(_source_, w);                                                   \
  Var serialized(buffer.data(), buffer.size());                                \
  CBVar output{};                                                              \
  Serialization rs;                                                            \
  Reader r(serialized);                                                        \
  rs.reset();                                                                  \
  rs.deserialize(r, output);                                                   \
  REQUIRE(_source_ == output);                                                 \
  rs.varFree(output)

TEST_CASE("CBType-type2Name", "[ops]") {
  REQUIRE_THROWS(type2Name(CBType::EndOfBlittableTypes));
  REQUIRE(type2Name(CBType::None) == "None");
  REQUIRE(type2Name(CBType::Any) == "Any");
  REQUIRE(type2Name(CBType::Object) == "Object");
  REQUIRE(type2Name(CBType::Enum) == "Enum");
  REQUIRE(type2Name(CBType::Bool) == "Bool");
  REQUIRE(type2Name(CBType::Bytes) == "Bytes");
  REQUIRE(type2Name(CBType::Color) == "Color");
  REQUIRE(type2Name(CBType::Int) == "Int");
  REQUIRE(type2Name(CBType::Int2) == "Int2");
  REQUIRE(type2Name(CBType::Int3) == "Int3");
  REQUIRE(type2Name(CBType::Int4) == "Int4");
  REQUIRE(type2Name(CBType::Int8) == "Int8");
  REQUIRE(type2Name(CBType::Int16) == "Int16");
  REQUIRE(type2Name(CBType::Float) == "Float");
  REQUIRE(type2Name(CBType::Float2) == "Float2");
  REQUIRE(type2Name(CBType::Float3) == "Float3");
  REQUIRE(type2Name(CBType::Float4) == "Float4");
  REQUIRE(type2Name(CBType::Block) == "Block");
  REQUIRE(type2Name(CBType::String) == "String");
  REQUIRE(type2Name(CBType::ContextVar) == "ContextVar");
  REQUIRE(type2Name(CBType::Path) == "Path");
  REQUIRE(type2Name(CBType::Image) == "Image");
  REQUIRE(type2Name(CBType::Seq) == "Seq");
  REQUIRE(type2Name(CBType::Table) == "Table");
  REQUIRE(type2Name(CBType::Set) == "Set");
  REQUIRE(type2Name(CBType::Array) == "Array");
  REQUIRE(type2Name(CBType::Audio) == "Audio");
}

TEST_CASE("CBVar-comparison", "[ops]") {
  SECTION("None, Any, EndOfBlittableTypes") {
    auto n1 = Var::Empty;
    auto n2 = Var::Empty;
    auto n3 = Var(100);
    REQUIRE(n1 == n2);
    REQUIRE(n1 != n3);
    auto hash1 = hash(n1);
    auto hash2 = hash(n2);
    REQUIRE(hash1 == hash2);
    CBLOG_INFO(n1); // logging coverage
    TEST_SERIALIZATION(n1);
  }

  SECTION("Bool") {
    auto t = Var::True;
    auto f = Var::False;
    auto t1 = Var(true);
    auto f1 = Var(false);
    REQUIRE(t == t1);
    REQUIRE(f == f1);
    REQUIRE(true == bool(t1));
    REQUIRE_FALSE(true == bool(f1));
    REQUIRE(false == bool(f1));
    REQUIRE(t > f);
    REQUIRE(t >= f);
    REQUIRE(f < t);
    REQUIRE(f <= t);
    auto hash1 = hash(t);
    auto hash2 = hash(t1);
    REQUIRE(hash1 == hash2);
    CBLOG_INFO(t); // logging coverage
    TEST_SERIALIZATION(t);
  }

  SECTION("Object") {
    int x = 10;
    int y = 20;
    auto o1 = Var::Object(&x, 10, 20);
    auto o2 = Var::Object(&x, 10, 20);
    auto o3 = Var::Object(&y, 10, 20);
    auto o4 = Var::Object(&y, 11, 20);
    auto empty = Var::Empty;
    REQUIRE(o1 == o2);
    REQUIRE(o1 != o3);
    REQUIRE(o3 != o4);
    REQUIRE_THROWS(o3 > o4);
    REQUIRE_THROWS(o3 >= o4);
    REQUIRE(o1 != empty);
    auto hash1 = hash(o1);
    auto hash2 = hash(o2);
    auto hash3 = hash(o3);
    REQUIRE(hash1 == hash2);
    REQUIRE_FALSE(hash1 == hash3);
    CBLOG_INFO(o1); // logging coverage
  }

  SECTION("Audio") {
    std::vector<float> b1(1024);
    std::vector<float> b2(1024);
    std::vector<float> b3(1024);
    CBVar x{.payload = {.audioValue = {44100, 512, 2, b1.data()}},
            .valueType = CBType::Audio};
    CBVar y{.payload = {.audioValue = {44100, 512, 2, b2.data()}},
            .valueType = CBType::Audio};
    CBVar z{.payload = {.audioValue = {44100, 1024, 1, b3.data()}},
            .valueType = CBType::Audio};
    auto empty = Var::Empty;
    REQUIRE(x == y);
    REQUIRE(x != z);
    REQUIRE(y != z);
    REQUIRE_THROWS(x > y);
    REQUIRE_THROWS(x >= z);
    REQUIRE(x != empty);
    auto hash1 = hash(x);
    auto hash2 = hash(y);
    auto hash3 = hash(z);
    REQUIRE(hash1 == hash2);
    REQUIRE_FALSE(hash1 == hash3);
    CBLOG_INFO(x); // logging coverage
    CBVar xx{};
    cloneVar(xx, x);
    TEST_SERIALIZATION(xx);
    destroyVar(xx);
  }

  SECTION("Enum") {
    int x = 10;
    int y = 20;
    auto o1 = Var::Enum(x, 10, 20);
    auto o2 = Var::Enum(x, 10, 20);
    auto o3 = Var::Enum(y, 10, 20);
    auto o4 = Var::Enum(y, 11, 20);
    auto empty = Var::Empty;
    REQUIRE(o1 == o2);
    REQUIRE(o1 != o3);
    REQUIRE(o3 != o4);
    REQUIRE(o1 < o3);
    REQUIRE_THROWS(o3 < o4);
    REQUIRE(o1 <= o3);
    REQUIRE_THROWS(o3 <= o4);
    REQUIRE_FALSE(o1 >= o3);
    REQUIRE(o3 >= o1);
    REQUIRE_THROWS(o3 >= o4);
    REQUIRE_THROWS(o4 >= o3);
    REQUIRE(o1 != empty);
    auto hash1 = hash(o1);
    auto hash2 = hash(o2);
    auto hash3 = hash(o3);
    REQUIRE(hash1 == hash2);
    REQUIRE_FALSE(hash1 == hash3);
    CBLOG_INFO(o1); // logging coverage
    TEST_SERIALIZATION(o1);
  }

  SECTION("Float") {
    auto f1 = Var(10.0);
    REQUIRE(f1.valueType == CBType::Float);
    float ff = float(f1);
    REQUIRE(ff == 10);
    REQUIRE_THROWS(int(f1));
    auto f2 = Var(10.0);
    auto f3 = Var(22.0);
    auto i1 = Var(10);
    REQUIRE(f1 == f2);
    REQUIRE(f1 != f3);
    REQUIRE(f1 != i1);
    REQUIRE(f1 <= f2);
    REQUIRE(f1 <= f3);
    REQUIRE(f1 >= f2);
    REQUIRE_FALSE(f1 >= f3);
    REQUIRE(f1 < f3);
    REQUIRE((f3 > f1 and f3 > f2));
    REQUIRE((f3 >= f1 and f3 >= f2));
    auto hash1 = hash(f1);
    auto hash2 = hash(f2);
    auto hash3 = hash(f3);
    REQUIRE(hash1 == hash2);
    REQUIRE_FALSE(hash1 == hash3);
    CBLOG_INFO(f1); // logging coverage
    TEST_SERIALIZATION(f1);
  }

#define CBVECTOR_TESTS                                                         \
  REQUIRE(f1 == f2);                                                           \
  REQUIRE(v1 == v2);                                                           \
  REQUIRE(f1 != f3);                                                           \
  REQUIRE(v1 != v3);                                                           \
  REQUIRE(f1 != i1);                                                           \
  REQUIRE_THROWS(f1 < i1);                                                     \
  REQUIRE_THROWS(f1 <= i1);                                                    \
  REQUIRE(f1 <= f2);                                                           \
  REQUIRE(v1 <= v2);                                                           \
  REQUIRE_FALSE(f3 <= f1);                                                     \
  REQUIRE_FALSE(v3 <= v1);                                                     \
  REQUIRE(f1 < f4);                                                            \
  REQUIRE(v1 < v4);                                                            \
  REQUIRE(f1 >= f2);                                                           \
  REQUIRE(v1 >= v2);                                                           \
  REQUIRE(f1 < f3);                                                            \
  REQUIRE(v1 < v3);                                                            \
  REQUIRE((f3 > f1 and f3 > f2));                                              \
  REQUIRE((v3 > v1 and v3 > v2));                                              \
  REQUIRE((f4 >= f1 and f4 >= f2));                                            \
  REQUIRE((v4 >= v1 and v4 >= v2));                                            \
  REQUIRE_FALSE(f3 < f1);                                                      \
  REQUIRE_FALSE(v3 < v1);                                                      \
  auto hash1 = hash(f1);                                                       \
  auto hash2 = hash(f2);                                                       \
  auto hash3 = hash(f3);                                                       \
  REQUIRE(hash1 == hash2);                                                     \
  REQUIRE_FALSE(hash1 == hash3);                                               \
  CBLOG_INFO(f1);                                                              \
  TEST_SERIALIZATION(f1)

  SECTION("Float2") {
    auto f1 = Var(10.0, 2.0);
    std::vector<double> v1{10.0, 2.0};
    auto f2 = Var(10.0, 2.0);
    std::vector<double> v2{10.0, 2.0};
    auto f3 = Var(22.0, 2.0);
    std::vector<double> v3{22.0, 2.0};
    auto f4 = Var(22.0, 2.0);
    std::vector<double> v4{22.0, 2.0};
    auto i1 = Var(10);
    REQUIRE(f1.valueType == CBType::Float2);
    CBVECTOR_TESTS;
  }

  SECTION("Float3") {
    auto f1 = Var(10.0, 2.0, 3.0);
    std::vector<float> v1{10.0, 2.0, 3.0};
    auto f2 = Var(10.0, 2.0, 3.0);
    std::vector<float> v2{10.0, 2.0, 3.0};
    auto f3 = Var(22.0, 2.0, 1.0);
    std::vector<float> v3{22.0, 2.0, 1.0};
    auto f4 = Var(22.0, 2.0, 4.0);
    std::vector<float> v4{22.0, 2.0, 4.0};
    auto i1 = Var(10);
    REQUIRE(f1.valueType == CBType::Float3);
    CBVECTOR_TESTS;
  }

  SECTION("Float4") {
    auto f1 = Var(10.0, 2.0, 3.0, 3.0);
    std::vector<float> v1{10.0, 2.0, 3.0, 3.0};
    auto f2 = Var(10.0, 2.0, 3.0, 3.0);
    std::vector<float> v2{10.0, 2.0, 3.0, 3.0};
    auto f3 = Var(22.0, 2.0, 1.0, 3.0);
    std::vector<float> v3{22.0, 2.0, 1.0, 3.0};
    auto f4 = Var(22.0, 2.0, 4.0, 3.0);
    std::vector<float> v4{22.0, 2.0, 4.0, 3.0};
    auto i1 = Var(10);
    REQUIRE(f1.valueType == CBType::Float4);
    CBVECTOR_TESTS;
  }

  SECTION("Int") {
    auto f1 = Var(10);
    REQUIRE(f1.valueType == CBType::Int);
    int ff = int(f1);
    REQUIRE(ff == 10);
    REQUIRE_NOTHROW(float(f1)); // will convert to float automatically
    auto f2 = Var(10);
    auto f3 = Var(22);
    auto i1 = Var(10.0);
    REQUIRE(f1 == f2);
    REQUIRE(f1 != f3);
    REQUIRE(f1 != i1);
    REQUIRE(f1 <= f2);
    REQUIRE(f1 <= f3);
    REQUIRE(f1 >= f2);
    REQUIRE_FALSE(f1 >= f3);
    REQUIRE(f1 < f3);
    REQUIRE((f3 > f1 and f3 > f2));
    REQUIRE((f3 >= f1 and f3 >= f2));
    auto hash1 = hash(f1);
    auto hash2 = hash(f2);
    auto hash3 = hash(f3);
    REQUIRE(hash1 == hash2);
    REQUIRE_FALSE(hash1 == hash3);
    CBLOG_INFO(f1); // logging coverage
  }

  SECTION("Int2") {
    auto f1 = Var(10, 2);
    std::vector<int64_t> v1{10, 2};
    auto f2 = Var(10, 2);
    std::vector<int64_t> v2{10, 2};
    auto f3 = Var(22, 2);
    std::vector<int64_t> v3{22, 2};
    auto f4 = Var(22, 2);
    std::vector<int64_t> v4{22, 2};
    auto i1 = Var(10.0);
    REQUIRE(f1.valueType == CBType::Int2);
    CBVECTOR_TESTS;
  }

  SECTION("Int3") {
    auto f1 = Var(10, 2, 3);
    std::vector<int32_t> v1{10, 2, 3};
    auto f2 = Var(10, 2, 3);
    std::vector<int32_t> v2{10, 2, 3};
    auto f3 = Var(22, 2, 1);
    std::vector<int32_t> v3{22, 2, 1};
    auto f4 = Var(22, 2, 4);
    std::vector<int32_t> v4{22, 2, 4};
    auto i1 = Var(10);
    REQUIRE(f1.valueType == CBType::Int3);
    CBVECTOR_TESTS;
  }

  SECTION("Int4") {
    auto f1 = Var(10, 2, 3, 0);
    std::vector<int32_t> v1{10, 2, 3, 0};
    auto f2 = Var(10, 2, 3, 0);
    std::vector<int32_t> v2{10, 2, 3, 0};
    auto f3 = Var(22, 2, 1, 0);
    std::vector<int32_t> v3{22, 2, 1, 0};
    auto f4 = Var(22, 2, 4, 0);
    std::vector<int32_t> v4{22, 2, 4, 0};
    auto i1 = Var(10);
    REQUIRE(f1.valueType == CBType::Int4);
    CBVECTOR_TESTS;
  }

  SECTION("Int8") {
    auto f1 = Var(10, 2, 3, 0, 1, 2, 3, 4);
    std::vector<int16_t> v1{10, 2, 3, 0, 1, 2, 3, 4};
    auto f2 = Var(10, 2, 3, 0, 1, 2, 3, 4);
    std::vector<int16_t> v2{10, 2, 3, 0, 1, 2, 3, 4};
    auto f3 = Var(22, 2, 1, 0, 1, 2, 3, 4);
    std::vector<int16_t> v3{22, 2, 1, 0, 1, 2, 3, 4};
    auto f4 = Var(22, 2, 4, 0, 1, 2, 3, 4);
    std::vector<int16_t> v4{22, 2, 4, 0, 1, 2, 3, 4};
    auto i1 = Var(10);
    REQUIRE(f1.valueType == CBType::Int8);
    CBVECTOR_TESTS;
  }

  SECTION("Int16") {
    auto f1 = Var(10, 2, 3, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2);
    std::vector<int8_t> v1{10, 2, 3, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2};
    auto f2 = Var(10, 2, 3, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2);
    std::vector<int8_t> v2{10, 2, 3, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2};
    auto f3 = Var(22, 2, 1, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2);
    std::vector<int8_t> v3{22, 2, 1, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2};
    auto f4 = Var(22, 2, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2);
    std::vector<int8_t> v4{22, 2, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2};
    auto i1 = Var(10);
    REQUIRE(f1.valueType == CBType::Int16);
    CBVECTOR_TESTS;
  }

#undef CBVECTOR_TESTS

  SECTION("Color") {
    auto c1 = Var(CBColor{20, 20, 20, 255});
    auto c2 = Var(CBColor{20, 20, 20, 255});
    auto c3 = Var(CBColor{20, 20, 20, 0});
    REQUIRE(c1 == c2);
    REQUIRE(c2 >= c1);
    REQUIRE(c1 != c3);
    REQUIRE(c1 > c3);
    REQUIRE(c1 >= c3);
    REQUIRE(c3 < c1);
    REQUIRE(c3 <= c1);
    auto hash1 = hash(c1);
    auto hash2 = hash(c2);
    auto hash3 = hash(c3);
    REQUIRE(hash1 == hash2);
    REQUIRE_FALSE(hash1 == hash3);
    CBLOG_INFO(c1);
  }

  SECTION("String") {
    auto s1 = Var("Hello world");
    REQUIRE(s1.valueType == CBType::String);
    auto s2 = Var("Hello world");
    auto s3 = Var("Hello world 2");
    auto s5 = CBVar();
    s5.valueType = CBType::String;
    s5.payload.stringValue = "Hello world";
    std::string q("qwerty");
    auto s4 = Var(q);
    REQUIRE_THROWS(float(s4));
    REQUIRE(s1 == s2);
    REQUIRE(s1 == s5);
    REQUIRE(s2 == s5);
    REQUIRE(s2 <= s5);
    REQUIRE_FALSE(s2 < s5);
    REQUIRE(s1 != s3);
    REQUIRE(s1 != s4);
    REQUIRE(s1 < s3);
    REQUIRE(s1 <= s3);
    REQUIRE(s3 >= s2);
    REQUIRE(s3 > s2);
    auto hash1 = hash(s1);
    auto hash2 = hash(s2);
    auto hash3 = hash(s3);
    REQUIRE(hash1 == hash2);
    REQUIRE_FALSE(hash1 == hash3);
    CBLOG_INFO(s1); // logging coverage
  }

  SECTION("std::vector-Var") {
    std::vector<Var> s1{Var(10), Var(20), Var(30), Var(40), Var(50)};
    Var v1(s1);
    REQUIRE(v1.valueType == CBType::Seq);
    std::vector<Var> s2{Var(10), Var(20), Var(30), Var(40), Var(50)};
    Var v2(s2);
    REQUIRE(v2.valueType == CBType::Seq);
    std::vector<Var> s3{Var(100), Var(200), Var(300)};
    Var v3(s3);
    REQUIRE(v3.valueType == CBType::Seq);
    std::vector<Var> s4{Var(10), Var(20), Var(30)};
    Var v4(s4);
    REQUIRE(v4.valueType == CBType::Seq);
    std::vector<Var> s5{Var(1), Var(2), Var(3), Var(4), Var(5), Var(6)};
    Var v5(s5);
    REQUIRE(v5.valueType == CBType::Seq);
    std::vector<Var> s6{Var(10), Var(20), Var(30), Var(45), Var(55)};
    Var v6(s6);
    REQUIRE(v6.valueType == CBType::Seq);
    std::vector<Var> s7{Var(10), Var(20), Var(30), Var(25), Var(35)};
    Var v7(s7);
    std::vector<Var> s77 = std::vector<Var>(v7);
    std::vector<int> i77 = std::vector<int>(v7);
    REQUIRE_THROWS(([&]() {
      std::vector<bool> f77 = std::vector<bool>(v7);
      return true;
    }()));
    REQUIRE_THROWS(([&]() {
      Var empty{};
      std::vector<Var> f77 = std::vector<Var>(empty);
      return true;
    }()));
    REQUIRE(i77[2] == 30);
    REQUIRE(s7 == s77);
    REQUIRE(v7.valueType == CBType::Seq);
    std::vector<Var> s8{Var(10)};
    Var v8(s8);
    REQUIRE(v8.valueType == CBType::Seq);
    std::vector<Var> s9{Var(1)};
    Var v9(s9);
    REQUIRE(v9.valueType == CBType::Seq);

    REQUIRE_FALSE(v8 < v9);
    REQUIRE_FALSE(v8 <= v9);
    REQUIRE_FALSE(s8 < s9);
    REQUIRE_FALSE(s8 <= s9);
    REQUIRE(s1 == s2);
    REQUIRE(v1 == v2);
    REQUIRE_FALSE(v1 < v2);
    REQUIRE(v1 <= v2);
    REQUIRE_FALSE(s4 <= s8);
    REQUIRE_FALSE(v4 <= v8);
    REQUIRE(s1 <= s2);
    REQUIRE(s1 <= s2);
    REQUIRE(s1 != s5);
    REQUIRE(v1 != v5);
    REQUIRE(s1 != s6);
    REQUIRE(v1 != v6);
    REQUIRE(s1 != s3);
    REQUIRE(v1 != v3);
    REQUIRE(s2 != s3);
    REQUIRE(v2 != v3);
    REQUIRE(s3 > s1);
    REQUIRE(v3 > v1);
    REQUIRE(s1 < s3);
    REQUIRE(v1 < v3);
    REQUIRE(s2 > s4);
    REQUIRE(v2 > v4);
    REQUIRE(s2 > s5);
    REQUIRE(v2 > v5);
    REQUIRE(s3 >= s1);
    REQUIRE(v3 >= v1);
    REQUIRE(s1 <= s3);
    REQUIRE(v1 <= v3);
    REQUIRE(s2 >= s4);
    REQUIRE(v2 >= v4);
    REQUIRE(s2 >= s5);
    REQUIRE(v2 >= v5);
    REQUIRE(v6 > v1);
    REQUIRE(s6 > s1);
    REQUIRE(v6 >= v1);
    REQUIRE(s6 >= s1);
    REQUIRE(v1 < v6);
    REQUIRE(s1 < s6);
    REQUIRE(v1 <= v6);
    REQUIRE(s1 <= s6);
    REQUIRE_FALSE(v6 < v1);
    REQUIRE_FALSE(s6 < s1);
    REQUIRE_FALSE(v6 <= v1);
    REQUIRE_FALSE(s6 <= s1);
    REQUIRE_FALSE(v1 > v6);
    REQUIRE_FALSE(s1 > s6);
    REQUIRE_FALSE(v1 >= v6);
    REQUIRE_FALSE(s1 >= s6);
    REQUIRE_FALSE(v7 > v1);
    REQUIRE_FALSE(s7 > s1);
    REQUIRE_FALSE(v7 >= v1);
    REQUIRE_FALSE(s7 >= s1);
    REQUIRE_FALSE(v1 < v7);
    REQUIRE_FALSE(s1 < s7);
    REQUIRE_FALSE(v1 <= v7);
    REQUIRE_FALSE(s1 <= s7);
    REQUIRE(v7 < v1);
    REQUIRE(s7 < s1);
    REQUIRE(v7 <= v1);
    REQUIRE(s7 <= s1);
    REQUIRE(v1 > v7);
    REQUIRE(s1 > s7);
    REQUIRE(v1 >= v7);
    REQUIRE(s1 >= s7);

    auto hash1 = hash(v1);
    auto hash2 = hash(v2);
    auto hash3 = hash(v3);
    REQUIRE(hash1 == hash2);
    REQUIRE_FALSE(hash1 == hash3);

    CBLOG_INFO(v1);
  }

  SECTION("std::vector-CBVar") {
    std::vector<CBVar> s1{Var(10), Var(20), Var(30), Var(40), Var(50)};
    Var v1(s1);
    REQUIRE(v1.valueType == CBType::Seq);
    std::vector<CBVar> s2{Var(10), Var(20), Var(30), Var(40), Var(50)};
    Var v2(s2);
    REQUIRE(v2.valueType == CBType::Seq);
    std::vector<CBVar> s3{Var(100), Var(200), Var(300)};
    Var v3(s3);
    REQUIRE(v3.valueType == CBType::Seq);
    std::vector<CBVar> s4{Var(10), Var(20), Var(30)};
    Var v4(s4);
    REQUIRE(v4.valueType == CBType::Seq);
    std::vector<CBVar> s5{Var(1), Var(2), Var(3), Var(4), Var(5), Var(6)};
    Var v5(s5);
    REQUIRE(v5.valueType == CBType::Seq);

    REQUIRE(s1 == s2);
    REQUIRE(v1 == v2);
    REQUIRE(s1 != s3);
    REQUIRE(v1 != v3);
    REQUIRE(s2 != s3);
    REQUIRE(v2 != v3);
    REQUIRE(s3 > s1);
    REQUIRE(v3 > v1);
    REQUIRE(s1 < s3);
    REQUIRE(v1 < v3);
    REQUIRE(s2 > s4);
    REQUIRE(v2 > v4);
    REQUIRE(s2 > s5);
    REQUIRE(v2 > v5);
    REQUIRE(s3 >= s1);
    REQUIRE(v3 >= v1);
    REQUIRE(s1 <= s3);
    REQUIRE(v1 <= v3);
    REQUIRE(s2 >= s4);
    REQUIRE(v2 >= v4);
    REQUIRE(s2 >= s5);
    REQUIRE(v2 >= v5);

    CBLOG_INFO(v1);
  }

  SECTION("std::array-Var") {
    std::array<Var, 5> s1{Var(10), Var(20), Var(30), Var(40), Var(50)};
    Var v1(s1);
    REQUIRE(v1.valueType == CBType::Seq);
    std::array<Var, 5> s2{Var(10), Var(20), Var(30), Var(40), Var(50)};
    Var v2(s2);
    REQUIRE(v2.valueType == CBType::Seq);
    std::array<Var, 3> s3{Var(100), Var(200), Var(300)};
    Var v3(s3);
    REQUIRE(v3.valueType == CBType::Seq);
    std::array<Var, 3> s4{Var(10), Var(20), Var(30)};
    Var v4(s4);
    REQUIRE(v4.valueType == CBType::Seq);
    std::array<Var, 6> s5{Var(1), Var(2), Var(3), Var(4), Var(5), Var(6)};
    Var v5(s5);
    REQUIRE(v5.valueType == CBType::Seq);

    REQUIRE(v1 == v2);
    REQUIRE(v1 != v3);
    REQUIRE(v2 != v3);
    REQUIRE(v3 > v1);
    REQUIRE(v1 < v3);
    REQUIRE(v2 > v4);
    REQUIRE(v2 > v5);
    REQUIRE(v3 >= v1);
    REQUIRE(v1 <= v3);
    REQUIRE(v2 >= v4);
    REQUIRE(v2 >= v5);

    CBLOG_INFO(v1);
  }

  SECTION("std::array-CBVar") {
    std::array<CBVar, 5> s1{Var(10), Var(20), Var(30), Var(40), Var(50)};
    Var v1(s1);
    REQUIRE(v1.valueType == CBType::Seq);
    std::array<CBVar, 5> s2{Var(10), Var(20), Var(30), Var(40), Var(50)};
    Var v2(s2);
    REQUIRE(v2.valueType == CBType::Seq);
    std::array<CBVar, 3> s3{Var(100), Var(200), Var(300)};
    Var v3(s3);
    REQUIRE(v3.valueType == CBType::Seq);
    std::array<CBVar, 3> s4{Var(10), Var(20), Var(30)};
    Var v4(s4);
    REQUIRE(v4.valueType == CBType::Seq);
    std::array<CBVar, 6> s5{Var(1), Var(2), Var(3), Var(4), Var(5), Var(6)};
    Var v5(s5);
    REQUIRE(v5.valueType == CBType::Seq);

    REQUIRE(v1 == v2);
    REQUIRE(v1 != v3);
    REQUIRE(v2 != v3);
    REQUIRE(v3 > v1);
    REQUIRE(v1 < v3);
    REQUIRE(v2 > v4);
    REQUIRE(v2 > v5);
    REQUIRE(v3 >= v1);
    REQUIRE(v1 <= v3);
    REQUIRE(v2 >= v4);
    REQUIRE(v2 >= v5);

    CBLOG_INFO(v1);
  }

  SECTION("Bytes") {
    std::random_device rd;
    std::uniform_int_distribution<uint8_t> dist(0, 0x07);

    std::vector<uint8_t> data1(1024);
    for (auto &b : data1) {
      b = dist(rd);
    }
    Var v1{&data1.front(), uint32_t(data1.size())};

    auto data2 = data1;
    Var v2{&data2.front(), uint32_t(data2.size())};

    std::vector<uint8_t> data3(1024);
    for (auto &b : data3) {
      b = dist(rd);
    }
    Var v3{&data3.front(), uint32_t(data3.size())};

    std::vector<uint8_t> data4(1050);
    for (auto &b : data4) {
      b = dist(rd);
    }
    Var v4{&data4.front(), uint32_t(data4.size())};

    std::vector<uint8_t> data5(200);
    for (auto &b : data5) {
      b = dist(rd);
    }
    Var v5{&data5.front(), uint32_t(data5.size())};

    REQUIRE((&data1[0]) != (&data2[0]));
    REQUIRE(data1.size() == 1024);

    REQUIRE(v1.valueType == CBType::Bytes);

    REQUIRE(data1 == data2);
    REQUIRE(v1 == v2);

    REQUIRE(data1 != data3);
    REQUIRE(v1 != v3);

    REQUIRE(data1 != data3);
    REQUIRE(v1 != v3);

    REQUIRE(data1 >= data2);
    REQUIRE(v1 >= v2);

    REQUIRE_FALSE(data1 < data2);
    REQUIRE_FALSE(v1 < v2);

    REQUIRE((v1 > v5) == (data1 > data5));
    REQUIRE((v1 > v4) == (data1 > data4));

    REQUIRE(v1 == v1);
    REQUIRE(v1 <= v1);
    REQUIRE_FALSE(v1 < v1);

    auto hash1 = hash(v1);
    auto hash2 = hash(v2);
    auto hash3 = hash(v3);
    REQUIRE(hash1 == hash2);
    REQUIRE_FALSE(hash1 == hash3);

    CBLOG_INFO(v1);
  }

  SECTION("Block") {
    CBlock foo;
    CBlock bar;
    auto b1 = CBVar();
    b1.valueType = CBType::Block;
    b1.payload.blockValue = &foo;
    auto b2 = CBVar();
    b2.valueType = CBType::Block;
    b2.payload.blockValue = &bar;
    REQUIRE_FALSE(b1 == b2);
    REQUIRE(b1 != b2);
  }
}

TEST_CASE("VarPayload") {
  SECTION("Int2") {
    VarPayload a1 = Int2VarPayload({2, 1});
    Var v1(std::get<Int2VarPayload>(a1));
    Int2VarPayload a2({2, 1});
    Var v2(a2);
    Var v3(2, 1);
    Int2VarPayload a4 = {2, 1};
    Var v4(a4);

    REQUIRE(v1 == v2);
    REQUIRE(v1 == v3);
    REQUIRE(v2 == v3);
    REQUIRE(v1 == v4);
  }

  SECTION("Float4") {
    VarPayload a1 = Float4VarPayload({2.0, 1.0, 3.0, 9.0});
    Var v1(std::get<Float4VarPayload>(a1));
    Float4VarPayload a2({2.0, 1.0, 3.0, 9.0});
    Var v2(a2);
    Var v3(2.0, 1.0, 3.0, 9.0);
    Float4VarPayload a4 = {2.0, 1.0, 3.0, 9.0};
    Var v4(a4);

    REQUIRE(v1 == v2);
    REQUIRE(v1 == v3);
    REQUIRE(v2 == v3);
    REQUIRE(v1 == v4);
  }
}

#define SET_TABLE_COMMON_TESTS                                                 \
  CBInstanceData data{};                                                       \
  auto hash1 = hash(vxx);                                                      \
  auto hash2 = hash(vx);                                                       \
  auto hash3 = hash(vyy);                                                      \
  REQUIRE(hash1 == hash2);                                                     \
  REQUIRE(hash1 == hash3);                                                     \
  auto typeHash1 = deriveTypeHash(vxx);                                        \
  auto typeInfo1 = deriveTypeInfo(vxx, data);                                  \
  auto typeInfo2 = deriveTypeInfo(vx, data);                                   \
  auto typeInfo3 = deriveTypeInfo(Var::Empty, data);                           \
  REQUIRE(deriveTypeHash(typeInfo1) == typeHash1);                             \
  auto stypeHash = std::hash<CBTypeInfo>()(typeInfo1);                         \
  REQUIRE(stypeHash != 0);                                                     \
  REQUIRE(typeInfo1 == typeInfo2);                                             \
  REQUIRE(typeInfo1 != typeInfo3);                                             \
  freeDerivedInfo(typeInfo1);                                                  \
  freeDerivedInfo(typeInfo2);                                                  \
  freeDerivedInfo(typeInfo3);                                                  \
  auto typeHash2 = deriveTypeHash(vx);                                         \
  auto typeHash3 = deriveTypeHash(vyy);                                        \
  CBLOG_INFO("{} - {} - {}", typeHash1, typeHash2, typeHash3);                 \
  REQUIRE(typeHash1 == typeHash2);                                             \
  REQUIRE(typeHash1 == typeHash3);                                             \
  vyy = vz;                                                                    \
  REQUIRE(vyy != vy);                                                          \
  auto typeHash4 = deriveTypeHash(vyy);                                        \
  REQUIRE(typeHash4 == typeHash3);                                             \
  auto typeHash5 = deriveTypeHash(vz);                                         \
  REQUIRE(typeHash4 == typeHash5)

TEST_CASE("CBMap") {
  CBMap x;
  x.emplace("x", Var(10));
  x.emplace("y", Var("Hello Set"));
  CBVar vx{};
  vx.valueType = CBType::Table;
  vx.payload.tableValue.opaque = &x;
  vx.payload.tableValue.api = &GetGlobals().TableInterface;

  CBMap y;
  y.emplace("y", Var("Hello Set"));
  y.emplace("x", Var(10));

  CBVar vy{};
  vy.valueType = CBType::Table;
  vy.payload.tableValue.opaque = &y;
  vy.payload.tableValue.api = &GetGlobals().TableInterface;

  CBMap z;
  z.emplace("y", Var("Hello Set"));
  z.emplace("x", Var(11));

  CBVar vz{};
  vz.valueType = CBType::Table;
  vz.payload.tableValue.opaque = &z;
  vz.payload.tableValue.api = &GetGlobals().TableInterface;

  REQUIRE(vx == vy);

  OwnedVar vxx = vx;
  OwnedVar vyy = vy;
  REQUIRE(vxx == vyy);

  int items = 0;
  ForEach(vxx.payload.tableValue, [&](auto k, auto &v) {
    REQUIRE(vx.payload.tableValue.api->tableContains(vx.payload.tableValue, k));
    items++;
  });
  REQUIRE(items == 2);

  SET_TABLE_COMMON_TESTS;

  REQUIRE(x.size() == 2);
  REQUIRE(x.count("x") == 1);
  x.erase("x");
  REQUIRE(x.count("x") == 0);
  REQUIRE(x.count("y") == 1);
  x.erase("y");
  REQUIRE(x.count("y") == 0);

  REQUIRE(vx != vy);
}

TEST_CASE("CBHashSet") {
  CBHashSet x;
  x.insert(Var(10));
  x.emplace(Var("Hello Set"));
  CBVar vx{};
  vx.valueType = CBType::Set;
  vx.payload.setValue.opaque = &x;
  vx.payload.setValue.api = &GetGlobals().SetInterface;

  CBHashSet y;
  y.emplace(Var("Hello Set"));
  y.insert(Var(10));

  CBVar vy{};
  vy.valueType = CBType::Set;
  vy.payload.setValue.opaque = &y;
  vy.payload.setValue.api = &GetGlobals().SetInterface;

  CBHashSet z;
  z.emplace(Var("Hello Set"));
  z.insert(Var(11));

  CBVar vz{};
  vz.valueType = CBType::Set;
  vz.payload.setValue.opaque = &z;
  vz.payload.setValue.api = &GetGlobals().SetInterface;

  REQUIRE(vx == vy);
  REQUIRE(vx != vz);

  OwnedVar vxx = vx;
  OwnedVar vyy = vy;
  REQUIRE(vxx == vyy);

  int items = 0;
  ForEach(vxx.payload.setValue, [&](auto &v) {
    REQUIRE(vx.payload.setValue.api->setContains(vx.payload.setValue, v));
    items++;
  });
  REQUIRE(items == 2);

  SET_TABLE_COMMON_TESTS;

  REQUIRE(x.size() == 2);
  REQUIRE(x.count(Var(10)) == 1);
  x.erase(Var(10));
  REQUIRE(x.count(Var(10)) == 0);
  REQUIRE(x.count(Var("Hello Set")) == 1);
  x.erase(Var("Hello Set"));
  REQUIRE(x.count(Var("Hello Set")) == 0);

  REQUIRE(vx != vy);
}

TEST_CASE("CXX-Chain-DSL") {
  // TODO, improve this
  auto chain = chainblocks::Chain("test-chain")
                   .looped(true)
                   .let(1)
                   .block("Log")
                   .block("Math.Add", 2)
                   .block("Assert.Is", 3, true);
  assert(chain->blocks.size() == 4);
}

TEST_CASE("DynamicArray") {
  CBSeq ts{};
  Var a{0}, b{1}, c{2}, d{3}, e{4}, f{5};
  arrayPush(ts, a);
  assert(ts.len == 1);
  assert(ts.cap == 4);
  arrayPush(ts, b);
  arrayPush(ts, c);
  arrayPush(ts, d);
  arrayPush(ts, e);
  assert(ts.len == 5);
  assert(ts.cap == 8);

  assert(ts.elements[0] == Var(0));
  assert(ts.elements[1] == Var(1));
  assert(ts.elements[2] == Var(2));
  assert(ts.elements[3] == Var(3));
  assert(ts.elements[4] == Var(4));

  arrayInsert(ts, 1, f);

  assert(ts.elements[0] == Var(0));
  assert(ts.elements[1] == Var(5));
  assert(ts.elements[2] == Var(1));
  assert(ts.elements[3] == Var(2));
  assert(ts.elements[4] == Var(3));

  arrayDel(ts, 2);

  assert(ts.elements[0] == Var(0));
  assert(ts.elements[1] == Var(5));
  assert(ts.elements[2] == Var(2));
  assert(ts.elements[3] == Var(3));
  assert(ts.elements[4] == Var(4));

  arrayDelFast(ts, 2);

  assert(ts.elements[0] == Var(0));
  assert(ts.elements[1] == Var(5));
  assert(ts.elements[2] == Var(4));
  assert(ts.elements[3] == Var(3));

  assert(ts.len == 4);

  arrayFree(ts);
  assert(ts.elements == nullptr);
  assert(ts.len == 0);
  assert(ts.cap == 0);
}

TEST_CASE("Type") {
  SECTION("TableOf") {
    Types VerticesSeqTypes{{CoreInfo::FloatType, CoreInfo::Float2Type,
                            CoreInfo::Float3Type, CoreInfo::ColorType}};
    Type VerticesSeq = Type::SeqOf(VerticesSeqTypes);
    Types IndicesSeqTypes{{
        CoreInfo::IntType,  // Triangle strip
        CoreInfo::Int2Type, // Line list
        CoreInfo::Int3Type  // Triangle list
    }};
    Type IndicesSeq = Type::SeqOf(IndicesSeqTypes);
    Types InputTableTypes{{VerticesSeq, IndicesSeq}};
    std::array<CBString, 2> InputTableKeys{"Vertices", "Indices"};
    Type InputTable = Type::TableOf(InputTableTypes, InputTableKeys);

    CBTypeInfo t1 = InputTable;
    REQUIRE(t1.basicType == CBType::Table);
    REQUIRE(t1.table.types.len == 2);
    REQUIRE(t1.table.keys.len == 2);
    REQUIRE(std::string(t1.table.keys.elements[0]) == "Vertices");
    REQUIRE(std::string(t1.table.keys.elements[1]) == "Indices");
    CBTypeInfo verticesSeq = VerticesSeq;
    CBTypeInfo indicesSeq = IndicesSeq;
    REQUIRE(t1.table.types.elements[0] == verticesSeq);
    REQUIRE(t1.table.types.elements[1] == indicesSeq);
  }
}

TEST_CASE("ObjectVar") {
  SECTION("ChainUse") {
    ObjectVar<int> myobject{"ObjectVarTestObject1", 100, 1};
    int *o1 = myobject.New();
    *o1 = 1000;
    CBVar v1 = myobject.Get(o1);
    REQUIRE(v1.valueType == CBType::Object);
    int *vo1 = reinterpret_cast<int *>(v1.payload.objectValue);
    REQUIRE(*vo1 == 1000);
    REQUIRE(vo1 == o1);

    // check internal ref count
    // this is internal magic but needed for testing
    struct ObjectRef {
      int shared;
      uint32_t refcount;
    };
    auto or1 = reinterpret_cast<ObjectRef *>(o1);
    REQUIRE(or1->refcount == 1);

    auto chain = chainblocks::Chain("test-chain-ObjectVar")
                     .let(v1)
                     .block("Set", "v1")
                     .block("Set", "v2")
                     .block("Get", "v1")
                     .block("Is", Var::ContextVar("v2"))
                     .block("Assert.Is", true);
    myobject.Release(o1);
    auto node = CBNode::make();
    node->schedule(chain);
    REQUIRE(node->tick());       // false is chain errors happened
    REQUIRE(or1->refcount == 1); // will be 0 when chain goes out of scope
  }
}

TEST_CASE("linalg compatibility") {
  static_assert(sizeof(linalg::aliases::double2) == 32);
  static_assert(sizeof(linalg::aliases::float3) == 32);
  static_assert(sizeof(linalg::aliases::float3) == sizeof(Vec3));
  static_assert(sizeof(linalg::aliases::float4) == 32);
  static_assert(sizeof(linalg::aliases::float4) == sizeof(Vec4));
  Var a{1.0, 2.0, 3.0, 4.0};
  Var b{4.0, 3.0, 2.0, 1.0};
  const linalg::aliases::float4 *va =
      reinterpret_cast<linalg::aliases::float4 *>(&a.payload.float4Value);
  const linalg::aliases::float4 *vb =
      reinterpret_cast<linalg::aliases::float4 *>(&b.payload.float4Value);
  auto c = (*va) + (*vb);
  linalg::aliases::float4 rc{5.0, 5.0, 5.0, 5.0};
  REQUIRE(c == rc);
  c = (*va) * (*vb);
  rc = {4.0, 6.0, 6.0, 4.0};
  REQUIRE(c == rc);

  std::vector<Var> ad{Var(1.0, 2.0, 3.0, 4.0), Var(1.0, 2.0, 3.0, 4.0),
                      Var(1.0, 2.0, 3.0, 4.0), Var(1.0, 2.0, 3.0, 4.0)};
  Var d(ad);
  const linalg::aliases::float4x4 *md =
      reinterpret_cast<linalg::aliases::float4x4 *>(
          &d.payload.seqValue.elements[0]);
  linalg::aliases::float4x4 rd{{1.0, 2.0, 3.0, 4.0},
                               {1.0, 2.0, 3.0, 4.0},
                               {1.0, 2.0, 3.0, 4.0},
                               {1.0, 2.0, 3.0, 4.0}};
  REQUIRE((*md) == rd);

  Mat4 sm{rd};
  REQUIRE(sm.x == rd.x);
  REQUIRE(sm.y == rd.y);
}

enum class XRHand { Left, Right };

struct GamePadTable : public TableVar {
  GamePadTable()
      : TableVar(),                      //
        buttons(get<SeqVar>("buttons")), //
        sticks(get<SeqVar>("sticks")),   //
        id((*this)["id"]),               //
        connected((*this)["connected"]) {
    connected = Var(false);
  }

  SeqVar &buttons;
  SeqVar &sticks;
  CBVar &id;
  CBVar &connected;
};

struct HandTable : public GamePadTable {
  static constexpr uint32_t HandednessCC = 'xrha';
  static inline Type HandEnumType{
      {CBType::Enum,
       {.enumeration = {.vendorId = CoreCC, .typeId = HandednessCC}}}};
  static inline EnumInfo<XRHand> HandEnumInfo{"XrHand", CoreCC, HandednessCC};

  HandTable()
      : GamePadTable(),                      //
        handedness((*this)["handedness"]),   //
        transform(get<SeqVar>("transform")), //
        inverseTransform(get<SeqVar>("inverseTransform")) {
    handedness = Var::Enum(XRHand::Left, CoreCC, HandednessCC);
  }

  CBVar &handedness;
  SeqVar &transform;
  SeqVar &inverseTransform;
};

TEST_CASE("TableVar") {
  HandTable hand;
  hand.buttons.push_back(Var(0.6, 1.0, 1.0));
  CBLOG_INFO(hand);
}
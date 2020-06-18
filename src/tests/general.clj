(def! Root (Node))

(def! inner1 (Chain "inner"
  (Assert.Is "My input" true)
  "My input 2"))

(def adder (Chain "adder"
  >= .adder-x
  (Pop) >= .adder-y
  .adder-x
  (Math.Add .adder-y)))

(def adderSeq (Chain "adderSeq"
  >= .args (Take 1) >= .adder-y
  .args (Take 0)
  (Math.Add .adder-y)))

(def! testChain 
  (Chain
   "namedChain"
   (Msg "Running tests!")

   true
   (Cond [~[(Is true)] ~[(Msg "Cond was true!!") false]
          (--> (Is false)) (--> (Msg "Cond was false!") true)] :Passthrough false)
   (Assert.Is false true)
   (Log)

   true
   (Cond [(--> (Is false) (Or) (Is true)) (--> (Msg "Cond was true!!") false)
          (--> (Is false)) (--> (Msg "Cond was false!") true)] :Passthrough false)
   (Assert.Is false true)
   (Log)

   true
   (Cond [(--> (Is false) (And) (Is true)) (--> (Assert.Is nil true))
          (--> (Is true)) (--> (Msg "Cond was false!") true)] :Passthrough false)
   (Assert.Is true true)
   (Log)

   false
   (Cond [(--> (Is true)) (--> (Msg "Cond was true!!") false)
          (--> (Is false)) (--> (Msg "Cond was false!") true)] :Passthrough false)
   (Assert.Is true true)
   (Log)

   false
   (Cond [(--> (Is true)) (--> (Msg "Cond was true!!") false)
          (--> (Is false) (And) (IsNot true)) (--> (Msg "Cond was false!") true)] :Passthrough false)
   (Assert.Is true true)
   (Log)

   false
   (Cond [(--> (Is false) (And) (IsNot false)) (--> (Assert.Is nil true))
          (--> (Is false)) (--> (Msg "Cond was true!!") false)] :Passthrough false)
   (Assert.Is false true)
   (Log)

   10
   (Cond [(--> true)    (--> (Msg "Cond was true!!") false)
          (--> (Is 10)) (--> (Msg "Cond was false!") true)] :Threading true :Passthrough false)
   (Assert.Is true true)
   (Log)

   10
   (Cond [(--> true)    (--> (Msg "Cond was true!!") false)
          (--> (Is 10)) (--> (Msg "Cond was false!") true)] :Threading false :Passthrough false)
   (Assert.Is false true)
   (Log)

   "Hello"
   (Assert.Is "Hello" true)
   (Log)

   77
   (Assert.Is 77 true)
   (Log)

   10
   (Math.Add 10)
   (Assert.Is (+ 10 10) true)
   (Log)

   11
   (Math.Subtract 10)
   (Assert.Is (- 11 10) true)
   (Log)

   (Float4 10.3 3.6 2.1 1.1)
   (Math.Multiply (Float4 2 2 2 2))
   (Assert.Is (Float4 (* 10.3 2) (* 3.6 2) (* 2.1 2) (* 1.1 2)) true)
   (Log)

   (Float3 10.3 2.1 1.1)
   (Math.Multiply (Float3 2 2 2))
   (Assert.Is (Float3 (* 10.3 2) (* 2.1 2) (* 1.1 2)) true)
   (Log)

   10
   (Push "list1")
   20
   (Push "list1")
   30
   (Push "list1")
   (Get "list1")
   (Log)
   (Take 0)
   (Assert.Is 10 true)
   (Get "list1")
   (Take 1)
   (Assert.Is 20 true)
   (Get "list1")
   (Take 2)
   (Assert.Is 30 true)
   (Log)

   (Repeat (-->
            10 (Push "list1")
            20 (Push "list1")
            30 (Push "list1")
            (Get "list1")
            (Log)
            (Take 0)
            (Assert.Is 10 true)
            (Get "list1")
            (Take 1)
            (Assert.Is 20 true)
            (Get "list1")
            (Take 2)
            (Assert.Is 30 true)
            (Log)) :Times 5)

   0
   (Set "counter")
   (Repeat (-->
            (Get "counter")
            (Math.Add 1)
            (Update "counter")) :Times 5)
   (Get "counter")
   (Assert.Is 5 true)
   (Log)

   20
   (Set "a")
   30
   (Set "b")
   (Swap "a" "b")
   (Get "a")
   (Assert.Is 30)
   (Get "b")
   (Assert.Is 20)
   (Log)

   "Value1" (Set "tab11" "v1")
   "Value2" (Set "tab11" "v2")
   (Get "tab11" "v1")
   (Assert.Is "Value1" true)
   (Log)
   (Get "tab11" "v2")
   (Assert.IsNot "Value1" true)
   (Log)

   (Count "tab11")
   (Assert.Is 2 true)

   (Clear "tab11")
   (Count "tab11")
   (Assert.Is 0 true)

   "My input"
   (Dispatch inner1)
   (Assert.Is "My input" true)
   (Log)

   "My input"
   (Do inner1)
   (Assert.Is "My input 2" true)
   (Log)

  ; b0r
   0
   (Math.And 0xFF)
   (Math.LShift 8)
   (Set "x")
  ; b1r
   0
   (Math.And 0xFF)
   (Math.Or .x)
   (Math.LShift 8)
   (Update "x")
  ; b2r
   59
   (Math.And 0xFF)
   (Math.Or .x)
   (Math.LShift 8)
   (Update "x")
  ; b3r
   156
   (Math.And 0xFF)
   (Math.Or .x)
   (Update "x")
  ; result
   (Get "x")
   (Log)

   (Color 2 2 2 255)
   (Log)

   (Const [(Float 1) (Float 2) (Float 3) (Float 4)])
   (Math.Multiply (Float 2.0))
   (Log)
   (Assert.Is [(Float 2) (Float 4) (Float 6) (Float 8)] true)

   (Const [(Float 1) (Float 2) (Float 3) (Float 4)])
   (Math.Multiply [(Float 2.0) (Float 1.0) (Float 1.0) (Float 2.0)])
   (Log)
   (Assert.Is [(Float 2) (Float 2) (Float 3) (Float 8)] true)

   5 (ToFloat) (Math.Divide (Float 10))
   (Assert.Is 0.5 true)

   (Int 10) (ToFloat) (Set "fx")
   (Get "fx") (Assert.Is (Float 10) true)
   (Get "fx") (Assert.IsNot (Int 10) true)
   (Float 5) (Math.Divide .fx) (Assert.Is 0.5 true)

   10 (Push "myseq")
   20 (Push "myseq")
   30 (Push "myseq")
   (Get "myseq")
   (Take 0)
   (Math.Add 1)
   (Assert.Is 11 true)
   (Log)

   (Clear "myseq")

   12 (Push "myseq")
   22 (Push "myseq")
   32 (Push "myseq")
   (Get "myseq")
   (Take 0)
   (Math.Add 1)
   (Assert.Is 13 true)
   (Log)

   10 (Push "tab1" "myseq")
   20 (Push "tab1" "myseq")
   30 (Push "tab1" "myseq")
   (Get "tab1" "myseq")
   (Take 0)
   (Math.Add 1)
   (Assert.Is 11 true)
   (Log)

   (Clear "tab1" "myseq")

   12 (Push "tab1" "myseq")
   22 (Push "tab1" "myseq")
   32 (Push "tab1" "myseq")
   (Get "tab1" "myseq")
   (Take 0)
   (Math.Add 1)
   (Assert.Is 13 true)
   (Log)

   10 (Push "tab1new" "myseq")
   20 (Push "tab1new" "myseq")
   30 (Push "tab1new" "myseq")
   (Get "tab1new" "myseq")
   (Take 0)
   (Math.Add 1)
   (Assert.Is 11 true)
   (Log)

   (Clear "tab1new" "myseq")

   12 (Push "tab1new" "myseq")
   22 (Push "tab1new" "myseq")
   32 (Push "tab1new" "myseq")
   (Get "tab1new" "myseq")
   (Take 0)
   (Math.Add 1)
   (Assert.Is 13 true)
   (Log)

   (Int2 10 11) (Push "myseq2")
   (Int2 20 21) (Push "myseq2")
   (Int2 30 31) (Push "myseq2")
   (Get "myseq2")
   (Flatten)
   (Set "myseq2flat")
   (Get "myseq2flat")
   (Take 0)
   (Math.Add 1)
   (Assert.Is 11 true)
   (Log)

   (Repeat (-->
            (Pop "myseq2flat")
            (Log)) 6)

   2 (Push "unsortedList")
   4 (Push "unsortedList")
   1 (Push "unsortedList")
   0 (Push "unsortedList")
   1 (Push "unsortedList")
   5 (Push "unsortedList")
   (Sort .unsortedList)
   (Log)
   (Assert.Is [0 1 1 2 4 5] true)

   2 (Push "unsortedList2")
   4 (Push "unsortedList2")
   1 (Push "unsortedList2")
   0 (Push "unsortedList2")
   1 (Push "unsortedList2")
   5 (Push "unsortedList2")

   2 (Push "unsortedList3")
   4 (Push "unsortedList3")
   1 (Push "unsortedList3")
   0 (Push "unsortedList3")
   1 (Push "unsortedList3")
   5 (Push "unsortedList3")

   (Sort .unsortedList2 [.unsortedList3])
   (Log)
   (Assert.Is [0 1 1 2 4 5] true)
   (Sort .unsortedList2 .unsortedList3)
   (Log)
   (Assert.Is [0 1 1 2 4 5] true)
   (Get "unsortedList3")
   (Log)
   (Assert.Is [0 1 1 2 4 5] true)

   (Clear "unsortedList")
   2 (Push "unsortedList")
   4 (Push "unsortedList")
   1 (Push "unsortedList")
   0 (Push "unsortedList")
   1 (Push "unsortedList")
   5 (Push "unsortedList")
   (Sort .unsortedList :Desc true)
   (Log)
   (Assert.Is [5 4 2 1 1 0] true)
   (Count "unsortedList")
   (Assert.Is 6 true)

   (Get "unsortedList")
   (IndexOf 2)
   (Assert.Is 2 true)

   (Get "unsortedList")
   (IndexOf 5)
   (Assert.Is 0 true)

   (Get "unsortedList")
   (IndexOf [1 1])
   (Assert.Is 3 true)

   (Get "unsortedList")
   (IndexOf [1 1 0])
   (Assert.Is 3 true)

   (Get "unsortedList")
   (IndexOf 10)
   (Assert.Is -1 true)

   (Get "unsortedList")
   (IndexOf [1 1 0 1])
   (Assert.IsNot 3 true)

   (Const [1 1 0])
   (Set "toFindVar")
   (Get "unsortedList")
   (IndexOf .toFindVar)
   (Assert.Is 3 true)

   (Remove .unsortedList :Predicate (--> (IsMore 3)))
   (Sort .unsortedList :Desc true)
   (Assert.Is [2 1 1 0] true)
   (Count "unsortedList")
   (Assert.Is 4 true)
   (Get "unsortedList")
   (Log)
   (Assert.Is [2 1 1 0] true)

   (Get "unsortedList2")
   (Set "unsortedList2Copy")
   (Remove .unsortedList2Copy [.unsortedList3] (--> (IsMore 3)))
   (Sort .unsortedList2Copy [.unsortedList3])
   (Log)
   (Assert.Is [0 1 1 2] true)
   (Get "unsortedList3")
   (Log)
   (Assert.Is [0 1 1 2] true)
   (Count "unsortedList2")
   (Assert.Is 6 true)

   (Const [[2 "x"] [3 "y"] [1 "z"]])
   (Ref "constSeq")
   (Sort .constSeq :Key (-->
                         (Take 0)))
   (Assert.Is [[1 "z"] [2 "x"] [3 "y"]] true)

   1.0 (Push "meanTest")
   2.0 (Push "meanTest")
   0.0 (Push "meanTest")
   3.0 (Push "meanTest")
   (Get "meanTest") (Math.Mean :Kind Mean.Arithmetic) (Log)
   (Assert.Is 1.5 true)

   1.0 (Push "meanTest2")
   2.0 (Push "meanTest2")
   3.0 (Push "meanTest2")
   (Get "meanTest2") (Math.Mean :Kind Mean.Geometric) (Log)
   (ToString)
   (Assert.Is "1.81712" true)

   (Get "meanTest2") (Math.Mean :Kind Mean.Harmonic) (Log)
   (ToString)
   (Assert.Is "1.63636" true)

   1 (Set "indexAsVar")
   (Get "meanTest")
   (Take .indexAsVar)
   (Assert.Is 2.0 true)
   (Const [1 2]) (Set "indexAsVar2")
   (Get "meanTest")
   (Take .indexAsVar2)
   (Assert.Is [2.0 0.0] true)

   (Repeat (-->
            (Pop "meanTest")
            (Math.Add 1.0)
            (Log)) 4)

   (Get "unsortedList") (Limit 2) (Set "limitTest") (Count "limitTest")
   (Assert.Is 2 true)

   10 (Set "repeatsn")
   0 >= .iterCount
   (Repeat (--> .iterCount (Math.Add 1) > .iterCount) :Times .repeatsn)
   .iterCount
   (Assert.Is 10 true)

   0 (Math.Add 10) (Assert.Is 10 true)

   0x0000000000070090
   (BitSwap64)
   (ToHex) (Log)
   (Assert.Is "0x9000070000000000" true)

   "Test"
   (Any ["Test" "A"])
   (Log)

   0x00070090
   (BitSwap32)
   (Assert.Is 0x90000700 true)
   (ToHex) (Log)

   "Hello file append..."
   (WriteFile "test.bin" :Append true)
   "Hello file append..."
   (WriteFile "test.bin" :Append true)

   "Hello Pandas"
   (ToBytes)
   (BytesToInt8)
   (Assert.Is [72 101 108 108 111 32 80 97 110 100 97 115 0] true)
   (Log)
   (Math.Xor [77 78 77 11 16])
   (Assert.Is [5 43 33 103 127 109 30 44 101 116 44 61 77] true)
   (Log)
   (Math.Xor [77 78 77 11 16])
   (Assert.Is [72 101 108 108 111 32 80 97 110 100 97 115 0] true)
   (Log)

  ; show induced mutability with Ref
   "Hello reference" ; Const
   (Ref "ref1") ; no copy will happen!
   (Get "ref1")
   (Assert.Is "Hello reference" true)

   (Const [1 2 3 4 5])
   (Set "pretest")
   (ToBytes)
   (BytesToInt64)
   (Assert.Is [1 2 3 4 5] true)

   0 (PrependTo .pretest)
   (Get "pretest")
   (Assert.Is [0 1 2 3 4 5] true)
   (Log)

   "." (FS.Iterate :Recursive false) (Log)
   (Take 4) (FS.Extension) (Log)

   "." (FS.Iterate :Recursive false) (Log)
   (Take 4) (FS.Filename :NoExtension true) (Log)

   "The result is: "   (Set "text1")
   "Hello world, "     (AppendTo .text1)
   "this is a string"  (AppendTo .text1)
   (Get "text1") (Log)
   (Assert.Is "The result is: Hello world, this is a string" true)

   "The result is: "   (Set "text2")
   "Hello world, "     (AppendTo .text2)
   "this is a string again"  (AppendTo .text2)
   (Get "text2") (Log)
   (Assert.IsNot "The result is: Hello world, this is a string" true)

   "## " (PrependTo .text2)
   (Get "text2") (Log)
   (Assert.Is "## The result is: Hello world, this is a string again" true)

   "test.txt" (FS.Write .text2 :Overwrite true)
   (FS.Read)
   (Assert.Is "## The result is: Hello world, this is a string again" true)
   (Log)

   (Get "text1")
   (ToJson)
   (Log)
   (Assert.Is "{\"type\":\"String\",\"value\":\"The result is: Hello world, this is a string\"}" true)
   (FromJson)
   (ExpectString)
   (Assert.Is "The result is: Hello world, this is a string" true)

   (Get "unsortedList2Copy")
   (ToJson)
   (Log)
   (Assert.Is "{\"type\":\"Seq\",\"values\":[{\"type\":\"Int\",\"value\":0},{\"type\":\"Int\",\"value\":1},{\"type\":\"Int\",\"value\":1},{\"type\":\"Int\",\"value\":2}]}" true)

   (Get "tab1new")
   (ToJson)
   (Log)
   (Assert.Is "{\"type\":\"Table\",\"values\":[{\"key\":\"myseq\",\"value\":{\"type\":\"Seq\",\"values\":[{\"type\":\"Int\",\"value\":12},{\"type\":\"Int\",\"value\":22},{\"type\":\"Int\",\"value\":32}]}}]}" true)

   (Float4 1 2 3 4)
   (Take 0)
   (Assert.Is 1.0 true)

   (Float4 1 2 3 4)
   (Take [2 3])
   (Assert.Is (Float2 3 4) true)

   1 (Push "s1")
   2 (Push "s1")
   3 (Push "s1")
   1 (Push "s2")
   2 (Push "s2")
   3 (Push "s2")
   (Get "s1")
   (Is .s2)
   (Assert.Is true true)

   (Get "s1")
   (DBlock "Is" [.s2])
   (Assert.Is true true)

   4 (Push "s1")
   5 (Push "s1")

   (Get "s1")
   (Slice 1 4)
   (Assert.Is [2 3 4] true)

   (Get "s1")
   (Slice :From 1 :To 5 :Step 2)
   (Assert.Is [2 4] true)

   (Get "s1")
   (Slice 3 -1)
   (Assert.Is [4] true)

   "passing by..."
   (Drop "s2")
   (Assert.Is "passing by..." true)
   (Get "s2")
   (Assert.Is [1 2] true)

   (Time.Now)
   (Log)
   (Time.NowMs)
   (Log)

   "baz.dat"
   (Regex.Match "([a-z]+)\\.([a-z]+)")
   (Log)
   (Assert.Is ["baz.dat" "baz" "dat"] true)

   "Quick brown fox"
   (Regex.Replace "a|e|i|o|u" "[$&]")
   (Log)
   (Assert.Is "Q[u][i]ck br[o]wn f[o]x" true)

   (ToBytes)
   (Set "bytesTest")
   (Get "bytesTest")
   (Log)
   (Update "bytesTest")
   (Log)

   1.0
   (Math.Inc)
   (Assert.Is 2.0 true)

   5
   (Math.Dec)
   (Assert.Is 4 true)

                                        ; test stack ops
   (Push)
   (Pop)
   (Assert.Is 4 true)
   (Log)

   8
   (Push)
   7
   (Push)
   (Drop)
   (Pop)
   (Assert.Is 8 true)

   (Const [111 112 101 114 97 116 111 114])
   (Set "seq-a")
   (Const [111 112 101 114 97 116 105 111 110 115])
   (Set "seq-b")
   (Get "seq-a")
   (IsMore .seq-b)
   (Assert.Is true true)

   (Const [111 112 101 114 97])
   (Update "seq-a")
   (Const [111 112 101 114 97 116 105])
   (Update "seq-b")
   (Get "seq-a")
   (IsLess .seq-b)
   (Assert.Is true true)

   (Const [111 112 101 114 97 99])
   (Update "seq-a")
   (Const [111 112 101 114 97 99])
   (Update "seq-b")
   (Get "seq-a")
   (IsMoreEqual .seq-b)
   (Assert.Is true true)

   (Int4 111 112 101 200)
   (Set "int4-a")
   (Int4 111 112 99 300)
   (Set "int4-b")
   (Get "int4-a")
   (IsMore .int4-b)
   (Assert.Is true true)

   (Int4 111 112 101 300)
   (Update "int4-a")
   (Int4 111 112 101 100)
   (Update "int4-b")
   (Get .int4-a)
   (IsMore .int4-b)
   (Assert.Is true true)

   "{
    \"pi\": 3.141,
    \"happy\": true,
    \"name\": \"Niels\",
    \"nothing\": null,
    \"answer\": {
      \"everything\": 42
    },
    \"list\": [1, 0, 2],
    \"object\": {
      \"currency\": \"USD\",
      \"value\": 42.99
    }
  }"
   (FromJson)
   (ExpectTable)
   (Log)
   (Take "pi")
   (Assert.Is 3.141 true)
   (Log)

   (Get .seq-a)
   (Map (-->
         (Log)))

   (Get .seq-a)
   (Map (-->
         (Math.Add 1)))
   (Log)
   (Assert.Is [112 113 102 115 98 100] true)
   (RTake [0 1 3 4])
   (Assert.Is [100 98 102 113] true) >= .s1
   (Reduce (Max .$0)) (Log)
   (Assert.Is 113 true)
   .s1 (Reduce (Min .$0)) (Log)
   (Assert.Is 98 true)

   (Await
    (-->
     (Get .seq-a)
     (Reduce (-->
              (Math.Add .$0)
              (Log)))))
   (Log)
   (Assert.Is 634 true)
   
   ; utf8 testing
   "在庫なし"
   (Assert.Is "在庫なし" true)
   (Log)

   (Const [[1 2 3] [2 3 4] [3 4 5]]) &> .nestedSeq
   (Reduce (Math.Add .$0))
   (Assert.Is [6 9 12] true)
   (Log)

   .nestedSeq (Math.Multiply [2 3 4])
   (Log)
   (Assert.Is [[2, 4, 6], [6, 9, 12], [12, 16, 20]] true)

   .nestedSeq (Math.Multiply [[2 3 4]])
   (Log)
   (Assert.Is [[2, 6, 12], [4, 9, 16], [6, 12, 20]] true)

   .nestedSeq >> .adder-args >> .adder-args
   .adder-args (Do adderSeq)
   (Log)
   (Assert.Is [[2, 4, 6], [4, 6, 8], [6, 8, 10]] true)

   (Clear .adder-args)

   (Const [2 3 4]) >> .adder-args >> .adder-args
   .adder-args (Do adderSeq)
   (Log)
   (Assert.Is [4 6 8] true)

   11 (Push)
   10 (Do adder)
   (Assert.Is 21 true)

   11 (Push)
   11 (Do adder)
   (Assert.Is 22 true)

   8.0
   (Math.Pow 2.0)
   (Assert.Is 64.0 true)

   (Erase [2 0] .seq-a)
   (Get .seq-a)
   (Log)
   (Assert.Is [112 114 97 99] true)

   (DropFront .seq-a)
  ;; this also tests that we fixup input as well
  ;; (Get .seq-a)
   (Assert.Is [114 97 99] true)

   (PopFront .seq-a)
   (Assert.Is 114 true)

   (Get .seq-a)
   (Assert.Is [97 99] true)

   (Maybe (Take 3) :Else (Take 0))
   (Assert.Is 97 true)

   (Time.EpochMs)
   (Log)

   (Int3 3 3 3) (Flatten) (Assert.Is [3 3 3] true) >> .i3s
   (Int3 6 6 6) (Flatten) (Assert.Is [6 6 6] true) >> .i3s
   .i3s (Flatten) (Assert.Is [3 3 3 6 6 6] true)
   (Log)

   (RandomFloat) (IsLess 1.0) (Assert.Is true true)

   true
   (When (Is true) (Assert.Is true true))
   (When (Is false) (Assert.Is false true))
   (WhenNot (Is true) (Assert.Is false true))
   (WhenNot (Is false) (Assert.Is true true))

   true
   (If (Is true) :Then (Msg "true!") :Else (Msg "false!"))
   (Assert.Is true true)

   false
   (If (Is true) :Then (Const 10) :Else (Const 20) :Passthrough false)
   (Assert.Is 20 true)

   false
   (If (Is false) :Then (Const 10) :Else (Const 20) :Passthrough false)
   (Assert.Is 10 true)

   "Hey" >= .fval (Log) >> .seq-a .seq-a (Log)

   (Const [-0.1 -0.2 0.4])
   (Math.Abs)
   (Assert.Is [0.1 0.2 0.4] true)

   "global1" (Set .global1 :Global true)

   (Msg "All looking good!")

  ; test for a possible issue with thread pool on ending
   (||
    (-->
     (Maybe
      (ExpectString)
      (Stop))))))

(prn "Doing SaveBinary")

(def saveBinary
  (Chain
   "SaveBinary"
   (Const testChain)
   (WriteFile "testChain.bin")))
(schedule Root saveBinary)
(if (run Root 0.001 100) nil (throw "Root tick failed"))
;; For now Node holds ref...
(def Root (Node))
(def saveBinary nil)

(prn "Doing LoadBinary")

(def loadBinary
  (Chain
   "LoadBinary"
   (ReadFile "testChain.bin")
   (ExpectChain) >= .loadedChain (Log)
   (ChainRunner .loadedChain :Mode RunChainMode.Detached)
   (WaitChain .loadedChain)
   (Assert.Is "global1" true)
   (Get .global1 :Global true :Default "nope")
   (Assert.Is "global1" true)
   (Log)
   (Process.StackTrace)
   (Log)))
(schedule Root loadBinary)
(if (run Root 0.001 100) nil (throw "Root tick failed"))
;; For now Node holds ref...
(def Root (Node))
(def loadBinary nil)

(prn "Doing normal run")

(schedule Root testChain)
(if (run Root 0.001 100) nil (throw "Root tick failed"))
;; For now Node holds ref...
(def Root (Node))
(def testChain nil)
(def inner1 nil)

(prn "Doing LoopedChain")

(def loopedChain (Chain "LoopedChain" :Looped
  10 (Push "loopVar1")
  11 (Push "loopVar1")
  12 (Push "loopVar1")
  (Count "loopVar1")
  (Assert.Is 3 true)))

 (def loopedChain2 (Chain "LoopedChain" :Looped
  10 (Push "loopVar2" :Clear false)
  11 (Push "loopVar2")
  12 (Push "loopVar2")
  (Count "loopVar2")))

(prepare loopedChain)
(start loopedChain)
(tick loopedChain)
(tick loopedChain)
(tick loopedChain)

(prepare loopedChain2)
(start loopedChain2)
(tick loopedChain2)
(tick loopedChain2)
(tick loopedChain2)

(if (not (= (stop loopedChain2) (Int 9))) (throw "Seq :Clear test failed"))

(def fileReader (Chain "readFile"
  (Repeat (-->
    (ReadFile "test.bin")
    (ExpectString)
    (Log)
    (Assert.Is "Hello file append..." true))
    2)
))

(schedule Root fileReader)
(if (run Root 0.001 100) nil (throw "Root tick failed"))

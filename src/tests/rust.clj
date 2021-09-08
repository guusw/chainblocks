; SPDX-License-Identifier: BSD-3-Clause
; Copyright © 2021 Fragcolor Pte. Ltd.

(def Root (Node))

(schedule
 Root
 (Chain
  "test"
  ""
  (| (Hash.Sha2-256) (ToHex) (Assert.Is "0xe3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"))
  (| (Hash.Sha2-512) (ToHex) (Assert.Is "0xcf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"))
  (| (Hash.Sha3-256) (ToHex) (Assert.Is "0xa7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a"))
  (| (Hash.Sha3-512) (ToHex) (Assert.Is "0xa69f73cca23a9ac5c8b567dc185a756e97c982164fe25859e0d1dcc1475c80a615b2123af1f5f94c11e3e9402c3ac558f500199d95b6d3e301758586281dcd26"))
  "Hello world"
  (| (ToBase58) (Log)
     (FromBase58) (BytesToString) (Log)
     (Assert.Is "Hello world" true))
  (Hash.Keccak-256) = .hash
  (ToHex)
  (Assert.Is "0xed6c11b0b5b808960df26f5bfc471d04c1995b0ffd2055925ad1be28d6baadfd" true)
  .hash (ECDSA.Sign "1536f1d756d1abf83aaf173bc5ee3fc487c93010f18624d80bd6d4038fadd59e")
  (Log "Signed")
  (Take 0)
  (ToHex) (Log "Signed Hex")

  128 (ToLEB128 :Signed false) (BytesToInts) (Log) (Assert.Is [128 1] true)
  (IntsToBytes) (FromLEB128 :Signed false) (Log) (Assert.Is 128 true)

  -1000 (ToLEB128 :Signed true) (BytesToInts) (Log) (Assert.Is [152 120] true)
  (IntsToBytes) (FromLEB128 :Signed true) (Log) (Assert.Is -1000 true)

  "hello worlds\n" (ToHex) (HexToBytes) = .payload
  (Count .payload) = .len
  "0x1220" (HexToBytes) >= .a ; sha256 multihash
  "0x0a" (HexToBytes) >> .b ; prefix 1
  .len (ToLEB128 :Signed false) = .leblen
  (Count .leblen) (Math.Multiply 2) = .lenX2
  .len (Math.Add 4) (Math.Add .lenX2) (ToLEB128 :Signed false) >> .b
  "0x080212" (HexToBytes) >> .b
  .leblen >> .b
  .payload >> .b
  "0x18" (HexToBytes) >> .b
  .leblen >> .b
  .b (Hash.Sha2-256) (AppendTo .a)
  .a (ToBase58) (Log) (Assert.Is "QmZ4tDuvesekSs4qM5ZBKpXiZGun7S2CYtEZRB3DYXkjGx" true)

  "../../assets/Freesample.svg"
  (FS.Read :Bytes true)
  (SVG.ToImage)
  (WritePNG "svgtest.png")

  "7bf19806aa6d5b31d7b7ea9e833c202e51ff8ee6311df6a036f0261f216f09ef"
  (| (ECDSA.PublicKey)
     (| (ToHex) (Log))
     (| (Slice :From 1)
        (Hash.Keccak-256)
        (Slice :From 12)
        (ToHex)
        (Log "Eth Address")
        (Assert.Is "0x3db763bbbb1ac900eb2eb8b106218f85f9f64a13" true)))
  (| (ECDSA.PublicKey true) (ToHex) (Log))))

(run Root)

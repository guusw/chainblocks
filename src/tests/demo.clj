; SPDX-License-Identifier: BSD-3-Clause
; Copyright © 2019 Fragcolor Pte. Ltd.

;; Sample 1

(schedule
 (Chain
  "chain-1" :Looped
  (Get "x" :Default 0)
  (Log)
  (Inc "x")))

(schedule
 (Chain
  "chain-2" :Looped
  (Get "x" :Default 0)
  (Log)))

                                        ;chain-1> 0
                                        ;chain-2> 1

;; Sample 2

(schedule
 (Chain
  "chain-1" :Looped
  (Get "x" :Default 0)
  (Log)
  (Pause)
  (Inc "x")))

(schedule
 (Chain
  "chain-2" :Looped
  (Get "x" :Default 0)
  (Log)))

                                        ;chain-1> 0
                                        ;chain-2> 0
                                        ;chain-1> 0 ;; chain yields by default at end of iteration!
                                        ;chain-2> 1


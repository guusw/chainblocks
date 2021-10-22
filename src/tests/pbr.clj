
; SPDX-License-Identifier: BSD-3-Clause
; Copyright © 2019 Fragcolor Pte. Ltd.

(def! Root (Node))

(def identity
  [(Float4 1 0 0 0)
   (Float4 0 1 0 0)
   (Float4 0 0 1 0)
   (Float4 0 0 0 1)])

(def test-chain
  (Chain
   "test-chain"
   :Looped
   (GFX.MainWindow
    :Title "SDL Window" :Width 1024 :Height 1024 :Debug true :Fullscreen false
    :Contents
    (->
     (Setup
      "../../external/glTF-Sample-Models/2.0/Duck/glTF-Binary/Duck.glb" (GLTF.Load) >= .model
      0.0 >= .time
      (Float3 0.0 0.9 0.0) >= .camera-offset
      
     .camera-offset (ToFloat4) >= .tmp4
     (Float4 0 0 -4 1) (Math.Add .tmp4) >= .camera-base
     .camera-offset >= .camera-lookat)

     (Time.Delta) (Math.Add .time) >= .time
     .time (Math.Multiply 1.5) >= .y-rotation

     .y-rotation (Math.AxisAngleY) (Math.Rotation) (Math.MatMul .camera-base) (ToFloat3) >= .camera-pos

     {:Position .camera-pos :Target .camera-lookat} (GFX.Camera)
     (Float3 0 0 0) (Math.Translation) (GLTF.Draw :Model .model)))))

(schedule Root test-chain)
(run Root (/ 1.0 60.0))

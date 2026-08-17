(module
  (func $simd_add (export "simd_add")
    (param $a i32) (param $b i32) (result i32)
    (i32x4.extract_lane 0
      (i32x4.add
        (i32x4.splat (local.get $a))
        (i32x4.splat (local.get $b))
      )
    )
  )

  (func $simd_mul (export "simd_mul")
    (param $x i32) (param $y i32) (result i32)
    (i32x4.extract_lane 0
      (i32x4.mul
        (i32x4.splat (local.get $x))
        (i32x4.splat (local.get $y))
      )
    )
  )
)

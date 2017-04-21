; ModuleID = 'ctor.cpp'
source_filename = "ctor.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.MyType = type { i32 }

$_ZN6MyTypeC2Ei = comdat any

; Function Attrs: norecurse uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.MyType, align 4
  store i32 0, i32* %1, align 4
  call void @_ZN6MyTypeC2Ei(%struct.MyType* %2, i32 100)
  ret i32 0
}

; Function Attrs: nounwind uwtable
define linkonce_odr void @_ZN6MyTypeC2Ei(%struct.MyType*, i32) unnamed_addr #1 comdat align 2 {
  %3 = alloca %struct.MyType*, align 8
  %4 = alloca i32, align 4
  store %struct.MyType* %0, %struct.MyType** %3, align 8
  store i32 %1, i32* %4, align 4
  %5 = load %struct.MyType*, %struct.MyType** %3, align 8
  %6 = getelementptr inbounds %struct.MyType, %struct.MyType* %5, i32 0, i32 0
  %7 = load i32, i32* %4, align 4
  store i32 %7, i32* %6, align 4
  ret void
}

attributes #0 = { norecurse uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.1-svn288847-1~exp1 (branches/release_39)"}

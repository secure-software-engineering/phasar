; ModuleID = 'ctor_default.cpp'
source_filename = "ctor_default.cpp"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.13.0"

%struct.MyType = type { i32 }

; Function Attrs: norecurse ssp uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.MyType, align 4
  store i32 0, i32* %1, align 4
  call void @_ZN6MyTypeC1Ev(%struct.MyType* %2)
  ret i32 0
}

; Function Attrs: ssp uwtable
define linkonce_odr void @_ZN6MyTypeC1Ev(%struct.MyType*) unnamed_addr #1 align 2 {
  %2 = alloca %struct.MyType*, align 8
  store %struct.MyType* %0, %struct.MyType** %2, align 8
  %3 = load %struct.MyType*, %struct.MyType** %2, align 8
  call void @_ZN6MyTypeC2Ev(%struct.MyType* %3)
  ret void
}

; Function Attrs: nounwind ssp uwtable
define linkonce_odr void @_ZN6MyTypeC2Ev(%struct.MyType*) unnamed_addr #2 align 2 {
  %2 = alloca %struct.MyType*, align 8
  store %struct.MyType* %0, %struct.MyType** %2, align 8
  %3 = load %struct.MyType*, %struct.MyType** %2, align 8
  ret void
}

attributes #0 = { norecurse ssp uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { ssp uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind ssp uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"PIC Level", i32 2}
!1 = !{!"clang version 3.9.1 (tags/RELEASE_391/final)"}

; ModuleID = 'summary_class_1.ll.pp'
source_filename = "summary_class_1.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%class.Point = type { i32, i32 }

$_ZN5PointC2Eii = comdat any

$_ZN5Point4getXEv = comdat any

$_ZN5Point4getYEv = comdat any

; Function Attrs: uwtable
define void @_Z11pseudo_userv() #0 {
  %1 = alloca %class.Point, align 4
  call void @_ZN5PointC2Eii(%class.Point* %1, i32 1, i32 2)
  %2 = call i32 @_ZN5Point4getXEv(%class.Point* %1)
  %3 = call i32 @_ZN5Point4getYEv(%class.Point* %1)
  ret void
}

; Function Attrs: nounwind uwtable
define linkonce_odr void @_ZN5PointC2Eii(%class.Point*, i32, i32) unnamed_addr #1 comdat align 2 {
  %4 = getelementptr inbounds %class.Point, %class.Point* %0, i32 0, i32 0
  store i32 %1, i32* %4, align 4
  %5 = getelementptr inbounds %class.Point, %class.Point* %0, i32 0, i32 1
  store i32 %2, i32* %5, align 4
  ret void
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZN5Point4getXEv(%class.Point*) #1 comdat align 2 {
  %2 = getelementptr inbounds %class.Point, %class.Point* %0, i32 0, i32 0
  %3 = load i32, i32* %2, align 4
  ret i32 %3
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZN5Point4getYEv(%class.Point*) #1 comdat align 2 {
  %2 = getelementptr inbounds %class.Point, %class.Point* %0, i32 0, i32 1
  %3 = load i32, i32* %2, align 4
  ret i32 %3
}

attributes #0 = { uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.1-svn288847-1~exp1 (branches/release_39)"}

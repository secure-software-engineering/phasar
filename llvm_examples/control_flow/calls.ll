; ModuleID = 'calls.cpp'
source_filename = "calls.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define void @_Z3varv() #0 {
  ret void
}

; Function Attrs: nounwind uwtable
define void @_Z3tarv() #0 {
  ret void
}

; Function Attrs: nounwind uwtable
define void @_Z3foov() #0 {
  ret void
}

; Function Attrs: nounwind uwtable
define void @_Z3barv() #0 {
  call void @_Z3varv()
  ret void
}

; Function Attrs: nounwind uwtable
define void @_Z5otherv() #0 {
  call void @_Z3foov()
  call void @_Z3barv()
  call void @_Z3tarv()
  ret void
}

; Function Attrs: nounwind uwtable
define void @_Z10yetanotherv() #0 {
  call void @_Z5otherv()
  call void @_Z3tarv()
  ret void
}

; Function Attrs: norecurse nounwind uwtable
define i32 @main() #1 {
  %1 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  call void @_Z3varv()
  call void @_Z5otherv()
  call void @_Z10yetanotherv()
  ret i32 0
}

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { norecurse nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

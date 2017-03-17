; ModuleID = 'child_2.cpp'
source_filename = "child_2.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.child = type { %struct.base }
%struct.base = type { i32 (...)** }

; Function Attrs: nounwind uwtable
define i32 @_ZN5child3barEv(%struct.child*) unnamed_addr #0 align 2 {
  %2 = alloca %struct.child*, align 8
  store %struct.child* %0, %struct.child** %2, align 8
  %3 = load %struct.child*, %struct.child** %2, align 8
  ret i32 200
}

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

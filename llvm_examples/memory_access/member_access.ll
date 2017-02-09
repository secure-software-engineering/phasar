; ModuleID = 'member_access.cpp'
source_filename = "member_access.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.Data = type { i32, i32 }

; Function Attrs: norecurse nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.Data, align 4
  store i32 0, i32* %1, align 4
  %3 = getelementptr inbounds %struct.Data, %struct.Data* %2, i32 0, i32 0
  store i32 13, i32* %3, align 4
  %4 = getelementptr inbounds %struct.Data, %struct.Data* %2, i32 0, i32 1
  store i32 42, i32* %4, align 4
  ret i32 0
}

attributes #0 = { norecurse nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

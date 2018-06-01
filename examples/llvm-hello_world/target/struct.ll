; ModuleID = 'struct.cpp'
source_filename = "struct.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.S = type { i32, i32, i32 }

; Function Attrs: noinline norecurse nounwind optnone uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.S, align 4
  store i32 0, i32* %1, align 4
  %3 = getelementptr inbounds %struct.S, %struct.S* %2, i32 0, i32 0
  store i32 1, i32* %3, align 4
  %4 = getelementptr inbounds %struct.S, %struct.S* %2, i32 0, i32 1
  store i32 2, i32* %4, align 4
  %5 = getelementptr inbounds %struct.S, %struct.S* %2, i32 0, i32 2
  store i32 3, i32* %5, align 4
  %6 = getelementptr inbounds %struct.S, %struct.S* %2, i32 0, i32 0
  %7 = load i32, i32* %6, align 4
  %8 = getelementptr inbounds %struct.S, %struct.S* %2, i32 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %7, %9
  %11 = getelementptr inbounds %struct.S, %struct.S* %2, i32 0, i32 2
  %12 = load i32, i32* %11, align 4
  %13 = add nsw i32 %10, %12
  ret i32 %13
}

attributes #0 = { noinline norecurse nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 5.0.1 (tags/RELEASE_501/final 332326)"}

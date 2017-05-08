; ModuleID = 'glibc_and_intrinsics_1.ll.pp'
source_filename = "glibc_and_intrinsics_1.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nounwind uwtable
define i32 @_Z3foov() #0 {
  ret i32 42
}

; Function Attrs: nounwind uwtable
define i32 @_Z3barv() #0 {
  ret i32 13
}

; Function Attrs: norecurse uwtable
define i32 @main() #1 {
  %1 = call noalias i8* @malloc(i64 40) #6
  %2 = bitcast i8* %1 to i32*
  %3 = bitcast i32* %2 to i8*
  call void @llvm.memset.p0i8.i64(i8* %3, i8 0, i64 40, i32 4, i1 false)
  %4 = bitcast i32* %2 to i8*
  call void @free(i8* %4) #6
  %5 = call i8* @_Znwm(i64 4) #7
  %6 = bitcast i8* %5 to i32*
  store i32 13, i32* %6, align 4
  %7 = call i32 @_Z3foov()
  store i32 %7, i32* %6, align 4
  %8 = call i32 @_Z3barv()
  %9 = load i32, i32* %6, align 4
  %10 = add nsw i32 %9, %8
  store i32 %10, i32* %6, align 4
  %11 = icmp eq i32* %6, null
  br i1 %11, label %14, label %12

; <label>:12:                                     ; preds = %0
  %13 = bitcast i32* %6 to i8*
  call void @_ZdlPv(i8* %13) #8
  br label %14

; <label>:14:                                     ; preds = %12, %0
  ret i32 0
}

; Function Attrs: nounwind
declare noalias i8* @malloc(i64) #2

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i32, i1) #3

; Function Attrs: nounwind
declare void @free(i8*) #2

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #4

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(i8*) #5

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { norecurse uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { argmemonly nounwind }
attributes #4 = { nobuiltin "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { nobuiltin nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #6 = { nounwind }
attributes #7 = { builtin }
attributes #8 = { builtin nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.1-svn288847-1~exp1 (branches/release_39)"}

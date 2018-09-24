; ModuleID = 'dyn_mem.cpp'
source_filename = "dyn_mem.cpp"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.13.0"

%struct.Y = type { %struct.X, i32 }
%struct.X = type { i32, i32 }

@global = global i32 1, align 4

; Function Attrs: norecurse ssp uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32*, align 8
  %6 = alloca i32*, align 8
  %7 = alloca i32*, align 8
  %8 = alloca %struct.Y*, align 8
  store i32 0, i32* %1, align 4
  %9 = load i32, i32* %2, align 4
  %10 = load i32, i32* %3, align 4
  %11 = add nsw i32 %9, %10
  store i32 %11, i32* %4, align 4
  store i32* %2, i32** %5, align 8
  store i32* %3, i32** %6, align 8
  %12 = load i32*, i32** %6, align 8
  store i32 42, i32* %12, align 4
  %13 = call i8* @_Znwm(i64 4) #3
  %14 = bitcast i8* %13 to i32*
  store i32 13, i32* %14, align 4
  store i32* %14, i32** %7, align 8
  %15 = load i32, i32* %2, align 4
  %16 = load i32, i32* %3, align 4
  %17 = add nsw i32 %15, %16
  %18 = load i32*, i32** %7, align 8
  %19 = load i32, i32* %18, align 4
  %20 = add nsw i32 %19, %17
  store i32 %20, i32* %18, align 4
  %21 = load i32, i32* %4, align 4
  %22 = load i32*, i32** %7, align 8
  %23 = load i32, i32* %22, align 4
  %24 = add nsw i32 %23, %21
  store i32 %24, i32* %22, align 4
  %25 = load i32*, i32** %7, align 8
  %26 = icmp eq i32* %25, null
  br i1 %26, label %29, label %27

; <label>:27:                                     ; preds = %0
  %28 = bitcast i32* %25 to i8*
  call void @_ZdlPv(i8* %28) #4
  br label %29

; <label>:29:                                     ; preds = %27, %0
  %30 = call i8* @_Znwm(i64 12) #3
  %31 = bitcast i8* %30 to %struct.Y*
  store %struct.Y* %31, %struct.Y** %8, align 8
  %32 = load %struct.Y*, %struct.Y** %8, align 8
  %33 = icmp eq %struct.Y* %32, null
  br i1 %33, label %36, label %34

; <label>:34:                                     ; preds = %29
  %35 = bitcast %struct.Y* %32 to i8*
  call void @_ZdlPv(i8* %35) #4
  br label %36

; <label>:36:                                     ; preds = %34, %29
  ret i32 0
}

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #1

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(i8*) #2

attributes #0 = { norecurse ssp uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nobuiltin "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nobuiltin nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { builtin }
attributes #4 = { builtin nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"PIC Level", i32 2}
!1 = !{!"clang version 3.9.1 (tags/RELEASE_391/final)"}

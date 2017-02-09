; ModuleID = 'read.c'
source_filename = "read.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@.str.1 = private unnamed_addr constant [14 x i8] c"taint_cpy.txt\00", align 1

; Function Attrs: nounwind uwtable
define i32 @main(i32, i8**) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i8**, align 8
  %6 = alloca i32, align 4
  %7 = alloca i64, align 8
  %8 = alloca i8*, align 8
  %9 = alloca i32, align 4
  store i32 0, i32* %3, align 4
  store i32 %0, i32* %4, align 4
  store i8** %1, i8*** %5, align 8
  %10 = load i8**, i8*** %5, align 8
  %11 = getelementptr inbounds i8*, i8** %10, i64 1
  %12 = load i8*, i8** %11, align 8
  %13 = call i32 (i8*, i32, ...) @open(i8* %12, i32 0)
  store i32 %13, i32* %6, align 4
  %14 = load i32, i32* %6, align 4
  %15 = call i64 @lseek(i32 %14, i64 0, i32 2) #3
  store i64 %15, i64* %7, align 8
  %16 = load i32, i32* %6, align 4
  %17 = call i64 @lseek(i32 %16, i64 0, i32 0) #3
  %18 = load i64, i64* %7, align 8
  %19 = add nsw i64 %18, 1
  %20 = call noalias i8* @malloc(i64 %19) #3
  store i8* %20, i8** %8, align 8
  %21 = load i32, i32* %6, align 4
  %22 = load i8*, i8** %8, align 8
  %23 = load i64, i64* %7, align 8
  %24 = call i64 @read(i32 %21, i8* %22, i64 %23)
  %25 = load i64, i64* %7, align 8
  %26 = load i8*, i8** %8, align 8
  %27 = getelementptr inbounds i8, i8* %26, i64 %25
  store i8 0, i8* %27, align 1
  %28 = load i32, i32* %6, align 4
  %29 = call i32 @close(i32 %28)
  %30 = load i8*, i8** %8, align 8
  %31 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i32 0, i32 0), i8* %30)
  %32 = call i32 (i8*, i32, ...) @open(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @.str.1, i32 0, i32 0), i32 64)
  store i32 %32, i32* %9, align 4
  %33 = load i32, i32* %9, align 4
  %34 = load i8*, i8** %8, align 8
  %35 = load i64, i64* %7, align 8
  %36 = call i64 @write(i32 %33, i8* %34, i64 %35)
  %37 = load i32, i32* %9, align 4
  %38 = call i32 @close(i32 %37)
  %39 = load i8*, i8** %8, align 8
  call void @free(i8* %39) #3
  ret i32 0
}

declare i32 @open(i8*, i32, ...) #1

; Function Attrs: nounwind
declare i64 @lseek(i32, i64, i32) #2

; Function Attrs: nounwind
declare noalias i8* @malloc(i64) #2

declare i64 @read(i32, i8*, i64) #1

declare i32 @close(i32) #1

declare i32 @printf(i8*, ...) #1

declare i64 @write(i32, i8*, i64) #1

; Function Attrs: nounwind
declare void @free(i8*) #2

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

; ModuleID = 'fread.c'
source_filename = "fread.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@.str = private unnamed_addr constant [2 x i8] c"r\00", align 1
@.str.1 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@.str.2 = private unnamed_addr constant [14 x i8] c"taint_cpy.txt\00", align 1
@.str.3 = private unnamed_addr constant [3 x i8] c"a+\00", align 1

; Function Attrs: nounwind uwtable
define i32 @main(i32, i8**) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i8**, align 8
  %6 = alloca %struct._IO_FILE*, align 8
  %7 = alloca i64, align 8
  %8 = alloca i8*, align 8
  %9 = alloca %struct._IO_FILE*, align 8
  store i32 0, i32* %3, align 4
  store i32 %0, i32* %4, align 4
  store i8** %1, i8*** %5, align 8
  %10 = load i8**, i8*** %5, align 8
  %11 = getelementptr inbounds i8*, i8** %10, i64 1
  %12 = load i8*, i8** %11, align 8
  %13 = call %struct._IO_FILE* @fopen(i8* %12, i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str, i32 0, i32 0))
  store %struct._IO_FILE* %13, %struct._IO_FILE** %6, align 8
  %14 = load %struct._IO_FILE*, %struct._IO_FILE** %6, align 8
  %15 = call i32 @fseek(%struct._IO_FILE* %14, i64 0, i32 2)
  %16 = load %struct._IO_FILE*, %struct._IO_FILE** %6, align 8
  %17 = call i64 @ftell(%struct._IO_FILE* %16)
  store i64 %17, i64* %7, align 8
  %18 = load %struct._IO_FILE*, %struct._IO_FILE** %6, align 8
  call void @rewind(%struct._IO_FILE* %18)
  %19 = load i64, i64* %7, align 8
  %20 = add nsw i64 %19, 1
  %21 = call noalias i8* @malloc(i64 %20) #3
  store i8* %21, i8** %8, align 8
  %22 = load i8*, i8** %8, align 8
  %23 = load i64, i64* %7, align 8
  %24 = load %struct._IO_FILE*, %struct._IO_FILE** %6, align 8
  %25 = call i64 @fread(i8* %22, i64 %23, i64 1, %struct._IO_FILE* %24)
  %26 = load i64, i64* %7, align 8
  %27 = load i8*, i8** %8, align 8
  %28 = getelementptr inbounds i8, i8* %27, i64 %26
  store i8 0, i8* %28, align 1
  %29 = load %struct._IO_FILE*, %struct._IO_FILE** %6, align 8
  %30 = call i32 @fclose(%struct._IO_FILE* %29)
  %31 = load i8*, i8** %8, align 8
  %32 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.1, i32 0, i32 0), i8* %31)
  %33 = call %struct._IO_FILE* @fopen(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @.str.2, i32 0, i32 0), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.3, i32 0, i32 0))
  store %struct._IO_FILE* %33, %struct._IO_FILE** %9, align 8
  %34 = load i8*, i8** %8, align 8
  %35 = load i64, i64* %7, align 8
  %36 = load %struct._IO_FILE*, %struct._IO_FILE** %9, align 8
  %37 = call i64 @fwrite(i8* %34, i64 %35, i64 1, %struct._IO_FILE* %36)
  %38 = load %struct._IO_FILE*, %struct._IO_FILE** %9, align 8
  %39 = call i32 @fclose(%struct._IO_FILE* %38)
  %40 = load i8*, i8** %8, align 8
  call void @free(i8* %40) #3
  ret i32 0
}

declare %struct._IO_FILE* @fopen(i8*, i8*) #1

declare i32 @fseek(%struct._IO_FILE*, i64, i32) #1

declare i64 @ftell(%struct._IO_FILE*) #1

declare void @rewind(%struct._IO_FILE*) #1

; Function Attrs: nounwind
declare noalias i8* @malloc(i64) #2

declare i64 @fread(i8*, i64, i64, %struct._IO_FILE*) #1

declare i32 @fclose(%struct._IO_FILE*) #1

declare i32 @printf(i8*, ...) #1

declare i64 @fwrite(i8*, i64, i64, %struct._IO_FILE*) #1

; Function Attrs: nounwind
declare void @free(i8*) #2

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

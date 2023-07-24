; ModuleID = '/home/lucas/git/itst-develop/02_sa/intellisec-sast/analysis/test/resources/llvm_test_code/two_analyses_approach/ta_test_inter_true_positive.c'
source_filename = "/home/lucas/git/itst-develop/02_sa/intellisec-sast/analysis/test/resources/llvm_test_code/two_analyses_approach/ta_test_inter_true_positive.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, %struct._IO_codecvt*, %struct._IO_wide_data*, %struct._IO_FILE*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type opaque
%struct._IO_codecvt = type opaque
%struct._IO_wide_data = type opaque

@stdin = external dso_local global %struct._IO_FILE*, align 8

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fun1(i32* %v, i32 %x) #0 !dbg !10 {
entry:
  %v.addr = alloca i32*, align 8
  %x.addr = alloca i32, align 4
  store i32* %v, i32** %v.addr, align 8
  call void @llvm.dbg.declare(metadata i32** %v.addr, metadata !14, metadata !DIExpression()), !dbg !15
  store i32 %x, i32* %x.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %x.addr, metadata !16, metadata !DIExpression()), !dbg !17
  %0 = load i32, i32* %x.addr, align 4, !dbg !18
  %cmp = icmp sgt i32 %0, 10, !dbg !20
  br i1 %cmp, label %if.then, label %if.end, !dbg !21

if.then:                                          ; preds = %entry
  %1 = load i32*, i32** %v.addr, align 8, !dbg !22
  %2 = bitcast i32* %1 to i8*, !dbg !22
  call void @free(i8* %2) #4, !dbg !24
  br label %if.end, !dbg !25

if.end:                                           ; preds = %if.then, %entry
  ret void, !dbg !26
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind
declare dso_local void @free(i8*) #2

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 !dbg !27 {
entry:
  %retval = alloca i32, align 4
  %i = alloca i32, align 4
  %b = alloca [10 x i8], align 1
  %foo = alloca i32*, align 8
  %c = alloca i32, align 4
  store i32 0, i32* %retval, align 4
  call void @llvm.dbg.declare(metadata i32* %i, metadata !30, metadata !DIExpression()), !dbg !31
  store i32 30, i32* %i, align 4, !dbg !31
  call void @llvm.dbg.declare(metadata [10 x i8]* %b, metadata !32, metadata !DIExpression()), !dbg !37
  %arraydecay = getelementptr inbounds [10 x i8], [10 x i8]* %b, i64 0, i64 0, !dbg !38
  %0 = load %struct._IO_FILE*, %struct._IO_FILE** @stdin, align 8, !dbg !39
  %call = call i8* @fgets(i8* %arraydecay, i32 10, %struct._IO_FILE* %0), !dbg !40
  call void @llvm.dbg.declare(metadata i32** %foo, metadata !41, metadata !DIExpression()), !dbg !42
  %call1 = call noalias i8* @malloc(i64 32) #4, !dbg !43
  %1 = bitcast i8* %call1 to i32*, !dbg !44
  store i32* %1, i32** %foo, align 8, !dbg !42
  %2 = load i32*, i32** %foo, align 8, !dbg !45
  %3 = bitcast i32* %2 to i8*, !dbg !45
  call void @free(i8* %3) #4, !dbg !46
  call void @llvm.dbg.declare(metadata i32* %c, metadata !47, metadata !DIExpression()), !dbg !48
  %arraydecay2 = getelementptr inbounds [10 x i8], [10 x i8]* %b, i64 0, i64 0, !dbg !49
  %4 = ptrtoint i8* %arraydecay2 to i32, !dbg !49
  store i32 %4, i32* %c, align 4, !dbg !48
  %5 = load i32, i32* %c, align 4, !dbg !50
  %cmp = icmp sgt i32 %5, 40, !dbg !52
  br i1 %cmp, label %if.then, label %if.end, !dbg !53

if.then:                                          ; preds = %entry
  %6 = load i32*, i32** %foo, align 8, !dbg !54
  %7 = load i32, i32* %c, align 4, !dbg !56
  call void @fun1(i32* %6, i32 %7), !dbg !57
  br label %if.end, !dbg !58

if.end:                                           ; preds = %if.then, %entry
  %8 = load i32, i32* %i, align 4, !dbg !59
  %cmp3 = icmp eq i32 %8, 20, !dbg !61
  br i1 %cmp3, label %if.then4, label %if.end5, !dbg !62

if.then4:                                         ; preds = %if.end
  br label %if.end5, !dbg !63

if.end5:                                          ; preds = %if.then4, %if.end
  ret i32 0, !dbg !65
}

declare dso_local i8* @fgets(i8*, i32, %struct._IO_FILE*) #3

; Function Attrs: nounwind
declare dso_local noalias i8* @malloc(i64) #2

attributes #0 = { noinline nounwind optnone uwtable "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { nounwind "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!6, !7, !8}
!llvm.ident = !{!9}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 12.0.0 (https://github.com/llvm/llvm-project.git d28af7c654d8db0b68c175db5ce212d74fb5e9bc)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !3, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "/home/lucas/git/itst-develop/02_sa/intellisec-sast/analysis/test/resources/llvm_test_code/two_analyses_approach/ta_test_inter_true_positive.c", directory: "/home/lucas/git/itst-develop/02_sa/build/intellisec-sast/analysis/test/resources/llvm_test_code/two_analyses_approach")
!2 = !{}
!3 = !{!4}
!4 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !5, size: 64)
!5 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!6 = !{i32 7, !"Dwarf Version", i32 4}
!7 = !{i32 2, !"Debug Info Version", i32 3}
!8 = !{i32 1, !"wchar_size", i32 4}
!9 = !{!"clang version 12.0.0 (https://github.com/llvm/llvm-project.git d28af7c654d8db0b68c175db5ce212d74fb5e9bc)"}
!10 = distinct !DISubprogram(name: "fun1", scope: !11, file: !11, line: 6, type: !12, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!11 = !DIFile(filename: "intellisec-sast/analysis/test/resources/llvm_test_code/two_analyses_approach/ta_test_inter_true_positive.c", directory: "/home/lucas/git/itst-develop/02_sa")
!12 = !DISubroutineType(types: !13)
!13 = !{null, !4, !5}
!14 = !DILocalVariable(name: "v", arg: 1, scope: !10, file: !11, line: 6, type: !4)
!15 = !DILocation(line: 6, column: 16, scope: !10)
!16 = !DILocalVariable(name: "x", arg: 2, scope: !10, file: !11, line: 6, type: !5)
!17 = !DILocation(line: 6, column: 23, scope: !10)
!18 = !DILocation(line: 7, column: 9, scope: !19)
!19 = distinct !DILexicalBlock(scope: !10, file: !11, line: 7, column: 9)
!20 = !DILocation(line: 7, column: 11, scope: !19)
!21 = !DILocation(line: 7, column: 9, scope: !10)
!22 = !DILocation(line: 8, column: 14, scope: !23)
!23 = distinct !DILexicalBlock(scope: !19, file: !11, line: 7, column: 17)
!24 = !DILocation(line: 8, column: 9, scope: !23)
!25 = !DILocation(line: 9, column: 5, scope: !23)
!26 = !DILocation(line: 10, column: 1, scope: !10)
!27 = distinct !DISubprogram(name: "main", scope: !11, file: !11, line: 12, type: !28, scopeLine: 12, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!28 = !DISubroutineType(types: !29)
!29 = !{!5}
!30 = !DILocalVariable(name: "i", scope: !27, file: !11, line: 13, type: !5)
!31 = !DILocation(line: 13, column: 9, scope: !27)
!32 = !DILocalVariable(name: "b", scope: !27, file: !11, line: 14, type: !33)
!33 = !DICompositeType(tag: DW_TAG_array_type, baseType: !34, size: 80, elements: !35)
!34 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!35 = !{!36}
!36 = !DISubrange(count: 10)
!37 = !DILocation(line: 14, column: 10, scope: !27)
!38 = !DILocation(line: 15, column: 11, scope: !27)
!39 = !DILocation(line: 15, column: 18, scope: !27)
!40 = !DILocation(line: 15, column: 5, scope: !27)
!41 = !DILocalVariable(name: "foo", scope: !27, file: !11, line: 16, type: !4)
!42 = !DILocation(line: 16, column: 10, scope: !27)
!43 = !DILocation(line: 16, column: 23, scope: !27)
!44 = !DILocation(line: 16, column: 16, scope: !27)
!45 = !DILocation(line: 17, column: 10, scope: !27)
!46 = !DILocation(line: 17, column: 5, scope: !27)
!47 = !DILocalVariable(name: "c", scope: !27, file: !11, line: 18, type: !5)
!48 = !DILocation(line: 18, column: 9, scope: !27)
!49 = !DILocation(line: 18, column: 13, scope: !27)
!50 = !DILocation(line: 19, column: 9, scope: !51)
!51 = distinct !DILexicalBlock(scope: !27, file: !11, line: 19, column: 9)
!52 = !DILocation(line: 19, column: 11, scope: !51)
!53 = !DILocation(line: 19, column: 9, scope: !27)
!54 = !DILocation(line: 20, column: 14, scope: !55)
!55 = distinct !DILexicalBlock(scope: !51, file: !11, line: 19, column: 17)
!56 = !DILocation(line: 20, column: 19, scope: !55)
!57 = !DILocation(line: 20, column: 9, scope: !55)
!58 = !DILocation(line: 21, column: 5, scope: !55)
!59 = !DILocation(line: 23, column: 9, scope: !60)
!60 = distinct !DILexicalBlock(scope: !27, file: !11, line: 23, column: 9)
!61 = !DILocation(line: 23, column: 11, scope: !60)
!62 = !DILocation(line: 23, column: 9, scope: !27)
!63 = !DILocation(line: 25, column: 5, scope: !64)
!64 = distinct !DILexicalBlock(scope: !60, file: !11, line: 23, column: 18)
!65 = !DILocation(line: 26, column: 5, scope: !27)

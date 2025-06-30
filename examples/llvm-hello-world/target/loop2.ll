; ModuleID = 'loop2.cpp'
source_filename = "loop2.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress noinline norecurse nounwind optnone uwtable
define dso_local noundef i32 @main() #0 !dbg !10 {
entry:
  %retval = alloca i32, align 4
  %n = alloca i32, align 4
  %sum = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  call void @llvm.dbg.declare(metadata ptr %n, metadata !15, metadata !DIExpression()), !dbg !16
  store i32 10, ptr %n, align 4, !dbg !16
  call void @llvm.dbg.declare(metadata ptr %sum, metadata !17, metadata !DIExpression()), !dbg !18
  store i32 0, ptr %sum, align 4, !dbg !18
  br label %while.cond, !dbg !19

while.cond:                                       ; preds = %while.body, %entry
  %0 = load i32, ptr %n, align 4, !dbg !20
  %dec = add nsw i32 %0, -1, !dbg !20
  store i32 %dec, ptr %n, align 4, !dbg !20
  %cmp = icmp sgt i32 %0, 0, !dbg !21
  br i1 %cmp, label %while.body, label %while.end, !dbg !19

while.body:                                       ; preds = %while.cond
  %1 = load i32, ptr %n, align 4, !dbg !22
  %2 = load i32, ptr %sum, align 4, !dbg !24
  %add = add nsw i32 %2, %1, !dbg !24
  store i32 %add, ptr %sum, align 4, !dbg !24
  br label %while.cond, !dbg !19, !llvm.loop !25

while.end:                                        ; preds = %while.cond
  ret i32 0, !dbg !28
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

attributes #0 = { mustprogress noinline norecurse nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}
!llvm.ident = !{!9}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "loop2.cpp", directory: "phasar/examples/llvm-hello-world/target", checksumkind: CSK_MD5, checksum: "44995f900f3acf1838287c2cf9c6a7cd")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 2}
!9 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!10 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 1, type: !11, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !14)
!11 = !DISubroutineType(types: !12)
!12 = !{!13}
!13 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!14 = !{}
!15 = !DILocalVariable(name: "n", scope: !10, file: !1, line: 2, type: !13)
!16 = !DILocation(line: 2, column: 7, scope: !10)
!17 = !DILocalVariable(name: "sum", scope: !10, file: !1, line: 3, type: !13)
!18 = !DILocation(line: 3, column: 7, scope: !10)
!19 = !DILocation(line: 4, column: 3, scope: !10)
!20 = !DILocation(line: 4, column: 11, scope: !10)
!21 = !DILocation(line: 4, column: 14, scope: !10)
!22 = !DILocation(line: 5, column: 12, scope: !23)
!23 = distinct !DILexicalBlock(scope: !10, file: !1, line: 4, column: 19)
!24 = !DILocation(line: 5, column: 9, scope: !23)
!25 = distinct !{!25, !19, !26, !27}
!26 = !DILocation(line: 6, column: 3, scope: !10)
!27 = !{!"llvm.loop.mustprogress"}
!28 = !DILocation(line: 7, column: 3, scope: !10)

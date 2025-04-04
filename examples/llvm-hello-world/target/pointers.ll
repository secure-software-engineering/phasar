; ModuleID = 'pointers.cpp'
source_filename = "pointers.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress noinline norecurse nounwind optnone uwtable
define dso_local noundef i32 @main(i32 noundef %argc, ptr noundef %argv) #0 !dbg !10 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca ptr, align 8
  %p = alloca ptr, align 8
  %i = alloca i32, align 4
  %j = alloca ptr, align 8
  store i32 0, ptr %retval, align 4
  store i32 %argc, ptr %argc.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %argc.addr, metadata !18, metadata !DIExpression()), !dbg !19
  store ptr %argv, ptr %argv.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %argv.addr, metadata !20, metadata !DIExpression()), !dbg !21
  call void @llvm.dbg.declare(metadata ptr %p, metadata !22, metadata !DIExpression()), !dbg !23
  %0 = load ptr, ptr %argv.addr, align 8, !dbg !24
  %arrayidx = getelementptr inbounds ptr, ptr %0, i64 0, !dbg !24
  %1 = load ptr, ptr %arrayidx, align 8, !dbg !24
  store ptr %1, ptr %p, align 8, !dbg !23
  call void @llvm.dbg.declare(metadata ptr %i, metadata !25, metadata !DIExpression()), !dbg !26
  store i32 42, ptr %i, align 4, !dbg !26
  call void @llvm.dbg.declare(metadata ptr %j, metadata !27, metadata !DIExpression()), !dbg !29
  store ptr %i, ptr %j, align 8, !dbg !29
  %2 = load ptr, ptr %j, align 8, !dbg !30
  %3 = load i32, ptr %2, align 4, !dbg !31
  %add = add nsw i32 %3, 13, !dbg !31
  store i32 %add, ptr %2, align 4, !dbg !31
  %4 = load i32, ptr %i, align 4, !dbg !32
  ret i32 %4, !dbg !33
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

attributes #0 = { mustprogress noinline norecurse nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}
!llvm.ident = !{!9}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "pointers.cpp", directory: "phasar/examples/llvm-hello-world/target", checksumkind: CSK_MD5, checksum: "c18fec392d76cd372065fb31e5a32029")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 2}
!9 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!10 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 1, type: !11, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !17)
!11 = !DISubroutineType(types: !12)
!12 = !{!13, !13, !14}
!13 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!14 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !15, size: 64)
!15 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !16, size: 64)
!16 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!17 = !{}
!18 = !DILocalVariable(name: "argc", arg: 1, scope: !10, file: !1, line: 1, type: !13)
!19 = !DILocation(line: 1, column: 14, scope: !10)
!20 = !DILocalVariable(name: "argv", arg: 2, scope: !10, file: !1, line: 1, type: !14)
!21 = !DILocation(line: 1, column: 27, scope: !10)
!22 = !DILocalVariable(name: "p", scope: !10, file: !1, line: 2, type: !15)
!23 = !DILocation(line: 2, column: 9, scope: !10)
!24 = !DILocation(line: 2, column: 13, scope: !10)
!25 = !DILocalVariable(name: "i", scope: !10, file: !1, line: 3, type: !13)
!26 = !DILocation(line: 3, column: 7, scope: !10)
!27 = !DILocalVariable(name: "j", scope: !10, file: !1, line: 4, type: !28)
!28 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !13, size: 64)
!29 = !DILocation(line: 4, column: 8, scope: !10)
!30 = !DILocation(line: 5, column: 4, scope: !10)
!31 = !DILocation(line: 5, column: 6, scope: !10)
!32 = !DILocation(line: 6, column: 10, scope: !10)
!33 = !DILocation(line: 6, column: 3, scope: !10)

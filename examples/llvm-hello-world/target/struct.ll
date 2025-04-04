; ModuleID = 'struct.cpp'
source_filename = "struct.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.S = type { i32, i32, i32 }

; Function Attrs: mustprogress noinline norecurse nounwind optnone uwtable
define dso_local noundef i32 @main() #0 !dbg !10 {
entry:
  %retval = alloca i32, align 4
  %s = alloca %struct.S, align 4
  store i32 0, ptr %retval, align 4
  call void @llvm.dbg.declare(metadata ptr %s, metadata !15, metadata !DIExpression()), !dbg !21
  %i = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 0, !dbg !22
  store i32 1, ptr %i, align 4, !dbg !23
  %j = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 1, !dbg !24
  store i32 2, ptr %j, align 4, !dbg !25
  %k = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 2, !dbg !26
  store i32 3, ptr %k, align 4, !dbg !27
  %i1 = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 0, !dbg !28
  %0 = load i32, ptr %i1, align 4, !dbg !28
  %j2 = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 1, !dbg !29
  %1 = load i32, ptr %j2, align 4, !dbg !29
  %add = add nsw i32 %0, %1, !dbg !30
  %k3 = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 2, !dbg !31
  %2 = load i32, ptr %k3, align 4, !dbg !31
  %add4 = add nsw i32 %add, %2, !dbg !32
  ret i32 %add4, !dbg !33
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

attributes #0 = { mustprogress noinline norecurse nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}
!llvm.ident = !{!9}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "struct.cpp", directory: "phasar/examples/llvm-hello-world/target", checksumkind: CSK_MD5, checksum: "29f5ab36c820666cfae36d31b65e1b2d")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 2}
!9 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!10 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 5, type: !11, scopeLine: 5, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !14)
!11 = !DISubroutineType(types: !12)
!12 = !{!13}
!13 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!14 = !{}
!15 = !DILocalVariable(name: "s", scope: !10, file: !1, line: 6, type: !16)
!16 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "S", file: !1, line: 1, size: 96, flags: DIFlagTypePassByValue, elements: !17, identifier: "_ZTS1S")
!17 = !{!18, !19, !20}
!18 = !DIDerivedType(tag: DW_TAG_member, name: "i", scope: !16, file: !1, line: 2, baseType: !13, size: 32)
!19 = !DIDerivedType(tag: DW_TAG_member, name: "j", scope: !16, file: !1, line: 2, baseType: !13, size: 32, offset: 32)
!20 = !DIDerivedType(tag: DW_TAG_member, name: "k", scope: !16, file: !1, line: 2, baseType: !13, size: 32, offset: 64)
!21 = !DILocation(line: 6, column: 5, scope: !10)
!22 = !DILocation(line: 7, column: 5, scope: !10)
!23 = !DILocation(line: 7, column: 7, scope: !10)
!24 = !DILocation(line: 8, column: 5, scope: !10)
!25 = !DILocation(line: 8, column: 7, scope: !10)
!26 = !DILocation(line: 9, column: 5, scope: !10)
!27 = !DILocation(line: 9, column: 7, scope: !10)
!28 = !DILocation(line: 10, column: 12, scope: !10)
!29 = !DILocation(line: 10, column: 18, scope: !10)
!30 = !DILocation(line: 10, column: 14, scope: !10)
!31 = !DILocation(line: 10, column: 24, scope: !10)
!32 = !DILocation(line: 10, column: 20, scope: !10)
!33 = !DILocation(line: 10, column: 3, scope: !10)

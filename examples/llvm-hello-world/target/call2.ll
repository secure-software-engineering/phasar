; ModuleID = 'call2.cpp'
source_filename = "call2.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define dso_local noundef i32 @_Z2idi(i32 noundef %i) #0 !dbg !10 {
entry:
  %i.addr = alloca i32, align 4
  store i32 %i, ptr %i.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %i.addr, metadata !15, metadata !DIExpression()), !dbg !16
  %0 = load i32, ptr %i.addr, align 4, !dbg !17
  ret i32 %0, !dbg !18
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define dso_local noundef i32 @_Z3inci(i32 noundef %i) #0 !dbg !19 {
entry:
  %i.addr = alloca i32, align 4
  store i32 %i, ptr %i.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %i.addr, metadata !20, metadata !DIExpression()), !dbg !21
  %0 = load i32, ptr %i.addr, align 4, !dbg !22
  %inc = add nsw i32 %0, 1, !dbg !22
  store i32 %inc, ptr %i.addr, align 4, !dbg !22
  ret i32 %inc, !dbg !23
}

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define dso_local noundef i32 @_Z3addii(i32 noundef %i, i32 noundef %j) #0 !dbg !24 {
entry:
  %i.addr = alloca i32, align 4
  %j.addr = alloca i32, align 4
  store i32 %i, ptr %i.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %i.addr, metadata !27, metadata !DIExpression()), !dbg !28
  store i32 %j, ptr %j.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %j.addr, metadata !29, metadata !DIExpression()), !dbg !30
  %0 = load i32, ptr %i.addr, align 4, !dbg !31
  %1 = load i32, ptr %j.addr, align 4, !dbg !32
  %add = add nsw i32 %0, %1, !dbg !33
  ret i32 %add, !dbg !34
}

; Function Attrs: mustprogress noinline norecurse nounwind optnone uwtable
define dso_local noundef i32 @main() #2 !dbg !35 {
entry:
  %retval = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  call void @llvm.dbg.declare(metadata ptr %a, metadata !38, metadata !DIExpression()), !dbg !39
  store i32 0, ptr %a, align 4, !dbg !39
  call void @llvm.dbg.declare(metadata ptr %b, metadata !40, metadata !DIExpression()), !dbg !41
  store i32 1, ptr %b, align 4, !dbg !41
  %0 = load i32, ptr %a, align 4, !dbg !42
  %call = call noundef i32 @_Z3inci(i32 noundef %0), !dbg !43
  store i32 %call, ptr %a, align 4, !dbg !44
  %1 = load i32, ptr %b, align 4, !dbg !45
  %call1 = call noundef i32 @_Z2idi(i32 noundef %1), !dbg !46
  store i32 %call1, ptr %b, align 4, !dbg !47
  call void @llvm.dbg.declare(metadata ptr %c, metadata !48, metadata !DIExpression()), !dbg !49
  %2 = load i32, ptr %a, align 4, !dbg !50
  %3 = load i32, ptr %b, align 4, !dbg !51
  %call2 = call noundef i32 @_Z3addii(i32 noundef %2, i32 noundef %3), !dbg !52
  store i32 %call2, ptr %c, align 4, !dbg !49
  ret i32 0, !dbg !53
}

attributes #0 = { mustprogress noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { mustprogress noinline norecurse nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}
!llvm.ident = !{!9}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "call2.cpp", directory: "phasar/examples/llvm-hello-world/target", checksumkind: CSK_MD5, checksum: "3183ac742a48f0a0d62574e2366f9a9f")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 2}
!9 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!10 = distinct !DISubprogram(name: "id", linkageName: "_Z2idi", scope: !1, file: !1, line: 1, type: !11, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !14)
!11 = !DISubroutineType(types: !12)
!12 = !{!13, !13}
!13 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!14 = !{}
!15 = !DILocalVariable(name: "i", arg: 1, scope: !10, file: !1, line: 1, type: !13)
!16 = !DILocation(line: 1, column: 12, scope: !10)
!17 = !DILocation(line: 1, column: 24, scope: !10)
!18 = !DILocation(line: 1, column: 17, scope: !10)
!19 = distinct !DISubprogram(name: "inc", linkageName: "_Z3inci", scope: !1, file: !1, line: 3, type: !11, scopeLine: 3, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !14)
!20 = !DILocalVariable(name: "i", arg: 1, scope: !19, file: !1, line: 3, type: !13)
!21 = !DILocation(line: 3, column: 13, scope: !19)
!22 = !DILocation(line: 3, column: 25, scope: !19)
!23 = !DILocation(line: 3, column: 18, scope: !19)
!24 = distinct !DISubprogram(name: "add", linkageName: "_Z3addii", scope: !1, file: !1, line: 5, type: !25, scopeLine: 5, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !14)
!25 = !DISubroutineType(types: !26)
!26 = !{!13, !13, !13}
!27 = !DILocalVariable(name: "i", arg: 1, scope: !24, file: !1, line: 5, type: !13)
!28 = !DILocation(line: 5, column: 13, scope: !24)
!29 = !DILocalVariable(name: "j", arg: 2, scope: !24, file: !1, line: 5, type: !13)
!30 = !DILocation(line: 5, column: 20, scope: !24)
!31 = !DILocation(line: 5, column: 32, scope: !24)
!32 = !DILocation(line: 5, column: 36, scope: !24)
!33 = !DILocation(line: 5, column: 34, scope: !24)
!34 = !DILocation(line: 5, column: 25, scope: !24)
!35 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 7, type: !36, scopeLine: 7, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !14)
!36 = !DISubroutineType(types: !37)
!37 = !{!13}
!38 = !DILocalVariable(name: "a", scope: !35, file: !1, line: 8, type: !13)
!39 = !DILocation(line: 8, column: 7, scope: !35)
!40 = !DILocalVariable(name: "b", scope: !35, file: !1, line: 9, type: !13)
!41 = !DILocation(line: 9, column: 7, scope: !35)
!42 = !DILocation(line: 10, column: 11, scope: !35)
!43 = !DILocation(line: 10, column: 7, scope: !35)
!44 = !DILocation(line: 10, column: 5, scope: !35)
!45 = !DILocation(line: 11, column: 10, scope: !35)
!46 = !DILocation(line: 11, column: 7, scope: !35)
!47 = !DILocation(line: 11, column: 5, scope: !35)
!48 = !DILocalVariable(name: "c", scope: !35, file: !1, line: 12, type: !13)
!49 = !DILocation(line: 12, column: 7, scope: !35)
!50 = !DILocation(line: 12, column: 15, scope: !35)
!51 = !DILocation(line: 12, column: 18, scope: !35)
!52 = !DILocation(line: 12, column: 11, scope: !35)
!53 = !DILocation(line: 13, column: 3, scope: !35)

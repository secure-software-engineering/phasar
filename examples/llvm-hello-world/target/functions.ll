; ModuleID = 'functions.cpp'
source_filename = "functions.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define dso_local noundef i32 @_Z3addii(i32 noundef %a, i32 noundef %b) #0 !dbg !10 {
entry:
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  store i32 %a, ptr %a.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %a.addr, metadata !15, metadata !DIExpression()), !dbg !16
  store i32 %b, ptr %b.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %b.addr, metadata !17, metadata !DIExpression()), !dbg !18
  %0 = load i32, ptr %a.addr, align 4, !dbg !19
  %1 = load i32, ptr %b.addr, align 4, !dbg !20
  %add = add nsw i32 %0, %1, !dbg !21
  ret i32 %add, !dbg !22
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define dso_local void @_Z3incRi(ptr noundef nonnull align 4 dereferenceable(4) %i) #0 !dbg !23 {
entry:
  %i.addr = alloca ptr, align 8
  store ptr %i, ptr %i.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %i.addr, metadata !27, metadata !DIExpression()), !dbg !28
  %0 = load ptr, ptr %i.addr, align 8, !dbg !29
  %1 = load i32, ptr %0, align 4, !dbg !30
  %inc = add nsw i32 %1, 1, !dbg !30
  store i32 %inc, ptr %0, align 4, !dbg !30
  ret void, !dbg !31
}

; Function Attrs: mustprogress noinline norecurse nounwind optnone uwtable
define dso_local noundef i32 @main(i32 noundef %argc, ptr noundef %argv) #2 !dbg !32 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca ptr, align 8
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  store i32 %argc, ptr %argc.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %argc.addr, metadata !38, metadata !DIExpression()), !dbg !39
  store ptr %argv, ptr %argv.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %argv.addr, metadata !40, metadata !DIExpression()), !dbg !41
  call void @llvm.dbg.declare(metadata ptr %a, metadata !42, metadata !DIExpression()), !dbg !43
  store i32 10, ptr %a, align 4, !dbg !43
  call void @llvm.dbg.declare(metadata ptr %b, metadata !44, metadata !DIExpression()), !dbg !45
  %0 = load i32, ptr %a, align 4, !dbg !46
  %call = call noundef i32 @_Z3addii(i32 noundef %0, i32 noundef 42), !dbg !47
  store i32 %call, ptr %b, align 4, !dbg !45
  call void @_Z3incRi(ptr noundef nonnull align 4 dereferenceable(4) %b), !dbg !48
  call void @llvm.dbg.declare(metadata ptr %c, metadata !49, metadata !DIExpression()), !dbg !50
  %1 = load i32, ptr %b, align 4, !dbg !51
  store i32 %1, ptr %c, align 4, !dbg !50
  ret i32 0, !dbg !52
}

attributes #0 = { mustprogress noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { mustprogress noinline norecurse nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}
!llvm.ident = !{!9}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "functions.cpp", directory: "phasar/examples/llvm-hello-world/target", checksumkind: CSK_MD5, checksum: "8f8ca1df314e426af2fde812f819fac4")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 2}
!9 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!10 = distinct !DISubprogram(name: "add", linkageName: "_Z3addii", scope: !1, file: !1, line: 1, type: !11, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !14)
!11 = !DISubroutineType(types: !12)
!12 = !{!13, !13, !13}
!13 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!14 = !{}
!15 = !DILocalVariable(name: "a", arg: 1, scope: !10, file: !1, line: 1, type: !13)
!16 = !DILocation(line: 1, column: 13, scope: !10)
!17 = !DILocalVariable(name: "b", arg: 2, scope: !10, file: !1, line: 1, type: !13)
!18 = !DILocation(line: 1, column: 20, scope: !10)
!19 = !DILocation(line: 1, column: 32, scope: !10)
!20 = !DILocation(line: 1, column: 36, scope: !10)
!21 = !DILocation(line: 1, column: 34, scope: !10)
!22 = !DILocation(line: 1, column: 25, scope: !10)
!23 = distinct !DISubprogram(name: "inc", linkageName: "_Z3incRi", scope: !1, file: !1, line: 3, type: !24, scopeLine: 3, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !14)
!24 = !DISubroutineType(types: !25)
!25 = !{null, !26}
!26 = !DIDerivedType(tag: DW_TAG_reference_type, baseType: !13, size: 64)
!27 = !DILocalVariable(name: "i", arg: 1, scope: !23, file: !1, line: 3, type: !26)
!28 = !DILocation(line: 3, column: 15, scope: !23)
!29 = !DILocation(line: 3, column: 22, scope: !23)
!30 = !DILocation(line: 3, column: 20, scope: !23)
!31 = !DILocation(line: 3, column: 25, scope: !23)
!32 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 5, type: !33, scopeLine: 5, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !14)
!33 = !DISubroutineType(types: !34)
!34 = !{!13, !13, !35}
!35 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !36, size: 64)
!36 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !37, size: 64)
!37 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!38 = !DILocalVariable(name: "argc", arg: 1, scope: !32, file: !1, line: 5, type: !13)
!39 = !DILocation(line: 5, column: 14, scope: !32)
!40 = !DILocalVariable(name: "argv", arg: 2, scope: !32, file: !1, line: 5, type: !35)
!41 = !DILocation(line: 5, column: 27, scope: !32)
!42 = !DILocalVariable(name: "a", scope: !32, file: !1, line: 6, type: !13)
!43 = !DILocation(line: 6, column: 7, scope: !32)
!44 = !DILocalVariable(name: "b", scope: !32, file: !1, line: 7, type: !13)
!45 = !DILocation(line: 7, column: 7, scope: !32)
!46 = !DILocation(line: 7, column: 15, scope: !32)
!47 = !DILocation(line: 7, column: 11, scope: !32)
!48 = !DILocation(line: 8, column: 3, scope: !32)
!49 = !DILocalVariable(name: "c", scope: !32, file: !1, line: 9, type: !13)
!50 = !DILocation(line: 9, column: 7, scope: !32)
!51 = !DILocation(line: 9, column: 11, scope: !32)
!52 = !DILocation(line: 10, column: 3, scope: !32)

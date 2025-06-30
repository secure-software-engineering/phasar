; ModuleID = 'struct2.cpp'
source_filename = "struct2.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.S = type { i32, i32 }

; Function Attrs: mustprogress noinline norecurse nounwind optnone uwtable
define dso_local noundef i32 @main(i32 noundef %argc, ptr noundef %argv) #0 !dbg !10 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca ptr, align 8
  %s = alloca %struct.S, align 4
  %z = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  store i32 %argc, ptr %argc.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %argc.addr, metadata !18, metadata !DIExpression()), !dbg !19
  store ptr %argv, ptr %argv.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %argv.addr, metadata !20, metadata !DIExpression()), !dbg !21
  call void @llvm.dbg.declare(metadata ptr %s, metadata !22, metadata !DIExpression()), !dbg !27
  %0 = load i32, ptr %argc.addr, align 4, !dbg !28
  %sub = sub nsw i32 %0, 1, !dbg !30
  %tobool = icmp ne i32 %sub, 0, !dbg !28
  br i1 %tobool, label %if.then, label %if.else, !dbg !31

if.then:                                          ; preds = %entry
  %x = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 0, !dbg !32
  store i32 4, ptr %x, align 4, !dbg !34
  %y = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 1, !dbg !35
  store i32 5, ptr %y, align 4, !dbg !36
  br label %if.end, !dbg !37

if.else:                                          ; preds = %entry
  %x1 = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 0, !dbg !38
  store i32 40, ptr %x1, align 4, !dbg !40
  %y2 = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 1, !dbg !41
  store i32 50, ptr %y2, align 4, !dbg !42
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  call void @llvm.dbg.declare(metadata ptr %z, metadata !43, metadata !DIExpression()), !dbg !44
  %x3 = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 0, !dbg !45
  %1 = load i32, ptr %x3, align 4, !dbg !45
  %y4 = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 1, !dbg !46
  %2 = load i32, ptr %y4, align 4, !dbg !46
  %add = add nsw i32 %1, %2, !dbg !47
  store i32 %add, ptr %z, align 4, !dbg !44
  %3 = load i32, ptr %z, align 4, !dbg !48
  ret i32 %3, !dbg !49
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

attributes #0 = { mustprogress noinline norecurse nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}
!llvm.ident = !{!9}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "struct2.cpp", directory: "phasar/examples/llvm-hello-world/target", checksumkind: CSK_MD5, checksum: "dcfc377f80d9d75aede4bfb21faad130")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 2}
!9 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!10 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 6, type: !11, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !17)
!11 = !DISubroutineType(types: !12)
!12 = !{!13, !13, !14}
!13 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!14 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !15, size: 64)
!15 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !16, size: 64)
!16 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!17 = !{}
!18 = !DILocalVariable(name: "argc", arg: 1, scope: !10, file: !1, line: 6, type: !13)
!19 = !DILocation(line: 6, column: 14, scope: !10)
!20 = !DILocalVariable(name: "argv", arg: 2, scope: !10, file: !1, line: 6, type: !14)
!21 = !DILocation(line: 6, column: 27, scope: !10)
!22 = !DILocalVariable(name: "s", scope: !10, file: !1, line: 7, type: !23)
!23 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "S", file: !1, line: 1, size: 64, flags: DIFlagTypePassByValue, elements: !24, identifier: "_ZTS1S")
!24 = !{!25, !26}
!25 = !DIDerivedType(tag: DW_TAG_member, name: "x", scope: !23, file: !1, line: 2, baseType: !13, size: 32)
!26 = !DIDerivedType(tag: DW_TAG_member, name: "y", scope: !23, file: !1, line: 3, baseType: !13, size: 32, offset: 32)
!27 = !DILocation(line: 7, column: 5, scope: !10)
!28 = !DILocation(line: 8, column: 7, scope: !29)
!29 = distinct !DILexicalBlock(scope: !10, file: !1, line: 8, column: 7)
!30 = !DILocation(line: 8, column: 12, scope: !29)
!31 = !DILocation(line: 8, column: 7, scope: !10)
!32 = !DILocation(line: 9, column: 7, scope: !33)
!33 = distinct !DILexicalBlock(scope: !29, file: !1, line: 8, column: 17)
!34 = !DILocation(line: 9, column: 9, scope: !33)
!35 = !DILocation(line: 10, column: 7, scope: !33)
!36 = !DILocation(line: 10, column: 9, scope: !33)
!37 = !DILocation(line: 11, column: 3, scope: !33)
!38 = !DILocation(line: 12, column: 7, scope: !39)
!39 = distinct !DILexicalBlock(scope: !29, file: !1, line: 11, column: 10)
!40 = !DILocation(line: 12, column: 9, scope: !39)
!41 = !DILocation(line: 13, column: 7, scope: !39)
!42 = !DILocation(line: 13, column: 9, scope: !39)
!43 = !DILocalVariable(name: "z", scope: !10, file: !1, line: 15, type: !13)
!44 = !DILocation(line: 15, column: 7, scope: !10)
!45 = !DILocation(line: 15, column: 13, scope: !10)
!46 = !DILocation(line: 15, column: 19, scope: !10)
!47 = !DILocation(line: 15, column: 15, scope: !10)
!48 = !DILocation(line: 16, column: 10, scope: !10)
!49 = !DILocation(line: 16, column: 3, scope: !10)

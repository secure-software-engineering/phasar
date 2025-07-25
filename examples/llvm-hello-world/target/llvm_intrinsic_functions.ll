; ModuleID = 'llvm_intrinsic_functions.cpp'
source_filename = "llvm_intrinsic_functions.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress noinline norecurse nounwind optnone uwtable
define dso_local noundef i32 @main() #0 !dbg !103 {
entry:
  %retval = alloca i32, align 4
  %buffer = alloca [10 x i8], align 1
  %i = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  call void @llvm.dbg.declare(metadata ptr %buffer, metadata !107, metadata !DIExpression()), !dbg !111
  %arraydecay = getelementptr inbounds [10 x i8], ptr %buffer, i64 0, i64 0, !dbg !112
  call void @llvm.memset.p0.i64(ptr align 1 %arraydecay, i8 0, i64 10, i1 false), !dbg !112
  call void @llvm.dbg.declare(metadata ptr %i, metadata !113, metadata !DIExpression()), !dbg !114
  %arrayidx = getelementptr inbounds [10 x i8], ptr %buffer, i64 0, i64 3, !dbg !115
  %0 = load i8, ptr %arrayidx, align 1, !dbg !115
  %conv = sext i8 %0 to i32, !dbg !115
  %add = add nsw i32 %conv, 42, !dbg !116
  store i32 %add, ptr %i, align 4, !dbg !114
  ret i32 0, !dbg !117
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: argmemonly nocallback nofree nounwind willreturn writeonly
declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg) #2

attributes #0 = { mustprogress noinline norecurse nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { argmemonly nocallback nofree nounwind willreturn writeonly }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!95, !96, !97, !98, !99, !100, !101}
!llvm.ident = !{!102}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, imports: !2, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "llvm_intrinsic_functions.cpp", directory: "phasar/examples/llvm-hello-world/target", checksumkind: CSK_MD5, checksum: "1251fce58a000c10750f957d831af1d6")
!2 = !{!3, !16, !20, !27, !31, !35, !45, !49, !51, !53, !57, !61, !65, !69, !73, !75, !77, !79, !83, !87, !91, !93}
!3 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !5, file: !15, line: 77)
!4 = !DINamespace(name: "std", scope: null)
!5 = !DISubprogram(name: "memchr", scope: !6, file: !6, line: 89, type: !7, flags: DIFlagPrototyped, spFlags: 0)
!6 = !DIFile(filename: "/usr/include/string.h", directory: "", checksumkind: CSK_MD5, checksum: "165db1185644f68894fa9e0d17055d70")
!7 = !DISubroutineType(types: !8)
!8 = !{!9, !9, !11, !12}
!9 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !10, size: 64)
!10 = !DIDerivedType(tag: DW_TAG_const_type, baseType: null)
!11 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!12 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !13, line: 46, baseType: !14)
!13 = !DIFile(filename: "/usr/local/llvm-15/lib/clang/15.0.7/include/stddef.h", directory: "", checksumkind: CSK_MD5, checksum: "b76978376d35d5cd171876ac58ac1256")
!14 = !DIBasicType(name: "unsigned long", size: 64, encoding: DW_ATE_unsigned)
!15 = !DIFile(filename: "/usr/lib/gcc/x86_64-linux-gnu/13/../../../../include/c++/13/cstring", directory: "")
!16 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !17, file: !15, line: 78)
!17 = !DISubprogram(name: "memcmp", scope: !6, file: !6, line: 64, type: !18, flags: DIFlagPrototyped, spFlags: 0)
!18 = !DISubroutineType(types: !19)
!19 = !{!11, !9, !9, !12}
!20 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !21, file: !15, line: 79)
!21 = !DISubprogram(name: "memcpy", scope: !6, file: !6, line: 43, type: !22, flags: DIFlagPrototyped, spFlags: 0)
!22 = !DISubroutineType(types: !23)
!23 = !{!24, !25, !26, !12}
!24 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!25 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !24)
!26 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !9)
!27 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !28, file: !15, line: 80)
!28 = !DISubprogram(name: "memmove", scope: !6, file: !6, line: 47, type: !29, flags: DIFlagPrototyped, spFlags: 0)
!29 = !DISubroutineType(types: !30)
!30 = !{!24, !24, !9, !12}
!31 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !32, file: !15, line: 81)
!32 = !DISubprogram(name: "memset", scope: !6, file: !6, line: 61, type: !33, flags: DIFlagPrototyped, spFlags: 0)
!33 = !DISubroutineType(types: !34)
!34 = !{!24, !24, !11, !12}
!35 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !36, file: !15, line: 82)
!36 = !DISubprogram(name: "strcat", scope: !6, file: !6, line: 149, type: !37, flags: DIFlagPrototyped, spFlags: 0)
!37 = !DISubroutineType(types: !38)
!38 = !{!39, !41, !42}
!39 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !40, size: 64)
!40 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!41 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !39)
!42 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !43)
!43 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !44, size: 64)
!44 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !40)
!45 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !46, file: !15, line: 83)
!46 = !DISubprogram(name: "strcmp", scope: !6, file: !6, line: 156, type: !47, flags: DIFlagPrototyped, spFlags: 0)
!47 = !DISubroutineType(types: !48)
!48 = !{!11, !43, !43}
!49 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !50, file: !15, line: 84)
!50 = !DISubprogram(name: "strcoll", scope: !6, file: !6, line: 163, type: !47, flags: DIFlagPrototyped, spFlags: 0)
!51 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !52, file: !15, line: 85)
!52 = !DISubprogram(name: "strcpy", scope: !6, file: !6, line: 141, type: !37, flags: DIFlagPrototyped, spFlags: 0)
!53 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !54, file: !15, line: 86)
!54 = !DISubprogram(name: "strcspn", scope: !6, file: !6, line: 293, type: !55, flags: DIFlagPrototyped, spFlags: 0)
!55 = !DISubroutineType(types: !56)
!56 = !{!12, !43, !43}
!57 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !58, file: !15, line: 87)
!58 = !DISubprogram(name: "strerror", scope: !6, file: !6, line: 419, type: !59, flags: DIFlagPrototyped, spFlags: 0)
!59 = !DISubroutineType(types: !60)
!60 = !{!39, !11}
!61 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !62, file: !15, line: 88)
!62 = !DISubprogram(name: "strlen", scope: !6, file: !6, line: 407, type: !63, flags: DIFlagPrototyped, spFlags: 0)
!63 = !DISubroutineType(types: !64)
!64 = !{!12, !43}
!65 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !66, file: !15, line: 89)
!66 = !DISubprogram(name: "strncat", scope: !6, file: !6, line: 152, type: !67, flags: DIFlagPrototyped, spFlags: 0)
!67 = !DISubroutineType(types: !68)
!68 = !{!39, !41, !42, !12}
!69 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !70, file: !15, line: 90)
!70 = !DISubprogram(name: "strncmp", scope: !6, file: !6, line: 159, type: !71, flags: DIFlagPrototyped, spFlags: 0)
!71 = !DISubroutineType(types: !72)
!72 = !{!11, !43, !43, !12}
!73 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !74, file: !15, line: 91)
!74 = !DISubprogram(name: "strncpy", scope: !6, file: !6, line: 144, type: !67, flags: DIFlagPrototyped, spFlags: 0)
!75 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !76, file: !15, line: 92)
!76 = !DISubprogram(name: "strspn", scope: !6, file: !6, line: 297, type: !55, flags: DIFlagPrototyped, spFlags: 0)
!77 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !78, file: !15, line: 93)
!78 = !DISubprogram(name: "strtok", scope: !6, file: !6, line: 356, type: !37, flags: DIFlagPrototyped, spFlags: 0)
!79 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !80, file: !15, line: 94)
!80 = !DISubprogram(name: "strxfrm", scope: !6, file: !6, line: 166, type: !81, flags: DIFlagPrototyped, spFlags: 0)
!81 = !DISubroutineType(types: !82)
!82 = !{!12, !41, !42, !12}
!83 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !84, file: !15, line: 95)
!84 = !DISubprogram(name: "strchr", scope: !6, file: !6, line: 228, type: !85, flags: DIFlagPrototyped, spFlags: 0)
!85 = !DISubroutineType(types: !86)
!86 = !{!43, !43, !11}
!87 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !88, file: !15, line: 96)
!88 = !DISubprogram(name: "strpbrk", scope: !6, file: !6, line: 305, type: !89, flags: DIFlagPrototyped, spFlags: 0)
!89 = !DISubroutineType(types: !90)
!90 = !{!43, !43, !43}
!91 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !92, file: !15, line: 97)
!92 = !DISubprogram(name: "strrchr", scope: !6, file: !6, line: 255, type: !85, flags: DIFlagPrototyped, spFlags: 0)
!93 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !94, file: !15, line: 98)
!94 = !DISubprogram(name: "strstr", scope: !6, file: !6, line: 332, type: !89, flags: DIFlagPrototyped, spFlags: 0)
!95 = !{i32 7, !"Dwarf Version", i32 5}
!96 = !{i32 2, !"Debug Info Version", i32 3}
!97 = !{i32 1, !"wchar_size", i32 4}
!98 = !{i32 7, !"PIC Level", i32 2}
!99 = !{i32 7, !"PIE Level", i32 2}
!100 = !{i32 7, !"uwtable", i32 2}
!101 = !{i32 7, !"frame-pointer", i32 2}
!102 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!103 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 3, type: !104, scopeLine: 3, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !106)
!104 = !DISubroutineType(types: !105)
!105 = !{!11}
!106 = !{}
!107 = !DILocalVariable(name: "buffer", scope: !103, file: !1, line: 4, type: !108)
!108 = !DICompositeType(tag: DW_TAG_array_type, baseType: !40, size: 80, elements: !109)
!109 = !{!110}
!110 = !DISubrange(count: 10)
!111 = !DILocation(line: 4, column: 8, scope: !103)
!112 = !DILocation(line: 5, column: 3, scope: !103)
!113 = !DILocalVariable(name: "i", scope: !103, file: !1, line: 6, type: !11)
!114 = !DILocation(line: 6, column: 7, scope: !103)
!115 = !DILocation(line: 6, column: 11, scope: !103)
!116 = !DILocation(line: 6, column: 21, scope: !103)
!117 = !DILocation(line: 7, column: 3, scope: !103)

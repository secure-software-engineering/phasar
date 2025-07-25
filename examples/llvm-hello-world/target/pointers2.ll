; ModuleID = 'pointers2.cpp'
source_filename = "pointers2.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress noinline norecurse nounwind optnone uwtable
define dso_local noundef i32 @main(i32 noundef %argc, ptr noundef %argv) #0 !dbg !221 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca ptr, align 8
  %N = alloca i32, align 4
  %mem = alloca ptr, align 8
  %i = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  store i32 %argc, ptr %argc.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %argc.addr, metadata !225, metadata !DIExpression()), !dbg !226
  store ptr %argv, ptr %argv.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %argv.addr, metadata !227, metadata !DIExpression()), !dbg !228
  call void @llvm.dbg.declare(metadata ptr %N, metadata !229, metadata !DIExpression()), !dbg !231
  store i32 10, ptr %N, align 4, !dbg !231
  call void @llvm.dbg.declare(metadata ptr %mem, metadata !232, metadata !DIExpression()), !dbg !233
  %call = call noalias ptr @malloc(i64 noundef 40) #4, !dbg !234
  store ptr %call, ptr %mem, align 8, !dbg !233
  call void @llvm.dbg.declare(metadata ptr %i, metadata !235, metadata !DIExpression()), !dbg !237
  store i32 0, ptr %i, align 4, !dbg !237
  br label %for.cond, !dbg !238

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, ptr %i, align 4, !dbg !239
  %cmp = icmp slt i32 %0, 10, !dbg !241
  br i1 %cmp, label %for.body, label %for.end, !dbg !242

for.body:                                         ; preds = %for.cond
  %1 = load ptr, ptr %mem, align 8, !dbg !243
  %2 = load i32, ptr %i, align 4, !dbg !245
  %idxprom = sext i32 %2 to i64, !dbg !243
  %arrayidx = getelementptr inbounds i32, ptr %1, i64 %idxprom, !dbg !243
  store i32 42, ptr %arrayidx, align 4, !dbg !246
  br label %for.inc, !dbg !247

for.inc:                                          ; preds = %for.body
  %3 = load i32, ptr %i, align 4, !dbg !248
  %inc = add nsw i32 %3, 1, !dbg !248
  store i32 %inc, ptr %i, align 4, !dbg !248
  br label %for.cond, !dbg !249, !llvm.loop !250

for.end:                                          ; preds = %for.cond
  %4 = load ptr, ptr %mem, align 8, !dbg !253
  call void @free(ptr noundef %4) #5, !dbg !254
  ret i32 0, !dbg !255
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind allocsize(0)
declare noalias ptr @malloc(i64 noundef) #2

; Function Attrs: nounwind
declare void @free(ptr noundef) #3

attributes #0 = { mustprogress noinline norecurse nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { nounwind allocsize(0) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nounwind allocsize(0) }
attributes #5 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!213, !214, !215, !216, !217, !218, !219}
!llvm.ident = !{!220}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !2, imports: !5, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "pointers2.cpp", directory: "phasar/examples/llvm-hello-world/target", checksumkind: CSK_MD5, checksum: "3ee743f33646636730f5f8306d67e3e6")
!2 = !{!3}
!3 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 64)
!4 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!5 = !{!6, !13, !17, !24, !28, !33, !35, !43, !47, !51, !65, !69, !73, !77, !81, !86, !90, !94, !98, !102, !110, !114, !118, !120, !124, !128, !133, !139, !143, !147, !149, !157, !161, !169, !171, !175, !179, !183, !187, !192, !197, !202, !203, !204, !205, !207, !208, !209, !210, !211, !212}
!6 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !8, file: !12, line: 52)
!7 = !DINamespace(name: "std", scope: null)
!8 = !DISubprogram(name: "abs", scope: !9, file: !9, line: 980, type: !10, flags: DIFlagPrototyped, spFlags: 0)
!9 = !DIFile(filename: "/usr/include/stdlib.h", directory: "", checksumkind: CSK_MD5, checksum: "7fa2ecb2348a66f8b44ab9a15abd0b72")
!10 = !DISubroutineType(types: !11)
!11 = !{!4, !4}
!12 = !DIFile(filename: "/usr/lib/gcc/x86_64-linux-gnu/13/../../../../include/c++/13/bits/std_abs.h", directory: "")
!13 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !14, file: !16, line: 131)
!14 = !DIDerivedType(tag: DW_TAG_typedef, name: "div_t", file: !9, line: 63, baseType: !15)
!15 = !DICompositeType(tag: DW_TAG_structure_type, file: !9, line: 59, size: 64, flags: DIFlagFwdDecl, identifier: "_ZTS5div_t")
!16 = !DIFile(filename: "/usr/lib/gcc/x86_64-linux-gnu/13/../../../../include/c++/13/cstdlib", directory: "")
!17 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !18, file: !16, line: 132)
!18 = !DIDerivedType(tag: DW_TAG_typedef, name: "ldiv_t", file: !9, line: 71, baseType: !19)
!19 = distinct !DICompositeType(tag: DW_TAG_structure_type, file: !9, line: 67, size: 128, flags: DIFlagTypePassByValue, elements: !20, identifier: "_ZTS6ldiv_t")
!20 = !{!21, !23}
!21 = !DIDerivedType(tag: DW_TAG_member, name: "quot", scope: !19, file: !9, line: 69, baseType: !22, size: 64)
!22 = !DIBasicType(name: "long", size: 64, encoding: DW_ATE_signed)
!23 = !DIDerivedType(tag: DW_TAG_member, name: "rem", scope: !19, file: !9, line: 70, baseType: !22, size: 64, offset: 64)
!24 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !25, file: !16, line: 134)
!25 = !DISubprogram(name: "abort", scope: !9, file: !9, line: 730, type: !26, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: 0)
!26 = !DISubroutineType(types: !27)
!27 = !{null}
!28 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !29, file: !16, line: 138)
!29 = !DISubprogram(name: "atexit", scope: !9, file: !9, line: 734, type: !30, flags: DIFlagPrototyped, spFlags: 0)
!30 = !DISubroutineType(types: !31)
!31 = !{!4, !32}
!32 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !26, size: 64)
!33 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !34, file: !16, line: 141)
!34 = !DISubprogram(name: "at_quick_exit", scope: !9, file: !9, line: 739, type: !30, flags: DIFlagPrototyped, spFlags: 0)
!35 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !36, file: !16, line: 144)
!36 = !DISubprogram(name: "atof", scope: !9, file: !9, line: 102, type: !37, flags: DIFlagPrototyped, spFlags: 0)
!37 = !DISubroutineType(types: !38)
!38 = !{!39, !40}
!39 = !DIBasicType(name: "double", size: 64, encoding: DW_ATE_float)
!40 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !41, size: 64)
!41 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !42)
!42 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!43 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !44, file: !16, line: 145)
!44 = !DISubprogram(name: "atoi", scope: !9, file: !9, line: 105, type: !45, flags: DIFlagPrototyped, spFlags: 0)
!45 = !DISubroutineType(types: !46)
!46 = !{!4, !40}
!47 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !48, file: !16, line: 146)
!48 = !DISubprogram(name: "atol", scope: !9, file: !9, line: 108, type: !49, flags: DIFlagPrototyped, spFlags: 0)
!49 = !DISubroutineType(types: !50)
!50 = !{!22, !40}
!51 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !52, file: !16, line: 147)
!52 = !DISubprogram(name: "bsearch", scope: !9, file: !9, line: 960, type: !53, flags: DIFlagPrototyped, spFlags: 0)
!53 = !DISubroutineType(types: !54)
!54 = !{!55, !56, !56, !58, !58, !61}
!55 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!56 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !57, size: 64)
!57 = !DIDerivedType(tag: DW_TAG_const_type, baseType: null)
!58 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !59, line: 46, baseType: !60)
!59 = !DIFile(filename: "/usr/local/llvm-15/lib/clang/15.0.7/include/stddef.h", directory: "", checksumkind: CSK_MD5, checksum: "b76978376d35d5cd171876ac58ac1256")
!60 = !DIBasicType(name: "unsigned long", size: 64, encoding: DW_ATE_unsigned)
!61 = !DIDerivedType(tag: DW_TAG_typedef, name: "__compar_fn_t", file: !9, line: 948, baseType: !62)
!62 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !63, size: 64)
!63 = !DISubroutineType(types: !64)
!64 = !{!4, !56, !56}
!65 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !66, file: !16, line: 148)
!66 = !DISubprogram(name: "calloc", scope: !9, file: !9, line: 675, type: !67, flags: DIFlagPrototyped, spFlags: 0)
!67 = !DISubroutineType(types: !68)
!68 = !{!55, !58, !58}
!69 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !70, file: !16, line: 149)
!70 = !DISubprogram(name: "div", scope: !9, file: !9, line: 992, type: !71, flags: DIFlagPrototyped, spFlags: 0)
!71 = !DISubroutineType(types: !72)
!72 = !{!14, !4, !4}
!73 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !74, file: !16, line: 150)
!74 = !DISubprogram(name: "exit", scope: !9, file: !9, line: 756, type: !75, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: 0)
!75 = !DISubroutineType(types: !76)
!76 = !{null, !4}
!77 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !78, file: !16, line: 151)
!78 = !DISubprogram(name: "free", scope: !9, file: !9, line: 687, type: !79, flags: DIFlagPrototyped, spFlags: 0)
!79 = !DISubroutineType(types: !80)
!80 = !{null, !55}
!81 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !82, file: !16, line: 152)
!82 = !DISubprogram(name: "getenv", scope: !9, file: !9, line: 773, type: !83, flags: DIFlagPrototyped, spFlags: 0)
!83 = !DISubroutineType(types: !84)
!84 = !{!85, !40}
!85 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !42, size: 64)
!86 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !87, file: !16, line: 153)
!87 = !DISubprogram(name: "labs", scope: !9, file: !9, line: 981, type: !88, flags: DIFlagPrototyped, spFlags: 0)
!88 = !DISubroutineType(types: !89)
!89 = !{!22, !22}
!90 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !91, file: !16, line: 154)
!91 = !DISubprogram(name: "ldiv", scope: !9, file: !9, line: 994, type: !92, flags: DIFlagPrototyped, spFlags: 0)
!92 = !DISubroutineType(types: !93)
!93 = !{!18, !22, !22}
!94 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !95, file: !16, line: 155)
!95 = !DISubprogram(name: "malloc", scope: !9, file: !9, line: 672, type: !96, flags: DIFlagPrototyped, spFlags: 0)
!96 = !DISubroutineType(types: !97)
!97 = !{!55, !58}
!98 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !99, file: !16, line: 157)
!99 = !DISubprogram(name: "mblen", scope: !9, file: !9, line: 1062, type: !100, flags: DIFlagPrototyped, spFlags: 0)
!100 = !DISubroutineType(types: !101)
!101 = !{!4, !40, !58}
!102 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !103, file: !16, line: 158)
!103 = !DISubprogram(name: "mbstowcs", scope: !9, file: !9, line: 1073, type: !104, flags: DIFlagPrototyped, spFlags: 0)
!104 = !DISubroutineType(types: !105)
!105 = !{!58, !106, !109, !58}
!106 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !107)
!107 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !108, size: 64)
!108 = !DIBasicType(name: "wchar_t", size: 32, encoding: DW_ATE_signed)
!109 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !40)
!110 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !111, file: !16, line: 159)
!111 = !DISubprogram(name: "mbtowc", scope: !9, file: !9, line: 1065, type: !112, flags: DIFlagPrototyped, spFlags: 0)
!112 = !DISubroutineType(types: !113)
!113 = !{!4, !106, !109, !58}
!114 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !115, file: !16, line: 161)
!115 = !DISubprogram(name: "qsort", scope: !9, file: !9, line: 970, type: !116, flags: DIFlagPrototyped, spFlags: 0)
!116 = !DISubroutineType(types: !117)
!117 = !{null, !55, !58, !58, !61}
!118 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !119, file: !16, line: 164)
!119 = !DISubprogram(name: "quick_exit", scope: !9, file: !9, line: 762, type: !75, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: 0)
!120 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !121, file: !16, line: 167)
!121 = !DISubprogram(name: "rand", scope: !9, file: !9, line: 573, type: !122, flags: DIFlagPrototyped, spFlags: 0)
!122 = !DISubroutineType(types: !123)
!123 = !{!4}
!124 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !125, file: !16, line: 168)
!125 = !DISubprogram(name: "realloc", scope: !9, file: !9, line: 683, type: !126, flags: DIFlagPrototyped, spFlags: 0)
!126 = !DISubroutineType(types: !127)
!127 = !{!55, !55, !58}
!128 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !129, file: !16, line: 169)
!129 = !DISubprogram(name: "srand", scope: !9, file: !9, line: 575, type: !130, flags: DIFlagPrototyped, spFlags: 0)
!130 = !DISubroutineType(types: !131)
!131 = !{null, !132}
!132 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!133 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !134, file: !16, line: 170)
!134 = !DISubprogram(name: "strtod", scope: !9, file: !9, line: 118, type: !135, flags: DIFlagPrototyped, spFlags: 0)
!135 = !DISubroutineType(types: !136)
!136 = !{!39, !109, !137}
!137 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !138)
!138 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !85, size: 64)
!139 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !140, file: !16, line: 171)
!140 = !DISubprogram(name: "strtol", linkageName: "__isoc23_strtol", scope: !9, file: !9, line: 215, type: !141, flags: DIFlagPrototyped, spFlags: 0)
!141 = !DISubroutineType(types: !142)
!142 = !{!22, !109, !137, !4}
!143 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !144, file: !16, line: 172)
!144 = !DISubprogram(name: "strtoul", linkageName: "__isoc23_strtoul", scope: !9, file: !9, line: 219, type: !145, flags: DIFlagPrototyped, spFlags: 0)
!145 = !DISubroutineType(types: !146)
!146 = !{!60, !109, !137, !4}
!147 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !148, file: !16, line: 173)
!148 = !DISubprogram(name: "system", scope: !9, file: !9, line: 923, type: !45, flags: DIFlagPrototyped, spFlags: 0)
!149 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !150, file: !16, line: 175)
!150 = !DISubprogram(name: "wcstombs", scope: !9, file: !9, line: 1077, type: !151, flags: DIFlagPrototyped, spFlags: 0)
!151 = !DISubroutineType(types: !152)
!152 = !{!58, !153, !154, !58}
!153 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !85)
!154 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !155)
!155 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !156, size: 64)
!156 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !108)
!157 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !158, file: !16, line: 176)
!158 = !DISubprogram(name: "wctomb", scope: !9, file: !9, line: 1069, type: !159, flags: DIFlagPrototyped, spFlags: 0)
!159 = !DISubroutineType(types: !160)
!160 = !{!4, !85, !108}
!161 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !162, entity: !163, file: !16, line: 204)
!162 = !DINamespace(name: "__gnu_cxx", scope: null)
!163 = !DIDerivedType(tag: DW_TAG_typedef, name: "lldiv_t", file: !9, line: 81, baseType: !164)
!164 = distinct !DICompositeType(tag: DW_TAG_structure_type, file: !9, line: 77, size: 128, flags: DIFlagTypePassByValue, elements: !165, identifier: "_ZTS7lldiv_t")
!165 = !{!166, !168}
!166 = !DIDerivedType(tag: DW_TAG_member, name: "quot", scope: !164, file: !9, line: 79, baseType: !167, size: 64)
!167 = !DIBasicType(name: "long long", size: 64, encoding: DW_ATE_signed)
!168 = !DIDerivedType(tag: DW_TAG_member, name: "rem", scope: !164, file: !9, line: 80, baseType: !167, size: 64, offset: 64)
!169 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !162, entity: !170, file: !16, line: 210)
!170 = !DISubprogram(name: "_Exit", scope: !9, file: !9, line: 768, type: !75, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: 0)
!171 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !162, entity: !172, file: !16, line: 214)
!172 = !DISubprogram(name: "llabs", scope: !9, file: !9, line: 984, type: !173, flags: DIFlagPrototyped, spFlags: 0)
!173 = !DISubroutineType(types: !174)
!174 = !{!167, !167}
!175 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !162, entity: !176, file: !16, line: 220)
!176 = !DISubprogram(name: "lldiv", scope: !9, file: !9, line: 998, type: !177, flags: DIFlagPrototyped, spFlags: 0)
!177 = !DISubroutineType(types: !178)
!178 = !{!163, !167, !167}
!179 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !162, entity: !180, file: !16, line: 231)
!180 = !DISubprogram(name: "atoll", scope: !9, file: !9, line: 113, type: !181, flags: DIFlagPrototyped, spFlags: 0)
!181 = !DISubroutineType(types: !182)
!182 = !{!167, !40}
!183 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !162, entity: !184, file: !16, line: 232)
!184 = !DISubprogram(name: "strtoll", linkageName: "__isoc23_strtoll", scope: !9, file: !9, line: 238, type: !185, flags: DIFlagPrototyped, spFlags: 0)
!185 = !DISubroutineType(types: !186)
!186 = !{!167, !109, !137, !4}
!187 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !162, entity: !188, file: !16, line: 233)
!188 = !DISubprogram(name: "strtoull", linkageName: "__isoc23_strtoull", scope: !9, file: !9, line: 243, type: !189, flags: DIFlagPrototyped, spFlags: 0)
!189 = !DISubroutineType(types: !190)
!190 = !{!191, !109, !137, !4}
!191 = !DIBasicType(name: "unsigned long long", size: 64, encoding: DW_ATE_unsigned)
!192 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !162, entity: !193, file: !16, line: 235)
!193 = !DISubprogram(name: "strtof", scope: !9, file: !9, line: 124, type: !194, flags: DIFlagPrototyped, spFlags: 0)
!194 = !DISubroutineType(types: !195)
!195 = !{!196, !109, !137}
!196 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!197 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !162, entity: !198, file: !16, line: 236)
!198 = !DISubprogram(name: "strtold", scope: !9, file: !9, line: 127, type: !199, flags: DIFlagPrototyped, spFlags: 0)
!199 = !DISubroutineType(types: !200)
!200 = !{!201, !109, !137}
!201 = !DIBasicType(name: "long double", size: 128, encoding: DW_ATE_float)
!202 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !163, file: !16, line: 244)
!203 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !170, file: !16, line: 246)
!204 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !172, file: !16, line: 248)
!205 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !206, file: !16, line: 249)
!206 = !DISubprogram(name: "div", linkageName: "_ZN9__gnu_cxx3divExx", scope: !162, file: !16, line: 217, type: !177, flags: DIFlagPrototyped, spFlags: 0)
!207 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !176, file: !16, line: 250)
!208 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !180, file: !16, line: 252)
!209 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !193, file: !16, line: 253)
!210 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !184, file: !16, line: 254)
!211 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !188, file: !16, line: 255)
!212 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !198, file: !16, line: 256)
!213 = !{i32 7, !"Dwarf Version", i32 5}
!214 = !{i32 2, !"Debug Info Version", i32 3}
!215 = !{i32 1, !"wchar_size", i32 4}
!216 = !{i32 7, !"PIC Level", i32 2}
!217 = !{i32 7, !"PIE Level", i32 2}
!218 = !{i32 7, !"uwtable", i32 2}
!219 = !{i32 7, !"frame-pointer", i32 2}
!220 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!221 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 3, type: !222, scopeLine: 3, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !224)
!222 = !DISubroutineType(types: !223)
!223 = !{!4, !4, !138}
!224 = !{}
!225 = !DILocalVariable(name: "argc", arg: 1, scope: !221, file: !1, line: 3, type: !4)
!226 = !DILocation(line: 3, column: 14, scope: !221)
!227 = !DILocalVariable(name: "argv", arg: 2, scope: !221, file: !1, line: 3, type: !138)
!228 = !DILocation(line: 3, column: 27, scope: !221)
!229 = !DILocalVariable(name: "N", scope: !221, file: !1, line: 4, type: !230)
!230 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !4)
!231 = !DILocation(line: 4, column: 17, scope: !221)
!232 = !DILocalVariable(name: "mem", scope: !221, file: !1, line: 5, type: !3)
!233 = !DILocation(line: 5, column: 8, scope: !221)
!234 = !DILocation(line: 5, column: 33, scope: !221)
!235 = !DILocalVariable(name: "i", scope: !236, file: !1, line: 6, type: !4)
!236 = distinct !DILexicalBlock(scope: !221, file: !1, line: 6, column: 3)
!237 = !DILocation(line: 6, column: 12, scope: !236)
!238 = !DILocation(line: 6, column: 8, scope: !236)
!239 = !DILocation(line: 6, column: 19, scope: !240)
!240 = distinct !DILexicalBlock(scope: !236, file: !1, line: 6, column: 3)
!241 = !DILocation(line: 6, column: 21, scope: !240)
!242 = !DILocation(line: 6, column: 3, scope: !236)
!243 = !DILocation(line: 7, column: 5, scope: !244)
!244 = distinct !DILexicalBlock(scope: !240, file: !1, line: 6, column: 31)
!245 = !DILocation(line: 7, column: 9, scope: !244)
!246 = !DILocation(line: 7, column: 12, scope: !244)
!247 = !DILocation(line: 8, column: 3, scope: !244)
!248 = !DILocation(line: 6, column: 26, scope: !240)
!249 = !DILocation(line: 6, column: 3, scope: !240)
!250 = distinct !{!250, !242, !251, !252}
!251 = !DILocation(line: 8, column: 3, scope: !236)
!252 = !{!"llvm.loop.mustprogress"}
!253 = !DILocation(line: 9, column: 8, scope: !221)
!254 = !DILocation(line: 9, column: 3, scope: !221)
!255 = !DILocation(line: 10, column: 3, scope: !221)

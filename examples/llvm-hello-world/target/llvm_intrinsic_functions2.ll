; ModuleID = 'llvm_intrinsic_functions2.cpp'
source_filename = "llvm_intrinsic_functions2.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define dso_local noundef i32 @_Z3foov() #0 !dbg !299 {
entry:
  ret i32 42, !dbg !301
}

; Function Attrs: mustprogress noinline norecurse optnone uwtable
define dso_local noundef i32 @main() #1 !dbg !302 {
entry:
  %retval = alloca i32, align 4
  %i = alloca ptr, align 8
  %j = alloca ptr, align 8
  store i32 0, ptr %retval, align 4
  call void @llvm.dbg.declare(metadata ptr %i, metadata !303, metadata !DIExpression()), !dbg !304
  %call = call noalias ptr @malloc(i64 noundef 40) #8, !dbg !305
  store ptr %call, ptr %i, align 8, !dbg !304
  %0 = load ptr, ptr %i, align 8, !dbg !306
  call void @llvm.memset.p0.i64(ptr align 4 %0, i8 0, i64 40, i1 false), !dbg !307
  %1 = load ptr, ptr %i, align 8, !dbg !308
  call void @free(ptr noundef %1) #9, !dbg !309
  call void @llvm.dbg.declare(metadata ptr %j, metadata !310, metadata !DIExpression()), !dbg !311
  %call1 = call noalias noundef nonnull ptr @_Znwm(i64 noundef 4) #10, !dbg !312, !heapallocsite !4
  store i32 13, ptr %call1, align 4, !dbg !312
  store ptr %call1, ptr %j, align 8, !dbg !311
  %call2 = call noundef i32 @_Z3foov(), !dbg !313
  %2 = load ptr, ptr %j, align 8, !dbg !314
  store i32 %call2, ptr %2, align 4, !dbg !315
  %3 = load ptr, ptr %j, align 8, !dbg !316
  %isnull = icmp eq ptr %3, null, !dbg !317
  br i1 %isnull, label %delete.end, label %delete.notnull, !dbg !317

delete.notnull:                                   ; preds = %entry
  call void @_ZdlPv(ptr noundef %3) #11, !dbg !317
  br label %delete.end, !dbg !317

delete.end:                                       ; preds = %delete.notnull, %entry
  ret i32 0, !dbg !318
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #2

; Function Attrs: nounwind allocsize(0)
declare noalias ptr @malloc(i64 noundef) #3

; Function Attrs: argmemonly nocallback nofree nounwind willreturn writeonly
declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg) #4

; Function Attrs: nounwind
declare void @free(ptr noundef) #5

; Function Attrs: nobuiltin allocsize(0)
declare noundef nonnull ptr @_Znwm(i64 noundef) #6

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(ptr noundef) #7

attributes #0 = { mustprogress noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { mustprogress noinline norecurse optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #3 = { nounwind allocsize(0) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { argmemonly nocallback nofree nounwind willreturn writeonly }
attributes #5 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #6 = { nobuiltin allocsize(0) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #7 = { nobuiltin nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #8 = { nounwind allocsize(0) }
attributes #9 = { nounwind }
attributes #10 = { builtin allocsize(0) }
attributes #11 = { builtin nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!291, !292, !293, !294, !295, !296, !297}
!llvm.ident = !{!298}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !2, imports: !5, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "llvm_intrinsic_functions2.cpp", directory: "phasar/examples/llvm-hello-world/target", checksumkind: CSK_MD5, checksum: "1e77ed792913c2be191abc6888725e26")
!2 = !{!3}
!3 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 64)
!4 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!5 = !{!6, !13, !17, !24, !28, !33, !35, !43, !47, !51, !65, !69, !73, !77, !81, !86, !90, !94, !98, !102, !110, !114, !118, !120, !124, !128, !133, !139, !143, !147, !149, !157, !161, !169, !171, !175, !179, !183, !187, !192, !197, !202, !203, !204, !205, !207, !208, !209, !210, !211, !212, !213, !219, !223, !229, !233, !237, !241, !245, !247, !249, !253, !257, !261, !265, !269, !271, !273, !275, !279, !283, !287, !289}
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
!213 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !214, file: !218, line: 77)
!214 = !DISubprogram(name: "memchr", scope: !215, file: !215, line: 89, type: !216, flags: DIFlagPrototyped, spFlags: 0)
!215 = !DIFile(filename: "/usr/include/string.h", directory: "", checksumkind: CSK_MD5, checksum: "165db1185644f68894fa9e0d17055d70")
!216 = !DISubroutineType(types: !217)
!217 = !{!56, !56, !4, !58}
!218 = !DIFile(filename: "/usr/lib/gcc/x86_64-linux-gnu/13/../../../../include/c++/13/cstring", directory: "")
!219 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !220, file: !218, line: 78)
!220 = !DISubprogram(name: "memcmp", scope: !215, file: !215, line: 64, type: !221, flags: DIFlagPrototyped, spFlags: 0)
!221 = !DISubroutineType(types: !222)
!222 = !{!4, !56, !56, !58}
!223 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !224, file: !218, line: 79)
!224 = !DISubprogram(name: "memcpy", scope: !215, file: !215, line: 43, type: !225, flags: DIFlagPrototyped, spFlags: 0)
!225 = !DISubroutineType(types: !226)
!226 = !{!55, !227, !228, !58}
!227 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !55)
!228 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !56)
!229 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !230, file: !218, line: 80)
!230 = !DISubprogram(name: "memmove", scope: !215, file: !215, line: 47, type: !231, flags: DIFlagPrototyped, spFlags: 0)
!231 = !DISubroutineType(types: !232)
!232 = !{!55, !55, !56, !58}
!233 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !234, file: !218, line: 81)
!234 = !DISubprogram(name: "memset", scope: !215, file: !215, line: 61, type: !235, flags: DIFlagPrototyped, spFlags: 0)
!235 = !DISubroutineType(types: !236)
!236 = !{!55, !55, !4, !58}
!237 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !238, file: !218, line: 82)
!238 = !DISubprogram(name: "strcat", scope: !215, file: !215, line: 149, type: !239, flags: DIFlagPrototyped, spFlags: 0)
!239 = !DISubroutineType(types: !240)
!240 = !{!85, !153, !109}
!241 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !242, file: !218, line: 83)
!242 = !DISubprogram(name: "strcmp", scope: !215, file: !215, line: 156, type: !243, flags: DIFlagPrototyped, spFlags: 0)
!243 = !DISubroutineType(types: !244)
!244 = !{!4, !40, !40}
!245 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !246, file: !218, line: 84)
!246 = !DISubprogram(name: "strcoll", scope: !215, file: !215, line: 163, type: !243, flags: DIFlagPrototyped, spFlags: 0)
!247 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !248, file: !218, line: 85)
!248 = !DISubprogram(name: "strcpy", scope: !215, file: !215, line: 141, type: !239, flags: DIFlagPrototyped, spFlags: 0)
!249 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !250, file: !218, line: 86)
!250 = !DISubprogram(name: "strcspn", scope: !215, file: !215, line: 293, type: !251, flags: DIFlagPrototyped, spFlags: 0)
!251 = !DISubroutineType(types: !252)
!252 = !{!58, !40, !40}
!253 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !254, file: !218, line: 87)
!254 = !DISubprogram(name: "strerror", scope: !215, file: !215, line: 419, type: !255, flags: DIFlagPrototyped, spFlags: 0)
!255 = !DISubroutineType(types: !256)
!256 = !{!85, !4}
!257 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !258, file: !218, line: 88)
!258 = !DISubprogram(name: "strlen", scope: !215, file: !215, line: 407, type: !259, flags: DIFlagPrototyped, spFlags: 0)
!259 = !DISubroutineType(types: !260)
!260 = !{!58, !40}
!261 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !262, file: !218, line: 89)
!262 = !DISubprogram(name: "strncat", scope: !215, file: !215, line: 152, type: !263, flags: DIFlagPrototyped, spFlags: 0)
!263 = !DISubroutineType(types: !264)
!264 = !{!85, !153, !109, !58}
!265 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !266, file: !218, line: 90)
!266 = !DISubprogram(name: "strncmp", scope: !215, file: !215, line: 159, type: !267, flags: DIFlagPrototyped, spFlags: 0)
!267 = !DISubroutineType(types: !268)
!268 = !{!4, !40, !40, !58}
!269 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !270, file: !218, line: 91)
!270 = !DISubprogram(name: "strncpy", scope: !215, file: !215, line: 144, type: !263, flags: DIFlagPrototyped, spFlags: 0)
!271 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !272, file: !218, line: 92)
!272 = !DISubprogram(name: "strspn", scope: !215, file: !215, line: 297, type: !251, flags: DIFlagPrototyped, spFlags: 0)
!273 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !274, file: !218, line: 93)
!274 = !DISubprogram(name: "strtok", scope: !215, file: !215, line: 356, type: !239, flags: DIFlagPrototyped, spFlags: 0)
!275 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !276, file: !218, line: 94)
!276 = !DISubprogram(name: "strxfrm", scope: !215, file: !215, line: 166, type: !277, flags: DIFlagPrototyped, spFlags: 0)
!277 = !DISubroutineType(types: !278)
!278 = !{!58, !153, !109, !58}
!279 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !280, file: !218, line: 95)
!280 = !DISubprogram(name: "strchr", scope: !215, file: !215, line: 228, type: !281, flags: DIFlagPrototyped, spFlags: 0)
!281 = !DISubroutineType(types: !282)
!282 = !{!40, !40, !4}
!283 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !284, file: !218, line: 96)
!284 = !DISubprogram(name: "strpbrk", scope: !215, file: !215, line: 305, type: !285, flags: DIFlagPrototyped, spFlags: 0)
!285 = !DISubroutineType(types: !286)
!286 = !{!40, !40, !40}
!287 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !288, file: !218, line: 97)
!288 = !DISubprogram(name: "strrchr", scope: !215, file: !215, line: 255, type: !281, flags: DIFlagPrototyped, spFlags: 0)
!289 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !7, entity: !290, file: !218, line: 98)
!290 = !DISubprogram(name: "strstr", scope: !215, file: !215, line: 332, type: !285, flags: DIFlagPrototyped, spFlags: 0)
!291 = !{i32 7, !"Dwarf Version", i32 5}
!292 = !{i32 2, !"Debug Info Version", i32 3}
!293 = !{i32 1, !"wchar_size", i32 4}
!294 = !{i32 7, !"PIC Level", i32 2}
!295 = !{i32 7, !"PIE Level", i32 2}
!296 = !{i32 7, !"uwtable", i32 2}
!297 = !{i32 7, !"frame-pointer", i32 2}
!298 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!299 = distinct !DISubprogram(name: "foo", linkageName: "_Z3foov", scope: !1, file: !1, line: 4, type: !122, scopeLine: 4, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !300)
!300 = !{}
!301 = !DILocation(line: 4, column: 13, scope: !299)
!302 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 6, type: !122, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !300)
!303 = !DILocalVariable(name: "i", scope: !302, file: !1, line: 7, type: !3)
!304 = !DILocation(line: 7, column: 8, scope: !302)
!305 = !DILocation(line: 7, column: 31, scope: !302)
!306 = !DILocation(line: 8, column: 10, scope: !302)
!307 = !DILocation(line: 8, column: 3, scope: !302)
!308 = !DILocation(line: 9, column: 8, scope: !302)
!309 = !DILocation(line: 9, column: 3, scope: !302)
!310 = !DILocalVariable(name: "j", scope: !302, file: !1, line: 10, type: !3)
!311 = !DILocation(line: 10, column: 8, scope: !302)
!312 = !DILocation(line: 10, column: 12, scope: !302)
!313 = !DILocation(line: 11, column: 8, scope: !302)
!314 = !DILocation(line: 11, column: 4, scope: !302)
!315 = !DILocation(line: 11, column: 6, scope: !302)
!316 = !DILocation(line: 12, column: 10, scope: !302)
!317 = !DILocation(line: 12, column: 3, scope: !302)
!318 = !DILocation(line: 13, column: 3, scope: !302)

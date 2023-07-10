; ModuleID = 'test/arithmetic-operators/Addition/Swift/SimpleAdd.swift.noOpt.ll'
source_filename = "test/arithmetic-operators/Addition/Swift/SimpleAdd.swift.noOpt.ll"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx13.0.0"

%swift.type_metadata_record = type { i32 }
%swift.type = type { i64 }
%TSi = type <{ i64 }>
%T8myModule6MyMainV = type opaque
%swift.metadata_response = type { %swift.type*, i64 }

@"\01l_entry_point" = private constant { i32 } { i32 trunc (i64 sub (i64 ptrtoint (i32 (i32, i8**)* @main to i64), i64 ptrtoint ({ i32 }* @"\01l_entry_point" to i64)) to i32) }, section "__TEXT, __swift5_entry, regular, no_dead_strip", align 4
@"$sytWV" = external global i8*, align 8
@.str = private constant [9 x i8] c"myModule\00"
@"$s8myModuleMXM" = linkonce_odr hidden constant <{ i32, i32, i32 }> <{ i32 0, i32 0, i32 trunc (i64 sub (i64 ptrtoint ([9 x i8]* @.str to i64), i64 ptrtoint (i32* getelementptr inbounds (<{ i32, i32, i32 }>, <{ i32, i32, i32 }>* @"$s8myModuleMXM", i32 0, i32 2) to i64)) to i32) }>, section "__TEXT,__const", align 4
@.str.1 = private constant [7 x i8] c"MyMain\00"
@"$s8myModule6MyMainVMn" = hidden constant <{ i32, i32, i32, i32, i32, i32, i32 }> <{ i32 81, i32 trunc (i64 sub (i64 ptrtoint (<{ i32, i32, i32 }>* @"$s8myModuleMXM" to i64), i64 ptrtoint (i32* getelementptr inbounds (<{ i32, i32, i32, i32, i32, i32, i32 }>, <{ i32, i32, i32, i32, i32, i32, i32 }>* @"$s8myModule6MyMainVMn", i32 0, i32 1) to i64)) to i32), i32 trunc (i64 sub (i64 ptrtoint ([7 x i8]* @.str.1 to i64), i64 ptrtoint (i32* getelementptr inbounds (<{ i32, i32, i32, i32, i32, i32, i32 }>, <{ i32, i32, i32, i32, i32, i32, i32 }>* @"$s8myModule6MyMainVMn", i32 0, i32 2) to i64)) to i32), i32 trunc (i64 sub (i64 ptrtoint (%swift.metadata_response (i64)* @"$s8myModule6MyMainVMa" to i64), i64 ptrtoint (i32* getelementptr inbounds (<{ i32, i32, i32, i32, i32, i32, i32 }>, <{ i32, i32, i32, i32, i32, i32, i32 }>* @"$s8myModule6MyMainVMn", i32 0, i32 3) to i64)) to i32), i32 trunc (i64 sub (i64 ptrtoint ({ i32, i32, i16, i16, i32 }* @"$s8myModule6MyMainVMF" to i64), i64 ptrtoint (i32* getelementptr inbounds (<{ i32, i32, i32, i32, i32, i32, i32 }>, <{ i32, i32, i32, i32, i32, i32, i32 }>* @"$s8myModule6MyMainVMn", i32 0, i32 4) to i64)) to i32), i32 0, i32 2 }>, section "__TEXT,__const", align 4
@"$s8myModule6MyMainVMf" = internal constant <{ i8**, i64, <{ i32, i32, i32, i32, i32, i32, i32 }>* }> <{ i8** @"$sytWV", i64 512, <{ i32, i32, i32, i32, i32, i32, i32 }>* @"$s8myModule6MyMainVMn" }>, align 8
@"symbolic _____ 8myModule6MyMainV" = linkonce_odr hidden constant <{ i8, i32, i8 }> <{ i8 1, i32 trunc (i64 sub (i64 ptrtoint (<{ i32, i32, i32, i32, i32, i32, i32 }>* @"$s8myModule6MyMainVMn" to i64), i64 ptrtoint (i32* getelementptr inbounds (<{ i8, i32, i8 }>, <{ i8, i32, i8 }>* @"symbolic _____ 8myModule6MyMainV", i32 0, i32 1) to i64)) to i32), i8 0 }>, section "__TEXT,__swift5_typeref, regular", align 2
@"$s8myModule6MyMainVMF" = internal constant { i32, i32, i16, i16, i32 } { i32 trunc (i64 sub (i64 ptrtoint (<{ i8, i32, i8 }>* @"symbolic _____ 8myModule6MyMainV" to i64), i64 ptrtoint ({ i32, i32, i16, i16, i32 }* @"$s8myModule6MyMainVMF" to i64)) to i32), i32 0, i16 0, i16 12, i32 0 }, section "__TEXT,__swift5_fieldmd, regular", align 4
@"$s8myModule6MyMainVHn" = private constant %swift.type_metadata_record { i32 trunc (i64 sub (i64 ptrtoint (<{ i32, i32, i32, i32, i32, i32, i32 }>* @"$s8myModule6MyMainVMn" to i64), i64 ptrtoint (%swift.type_metadata_record* @"$s8myModule6MyMainVHn" to i64)) to i32) }, section "__TEXT, __swift5_types, regular", align 4
@__swift_reflection_version = linkonce_odr hidden constant i16 3
@llvm.used = appending global [5 x i8*] [i8* bitcast (i32 (i32, i8**)* @main to i8*), i8* bitcast ({ i32 }* @"\01l_entry_point" to i8*), i8* bitcast ({ i32, i32, i16, i16, i32 }* @"$s8myModule6MyMainVMF" to i8*), i8* bitcast (%swift.type_metadata_record* @"$s8myModule6MyMainVHn" to i8*), i8* bitcast (i16* @__swift_reflection_version to i8*)], section "llvm.metadata"

@"$s8myModule6MyMainVN" = hidden alias %swift.type, bitcast (i64* getelementptr inbounds (<{ i8**, i64, <{ i32, i32, i32, i32, i32, i32, i32 }>* }>, <{ i8**, i64, <{ i32, i32, i32, i32, i32, i32, i32 }>* }>* @"$s8myModule6MyMainVMf", i32 0, i32 1) to %swift.type*)

define hidden swiftcc void @"$s8myModule6MyMainV4mainyyFZ"() #0 !dbg !44 {
entry:
  call void @llvm.dbg.value(metadata i64 0, metadata !53, metadata !DIExpression()), !dbg !55
  %0 = call swiftcc i64 @"$s8myModule6MyMainV9simpleAdd1xS2i_tFZ"(i64 1), !dbg !56
  ret void, !dbg !58
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

define hidden swiftcc i64 @"$s8myModule6MyMainV9simpleAdd1xS2i_tFZ"(i64 %0) #0 !dbg !59 {
entry:
  %x.debug = alloca i64, align 8
  call void @llvm.dbg.declare(metadata i64* %x.debug, metadata !65, metadata !DIExpression()), !dbg !71
  %1 = bitcast i64* %x.debug to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %1, i8 0, i64 8, i1 false)
  %2 = alloca %TSi, align 8
  call void @llvm.dbg.declare(metadata %TSi* %2, metadata !68, metadata !DIExpression()), !dbg !72
  %3 = bitcast %TSi* %2 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %3, i8 0, i64 8, i1 false)
  %4 = alloca %TSi, align 8
  call void @llvm.dbg.declare(metadata %TSi* %4, metadata !70, metadata !DIExpression()), !dbg !73
  %5 = bitcast %TSi* %4 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %5, i8 0, i64 8, i1 false)
  store i64 %0, i64* %x.debug, align 8, !dbg !74
  call void @llvm.dbg.value(metadata i64 0, metadata !67, metadata !DIExpression()), !dbg !75
  %6 = bitcast %TSi* %2 to i8*, !dbg !76
  call void @llvm.lifetime.start.p0i8(i64 8, i8* %6), !dbg !76
  %._value = getelementptr inbounds %TSi, %TSi* %2, i32 0, i32 0, !dbg !78
  store i64 %0, i64* %._value, align 8, !dbg !78
  %7 = bitcast %TSi* %4 to i8*, !dbg !76
  call void @llvm.lifetime.start.p0i8(i64 8, i8* %7), !dbg !76
  %8 = call { i64, i1 } @llvm.sadd.with.overflow.i64(i64 %0, i64 41), !dbg !79
  %9 = extractvalue { i64, i1 } %8, 0, !dbg !79
  %10 = extractvalue { i64, i1 } %8, 1, !dbg !79
  %11 = call i1 @llvm.expect.i1(i1 %10, i1 false), !dbg !79
  br i1 %11, label %15, label %12, !dbg !79

12:                                               ; preds = %entry
  %._value1 = getelementptr inbounds %TSi, %TSi* %4, i32 0, i32 0, !dbg !79
  store i64 %9, i64* %._value1, align 8, !dbg !79
  %13 = bitcast %TSi* %4 to i8*, !dbg !80
  call void @llvm.lifetime.end.p0i8(i64 8, i8* %13), !dbg !80
  %14 = bitcast %TSi* %2 to i8*, !dbg !80
  call void @llvm.lifetime.end.p0i8(i64 8, i8* %14), !dbg !80
  ret i64 %9, !dbg !80

15:                                               ; preds = %entry
  call void @llvm.trap(), !dbg !81
  unreachable, !dbg !81
}

; Function Attrs: argmemonly nofree nounwind willreturn writeonly
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg) #2

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #3

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare { i64, i1 } @llvm.sadd.with.overflow.i64(i64, i64) #1

; Function Attrs: nofree nosync nounwind readnone willreturn
declare i1 @llvm.expect.i1(i1, i1) #4

; Function Attrs: cold noreturn nounwind
declare void @llvm.trap() #5

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #3

define hidden swiftcc void @"$s8myModule6MyMainV5$mainyyFZ"() #0 !dbg !84 {
entry:
  call void @llvm.dbg.value(metadata i64 0, metadata !86, metadata !DIExpression()), !dbg !87
  call swiftcc void @"$s8myModule6MyMainV4mainyyFZ"(), !dbg !88
  ret void, !dbg !91
}

define hidden swiftcc void @"$s8myModule6MyMainVACycfC"() #0 !dbg !92 {
entry:
  call void @llvm.dbg.value(metadata %T8myModule6MyMainV* undef, metadata !96, metadata !DIExpression()), !dbg !97
  ret void, !dbg !98
}

define i32 @main(i32 %0, i8** %1) #0 !dbg !100 {
entry:
  %2 = bitcast i8** %1 to i8*
  call swiftcc void @"$s8myModule6MyMainV5$mainyyFZ"(), !dbg !111
  ret i32 0, !dbg !111
}

; Function Attrs: noinline nounwind readnone
define hidden swiftcc %swift.metadata_response @"$s8myModule6MyMainVMa"(i64 %0) #6 !dbg !113 {
entry:
  ret %swift.metadata_response { %swift.type* bitcast (i64* getelementptr inbounds (<{ i8**, i64, <{ i32, i32, i32, i32, i32, i32, i32 }>* }>, <{ i8**, i64, <{ i32, i32, i32, i32, i32, i32, i32 }>* }>* @"$s8myModule6MyMainVMf", i32 0, i32 1) to %swift.type*), i64 0 }, !dbg !114
}

attributes #0 = { "frame-pointer"="non-leaf" "no-trapping-math"="true" "probe-stack"="__chkstk_darwin" "stack-protector-buffer-size"="8" "target-cpu"="apple-a12" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+v8.3a,+zcm,+zcz" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { argmemonly nofree nounwind willreturn writeonly }
attributes #3 = { argmemonly nofree nosync nounwind willreturn }
attributes #4 = { nofree nosync nounwind readnone willreturn }
attributes #5 = { cold noreturn nounwind }
attributes #6 = { noinline nounwind readnone "frame-pointer"="none" "no-trapping-math"="true" "probe-stack"="__chkstk_darwin" "stack-protector-buffer-size"="8" "target-cpu"="apple-a12" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+v8.3a,+zcm,+zcz" }

!llvm.module.flags = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15, !16, !17}
!llvm.dbg.cu = !{!18, !31}
!swift.module.flags = !{!33}
!llvm.asan.globals = !{!34, !35, !36, !37, !38}
!llvm.linker.options = !{!39, !40, !41, !42, !43}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 13, i32 1]}
!1 = !{i32 1, !"Objective-C Version", i32 2}
!2 = !{i32 1, !"Objective-C Image Info Version", i32 0}
!3 = !{i32 1, !"Objective-C Image Info Section", !"__DATA,__objc_imageinfo,regular,no_dead_strip"}
!4 = !{i32 4, !"Objective-C Garbage Collection", i32 84346624}
!5 = !{i32 1, !"Objective-C Class Properties", i32 64}
!6 = !{i32 1, !"Objective-C Enforce ClassRO Pointer Signing", i8 0}
!7 = !{i32 7, !"Dwarf Version", i32 4}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = !{i32 1, !"wchar_size", i32 4}
!10 = !{i32 1, !"branch-target-enforcement", i32 0}
!11 = !{i32 1, !"sign-return-address", i32 0}
!12 = !{i32 1, !"sign-return-address-all", i32 0}
!13 = !{i32 1, !"sign-return-address-with-bkey", i32 0}
!14 = !{i32 7, !"PIC Level", i32 2}
!15 = !{i32 7, !"uwtable", i32 1}
!16 = !{i32 7, !"frame-pointer", i32 1}
!17 = !{i32 1, !"Swift Version", i32 7}
!18 = distinct !DICompileUnit(language: DW_LANG_Swift, file: !19, producer: "Apple Swift version 5.7.2 (swiftlang-5.7.2.135.5 clang-1400.0.29.51)", isOptimized: false, runtimeVersion: 5, emissionKind: FullDebug, imports: !20, sysroot: "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.1.sdk", sdk: "MacOSX13.1.sdk")
!19 = !DIFile(filename: "test/arithmetic-operators/Addition/Swift/SimpleAdd.swift", directory: "/Users/struewer/git/swift-llvm-statistics-comparison/Swift-C++-Testsuite")
!20 = !{!21, !23, !25, !27, !29}
!21 = !DIImportedEntity(tag: DW_TAG_imported_module, scope: !19, entity: !22, file: !19)
!22 = !DIModule(scope: null, name: "myModule", includePath: "test/arithmetic-operators/Addition/Swift")
!23 = !DIImportedEntity(tag: DW_TAG_imported_module, scope: !19, entity: !24, file: !19)
!24 = !DIModule(scope: null, name: "Swift", includePath: "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.1.sdk/usr/lib/swift/Swift.swiftmodule/arm64e-apple-macos.swiftinterface")
!25 = !DIImportedEntity(tag: DW_TAG_imported_module, scope: !19, entity: !26, file: !19)
!26 = !DIModule(scope: null, name: "_StringProcessing", includePath: "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.1.sdk/usr/lib/swift/_StringProcessing.swiftmodule/arm64e-apple-macos.swiftinterface")
!27 = !DIImportedEntity(tag: DW_TAG_imported_module, scope: !19, entity: !28, file: !19)
!28 = !DIModule(scope: null, name: "_Concurrency", includePath: "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.1.sdk/usr/lib/swift/_Concurrency.swiftmodule/arm64e-apple-macos.swiftinterface")
!29 = !DIImportedEntity(tag: DW_TAG_imported_module, scope: !19, entity: !30, file: !19)
!30 = !DIModule(scope: null, name: "SwiftOnoneSupport", includePath: "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.1.sdk/usr/lib/swift/SwiftOnoneSupport.swiftmodule/arm64e-apple-macos.swiftinterface")
!31 = distinct !DICompileUnit(language: DW_LANG_ObjC, file: !32, producer: "Apple clang version 14.0.0 (clang-1400.0.29.51)", isOptimized: false, runtimeVersion: 2, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None, sysroot: "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.1.sdk", sdk: "MacOSX13.1.sdk")
!32 = !DIFile(filename: "<swift-imported-modules>", directory: "/Users/struewer/git/swift-llvm-statistics-comparison/Swift-C++-Testsuite")
!33 = !{!"standard-library", i1 false}
!34 = !{<{ i32, i32, i32 }>* @"$s8myModuleMXM", null, null, i1 false, i1 true}
!35 = !{<{ i32, i32, i32, i32, i32, i32, i32 }>* @"$s8myModule6MyMainVMn", null, null, i1 false, i1 true}
!36 = !{<{ i8, i32, i8 }>* @"symbolic _____ 8myModule6MyMainV", null, null, i1 false, i1 true}
!37 = !{{ i32, i32, i16, i16, i32 }* @"$s8myModule6MyMainVMF", null, null, i1 false, i1 true}
!38 = !{%swift.type_metadata_record* @"$s8myModule6MyMainVHn", null, null, i1 false, i1 true}
!39 = !{!"-lswiftSwiftOnoneSupport"}
!40 = !{!"-lswiftCore"}
!41 = !{!"-lswift_Concurrency"}
!42 = !{!"-lswift_StringProcessing"}
!43 = !{!"-lobjc"}
!44 = distinct !DISubprogram(name: "main", linkageName: "$s8myModule6MyMainV4mainyyFZ", scope: !45, file: !19, line: 4, type: !47, scopeLine: 4, spFlags: DISPFlagDefinition, unit: !18, retainedNodes: !52)
!45 = !DICompositeType(tag: DW_TAG_structure_type, name: "MyMain", scope: !22, file: !19, elements: !46, runtimeLang: DW_LANG_Swift, identifier: "$s8myModule6MyMainVD")
!46 = !{}
!47 = !DISubroutineType(types: !48)
!48 = !{!49, !50}
!49 = !DICompositeType(tag: DW_TAG_structure_type, name: "$sytD", file: !19, elements: !46, runtimeLang: DW_LANG_Swift, identifier: "$sytD")
!50 = !DICompositeType(tag: DW_TAG_structure_type, name: "$s8myModule6MyMainVXMtD", file: !51, flags: DIFlagArtificial, runtimeLang: DW_LANG_Swift, identifier: "$s8myModule6MyMainVXMtD")
!51 = !DIFile(filename: "<compiler-generated>", directory: "")
!52 = !{!53}
!53 = !DILocalVariable(name: "self", arg: 1, scope: !44, file: !19, line: 4, type: !54, flags: DIFlagArtificial)
!54 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !50)
!55 = !DILocation(line: 4, column: 17, scope: !44)
!56 = !DILocation(line: 8, column: 17, scope: !57)
!57 = distinct !DILexicalBlock(scope: !44, file: !19, line: 4, column: 24)
!58 = !DILocation(line: 9, column: 5, scope: !57)
!59 = distinct !DISubprogram(name: "simpleAdd", linkageName: "$s8myModule6MyMainV9simpleAdd1xS2i_tFZ", scope: !45, file: !19, line: 11, type: !60, scopeLine: 11, spFlags: DISPFlagDefinition, unit: !18, retainedNodes: !64)
!60 = !DISubroutineType(types: !61)
!61 = !{!62, !62, !50}
!62 = !DICompositeType(tag: DW_TAG_structure_type, name: "Int", scope: !24, file: !63, size: 64, elements: !46, runtimeLang: DW_LANG_Swift, identifier: "$sSiD")
!63 = !DIFile(filename: "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.1.sdk/usr/lib/swift/Swift.swiftmodule/arm64e-apple-macos.swiftinterface", directory: "")
!64 = !{!65, !67, !68, !70}
!65 = !DILocalVariable(name: "x", arg: 1, scope: !59, file: !19, line: 11, type: !66)
!66 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !62)
!67 = !DILocalVariable(name: "self", arg: 2, scope: !59, file: !19, line: 11, type: !54, flags: DIFlagArtificial)
!68 = !DILocalVariable(name: "a", scope: !69, file: !19, line: 12, type: !62)
!69 = distinct !DILexicalBlock(scope: !59, file: !19, line: 11, column: 42)
!70 = !DILocalVariable(name: "b", scope: !69, file: !19, line: 13, type: !62)
!71 = !DILocation(line: 11, column: 27, scope: !59)
!72 = !DILocation(line: 12, column: 13, scope: !69)
!73 = !DILocation(line: 13, column: 13, scope: !69)
!74 = !DILocation(line: 0, scope: !59)
!75 = !DILocation(line: 11, column: 17, scope: !59)
!76 = !DILocation(line: 0, scope: !77)
!77 = !DILexicalBlockFile(scope: !69, file: !51, discriminator: 0)
!78 = !DILocation(line: 12, column: 9, scope: !69)
!79 = !DILocation(line: 13, column: 19, scope: !69)
!80 = !DILocation(line: 14, column: 9, scope: !69)
!81 = !DILocation(line: 0, scope: !82, inlinedAt: !79)
!82 = distinct !DISubprogram(name: "Swift runtime failure: arithmetic overflow", scope: !22, file: !19, type: !83, flags: DIFlagArtificial, spFlags: DISPFlagDefinition, unit: !18, retainedNodes: !46)
!83 = !DISubroutineType(types: null)
!84 = distinct !DISubprogram(name: "$main", linkageName: "$s8myModule6MyMainV5$mainyyFZ", scope: !45, file: !51, type: !47, spFlags: DISPFlagDefinition, unit: !18, retainedNodes: !85)
!85 = !{!86}
!86 = !DILocalVariable(name: "self", arg: 1, scope: !84, file: !19, type: !54, flags: DIFlagArtificial)
!87 = !DILocation(line: 0, scope: !84)
!88 = !DILocation(line: 1, column: 1, scope: !89)
!89 = !DILexicalBlockFile(scope: !90, file: !19, discriminator: 0)
!90 = distinct !DILexicalBlock(scope: !84, file: !51)
!91 = !DILocation(line: 0, scope: !90)
!92 = distinct !DISubprogram(name: "init", linkageName: "$s8myModule6MyMainVACycfC", scope: !45, file: !19, line: 2, type: !93, scopeLine: 2, spFlags: DISPFlagDefinition, unit: !18, retainedNodes: !95)
!93 = !DISubroutineType(types: !94)
!94 = !{!45, !50}
!95 = !{!96}
!96 = !DILocalVariable(name: "self", scope: !92, file: !19, line: 2, type: !45)
!97 = !DILocation(line: 2, column: 8, scope: !92)
!98 = !DILocation(line: 2, column: 8, scope: !99)
!99 = distinct !DILexicalBlock(scope: !92, file: !19, line: 2, column: 8)
!100 = distinct !DISubprogram(name: "main", linkageName: "main", scope: !22, file: !19, line: 1, type: !101, spFlags: DISPFlagDefinition, unit: !18, retainedNodes: !46)
!101 = !DISubroutineType(types: !102)
!102 = !{!103, !103, !104}
!103 = !DICompositeType(tag: DW_TAG_structure_type, name: "Int32", scope: !24, file: !63, size: 32, elements: !46, runtimeLang: DW_LANG_Swift, identifier: "$ss5Int32VD")
!104 = !DICompositeType(tag: DW_TAG_structure_type, scope: !24, file: !63, size: 64, elements: !105, runtimeLang: DW_LANG_Swift)
!105 = !{!106}
!106 = !DIDerivedType(tag: DW_TAG_member, scope: !24, file: !63, baseType: !107, size: 64)
!107 = !DICompositeType(tag: DW_TAG_structure_type, name: "UnsafeMutablePointer", scope: !24, file: !63, flags: DIFlagFwdDecl, runtimeLang: DW_LANG_Swift, templateParams: !108, identifier: "$sSpySpys4Int8VGSgGD")
!108 = !{!109}
!109 = !DITemplateTypeParameter(type: !110)
!110 = !DICompositeType(tag: DW_TAG_structure_type, name: "$sSpys4Int8VGSgD", scope: !24, flags: DIFlagFwdDecl, runtimeLang: DW_LANG_Swift, identifier: "$sSpys4Int8VGSgD")
!111 = !DILocation(line: 0, scope: !112)
!112 = !DILexicalBlockFile(scope: !100, file: !51, discriminator: 0)
!113 = distinct !DISubprogram(linkageName: "$s8myModule6MyMainVMa", scope: !22, file: !51, type: !83, flags: DIFlagArtificial, spFlags: DISPFlagDefinition, unit: !18, retainedNodes: !46)
!114 = !DILocation(line: 0, scope: !113)

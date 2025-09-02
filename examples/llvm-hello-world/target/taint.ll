; ModuleID = 'taint.cpp'
source_filename = "taint.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [11 x i8] c"psr.source\00", section "llvm.metadata"
@.str.1 = private unnamed_addr constant [10 x i8] c"taint.cpp\00", section "llvm.metadata"
@stdin = external global ptr, align 8
@.str.2 = private unnamed_addr constant [9 x i8] c"psr.sink\00", section "llvm.metadata"
@stdout = external global ptr, align 8
@__const.main.AnotherBuffer = private unnamed_addr constant <{ [13 x i8], [115 x i8] }> <{ [13 x i8] c"Hello, World!", [115 x i8] zeroinitializer }>, align 16

; Function Attrs: mustprogress noinline optnone uwtable
define dso_local noundef i64 @_Z6sourcePcm(ptr noundef %Buffer, i64 noundef %BufferSize) #0 !dbg !474 {
entry:
  %Buffer.addr = alloca ptr, align 8
  %BufferSize.addr = alloca i64, align 8
  store ptr %Buffer, ptr %Buffer.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %Buffer.addr, metadata !478, metadata !DIExpression()), !dbg !479
  call void @llvm.var.annotation(ptr %Buffer.addr, ptr @.str, ptr @.str.1, i32 7, ptr null)
  store i64 %BufferSize, ptr %BufferSize.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %BufferSize.addr, metadata !480, metadata !DIExpression()), !dbg !481
  %0 = load ptr, ptr %Buffer.addr, align 8, !dbg !482
  %1 = load i64, ptr %BufferSize.addr, align 8, !dbg !483
  %2 = load ptr, ptr @stdin, align 8, !dbg !484
  %call = call i64 @fread(ptr noundef %0, i64 noundef %1, i64 noundef 1, ptr noundef %2), !dbg !485
  ret i64 %call, !dbg !486
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: inaccessiblememonly nocallback nofree nosync nounwind willreturn
declare void @llvm.var.annotation(ptr, ptr, ptr, i32, ptr) #2

declare i64 @fread(ptr noundef, i64 noundef, i64 noundef, ptr noundef) #3

; Function Attrs: mustprogress noinline optnone uwtable
define dso_local void @_Z4sinkPKcm(ptr noundef %Buf, i64 noundef %BufferSize) #0 !dbg !487 {
entry:
  %Buf.addr = alloca ptr, align 8
  %BufferSize.addr = alloca i64, align 8
  store ptr %Buf, ptr %Buf.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %Buf.addr, metadata !490, metadata !DIExpression()), !dbg !491
  call void @llvm.var.annotation(ptr %Buf.addr, ptr @.str.2, ptr @.str.1, i32 12, ptr null)
  store i64 %BufferSize, ptr %BufferSize.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %BufferSize.addr, metadata !492, metadata !DIExpression()), !dbg !493
  %0 = load ptr, ptr %Buf.addr, align 8, !dbg !494
  %1 = load ptr, ptr %Buf.addr, align 8, !dbg !495
  %2 = load i64, ptr %BufferSize.addr, align 8, !dbg !496
  %call = call i64 @strnlen(ptr noundef %1, i64 noundef %2) #8, !dbg !497
  %3 = load ptr, ptr @stdout, align 8, !dbg !498
  %call1 = call i64 @fwrite(ptr noundef %0, i64 noundef %call, i64 noundef 1, ptr noundef %3), !dbg !499
  ret void, !dbg !500
}

declare i64 @fwrite(ptr noundef, i64 noundef, i64 noundef, ptr noundef) #3

; Function Attrs: nounwind readonly willreturn
declare i64 @strnlen(ptr noundef, i64 noundef) #4

; Function Attrs: mustprogress noinline optnone uwtable
define dso_local void @_Z5printPKcS0_m(ptr noundef %Buf1, ptr noundef %Buf2, i64 noundef %Sz) #0 !dbg !501 {
entry:
  %Buf1.addr = alloca ptr, align 8
  %Buf2.addr = alloca ptr, align 8
  %Sz.addr = alloca i64, align 8
  %ToPrint = alloca ptr, align 8
  store ptr %Buf1, ptr %Buf1.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %Buf1.addr, metadata !504, metadata !DIExpression()), !dbg !505
  store ptr %Buf2, ptr %Buf2.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %Buf2.addr, metadata !506, metadata !DIExpression()), !dbg !507
  store i64 %Sz, ptr %Sz.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %Sz.addr, metadata !508, metadata !DIExpression()), !dbg !509
  call void @llvm.dbg.declare(metadata ptr %ToPrint, metadata !510, metadata !DIExpression()), !dbg !511
  %call = call i32 @rand() #9, !dbg !512
  %tobool = icmp ne i32 %call, 0, !dbg !512
  br i1 %tobool, label %if.then, label %if.else, !dbg !514

if.then:                                          ; preds = %entry
  %0 = load ptr, ptr %Buf1.addr, align 8, !dbg !515
  store ptr %0, ptr %ToPrint, align 8, !dbg !517
  br label %if.end, !dbg !518

if.else:                                          ; preds = %entry
  %1 = load ptr, ptr %Buf2.addr, align 8, !dbg !519
  store ptr %1, ptr %ToPrint, align 8, !dbg !521
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %2 = load ptr, ptr %ToPrint, align 8, !dbg !522
  %3 = load i64, ptr %Sz.addr, align 8, !dbg !523
  call void @_Z4sinkPKcm(ptr noundef %2, i64 noundef %3), !dbg !524
  ret void, !dbg !525
}

; Function Attrs: nounwind
declare i32 @rand() #5

; Function Attrs: mustprogress noinline norecurse optnone uwtable
define dso_local noundef i32 @main() #6 !dbg !526 {
entry:
  %Buffer = alloca [128 x i8], align 16
  %AnotherBuffer = alloca [128 x i8], align 16
  call void @llvm.dbg.declare(metadata ptr %Buffer, metadata !527, metadata !DIExpression()), !dbg !531
  %arraydecay = getelementptr inbounds [128 x i8], ptr %Buffer, i64 0, i64 0, !dbg !532
  %call = call noundef i64 @_Z6sourcePcm(ptr noundef %arraydecay, i64 noundef 128), !dbg !533
  call void @llvm.dbg.declare(metadata ptr %AnotherBuffer, metadata !534, metadata !DIExpression()), !dbg !535
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %AnotherBuffer, ptr align 16 @__const.main.AnotherBuffer, i64 128, i1 false), !dbg !535
  %arraydecay1 = getelementptr inbounds [128 x i8], ptr %Buffer, i64 0, i64 0, !dbg !536
  %arraydecay2 = getelementptr inbounds [128 x i8], ptr %AnotherBuffer, i64 0, i64 0, !dbg !537
  call void @_Z5printPKcS0_m(ptr noundef %arraydecay1, ptr noundef %arraydecay2, i64 noundef 128), !dbg !538
  ret i32 0, !dbg !539
}

; Function Attrs: argmemonly nocallback nofree nounwind willreturn
declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg) #7

attributes #0 = { mustprogress noinline optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { inaccessiblememonly nocallback nofree nosync nounwind willreturn }
attributes #3 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nounwind readonly willreturn "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #5 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #6 = { mustprogress noinline norecurse optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #7 = { argmemonly nocallback nofree nounwind willreturn }
attributes #8 = { nounwind readonly willreturn }
attributes #9 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!466, !467, !468, !469, !470, !471, !472}
!llvm.ident = !{!473}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, imports: !2, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "taint.cpp", directory: "phasar/examples/llvm-hello-world/target", checksumkind: CSK_MD5, checksum: "8c1eb5daa247b40a4b4eba02b5bbf906")
!2 = !{!3, !9, !15, !21, !26, !31, !33, !35, !37, !39, !46, !53, !60, !64, !68, !72, !81, !85, !87, !92, !98, !102, !109, !111, !115, !119, !123, !125, !129, !133, !135, !139, !141, !143, !147, !151, !155, !159, !163, !167, !169, !176, !180, !184, !189, !191, !193, !197, !201, !202, !203, !204, !205, !206, !210, !214, !220, !224, !229, !231, !236, !238, !242, !250, !254, !258, !262, !266, !270, !274, !278, !282, !286, !293, !297, !301, !303, !305, !309, !314, !320, !324, !328, !330, !337, !341, !348, !350, !354, !358, !362, !366, !371, !376, !381, !382, !383, !384, !386, !387, !388, !389, !390, !391, !392, !398, !402, !406, !410, !414, !418, !420, !422, !424, !428, !432, !436, !440, !444, !446, !448, !450, !454, !458, !462, !464}
!3 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !5, file: !8, line: 58)
!4 = !DINamespace(name: "std", scope: null)
!5 = !DIDerivedType(tag: DW_TAG_typedef, name: "max_align_t", file: !6, line: 24, baseType: !7)
!6 = !DIFile(filename: "/usr/local/llvm-15/lib/clang/15.0.7/include/__stddef_max_align_t.h", directory: "", checksumkind: CSK_MD5, checksum: "48e8e2456f77e6cda35d245130fa7259")
!7 = !DICompositeType(tag: DW_TAG_structure_type, file: !6, line: 19, size: 256, flags: DIFlagFwdDecl, identifier: "_ZTS11max_align_t")
!8 = !DIFile(filename: "/usr/lib/gcc/x86_64-linux-gnu/13/../../../../include/c++/13/cstddef", directory: "")
!9 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !10, file: !14, line: 98)
!10 = !DIDerivedType(tag: DW_TAG_typedef, name: "FILE", file: !11, line: 7, baseType: !12)
!11 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/FILE.h", directory: "", checksumkind: CSK_MD5, checksum: "571f9fb6223c42439075fdde11a0de5d")
!12 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_FILE", file: !13, line: 49, size: 1728, flags: DIFlagFwdDecl, identifier: "_ZTS8_IO_FILE")
!13 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h", directory: "", checksumkind: CSK_MD5, checksum: "7a6d4a00a37ee6b9a40cd04bd01f5d00")
!14 = !DIFile(filename: "/usr/lib/gcc/x86_64-linux-gnu/13/../../../../include/c++/13/cstdio", directory: "")
!15 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !16, file: !14, line: 99)
!16 = !DIDerivedType(tag: DW_TAG_typedef, name: "fpos_t", file: !17, line: 85, baseType: !18)
!17 = !DIFile(filename: "/usr/include/stdio.h", directory: "", checksumkind: CSK_MD5, checksum: "1e435c46987a169d9f9186f63a512303")
!18 = !DIDerivedType(tag: DW_TAG_typedef, name: "__fpos_t", file: !19, line: 14, baseType: !20)
!19 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/__fpos_t.h", directory: "", checksumkind: CSK_MD5, checksum: "32de8bdaf3551a6c0a9394f9af4389ce")
!20 = !DICompositeType(tag: DW_TAG_structure_type, name: "_G_fpos_t", file: !19, line: 10, size: 128, flags: DIFlagFwdDecl, identifier: "_ZTS9_G_fpos_t")
!21 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !22, file: !14, line: 101)
!22 = !DISubprogram(name: "clearerr", scope: !17, file: !17, line: 860, type: !23, flags: DIFlagPrototyped, spFlags: 0)
!23 = !DISubroutineType(types: !24)
!24 = !{null, !25}
!25 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !10, size: 64)
!26 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !27, file: !14, line: 102)
!27 = !DISubprogram(name: "fclose", scope: !17, file: !17, line: 184, type: !28, flags: DIFlagPrototyped, spFlags: 0)
!28 = !DISubroutineType(types: !29)
!29 = !{!30, !25}
!30 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!31 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !32, file: !14, line: 103)
!32 = !DISubprogram(name: "feof", scope: !17, file: !17, line: 862, type: !28, flags: DIFlagPrototyped, spFlags: 0)
!33 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !34, file: !14, line: 104)
!34 = !DISubprogram(name: "ferror", scope: !17, file: !17, line: 864, type: !28, flags: DIFlagPrototyped, spFlags: 0)
!35 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !36, file: !14, line: 105)
!36 = !DISubprogram(name: "fflush", scope: !17, file: !17, line: 236, type: !28, flags: DIFlagPrototyped, spFlags: 0)
!37 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !38, file: !14, line: 106)
!38 = !DISubprogram(name: "fgetc", scope: !17, file: !17, line: 575, type: !28, flags: DIFlagPrototyped, spFlags: 0)
!39 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !40, file: !14, line: 107)
!40 = !DISubprogram(name: "fgetpos", scope: !17, file: !17, line: 829, type: !41, flags: DIFlagPrototyped, spFlags: 0)
!41 = !DISubroutineType(types: !42)
!42 = !{!30, !43, !44}
!43 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !25)
!44 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !45)
!45 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !16, size: 64)
!46 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !47, file: !14, line: 108)
!47 = !DISubprogram(name: "fgets", scope: !17, file: !17, line: 654, type: !48, flags: DIFlagPrototyped, spFlags: 0)
!48 = !DISubroutineType(types: !49)
!49 = !{!50, !52, !30, !43}
!50 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !51, size: 64)
!51 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!52 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !50)
!53 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !54, file: !14, line: 109)
!54 = !DISubprogram(name: "fopen", scope: !17, file: !17, line: 264, type: !55, flags: DIFlagPrototyped, spFlags: 0)
!55 = !DISubroutineType(types: !56)
!56 = !{!25, !57, !57}
!57 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !58)
!58 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !59, size: 64)
!59 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !51)
!60 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !61, file: !14, line: 110)
!61 = !DISubprogram(name: "fprintf", scope: !17, file: !17, line: 357, type: !62, flags: DIFlagPrototyped, spFlags: 0)
!62 = !DISubroutineType(types: !63)
!63 = !{!30, !43, !57, null}
!64 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !65, file: !14, line: 111)
!65 = !DISubprogram(name: "fputc", scope: !17, file: !17, line: 611, type: !66, flags: DIFlagPrototyped, spFlags: 0)
!66 = !DISubroutineType(types: !67)
!67 = !{!30, !30, !25}
!68 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !69, file: !14, line: 112)
!69 = !DISubprogram(name: "fputs", scope: !17, file: !17, line: 717, type: !70, flags: DIFlagPrototyped, spFlags: 0)
!70 = !DISubroutineType(types: !71)
!71 = !{!30, !57, !43}
!72 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !73, file: !14, line: 113)
!73 = !DISubprogram(name: "fread", scope: !17, file: !17, line: 738, type: !74, flags: DIFlagPrototyped, spFlags: 0)
!74 = !DISubroutineType(types: !75)
!75 = !{!76, !79, !76, !76, !43}
!76 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !77, line: 46, baseType: !78)
!77 = !DIFile(filename: "/usr/local/llvm-15/lib/clang/15.0.7/include/stddef.h", directory: "", checksumkind: CSK_MD5, checksum: "b76978376d35d5cd171876ac58ac1256")
!78 = !DIBasicType(name: "unsigned long", size: 64, encoding: DW_ATE_unsigned)
!79 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !80)
!80 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!81 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !82, file: !14, line: 114)
!82 = !DISubprogram(name: "freopen", scope: !17, file: !17, line: 271, type: !83, flags: DIFlagPrototyped, spFlags: 0)
!83 = !DISubroutineType(types: !84)
!84 = !{!25, !57, !57, !43}
!85 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !86, file: !14, line: 115)
!86 = !DISubprogram(name: "fscanf", linkageName: "__isoc23_fscanf", scope: !17, file: !17, line: 442, type: !62, flags: DIFlagPrototyped, spFlags: 0)
!87 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !88, file: !14, line: 116)
!88 = !DISubprogram(name: "fseek", scope: !17, file: !17, line: 779, type: !89, flags: DIFlagPrototyped, spFlags: 0)
!89 = !DISubroutineType(types: !90)
!90 = !{!30, !25, !91, !30}
!91 = !DIBasicType(name: "long", size: 64, encoding: DW_ATE_signed)
!92 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !93, file: !14, line: 117)
!93 = !DISubprogram(name: "fsetpos", scope: !17, file: !17, line: 835, type: !94, flags: DIFlagPrototyped, spFlags: 0)
!94 = !DISubroutineType(types: !95)
!95 = !{!30, !25, !96}
!96 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !97, size: 64)
!97 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !16)
!98 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !99, file: !14, line: 118)
!99 = !DISubprogram(name: "ftell", scope: !17, file: !17, line: 785, type: !100, flags: DIFlagPrototyped, spFlags: 0)
!100 = !DISubroutineType(types: !101)
!101 = !{!91, !25}
!102 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !103, file: !14, line: 119)
!103 = !DISubprogram(name: "fwrite", scope: !17, file: !17, line: 745, type: !104, flags: DIFlagPrototyped, spFlags: 0)
!104 = !DISubroutineType(types: !105)
!105 = !{!76, !106, !76, !76, !43}
!106 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !107)
!107 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !108, size: 64)
!108 = !DIDerivedType(tag: DW_TAG_const_type, baseType: null)
!109 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !110, file: !14, line: 120)
!110 = !DISubprogram(name: "getc", scope: !17, file: !17, line: 576, type: !28, flags: DIFlagPrototyped, spFlags: 0)
!111 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !112, file: !14, line: 121)
!112 = !DISubprogram(name: "getchar", scope: !17, file: !17, line: 582, type: !113, flags: DIFlagPrototyped, spFlags: 0)
!113 = !DISubroutineType(types: !114)
!114 = !{!30}
!115 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !116, file: !14, line: 126)
!116 = !DISubprogram(name: "perror", scope: !17, file: !17, line: 878, type: !117, flags: DIFlagPrototyped, spFlags: 0)
!117 = !DISubroutineType(types: !118)
!118 = !{null, !58}
!119 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !120, file: !14, line: 127)
!120 = !DISubprogram(name: "printf", scope: !17, file: !17, line: 363, type: !121, flags: DIFlagPrototyped, spFlags: 0)
!121 = !DISubroutineType(types: !122)
!122 = !{!30, !57, null}
!123 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !124, file: !14, line: 128)
!124 = !DISubprogram(name: "putc", scope: !17, file: !17, line: 612, type: !66, flags: DIFlagPrototyped, spFlags: 0)
!125 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !126, file: !14, line: 129)
!126 = !DISubprogram(name: "putchar", scope: !17, file: !17, line: 618, type: !127, flags: DIFlagPrototyped, spFlags: 0)
!127 = !DISubroutineType(types: !128)
!128 = !{!30, !30}
!129 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !130, file: !14, line: 130)
!130 = !DISubprogram(name: "puts", scope: !17, file: !17, line: 724, type: !131, flags: DIFlagPrototyped, spFlags: 0)
!131 = !DISubroutineType(types: !132)
!132 = !{!30, !58}
!133 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !134, file: !14, line: 131)
!134 = !DISubprogram(name: "remove", scope: !17, file: !17, line: 158, type: !131, flags: DIFlagPrototyped, spFlags: 0)
!135 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !136, file: !14, line: 132)
!136 = !DISubprogram(name: "rename", scope: !17, file: !17, line: 160, type: !137, flags: DIFlagPrototyped, spFlags: 0)
!137 = !DISubroutineType(types: !138)
!138 = !{!30, !58, !58}
!139 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !140, file: !14, line: 133)
!140 = !DISubprogram(name: "rewind", scope: !17, file: !17, line: 790, type: !23, flags: DIFlagPrototyped, spFlags: 0)
!141 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !142, file: !14, line: 134)
!142 = !DISubprogram(name: "scanf", linkageName: "__isoc23_scanf", scope: !17, file: !17, line: 445, type: !121, flags: DIFlagPrototyped, spFlags: 0)
!143 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !144, file: !14, line: 135)
!144 = !DISubprogram(name: "setbuf", scope: !17, file: !17, line: 334, type: !145, flags: DIFlagPrototyped, spFlags: 0)
!145 = !DISubroutineType(types: !146)
!146 = !{null, !43, !52}
!147 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !148, file: !14, line: 136)
!148 = !DISubprogram(name: "setvbuf", scope: !17, file: !17, line: 339, type: !149, flags: DIFlagPrototyped, spFlags: 0)
!149 = !DISubroutineType(types: !150)
!150 = !{!30, !43, !52, !30, !76}
!151 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !152, file: !14, line: 137)
!152 = !DISubprogram(name: "sprintf", scope: !17, file: !17, line: 365, type: !153, flags: DIFlagPrototyped, spFlags: 0)
!153 = !DISubroutineType(types: !154)
!154 = !{!30, !52, !57, null}
!155 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !156, file: !14, line: 138)
!156 = !DISubprogram(name: "sscanf", linkageName: "__isoc23_sscanf", scope: !17, file: !17, line: 447, type: !157, flags: DIFlagPrototyped, spFlags: 0)
!157 = !DISubroutineType(types: !158)
!158 = !{!30, !57, !57, null}
!159 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !160, file: !14, line: 139)
!160 = !DISubprogram(name: "tmpfile", scope: !17, file: !17, line: 194, type: !161, flags: DIFlagPrototyped, spFlags: 0)
!161 = !DISubroutineType(types: !162)
!162 = !{!25}
!163 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !164, file: !14, line: 141)
!164 = !DISubprogram(name: "tmpnam", scope: !17, file: !17, line: 211, type: !165, flags: DIFlagPrototyped, spFlags: 0)
!165 = !DISubroutineType(types: !166)
!166 = !{!50, !50}
!167 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !168, file: !14, line: 143)
!168 = !DISubprogram(name: "ungetc", scope: !17, file: !17, line: 731, type: !66, flags: DIFlagPrototyped, spFlags: 0)
!169 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !170, file: !14, line: 144)
!170 = !DISubprogram(name: "vfprintf", scope: !17, file: !17, line: 372, type: !171, flags: DIFlagPrototyped, spFlags: 0)
!171 = !DISubroutineType(types: !172)
!172 = !{!30, !43, !57, !173}
!173 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !174, size: 64)
!174 = !DICompositeType(tag: DW_TAG_structure_type, name: "__va_list_tag", file: !175, size: 192, flags: DIFlagFwdDecl, identifier: "_ZTS13__va_list_tag")
!175 = !DIFile(filename: "taint.cpp", directory: "phasar/examples/llvm-hello-world/target")
!176 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !177, file: !14, line: 145)
!177 = !DISubprogram(name: "vprintf", scope: !17, file: !17, line: 378, type: !178, flags: DIFlagPrototyped, spFlags: 0)
!178 = !DISubroutineType(types: !179)
!179 = !{!30, !57, !173}
!180 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !181, file: !14, line: 146)
!181 = !DISubprogram(name: "vsprintf", scope: !17, file: !17, line: 380, type: !182, flags: DIFlagPrototyped, spFlags: 0)
!182 = !DISubroutineType(types: !183)
!183 = !{!30, !52, !57, !173}
!184 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !186, file: !14, line: 175)
!185 = !DINamespace(name: "__gnu_cxx", scope: null)
!186 = !DISubprogram(name: "snprintf", scope: !17, file: !17, line: 385, type: !187, flags: DIFlagPrototyped, spFlags: 0)
!187 = !DISubroutineType(types: !188)
!188 = !{!30, !52, !76, !57, null}
!189 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !190, file: !14, line: 176)
!190 = !DISubprogram(name: "vfscanf", linkageName: "__isoc23_vfscanf", scope: !17, file: !17, line: 511, type: !171, flags: DIFlagPrototyped, spFlags: 0)
!191 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !192, file: !14, line: 177)
!192 = !DISubprogram(name: "vscanf", linkageName: "__isoc23_vscanf", scope: !17, file: !17, line: 516, type: !178, flags: DIFlagPrototyped, spFlags: 0)
!193 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !194, file: !14, line: 178)
!194 = !DISubprogram(name: "vsnprintf", scope: !17, file: !17, line: 389, type: !195, flags: DIFlagPrototyped, spFlags: 0)
!195 = !DISubroutineType(types: !196)
!196 = !{!30, !52, !76, !57, !173}
!197 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !198, file: !14, line: 179)
!198 = !DISubprogram(name: "vsscanf", linkageName: "__isoc23_vsscanf", scope: !17, file: !17, line: 519, type: !199, flags: DIFlagPrototyped, spFlags: 0)
!199 = !DISubroutineType(types: !200)
!200 = !{!30, !57, !57, !173}
!201 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !186, file: !14, line: 185)
!202 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !190, file: !14, line: 186)
!203 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !192, file: !14, line: 187)
!204 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !194, file: !14, line: 188)
!205 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !198, file: !14, line: 189)
!206 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !207, file: !209, line: 52)
!207 = !DISubprogram(name: "abs", scope: !208, file: !208, line: 980, type: !127, flags: DIFlagPrototyped, spFlags: 0)
!208 = !DIFile(filename: "/usr/include/stdlib.h", directory: "", checksumkind: CSK_MD5, checksum: "7fa2ecb2348a66f8b44ab9a15abd0b72")
!209 = !DIFile(filename: "/usr/lib/gcc/x86_64-linux-gnu/13/../../../../include/c++/13/bits/std_abs.h", directory: "")
!210 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !211, file: !213, line: 131)
!211 = !DIDerivedType(tag: DW_TAG_typedef, name: "div_t", file: !208, line: 63, baseType: !212)
!212 = !DICompositeType(tag: DW_TAG_structure_type, file: !208, line: 59, size: 64, flags: DIFlagFwdDecl, identifier: "_ZTS5div_t")
!213 = !DIFile(filename: "/usr/lib/gcc/x86_64-linux-gnu/13/../../../../include/c++/13/cstdlib", directory: "")
!214 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !215, file: !213, line: 132)
!215 = !DIDerivedType(tag: DW_TAG_typedef, name: "ldiv_t", file: !208, line: 71, baseType: !216)
!216 = distinct !DICompositeType(tag: DW_TAG_structure_type, file: !208, line: 67, size: 128, flags: DIFlagTypePassByValue, elements: !217, identifier: "_ZTS6ldiv_t")
!217 = !{!218, !219}
!218 = !DIDerivedType(tag: DW_TAG_member, name: "quot", scope: !216, file: !208, line: 69, baseType: !91, size: 64)
!219 = !DIDerivedType(tag: DW_TAG_member, name: "rem", scope: !216, file: !208, line: 70, baseType: !91, size: 64, offset: 64)
!220 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !221, file: !213, line: 134)
!221 = !DISubprogram(name: "abort", scope: !208, file: !208, line: 730, type: !222, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: 0)
!222 = !DISubroutineType(types: !223)
!223 = !{null}
!224 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !225, file: !213, line: 138)
!225 = !DISubprogram(name: "atexit", scope: !208, file: !208, line: 734, type: !226, flags: DIFlagPrototyped, spFlags: 0)
!226 = !DISubroutineType(types: !227)
!227 = !{!30, !228}
!228 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !222, size: 64)
!229 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !230, file: !213, line: 141)
!230 = !DISubprogram(name: "at_quick_exit", scope: !208, file: !208, line: 739, type: !226, flags: DIFlagPrototyped, spFlags: 0)
!231 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !232, file: !213, line: 144)
!232 = !DISubprogram(name: "atof", scope: !208, file: !208, line: 102, type: !233, flags: DIFlagPrototyped, spFlags: 0)
!233 = !DISubroutineType(types: !234)
!234 = !{!235, !58}
!235 = !DIBasicType(name: "double", size: 64, encoding: DW_ATE_float)
!236 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !237, file: !213, line: 145)
!237 = !DISubprogram(name: "atoi", scope: !208, file: !208, line: 105, type: !131, flags: DIFlagPrototyped, spFlags: 0)
!238 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !239, file: !213, line: 146)
!239 = !DISubprogram(name: "atol", scope: !208, file: !208, line: 108, type: !240, flags: DIFlagPrototyped, spFlags: 0)
!240 = !DISubroutineType(types: !241)
!241 = !{!91, !58}
!242 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !243, file: !213, line: 147)
!243 = !DISubprogram(name: "bsearch", scope: !208, file: !208, line: 960, type: !244, flags: DIFlagPrototyped, spFlags: 0)
!244 = !DISubroutineType(types: !245)
!245 = !{!80, !107, !107, !76, !76, !246}
!246 = !DIDerivedType(tag: DW_TAG_typedef, name: "__compar_fn_t", file: !208, line: 948, baseType: !247)
!247 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !248, size: 64)
!248 = !DISubroutineType(types: !249)
!249 = !{!30, !107, !107}
!250 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !251, file: !213, line: 148)
!251 = !DISubprogram(name: "calloc", scope: !208, file: !208, line: 675, type: !252, flags: DIFlagPrototyped, spFlags: 0)
!252 = !DISubroutineType(types: !253)
!253 = !{!80, !76, !76}
!254 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !255, file: !213, line: 149)
!255 = !DISubprogram(name: "div", scope: !208, file: !208, line: 992, type: !256, flags: DIFlagPrototyped, spFlags: 0)
!256 = !DISubroutineType(types: !257)
!257 = !{!211, !30, !30}
!258 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !259, file: !213, line: 150)
!259 = !DISubprogram(name: "exit", scope: !208, file: !208, line: 756, type: !260, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: 0)
!260 = !DISubroutineType(types: !261)
!261 = !{null, !30}
!262 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !263, file: !213, line: 151)
!263 = !DISubprogram(name: "free", scope: !208, file: !208, line: 687, type: !264, flags: DIFlagPrototyped, spFlags: 0)
!264 = !DISubroutineType(types: !265)
!265 = !{null, !80}
!266 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !267, file: !213, line: 152)
!267 = !DISubprogram(name: "getenv", scope: !208, file: !208, line: 773, type: !268, flags: DIFlagPrototyped, spFlags: 0)
!268 = !DISubroutineType(types: !269)
!269 = !{!50, !58}
!270 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !271, file: !213, line: 153)
!271 = !DISubprogram(name: "labs", scope: !208, file: !208, line: 981, type: !272, flags: DIFlagPrototyped, spFlags: 0)
!272 = !DISubroutineType(types: !273)
!273 = !{!91, !91}
!274 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !275, file: !213, line: 154)
!275 = !DISubprogram(name: "ldiv", scope: !208, file: !208, line: 994, type: !276, flags: DIFlagPrototyped, spFlags: 0)
!276 = !DISubroutineType(types: !277)
!277 = !{!215, !91, !91}
!278 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !279, file: !213, line: 155)
!279 = !DISubprogram(name: "malloc", scope: !208, file: !208, line: 672, type: !280, flags: DIFlagPrototyped, spFlags: 0)
!280 = !DISubroutineType(types: !281)
!281 = !{!80, !76}
!282 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !283, file: !213, line: 157)
!283 = !DISubprogram(name: "mblen", scope: !208, file: !208, line: 1062, type: !284, flags: DIFlagPrototyped, spFlags: 0)
!284 = !DISubroutineType(types: !285)
!285 = !{!30, !58, !76}
!286 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !287, file: !213, line: 158)
!287 = !DISubprogram(name: "mbstowcs", scope: !208, file: !208, line: 1073, type: !288, flags: DIFlagPrototyped, spFlags: 0)
!288 = !DISubroutineType(types: !289)
!289 = !{!76, !290, !57, !76}
!290 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !291)
!291 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !292, size: 64)
!292 = !DIBasicType(name: "wchar_t", size: 32, encoding: DW_ATE_signed)
!293 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !294, file: !213, line: 159)
!294 = !DISubprogram(name: "mbtowc", scope: !208, file: !208, line: 1065, type: !295, flags: DIFlagPrototyped, spFlags: 0)
!295 = !DISubroutineType(types: !296)
!296 = !{!30, !290, !57, !76}
!297 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !298, file: !213, line: 161)
!298 = !DISubprogram(name: "qsort", scope: !208, file: !208, line: 970, type: !299, flags: DIFlagPrototyped, spFlags: 0)
!299 = !DISubroutineType(types: !300)
!300 = !{null, !80, !76, !76, !246}
!301 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !302, file: !213, line: 164)
!302 = !DISubprogram(name: "quick_exit", scope: !208, file: !208, line: 762, type: !260, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: 0)
!303 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !304, file: !213, line: 167)
!304 = !DISubprogram(name: "rand", scope: !208, file: !208, line: 573, type: !113, flags: DIFlagPrototyped, spFlags: 0)
!305 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !306, file: !213, line: 168)
!306 = !DISubprogram(name: "realloc", scope: !208, file: !208, line: 683, type: !307, flags: DIFlagPrototyped, spFlags: 0)
!307 = !DISubroutineType(types: !308)
!308 = !{!80, !80, !76}
!309 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !310, file: !213, line: 169)
!310 = !DISubprogram(name: "srand", scope: !208, file: !208, line: 575, type: !311, flags: DIFlagPrototyped, spFlags: 0)
!311 = !DISubroutineType(types: !312)
!312 = !{null, !313}
!313 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!314 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !315, file: !213, line: 170)
!315 = !DISubprogram(name: "strtod", scope: !208, file: !208, line: 118, type: !316, flags: DIFlagPrototyped, spFlags: 0)
!316 = !DISubroutineType(types: !317)
!317 = !{!235, !57, !318}
!318 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !319)
!319 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !50, size: 64)
!320 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !321, file: !213, line: 171)
!321 = !DISubprogram(name: "strtol", linkageName: "__isoc23_strtol", scope: !208, file: !208, line: 215, type: !322, flags: DIFlagPrototyped, spFlags: 0)
!322 = !DISubroutineType(types: !323)
!323 = !{!91, !57, !318, !30}
!324 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !325, file: !213, line: 172)
!325 = !DISubprogram(name: "strtoul", linkageName: "__isoc23_strtoul", scope: !208, file: !208, line: 219, type: !326, flags: DIFlagPrototyped, spFlags: 0)
!326 = !DISubroutineType(types: !327)
!327 = !{!78, !57, !318, !30}
!328 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !329, file: !213, line: 173)
!329 = !DISubprogram(name: "system", scope: !208, file: !208, line: 923, type: !131, flags: DIFlagPrototyped, spFlags: 0)
!330 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !331, file: !213, line: 175)
!331 = !DISubprogram(name: "wcstombs", scope: !208, file: !208, line: 1077, type: !332, flags: DIFlagPrototyped, spFlags: 0)
!332 = !DISubroutineType(types: !333)
!333 = !{!76, !52, !334, !76}
!334 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !335)
!335 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !336, size: 64)
!336 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !292)
!337 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !338, file: !213, line: 176)
!338 = !DISubprogram(name: "wctomb", scope: !208, file: !208, line: 1069, type: !339, flags: DIFlagPrototyped, spFlags: 0)
!339 = !DISubroutineType(types: !340)
!340 = !{!30, !50, !292}
!341 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !342, file: !213, line: 204)
!342 = !DIDerivedType(tag: DW_TAG_typedef, name: "lldiv_t", file: !208, line: 81, baseType: !343)
!343 = distinct !DICompositeType(tag: DW_TAG_structure_type, file: !208, line: 77, size: 128, flags: DIFlagTypePassByValue, elements: !344, identifier: "_ZTS7lldiv_t")
!344 = !{!345, !347}
!345 = !DIDerivedType(tag: DW_TAG_member, name: "quot", scope: !343, file: !208, line: 79, baseType: !346, size: 64)
!346 = !DIBasicType(name: "long long", size: 64, encoding: DW_ATE_signed)
!347 = !DIDerivedType(tag: DW_TAG_member, name: "rem", scope: !343, file: !208, line: 80, baseType: !346, size: 64, offset: 64)
!348 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !349, file: !213, line: 210)
!349 = !DISubprogram(name: "_Exit", scope: !208, file: !208, line: 768, type: !260, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: 0)
!350 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !351, file: !213, line: 214)
!351 = !DISubprogram(name: "llabs", scope: !208, file: !208, line: 984, type: !352, flags: DIFlagPrototyped, spFlags: 0)
!352 = !DISubroutineType(types: !353)
!353 = !{!346, !346}
!354 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !355, file: !213, line: 220)
!355 = !DISubprogram(name: "lldiv", scope: !208, file: !208, line: 998, type: !356, flags: DIFlagPrototyped, spFlags: 0)
!356 = !DISubroutineType(types: !357)
!357 = !{!342, !346, !346}
!358 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !359, file: !213, line: 231)
!359 = !DISubprogram(name: "atoll", scope: !208, file: !208, line: 113, type: !360, flags: DIFlagPrototyped, spFlags: 0)
!360 = !DISubroutineType(types: !361)
!361 = !{!346, !58}
!362 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !363, file: !213, line: 232)
!363 = !DISubprogram(name: "strtoll", linkageName: "__isoc23_strtoll", scope: !208, file: !208, line: 238, type: !364, flags: DIFlagPrototyped, spFlags: 0)
!364 = !DISubroutineType(types: !365)
!365 = !{!346, !57, !318, !30}
!366 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !367, file: !213, line: 233)
!367 = !DISubprogram(name: "strtoull", linkageName: "__isoc23_strtoull", scope: !208, file: !208, line: 243, type: !368, flags: DIFlagPrototyped, spFlags: 0)
!368 = !DISubroutineType(types: !369)
!369 = !{!370, !57, !318, !30}
!370 = !DIBasicType(name: "unsigned long long", size: 64, encoding: DW_ATE_unsigned)
!371 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !372, file: !213, line: 235)
!372 = !DISubprogram(name: "strtof", scope: !208, file: !208, line: 124, type: !373, flags: DIFlagPrototyped, spFlags: 0)
!373 = !DISubroutineType(types: !374)
!374 = !{!375, !57, !318}
!375 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!376 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !185, entity: !377, file: !213, line: 236)
!377 = !DISubprogram(name: "strtold", scope: !208, file: !208, line: 127, type: !378, flags: DIFlagPrototyped, spFlags: 0)
!378 = !DISubroutineType(types: !379)
!379 = !{!380, !57, !318}
!380 = !DIBasicType(name: "long double", size: 128, encoding: DW_ATE_float)
!381 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !342, file: !213, line: 244)
!382 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !349, file: !213, line: 246)
!383 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !351, file: !213, line: 248)
!384 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !385, file: !213, line: 249)
!385 = !DISubprogram(name: "div", linkageName: "_ZN9__gnu_cxx3divExx", scope: !185, file: !213, line: 217, type: !356, flags: DIFlagPrototyped, spFlags: 0)
!386 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !355, file: !213, line: 250)
!387 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !359, file: !213, line: 252)
!388 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !372, file: !213, line: 253)
!389 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !363, file: !213, line: 254)
!390 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !367, file: !213, line: 255)
!391 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !377, file: !213, line: 256)
!392 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !393, file: !397, line: 77)
!393 = !DISubprogram(name: "memchr", scope: !394, file: !394, line: 89, type: !395, flags: DIFlagPrototyped, spFlags: 0)
!394 = !DIFile(filename: "/usr/include/string.h", directory: "", checksumkind: CSK_MD5, checksum: "165db1185644f68894fa9e0d17055d70")
!395 = !DISubroutineType(types: !396)
!396 = !{!107, !107, !30, !76}
!397 = !DIFile(filename: "/usr/lib/gcc/x86_64-linux-gnu/13/../../../../include/c++/13/cstring", directory: "")
!398 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !399, file: !397, line: 78)
!399 = !DISubprogram(name: "memcmp", scope: !394, file: !394, line: 64, type: !400, flags: DIFlagPrototyped, spFlags: 0)
!400 = !DISubroutineType(types: !401)
!401 = !{!30, !107, !107, !76}
!402 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !403, file: !397, line: 79)
!403 = !DISubprogram(name: "memcpy", scope: !394, file: !394, line: 43, type: !404, flags: DIFlagPrototyped, spFlags: 0)
!404 = !DISubroutineType(types: !405)
!405 = !{!80, !79, !106, !76}
!406 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !407, file: !397, line: 80)
!407 = !DISubprogram(name: "memmove", scope: !394, file: !394, line: 47, type: !408, flags: DIFlagPrototyped, spFlags: 0)
!408 = !DISubroutineType(types: !409)
!409 = !{!80, !80, !107, !76}
!410 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !411, file: !397, line: 81)
!411 = !DISubprogram(name: "memset", scope: !394, file: !394, line: 61, type: !412, flags: DIFlagPrototyped, spFlags: 0)
!412 = !DISubroutineType(types: !413)
!413 = !{!80, !80, !30, !76}
!414 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !415, file: !397, line: 82)
!415 = !DISubprogram(name: "strcat", scope: !394, file: !394, line: 149, type: !416, flags: DIFlagPrototyped, spFlags: 0)
!416 = !DISubroutineType(types: !417)
!417 = !{!50, !52, !57}
!418 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !419, file: !397, line: 83)
!419 = !DISubprogram(name: "strcmp", scope: !394, file: !394, line: 156, type: !137, flags: DIFlagPrototyped, spFlags: 0)
!420 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !421, file: !397, line: 84)
!421 = !DISubprogram(name: "strcoll", scope: !394, file: !394, line: 163, type: !137, flags: DIFlagPrototyped, spFlags: 0)
!422 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !423, file: !397, line: 85)
!423 = !DISubprogram(name: "strcpy", scope: !394, file: !394, line: 141, type: !416, flags: DIFlagPrototyped, spFlags: 0)
!424 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !425, file: !397, line: 86)
!425 = !DISubprogram(name: "strcspn", scope: !394, file: !394, line: 293, type: !426, flags: DIFlagPrototyped, spFlags: 0)
!426 = !DISubroutineType(types: !427)
!427 = !{!76, !58, !58}
!428 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !429, file: !397, line: 87)
!429 = !DISubprogram(name: "strerror", scope: !394, file: !394, line: 419, type: !430, flags: DIFlagPrototyped, spFlags: 0)
!430 = !DISubroutineType(types: !431)
!431 = !{!50, !30}
!432 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !433, file: !397, line: 88)
!433 = !DISubprogram(name: "strlen", scope: !394, file: !394, line: 407, type: !434, flags: DIFlagPrototyped, spFlags: 0)
!434 = !DISubroutineType(types: !435)
!435 = !{!76, !58}
!436 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !437, file: !397, line: 89)
!437 = !DISubprogram(name: "strncat", scope: !394, file: !394, line: 152, type: !438, flags: DIFlagPrototyped, spFlags: 0)
!438 = !DISubroutineType(types: !439)
!439 = !{!50, !52, !57, !76}
!440 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !441, file: !397, line: 90)
!441 = !DISubprogram(name: "strncmp", scope: !394, file: !394, line: 159, type: !442, flags: DIFlagPrototyped, spFlags: 0)
!442 = !DISubroutineType(types: !443)
!443 = !{!30, !58, !58, !76}
!444 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !445, file: !397, line: 91)
!445 = !DISubprogram(name: "strncpy", scope: !394, file: !394, line: 144, type: !438, flags: DIFlagPrototyped, spFlags: 0)
!446 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !447, file: !397, line: 92)
!447 = !DISubprogram(name: "strspn", scope: !394, file: !394, line: 297, type: !426, flags: DIFlagPrototyped, spFlags: 0)
!448 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !449, file: !397, line: 93)
!449 = !DISubprogram(name: "strtok", scope: !394, file: !394, line: 356, type: !416, flags: DIFlagPrototyped, spFlags: 0)
!450 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !451, file: !397, line: 94)
!451 = !DISubprogram(name: "strxfrm", scope: !394, file: !394, line: 166, type: !452, flags: DIFlagPrototyped, spFlags: 0)
!452 = !DISubroutineType(types: !453)
!453 = !{!76, !52, !57, !76}
!454 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !455, file: !397, line: 95)
!455 = !DISubprogram(name: "strchr", scope: !394, file: !394, line: 228, type: !456, flags: DIFlagPrototyped, spFlags: 0)
!456 = !DISubroutineType(types: !457)
!457 = !{!58, !58, !30}
!458 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !459, file: !397, line: 96)
!459 = !DISubprogram(name: "strpbrk", scope: !394, file: !394, line: 305, type: !460, flags: DIFlagPrototyped, spFlags: 0)
!460 = !DISubroutineType(types: !461)
!461 = !{!58, !58, !58}
!462 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !463, file: !397, line: 97)
!463 = !DISubprogram(name: "strrchr", scope: !394, file: !394, line: 255, type: !456, flags: DIFlagPrototyped, spFlags: 0)
!464 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !465, file: !397, line: 98)
!465 = !DISubprogram(name: "strstr", scope: !394, file: !394, line: 332, type: !460, flags: DIFlagPrototyped, spFlags: 0)
!466 = !{i32 7, !"Dwarf Version", i32 5}
!467 = !{i32 2, !"Debug Info Version", i32 3}
!468 = !{i32 1, !"wchar_size", i32 4}
!469 = !{i32 7, !"PIC Level", i32 2}
!470 = !{i32 7, !"PIE Level", i32 2}
!471 = !{i32 7, !"uwtable", i32 2}
!472 = !{i32 7, !"frame-pointer", i32 2}
!473 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!474 = distinct !DISubprogram(name: "source", linkageName: "_Z6sourcePcm", scope: !1, file: !1, line: 7, type: !475, scopeLine: 8, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !477)
!475 = !DISubroutineType(types: !476)
!476 = !{!76, !50, !76}
!477 = !{}
!478 = !DILocalVariable(name: "Buffer", arg: 1, scope: !474, file: !1, line: 7, type: !50)
!479 = !DILocation(line: 7, column: 55, scope: !474)
!480 = !DILocalVariable(name: "BufferSize", arg: 2, scope: !474, file: !1, line: 8, type: !76)
!481 = !DILocation(line: 8, column: 22, scope: !474)
!482 = !DILocation(line: 9, column: 16, scope: !474)
!483 = !DILocation(line: 9, column: 24, scope: !474)
!484 = !DILocation(line: 9, column: 39, scope: !474)
!485 = !DILocation(line: 9, column: 10, scope: !474)
!486 = !DILocation(line: 9, column: 3, scope: !474)
!487 = distinct !DISubprogram(name: "sink", linkageName: "_Z4sinkPKcm", scope: !1, file: !1, line: 12, type: !488, scopeLine: 12, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !477)
!488 = !DISubroutineType(types: !489)
!489 = !{null, !58, !76}
!490 = !DILocalVariable(name: "Buf", arg: 1, scope: !487, file: !1, line: 12, type: !58)
!491 = !DILocation(line: 12, column: 55, scope: !487)
!492 = !DILocalVariable(name: "BufferSize", arg: 2, scope: !487, file: !1, line: 12, type: !76)
!493 = !DILocation(line: 12, column: 67, scope: !487)
!494 = !DILocation(line: 13, column: 10, scope: !487)
!495 = !DILocation(line: 13, column: 23, scope: !487)
!496 = !DILocation(line: 13, column: 28, scope: !487)
!497 = !DILocation(line: 13, column: 15, scope: !487)
!498 = !DILocation(line: 13, column: 44, scope: !487)
!499 = !DILocation(line: 13, column: 3, scope: !487)
!500 = !DILocation(line: 14, column: 1, scope: !487)
!501 = distinct !DISubprogram(name: "print", linkageName: "_Z5printPKcS0_m", scope: !1, file: !1, line: 16, type: !502, scopeLine: 16, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !477)
!502 = !DISubroutineType(types: !503)
!503 = !{null, !58, !58, !76}
!504 = !DILocalVariable(name: "Buf1", arg: 1, scope: !501, file: !1, line: 16, type: !58)
!505 = !DILocation(line: 16, column: 24, scope: !501)
!506 = !DILocalVariable(name: "Buf2", arg: 2, scope: !501, file: !1, line: 16, type: !58)
!507 = !DILocation(line: 16, column: 42, scope: !501)
!508 = !DILocalVariable(name: "Sz", arg: 3, scope: !501, file: !1, line: 16, type: !76)
!509 = !DILocation(line: 16, column: 55, scope: !501)
!510 = !DILocalVariable(name: "ToPrint", scope: !501, file: !1, line: 18, type: !58)
!511 = !DILocation(line: 18, column: 15, scope: !501)
!512 = !DILocation(line: 19, column: 7, scope: !513)
!513 = distinct !DILexicalBlock(scope: !501, file: !1, line: 19, column: 7)
!514 = !DILocation(line: 19, column: 7, scope: !501)
!515 = !DILocation(line: 20, column: 15, scope: !516)
!516 = distinct !DILexicalBlock(scope: !513, file: !1, line: 19, column: 15)
!517 = !DILocation(line: 20, column: 13, scope: !516)
!518 = !DILocation(line: 21, column: 3, scope: !516)
!519 = !DILocation(line: 22, column: 15, scope: !520)
!520 = distinct !DILexicalBlock(scope: !513, file: !1, line: 21, column: 10)
!521 = !DILocation(line: 22, column: 13, scope: !520)
!522 = !DILocation(line: 25, column: 8, scope: !501)
!523 = !DILocation(line: 25, column: 17, scope: !501)
!524 = !DILocation(line: 25, column: 3, scope: !501)
!525 = !DILocation(line: 26, column: 1, scope: !501)
!526 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 28, type: !113, scopeLine: 28, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !477)
!527 = !DILocalVariable(name: "Buffer", scope: !526, file: !1, line: 29, type: !528)
!528 = !DICompositeType(tag: DW_TAG_array_type, baseType: !51, size: 1024, elements: !529)
!529 = !{!530}
!530 = !DISubrange(count: 128)
!531 = !DILocation(line: 29, column: 8, scope: !526)
!532 = !DILocation(line: 31, column: 10, scope: !526)
!533 = !DILocation(line: 31, column: 3, scope: !526)
!534 = !DILocalVariable(name: "AnotherBuffer", scope: !526, file: !1, line: 32, type: !528)
!535 = !DILocation(line: 32, column: 8, scope: !526)
!536 = !DILocation(line: 34, column: 9, scope: !526)
!537 = !DILocation(line: 34, column: 17, scope: !526)
!538 = !DILocation(line: 34, column: 3, scope: !526)
!539 = !DILocation(line: 35, column: 1, scope: !526)

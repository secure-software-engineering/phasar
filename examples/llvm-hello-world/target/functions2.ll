; ModuleID = 'functions2.cpp'
source_filename = "functions2.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.R = type { i8 }

$_ZN1SC2Ev = comdat any

$_ZN1R4multEii = comdat any

$_ZN5AdderC2Ev = comdat any

$_ZN1S3addEii = comdat any

$_ZTV1S = comdat any

$_ZTS1S = comdat any

$_ZTS5Adder = comdat any

$_ZTI5Adder = comdat any

$_ZTI1S = comdat any

$_ZTV5Adder = comdat any

@F = dso_local global ptr @_Z3addii, align 8, !dbg !0
@A = dso_local global ptr null, align 8, !dbg !26
@M = dso_local global { i64, i64 } { i64 ptrtoint (ptr @_ZN1R4multEii to i64), i64 0 }, align 8, !dbg !29
@_ZTV1S = linkonce_odr dso_local unnamed_addr constant { [3 x ptr] } { [3 x ptr] [ptr null, ptr @_ZTI1S, ptr @_ZN1S3addEii] }, comdat, align 8
@_ZTVN10__cxxabiv120__si_class_type_infoE = external global ptr
@_ZTS1S = linkonce_odr dso_local constant [3 x i8] c"1S\00", comdat, align 1
@_ZTVN10__cxxabiv117__class_type_infoE = external global ptr
@_ZTS5Adder = linkonce_odr dso_local constant [7 x i8] c"5Adder\00", comdat, align 1
@_ZTI5Adder = linkonce_odr dso_local constant { ptr, ptr } { ptr getelementptr inbounds (ptr, ptr @_ZTVN10__cxxabiv117__class_type_infoE, i64 2), ptr @_ZTS5Adder }, comdat, align 8
@_ZTI1S = linkonce_odr dso_local constant { ptr, ptr, ptr } { ptr getelementptr inbounds (ptr, ptr @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2), ptr @_ZTS1S, ptr @_ZTI5Adder }, comdat, align 8
@_ZTV5Adder = linkonce_odr dso_local unnamed_addr constant { [3 x ptr] } { [3 x ptr] [ptr null, ptr @_ZTI5Adder, ptr @__cxa_pure_virtual] }, comdat, align 8
@llvm.global_ctors = appending global [1 x { i32, ptr, ptr }] [{ i32, ptr, ptr } { i32 65535, ptr @_GLOBAL__sub_I_functions2.cpp, ptr null }]

; Function Attrs: mustprogress noinline optnone uwtable
define dso_local noundef ptr @_Z9makeAdderv() #0 !dbg !49 {
entry:
  %call = call noalias noundef nonnull ptr @_Znwm(i64 noundef 8) #7, !dbg !53, !heapallocsite !5
  call void @_ZN1SC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %call) #8, !dbg !54
  ret ptr %call, !dbg !55
}

; Function Attrs: nobuiltin allocsize(0)
declare noundef nonnull ptr @_Znwm(i64 noundef) #1

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN1SC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this) unnamed_addr #2 comdat align 2 !dbg !56 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !60, metadata !DIExpression()), !dbg !62
  %this1 = load ptr, ptr %this.addr, align 8
  call void @_ZN5AdderC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this1) #8, !dbg !63
  store ptr getelementptr inbounds ({ [3 x ptr] }, ptr @_ZTV1S, i32 0, inrange i32 0, i32 2), ptr %this1, align 8, !dbg !63
  ret void, !dbg !63
}

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define dso_local noundef i32 @_Z3addii(i32 noundef %a, i32 noundef %b) #3 !dbg !64 {
entry:
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  store i32 %a, ptr %a.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %a.addr, metadata !65, metadata !DIExpression()), !dbg !66
  store i32 %b, ptr %b.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %b.addr, metadata !67, metadata !DIExpression()), !dbg !68
  %0 = load i32, ptr %a.addr, align 4, !dbg !69
  %1 = load i32, ptr %b.addr, align 4, !dbg !70
  %add = add nsw i32 %0, %1, !dbg !71
  ret i32 %add, !dbg !72
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #4

; Function Attrs: noinline uwtable
define internal void @__cxx_global_var_init() #5 section ".text.startup" !dbg !73 {
entry:
  %call = call noundef ptr @_Z9makeAdderv(), !dbg !77
  store ptr %call, ptr @A, align 8, !dbg !79
  ret void, !dbg !77
}

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define linkonce_odr dso_local noundef i32 @_ZN1R4multEii(ptr noundef nonnull align 1 dereferenceable(1) %this, i32 noundef %a, i32 noundef %b) #3 comdat align 2 !dbg !80 {
entry:
  %this.addr = alloca ptr, align 8
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !81, metadata !DIExpression()), !dbg !83
  store i32 %a, ptr %a.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %a.addr, metadata !84, metadata !DIExpression()), !dbg !85
  store i32 %b, ptr %b.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %b.addr, metadata !86, metadata !DIExpression()), !dbg !87
  %this1 = load ptr, ptr %this.addr, align 8
  %0 = load i32, ptr %a.addr, align 4, !dbg !88
  %1 = load i32, ptr %b.addr, align 4, !dbg !89
  %mul = mul nsw i32 %0, %1, !dbg !90
  ret i32 %mul, !dbg !91
}

; Function Attrs: mustprogress noinline norecurse optnone uwtable
define dso_local noundef i32 @main() #6 !dbg !92 {
entry:
  %retval = alloca i32, align 4
  %a = alloca i32, align 4
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %r = alloca %struct.R, align 1
  %z = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  call void @llvm.dbg.declare(metadata ptr %a, metadata !93, metadata !DIExpression()), !dbg !94
  %call = call noundef i32 @_Z3addii(i32 noundef 4, i32 noundef 5), !dbg !95
  store i32 %call, ptr %a, align 4, !dbg !94
  call void @llvm.dbg.declare(metadata ptr %x, metadata !96, metadata !DIExpression()), !dbg !97
  %0 = load ptr, ptr @A, align 8, !dbg !98
  %vtable = load ptr, ptr %0, align 8, !dbg !99
  %vfn = getelementptr inbounds ptr, ptr %vtable, i64 0, !dbg !99
  %1 = load ptr, ptr %vfn, align 8, !dbg !99
  %call1 = call noundef i32 %1(ptr noundef nonnull align 8 dereferenceable(8) %0, i32 noundef 1, i32 noundef 2), !dbg !99
  store i32 %call1, ptr %x, align 4, !dbg !97
  call void @llvm.dbg.declare(metadata ptr %y, metadata !100, metadata !DIExpression()), !dbg !101
  %2 = load ptr, ptr @F, align 8, !dbg !102
  %call2 = call noundef i32 %2(i32 noundef 1, i32 noundef 2), !dbg !102
  store i32 %call2, ptr %y, align 4, !dbg !101
  call void @llvm.dbg.declare(metadata ptr %r, metadata !103, metadata !DIExpression()), !dbg !104
  call void @llvm.dbg.declare(metadata ptr %z, metadata !105, metadata !DIExpression()), !dbg !106
  %3 = load { i64, i64 }, ptr @M, align 8, !dbg !107
  %memptr.adj = extractvalue { i64, i64 } %3, 1, !dbg !108
  %4 = getelementptr inbounds i8, ptr %r, i64 %memptr.adj, !dbg !108
  %memptr.ptr = extractvalue { i64, i64 } %3, 0, !dbg !108
  %5 = and i64 %memptr.ptr, 1, !dbg !108
  %memptr.isvirtual = icmp ne i64 %5, 0, !dbg !108
  br i1 %memptr.isvirtual, label %memptr.virtual, label %memptr.nonvirtual, !dbg !108

memptr.virtual:                                   ; preds = %entry
  %vtable3 = load ptr, ptr %4, align 8, !dbg !108
  %6 = sub i64 %memptr.ptr, 1, !dbg !108
  %7 = getelementptr i8, ptr %vtable3, i64 %6, !dbg !108, !nosanitize !52
  %memptr.virtualfn = load ptr, ptr %7, align 8, !dbg !108, !nosanitize !52
  br label %memptr.end, !dbg !108

memptr.nonvirtual:                                ; preds = %entry
  %memptr.nonvirtualfn = inttoptr i64 %memptr.ptr to ptr, !dbg !108
  br label %memptr.end, !dbg !108

memptr.end:                                       ; preds = %memptr.nonvirtual, %memptr.virtual
  %8 = phi ptr [ %memptr.virtualfn, %memptr.virtual ], [ %memptr.nonvirtualfn, %memptr.nonvirtual ], !dbg !108
  %call4 = call noundef i32 %8(ptr noundef nonnull align 1 dereferenceable(1) %4, i32 noundef 2, i32 noundef 3), !dbg !108
  store i32 %call4, ptr %z, align 4, !dbg !106
  ret i32 0, !dbg !109
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN5AdderC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this) unnamed_addr #2 comdat align 2 !dbg !110 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !114, metadata !DIExpression()), !dbg !115
  %this1 = load ptr, ptr %this.addr, align 8
  store ptr getelementptr inbounds ({ [3 x ptr] }, ptr @_ZTV5Adder, i32 0, inrange i32 0, i32 2), ptr %this1, align 8, !dbg !116
  ret void, !dbg !116
}

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define linkonce_odr dso_local noundef i32 @_ZN1S3addEii(ptr noundef nonnull align 8 dereferenceable(8) %this, i32 noundef %a, i32 noundef %b) unnamed_addr #3 comdat align 2 !dbg !117 {
entry:
  %this.addr = alloca ptr, align 8
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !118, metadata !DIExpression()), !dbg !119
  store i32 %a, ptr %a.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %a.addr, metadata !120, metadata !DIExpression()), !dbg !121
  store i32 %b, ptr %b.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %b.addr, metadata !122, metadata !DIExpression()), !dbg !123
  %this1 = load ptr, ptr %this.addr, align 8
  %0 = load i32, ptr %a.addr, align 4, !dbg !124
  %1 = load i32, ptr %b.addr, align 4, !dbg !125
  %add = add nsw i32 %0, %1, !dbg !126
  ret i32 %add, !dbg !127
}

declare void @__cxa_pure_virtual() unnamed_addr

; Function Attrs: noinline uwtable
define internal void @_GLOBAL__sub_I_functions2.cpp() #5 section ".text.startup" !dbg !128 {
entry:
  call void @__cxx_global_var_init(), !dbg !130
  ret void
}

attributes #0 = { mustprogress noinline optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nobuiltin allocsize(0) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { mustprogress noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #5 = { noinline uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #6 = { mustprogress noinline norecurse optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #7 = { builtin allocsize(0) }
attributes #8 = { nounwind }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!41, !42, !43, !44, !45, !46, !47}
!llvm.ident = !{!48}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "F", scope: !2, file: !3, line: 22, type: !38, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !3, producer: "clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !4, globals: !25, splitDebugInlining: false, nameTableKind: None)
!3 = !DIFile(filename: "functions2.cpp", directory: "phasar/examples/llvm-hello-world/target", checksumkind: CSK_MD5, checksum: "5b2c3959cc2795e7d87a9b2bbe339351")
!4 = !{!5, !8}
!5 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "S", file: !3, line: 9, size: 64, flags: DIFlagTypePassByReference | DIFlagNonTrivial, elements: !6, vtableHolder: !8, identifier: "_ZTS1S")
!6 = !{!7, !20, !24}
!7 = !DIDerivedType(tag: DW_TAG_inheritance, scope: !5, baseType: !8, extraData: i32 0)
!8 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "Adder", file: !3, line: 1, size: 64, flags: DIFlagTypePassByReference | DIFlagNonTrivial, elements: !9, vtableHolder: !8, identifier: "_ZTS5Adder")
!9 = !{!10, !16}
!10 = !DIDerivedType(tag: DW_TAG_member, name: "_vptr$Adder", scope: !3, file: !3, baseType: !11, size: 64, flags: DIFlagArtificial)
!11 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !12, size: 64)
!12 = !DIDerivedType(tag: DW_TAG_pointer_type, name: "__vtbl_ptr_type", baseType: !13, size: 64)
!13 = !DISubroutineType(types: !14)
!14 = !{!15}
!15 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!16 = !DISubprogram(name: "add", linkageName: "_ZN5Adder3addEii", scope: !8, file: !3, line: 2, type: !17, scopeLine: 2, containingType: !8, virtualIndex: 0, flags: DIFlagPrototyped, spFlags: DISPFlagPureVirtual)
!17 = !DISubroutineType(types: !18)
!18 = !{!15, !19, !15, !15}
!19 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !8, size: 64, flags: DIFlagArtificial | DIFlagObjectPointer)
!20 = !DISubprogram(name: "add", linkageName: "_ZN1S3addEii", scope: !5, file: !3, line: 10, type: !21, scopeLine: 10, containingType: !5, virtualIndex: 0, flags: DIFlagPrototyped, spFlags: DISPFlagVirtual)
!21 = !DISubroutineType(types: !22)
!22 = !{!15, !23, !15, !15}
!23 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !5, size: 64, flags: DIFlagArtificial | DIFlagObjectPointer)
!24 = !DISubprogram(name: "sub", linkageName: "_ZN1S3subEii", scope: !5, file: !3, line: 11, type: !21, scopeLine: 11, flags: DIFlagPrototyped, spFlags: 0)
!25 = !{!0, !26, !29}
!26 = !DIGlobalVariableExpression(var: !27, expr: !DIExpression())
!27 = distinct !DIGlobalVariable(name: "A", scope: !2, file: !3, line: 24, type: !28, isLocal: false, isDefinition: true)
!28 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !8, size: 64)
!29 = !DIGlobalVariableExpression(var: !30, expr: !DIExpression())
!30 = distinct !DIGlobalVariable(name: "M", scope: !2, file: !3, line: 26, type: !31, isLocal: false, isDefinition: true)
!31 = !DIDerivedType(tag: DW_TAG_ptr_to_member_type, baseType: !32, size: 128, extraData: !35)
!32 = !DISubroutineType(types: !33)
!33 = !{!15, !34, !15, !15}
!34 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !35, size: 64, flags: DIFlagArtificial | DIFlagObjectPointer)
!35 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "R", file: !3, line: 5, size: 8, flags: DIFlagTypePassByValue, elements: !36, identifier: "_ZTS1R")
!36 = !{!37}
!37 = !DISubprogram(name: "mult", linkageName: "_ZN1R4multEii", scope: !35, file: !3, line: 6, type: !32, scopeLine: 6, flags: DIFlagPrototyped, spFlags: 0)
!38 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !39, size: 64)
!39 = !DISubroutineType(types: !40)
!40 = !{!15, !15, !15}
!41 = !{i32 7, !"Dwarf Version", i32 5}
!42 = !{i32 2, !"Debug Info Version", i32 3}
!43 = !{i32 1, !"wchar_size", i32 4}
!44 = !{i32 7, !"PIC Level", i32 2}
!45 = !{i32 7, !"PIE Level", i32 2}
!46 = !{i32 7, !"uwtable", i32 2}
!47 = !{i32 7, !"frame-pointer", i32 2}
!48 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!49 = distinct !DISubprogram(name: "makeAdder", linkageName: "_Z9makeAdderv", scope: !3, file: !3, line: 18, type: !50, scopeLine: 18, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !52)
!50 = !DISubroutineType(types: !51)
!51 = !{!28}
!52 = !{}
!53 = !DILocation(line: 18, column: 29, scope: !49)
!54 = !DILocation(line: 18, column: 33, scope: !49)
!55 = !DILocation(line: 18, column: 22, scope: !49)
!56 = distinct !DISubprogram(name: "S", linkageName: "_ZN1SC2Ev", scope: !5, file: !3, line: 9, type: !57, scopeLine: 9, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, declaration: !59, retainedNodes: !52)
!57 = !DISubroutineType(types: !58)
!58 = !{null, !23}
!59 = !DISubprogram(name: "S", scope: !5, type: !57, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: 0)
!60 = !DILocalVariable(name: "this", arg: 1, scope: !56, type: !61, flags: DIFlagArtificial | DIFlagObjectPointer)
!61 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !5, size: 64)
!62 = !DILocation(line: 0, scope: !56)
!63 = !DILocation(line: 9, column: 8, scope: !56)
!64 = distinct !DISubprogram(name: "add", linkageName: "_Z3addii", scope: !3, file: !3, line: 20, type: !39, scopeLine: 20, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !52)
!65 = !DILocalVariable(name: "a", arg: 1, scope: !64, file: !3, line: 20, type: !15)
!66 = !DILocation(line: 20, column: 13, scope: !64)
!67 = !DILocalVariable(name: "b", arg: 2, scope: !64, file: !3, line: 20, type: !15)
!68 = !DILocation(line: 20, column: 20, scope: !64)
!69 = !DILocation(line: 20, column: 32, scope: !64)
!70 = !DILocation(line: 20, column: 36, scope: !64)
!71 = !DILocation(line: 20, column: 34, scope: !64)
!72 = !DILocation(line: 20, column: 25, scope: !64)
!73 = distinct !DISubprogram(name: "__cxx_global_var_init", scope: !74, file: !74, type: !75, flags: DIFlagArtificial, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !52)
!74 = !DIFile(filename: "functions2.cpp", directory: "phasar/examples/llvm-hello-world/target")
!75 = !DISubroutineType(types: !76)
!76 = !{null}
!77 = !DILocation(line: 24, column: 12, scope: !78)
!78 = !DILexicalBlockFile(scope: !73, file: !3, discriminator: 0)
!79 = !DILocation(line: 0, scope: !73)
!80 = distinct !DISubprogram(name: "mult", linkageName: "_ZN1R4multEii", scope: !35, file: !3, line: 6, type: !32, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, declaration: !37, retainedNodes: !52)
!81 = !DILocalVariable(name: "this", arg: 1, scope: !80, type: !82, flags: DIFlagArtificial | DIFlagObjectPointer)
!82 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !35, size: 64)
!83 = !DILocation(line: 0, scope: !80)
!84 = !DILocalVariable(name: "a", arg: 2, scope: !80, file: !3, line: 6, type: !15)
!85 = !DILocation(line: 6, column: 16, scope: !80)
!86 = !DILocalVariable(name: "b", arg: 3, scope: !80, file: !3, line: 6, type: !15)
!87 = !DILocation(line: 6, column: 23, scope: !80)
!88 = !DILocation(line: 6, column: 35, scope: !80)
!89 = !DILocation(line: 6, column: 39, scope: !80)
!90 = !DILocation(line: 6, column: 37, scope: !80)
!91 = !DILocation(line: 6, column: 28, scope: !80)
!92 = distinct !DISubprogram(name: "main", scope: !3, file: !3, line: 28, type: !13, scopeLine: 28, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !52)
!93 = !DILocalVariable(name: "a", scope: !92, file: !3, line: 29, type: !15)
!94 = !DILocation(line: 29, column: 7, scope: !92)
!95 = !DILocation(line: 29, column: 11, scope: !92)
!96 = !DILocalVariable(name: "x", scope: !92, file: !3, line: 30, type: !15)
!97 = !DILocation(line: 30, column: 7, scope: !92)
!98 = !DILocation(line: 30, column: 11, scope: !92)
!99 = !DILocation(line: 30, column: 14, scope: !92)
!100 = !DILocalVariable(name: "y", scope: !92, file: !3, line: 31, type: !15)
!101 = !DILocation(line: 31, column: 7, scope: !92)
!102 = !DILocation(line: 31, column: 11, scope: !92)
!103 = !DILocalVariable(name: "r", scope: !92, file: !3, line: 32, type: !35)
!104 = !DILocation(line: 32, column: 5, scope: !92)
!105 = !DILocalVariable(name: "z", scope: !92, file: !3, line: 33, type: !15)
!106 = !DILocation(line: 33, column: 7, scope: !92)
!107 = !DILocation(line: 33, column: 15, scope: !92)
!108 = !DILocation(line: 33, column: 11, scope: !92)
!109 = !DILocation(line: 34, column: 3, scope: !92)
!110 = distinct !DISubprogram(name: "Adder", linkageName: "_ZN5AdderC2Ev", scope: !8, file: !3, line: 1, type: !111, scopeLine: 1, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, declaration: !113, retainedNodes: !52)
!111 = !DISubroutineType(types: !112)
!112 = !{null, !19}
!113 = !DISubprogram(name: "Adder", scope: !8, type: !111, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: 0)
!114 = !DILocalVariable(name: "this", arg: 1, scope: !110, type: !28, flags: DIFlagArtificial | DIFlagObjectPointer)
!115 = !DILocation(line: 0, scope: !110)
!116 = !DILocation(line: 1, column: 8, scope: !110)
!117 = distinct !DISubprogram(name: "add", linkageName: "_ZN1S3addEii", scope: !5, file: !3, line: 10, type: !21, scopeLine: 10, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, declaration: !20, retainedNodes: !52)
!118 = !DILocalVariable(name: "this", arg: 1, scope: !117, type: !61, flags: DIFlagArtificial | DIFlagObjectPointer)
!119 = !DILocation(line: 0, scope: !117)
!120 = !DILocalVariable(name: "a", arg: 2, scope: !117, file: !3, line: 10, type: !15)
!121 = !DILocation(line: 10, column: 23, scope: !117)
!122 = !DILocalVariable(name: "b", arg: 3, scope: !117, file: !3, line: 10, type: !15)
!123 = !DILocation(line: 10, column: 30, scope: !117)
!124 = !DILocation(line: 10, column: 51, scope: !117)
!125 = !DILocation(line: 10, column: 55, scope: !117)
!126 = !DILocation(line: 10, column: 53, scope: !117)
!127 = !DILocation(line: 10, column: 44, scope: !117)
!128 = distinct !DISubprogram(linkageName: "_GLOBAL__sub_I_functions2.cpp", scope: !74, file: !74, type: !129, flags: DIFlagArtificial, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !52)
!129 = !DISubroutineType(types: !52)
!130 = !DILocation(line: 0, scope: !128)

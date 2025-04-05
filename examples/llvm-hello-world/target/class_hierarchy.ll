; ModuleID = 'class_hierarchy.cpp'
source_filename = "class_hierarchy.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%class.B = type { %class.A }
%class.A = type { ptr }
%class.C = type { %class.A }

$_ZN1BC2Ev = comdat any

$_ZN1B3fooEv = comdat any

$_ZN1CC2Ev = comdat any

$_ZN1C3fooEv = comdat any

$_ZN1CD2Ev = comdat any

$_ZN1BD2Ev = comdat any

$_ZN1AC2Ev = comdat any

$_ZN1BD0Ev = comdat any

$_ZN1AD2Ev = comdat any

$_ZN1AD0Ev = comdat any

$_ZN1CD0Ev = comdat any

$_ZTV1B = comdat any

$_ZTS1B = comdat any

$_ZTS1A = comdat any

$_ZTI1A = comdat any

$_ZTI1B = comdat any

$_ZTV1A = comdat any

$_ZTV1C = comdat any

$_ZTS1C = comdat any

$_ZTI1C = comdat any

@_ZTV1B = linkonce_odr dso_local unnamed_addr constant { [5 x ptr] } { [5 x ptr] [ptr null, ptr @_ZTI1B, ptr @_ZN1BD2Ev, ptr @_ZN1BD0Ev, ptr @_ZN1B3fooEv] }, comdat, align 8
@_ZTVN10__cxxabiv120__si_class_type_infoE = external global ptr
@_ZTS1B = linkonce_odr dso_local constant [3 x i8] c"1B\00", comdat, align 1
@_ZTVN10__cxxabiv117__class_type_infoE = external global ptr
@_ZTS1A = linkonce_odr dso_local constant [3 x i8] c"1A\00", comdat, align 1
@_ZTI1A = linkonce_odr dso_local constant { ptr, ptr } { ptr getelementptr inbounds (ptr, ptr @_ZTVN10__cxxabiv117__class_type_infoE, i64 2), ptr @_ZTS1A }, comdat, align 8
@_ZTI1B = linkonce_odr dso_local constant { ptr, ptr, ptr } { ptr getelementptr inbounds (ptr, ptr @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2), ptr @_ZTS1B, ptr @_ZTI1A }, comdat, align 8
@_ZTV1A = linkonce_odr dso_local unnamed_addr constant { [5 x ptr] } { [5 x ptr] [ptr null, ptr @_ZTI1A, ptr @_ZN1AD2Ev, ptr @_ZN1AD0Ev, ptr @__cxa_pure_virtual] }, comdat, align 8
@.str = private unnamed_addr constant [17 x i8] c"Calling B::foo()\00", align 1, !dbg !0
@_ZTV1C = linkonce_odr dso_local unnamed_addr constant { [5 x ptr] } { [5 x ptr] [ptr null, ptr @_ZTI1C, ptr @_ZN1CD2Ev, ptr @_ZN1CD0Ev, ptr @_ZN1C3fooEv] }, comdat, align 8
@_ZTS1C = linkonce_odr dso_local constant [3 x i8] c"1C\00", comdat, align 1
@_ZTI1C = linkonce_odr dso_local constant { ptr, ptr, ptr } { ptr getelementptr inbounds (ptr, ptr @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2), ptr @_ZTS1C, ptr @_ZTI1A }, comdat, align 8
@.str.1 = private unnamed_addr constant [17 x i8] c"Calling C::foo()\00", align 1, !dbg !8

; Function Attrs: mustprogress noinline norecurse optnone uwtable
define dso_local noundef i32 @main() #0 personality ptr @__gxx_personality_v0 !dbg !242 {
entry:
  %b = alloca %class.B, align 8
  %exn.slot = alloca ptr, align 8
  %ehselector.slot = alloca i32, align 4
  %c = alloca %class.C, align 8
  %a = alloca ptr, align 8
  call void @llvm.dbg.declare(metadata ptr %b, metadata !244, metadata !DIExpression()), !dbg !245
  call void @_ZN1BC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %b) #7, !dbg !245
  invoke void @_ZN1B3fooEv(ptr noundef nonnull align 8 dereferenceable(8) %b)
          to label %invoke.cont unwind label %lpad, !dbg !246

invoke.cont:                                      ; preds = %entry
  call void @llvm.dbg.declare(metadata ptr %c, metadata !247, metadata !DIExpression()), !dbg !248
  call void @_ZN1CC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %c) #7, !dbg !248
  invoke void @_ZN1C3fooEv(ptr noundef nonnull align 8 dereferenceable(8) %c)
          to label %invoke.cont2 unwind label %lpad1, !dbg !249

invoke.cont2:                                     ; preds = %invoke.cont
  call void @llvm.dbg.declare(metadata ptr %a, metadata !250, metadata !DIExpression()), !dbg !252
  store ptr %b, ptr %a, align 8, !dbg !252
  %0 = load ptr, ptr %a, align 8, !dbg !253
  %vtable = load ptr, ptr %0, align 8, !dbg !254
  %vfn = getelementptr inbounds ptr, ptr %vtable, i64 2, !dbg !254
  %1 = load ptr, ptr %vfn, align 8, !dbg !254
  invoke void %1(ptr noundef nonnull align 8 dereferenceable(8) %0)
          to label %invoke.cont3 unwind label %lpad1, !dbg !254

invoke.cont3:                                     ; preds = %invoke.cont2
  call void @_ZN1CD2Ev(ptr noundef nonnull align 8 dereferenceable(8) %c) #7, !dbg !255
  call void @_ZN1BD2Ev(ptr noundef nonnull align 8 dereferenceable(8) %b) #7, !dbg !255
  ret i32 0, !dbg !255

lpad:                                             ; preds = %entry
  %2 = landingpad { ptr, i32 }
          cleanup, !dbg !255
  %3 = extractvalue { ptr, i32 } %2, 0, !dbg !255
  store ptr %3, ptr %exn.slot, align 8, !dbg !255
  %4 = extractvalue { ptr, i32 } %2, 1, !dbg !255
  store i32 %4, ptr %ehselector.slot, align 4, !dbg !255
  br label %ehcleanup, !dbg !255

lpad1:                                            ; preds = %invoke.cont2, %invoke.cont
  %5 = landingpad { ptr, i32 }
          cleanup, !dbg !255
  %6 = extractvalue { ptr, i32 } %5, 0, !dbg !255
  store ptr %6, ptr %exn.slot, align 8, !dbg !255
  %7 = extractvalue { ptr, i32 } %5, 1, !dbg !255
  store i32 %7, ptr %ehselector.slot, align 4, !dbg !255
  call void @_ZN1CD2Ev(ptr noundef nonnull align 8 dereferenceable(8) %c) #7, !dbg !255
  br label %ehcleanup, !dbg !255

ehcleanup:                                        ; preds = %lpad1, %lpad
  call void @_ZN1BD2Ev(ptr noundef nonnull align 8 dereferenceable(8) %b) #7, !dbg !255
  br label %eh.resume, !dbg !255

eh.resume:                                        ; preds = %ehcleanup
  %exn = load ptr, ptr %exn.slot, align 8, !dbg !255
  %sel = load i32, ptr %ehselector.slot, align 4, !dbg !255
  %lpad.val = insertvalue { ptr, i32 } undef, ptr %exn, 0, !dbg !255
  %lpad.val4 = insertvalue { ptr, i32 } %lpad.val, i32 %sel, 1, !dbg !255
  resume { ptr, i32 } %lpad.val4, !dbg !255
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN1BC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this) unnamed_addr #2 comdat align 2 !dbg !256 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !258, metadata !DIExpression()), !dbg !260
  %this1 = load ptr, ptr %this.addr, align 8
  call void @_ZN1AC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this1) #7, !dbg !261
  store ptr getelementptr inbounds ({ [5 x ptr] }, ptr @_ZTV1B, i32 0, inrange i32 0, i32 2), ptr %this1, align 8, !dbg !261
  ret void, !dbg !261
}

; Function Attrs: mustprogress noinline optnone uwtable
define linkonce_odr dso_local void @_ZN1B3fooEv(ptr noundef nonnull align 8 dereferenceable(8) %this) unnamed_addr #3 comdat align 2 !dbg !262 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !263, metadata !DIExpression()), !dbg !264
  %this1 = load ptr, ptr %this.addr, align 8
  %call = call i32 @puts(ptr noundef @.str), !dbg !265
  ret void, !dbg !266
}

declare i32 @__gxx_personality_v0(...)

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN1CC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this) unnamed_addr #2 comdat align 2 !dbg !267 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !269, metadata !DIExpression()), !dbg !271
  %this1 = load ptr, ptr %this.addr, align 8
  call void @_ZN1AC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this1) #7, !dbg !272
  store ptr getelementptr inbounds ({ [5 x ptr] }, ptr @_ZTV1C, i32 0, inrange i32 0, i32 2), ptr %this1, align 8, !dbg !272
  ret void, !dbg !272
}

; Function Attrs: mustprogress noinline optnone uwtable
define linkonce_odr dso_local void @_ZN1C3fooEv(ptr noundef nonnull align 8 dereferenceable(8) %this) unnamed_addr #3 comdat align 2 !dbg !273 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !274, metadata !DIExpression()), !dbg !275
  %this1 = load ptr, ptr %this.addr, align 8
  %call = call i32 @puts(ptr noundef @.str.1), !dbg !276
  ret void, !dbg !277
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN1CD2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this) unnamed_addr #2 comdat align 2 !dbg !278 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !280, metadata !DIExpression()), !dbg !281
  %this1 = load ptr, ptr %this.addr, align 8
  call void @_ZN1AD2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this1) #7, !dbg !282
  ret void, !dbg !284
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN1BD2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this) unnamed_addr #2 comdat align 2 !dbg !285 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !287, metadata !DIExpression()), !dbg !288
  %this1 = load ptr, ptr %this.addr, align 8
  call void @_ZN1AD2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this1) #7, !dbg !289
  ret void, !dbg !291
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN1AC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this) unnamed_addr #2 comdat align 2 !dbg !292 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !294, metadata !DIExpression()), !dbg !296
  %this1 = load ptr, ptr %this.addr, align 8
  store ptr getelementptr inbounds ({ [5 x ptr] }, ptr @_ZTV1A, i32 0, inrange i32 0, i32 2), ptr %this1, align 8, !dbg !297
  ret void, !dbg !297
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN1BD0Ev(ptr noundef nonnull align 8 dereferenceable(8) %this) unnamed_addr #2 comdat align 2 !dbg !298 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !299, metadata !DIExpression()), !dbg !300
  %this1 = load ptr, ptr %this.addr, align 8
  call void @_ZN1BD2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this1) #7, !dbg !301
  call void @_ZdlPv(ptr noundef %this1) #8, !dbg !301
  ret void, !dbg !301
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN1AD2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this) unnamed_addr #2 comdat align 2 !dbg !302 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !303, metadata !DIExpression()), !dbg !304
  %this1 = load ptr, ptr %this.addr, align 8
  ret void, !dbg !305
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN1AD0Ev(ptr noundef nonnull align 8 dereferenceable(8) %this) unnamed_addr #2 comdat align 2 !dbg !306 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !307, metadata !DIExpression()), !dbg !308
  %this1 = load ptr, ptr %this.addr, align 8
  call void @llvm.trap() #9, !dbg !309
  unreachable, !dbg !309
}

declare void @__cxa_pure_virtual() unnamed_addr

; Function Attrs: cold noreturn nounwind
declare void @llvm.trap() #4

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(ptr noundef) #5

declare i32 @puts(ptr noundef) #6

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN1CD0Ev(ptr noundef nonnull align 8 dereferenceable(8) %this) unnamed_addr #2 comdat align 2 !dbg !310 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !311, metadata !DIExpression()), !dbg !312
  %this1 = load ptr, ptr %this.addr, align 8
  call void @_ZN1CD2Ev(ptr noundef nonnull align 8 dereferenceable(8) %this1) #7, !dbg !313
  call void @_ZdlPv(ptr noundef %this1) #8, !dbg !313
  ret void, !dbg !313
}

attributes #0 = { mustprogress noinline norecurse optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { mustprogress noinline optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { cold noreturn nounwind }
attributes #5 = { nobuiltin nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #6 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #7 = { nounwind }
attributes #8 = { builtin nounwind }
attributes #9 = { noreturn nounwind }

!llvm.dbg.cu = !{!10}
!llvm.module.flags = !{!234, !235, !236, !237, !238, !239, !240}
!llvm.ident = !{!241}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 12, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "class_hierarchy.cpp", directory: "phasar/examples/llvm-hello-world/target", checksumkind: CSK_MD5, checksum: "1c279c8f3da97407dfe236d9171aa011")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 136, elements: !6)
!4 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !5)
!5 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!6 = !{!7}
!7 = !DISubrange(count: 17)
!8 = !DIGlobalVariableExpression(var: !9, expr: !DIExpression())
!9 = distinct !DIGlobalVariable(scope: null, file: !2, line: 19, type: !3, isLocal: true, isDefinition: true)
!10 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !2, producer: "clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !11, globals: !39, imports: !40, splitDebugInlining: false, nameTableKind: None)
!11 = !{!12, !15, !32}
!12 = distinct !DICompositeType(tag: DW_TAG_class_type, name: "B", file: !2, line: 9, size: 64, flags: DIFlagTypePassByReference | DIFlagNonTrivial, elements: !13, vtableHolder: !15, identifier: "_ZTS1B")
!13 = !{!14, !28}
!14 = !DIDerivedType(tag: DW_TAG_inheritance, scope: !12, baseType: !15, flags: DIFlagPublic, extraData: i32 0)
!15 = distinct !DICompositeType(tag: DW_TAG_class_type, name: "A", file: !2, line: 2, size: 64, flags: DIFlagTypePassByReference | DIFlagNonTrivial, elements: !16, vtableHolder: !15, identifier: "_ZTS1A")
!16 = !{!17, !23, !27}
!17 = !DIDerivedType(tag: DW_TAG_member, name: "_vptr$A", scope: !2, file: !2, baseType: !18, size: 64, flags: DIFlagArtificial)
!18 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !19, size: 64)
!19 = !DIDerivedType(tag: DW_TAG_pointer_type, name: "__vtbl_ptr_type", baseType: !20, size: 64)
!20 = !DISubroutineType(types: !21)
!21 = !{!22}
!22 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!23 = !DISubprogram(name: "~A", scope: !15, file: !2, line: 4, type: !24, scopeLine: 4, containingType: !15, virtualIndex: 0, flags: DIFlagPublic | DIFlagPrototyped, spFlags: DISPFlagVirtual)
!24 = !DISubroutineType(types: !25)
!25 = !{null, !26}
!26 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !15, size: 64, flags: DIFlagArtificial | DIFlagObjectPointer)
!27 = !DISubprogram(name: "foo", linkageName: "_ZN1A3fooEv", scope: !15, file: !2, line: 6, type: !24, scopeLine: 6, containingType: !15, virtualIndex: 2, flags: DIFlagPublic | DIFlagPrototyped, spFlags: DISPFlagPureVirtual)
!28 = !DISubprogram(name: "foo", linkageName: "_ZN1B3fooEv", scope: !12, file: !2, line: 11, type: !29, scopeLine: 11, containingType: !12, virtualIndex: 2, flags: DIFlagPublic | DIFlagPrototyped, spFlags: DISPFlagVirtual)
!29 = !DISubroutineType(types: !30)
!30 = !{null, !31}
!31 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !12, size: 64, flags: DIFlagArtificial | DIFlagObjectPointer)
!32 = distinct !DICompositeType(tag: DW_TAG_class_type, name: "C", file: !2, line: 16, size: 64, flags: DIFlagTypePassByReference | DIFlagNonTrivial, elements: !33, vtableHolder: !15, identifier: "_ZTS1C")
!33 = !{!34, !35}
!34 = !DIDerivedType(tag: DW_TAG_inheritance, scope: !32, baseType: !15, flags: DIFlagPublic, extraData: i32 0)
!35 = !DISubprogram(name: "foo", linkageName: "_ZN1C3fooEv", scope: !32, file: !2, line: 18, type: !36, scopeLine: 18, containingType: !32, virtualIndex: 2, flags: DIFlagPublic | DIFlagPrototyped, spFlags: DISPFlagVirtual)
!36 = !DISubroutineType(types: !37)
!37 = !{null, !38}
!38 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !32, size: 64, flags: DIFlagArtificial | DIFlagObjectPointer)
!39 = !{!0, !8}
!40 = !{!41, !48, !54, !59, !63, !65, !67, !69, !71, !78, !84, !90, !94, !98, !102, !111, !115, !117, !122, !128, !132, !139, !141, !143, !147, !151, !153, !157, !161, !163, !167, !169, !171, !175, !179, !183, !187, !191, !195, !197, !204, !208, !212, !217, !219, !221, !225, !229, !230, !231, !232, !233}
!41 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !43, file: !47, line: 98)
!42 = !DINamespace(name: "std", scope: null)
!43 = !DIDerivedType(tag: DW_TAG_typedef, name: "FILE", file: !44, line: 7, baseType: !45)
!44 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/FILE.h", directory: "", checksumkind: CSK_MD5, checksum: "571f9fb6223c42439075fdde11a0de5d")
!45 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_FILE", file: !46, line: 49, size: 1728, flags: DIFlagFwdDecl, identifier: "_ZTS8_IO_FILE")
!46 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h", directory: "", checksumkind: CSK_MD5, checksum: "7a6d4a00a37ee6b9a40cd04bd01f5d00")
!47 = !DIFile(filename: "/usr/lib/gcc/x86_64-linux-gnu/13/../../../../include/c++/13/cstdio", directory: "")
!48 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !49, file: !47, line: 99)
!49 = !DIDerivedType(tag: DW_TAG_typedef, name: "fpos_t", file: !50, line: 85, baseType: !51)
!50 = !DIFile(filename: "/usr/include/stdio.h", directory: "", checksumkind: CSK_MD5, checksum: "1e435c46987a169d9f9186f63a512303")
!51 = !DIDerivedType(tag: DW_TAG_typedef, name: "__fpos_t", file: !52, line: 14, baseType: !53)
!52 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/__fpos_t.h", directory: "", checksumkind: CSK_MD5, checksum: "32de8bdaf3551a6c0a9394f9af4389ce")
!53 = !DICompositeType(tag: DW_TAG_structure_type, name: "_G_fpos_t", file: !52, line: 10, size: 128, flags: DIFlagFwdDecl, identifier: "_ZTS9_G_fpos_t")
!54 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !55, file: !47, line: 101)
!55 = !DISubprogram(name: "clearerr", scope: !50, file: !50, line: 860, type: !56, flags: DIFlagPrototyped, spFlags: 0)
!56 = !DISubroutineType(types: !57)
!57 = !{null, !58}
!58 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !43, size: 64)
!59 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !60, file: !47, line: 102)
!60 = !DISubprogram(name: "fclose", scope: !50, file: !50, line: 184, type: !61, flags: DIFlagPrototyped, spFlags: 0)
!61 = !DISubroutineType(types: !62)
!62 = !{!22, !58}
!63 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !64, file: !47, line: 103)
!64 = !DISubprogram(name: "feof", scope: !50, file: !50, line: 862, type: !61, flags: DIFlagPrototyped, spFlags: 0)
!65 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !66, file: !47, line: 104)
!66 = !DISubprogram(name: "ferror", scope: !50, file: !50, line: 864, type: !61, flags: DIFlagPrototyped, spFlags: 0)
!67 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !68, file: !47, line: 105)
!68 = !DISubprogram(name: "fflush", scope: !50, file: !50, line: 236, type: !61, flags: DIFlagPrototyped, spFlags: 0)
!69 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !70, file: !47, line: 106)
!70 = !DISubprogram(name: "fgetc", scope: !50, file: !50, line: 575, type: !61, flags: DIFlagPrototyped, spFlags: 0)
!71 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !72, file: !47, line: 107)
!72 = !DISubprogram(name: "fgetpos", scope: !50, file: !50, line: 829, type: !73, flags: DIFlagPrototyped, spFlags: 0)
!73 = !DISubroutineType(types: !74)
!74 = !{!22, !75, !76}
!75 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !58)
!76 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !77)
!77 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !49, size: 64)
!78 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !79, file: !47, line: 108)
!79 = !DISubprogram(name: "fgets", scope: !50, file: !50, line: 654, type: !80, flags: DIFlagPrototyped, spFlags: 0)
!80 = !DISubroutineType(types: !81)
!81 = !{!82, !83, !22, !75}
!82 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !5, size: 64)
!83 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !82)
!84 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !85, file: !47, line: 109)
!85 = !DISubprogram(name: "fopen", scope: !50, file: !50, line: 264, type: !86, flags: DIFlagPrototyped, spFlags: 0)
!86 = !DISubroutineType(types: !87)
!87 = !{!58, !88, !88}
!88 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !89)
!89 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 64)
!90 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !91, file: !47, line: 110)
!91 = !DISubprogram(name: "fprintf", scope: !50, file: !50, line: 357, type: !92, flags: DIFlagPrototyped, spFlags: 0)
!92 = !DISubroutineType(types: !93)
!93 = !{!22, !75, !88, null}
!94 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !95, file: !47, line: 111)
!95 = !DISubprogram(name: "fputc", scope: !50, file: !50, line: 611, type: !96, flags: DIFlagPrototyped, spFlags: 0)
!96 = !DISubroutineType(types: !97)
!97 = !{!22, !22, !58}
!98 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !99, file: !47, line: 112)
!99 = !DISubprogram(name: "fputs", scope: !50, file: !50, line: 717, type: !100, flags: DIFlagPrototyped, spFlags: 0)
!100 = !DISubroutineType(types: !101)
!101 = !{!22, !88, !75}
!102 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !103, file: !47, line: 113)
!103 = !DISubprogram(name: "fread", scope: !50, file: !50, line: 738, type: !104, flags: DIFlagPrototyped, spFlags: 0)
!104 = !DISubroutineType(types: !105)
!105 = !{!106, !109, !106, !106, !75}
!106 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !107, line: 46, baseType: !108)
!107 = !DIFile(filename: "/usr/local/llvm-15/lib/clang/15.0.7/include/stddef.h", directory: "", checksumkind: CSK_MD5, checksum: "b76978376d35d5cd171876ac58ac1256")
!108 = !DIBasicType(name: "unsigned long", size: 64, encoding: DW_ATE_unsigned)
!109 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !110)
!110 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!111 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !112, file: !47, line: 114)
!112 = !DISubprogram(name: "freopen", scope: !50, file: !50, line: 271, type: !113, flags: DIFlagPrototyped, spFlags: 0)
!113 = !DISubroutineType(types: !114)
!114 = !{!58, !88, !88, !75}
!115 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !116, file: !47, line: 115)
!116 = !DISubprogram(name: "fscanf", linkageName: "__isoc23_fscanf", scope: !50, file: !50, line: 442, type: !92, flags: DIFlagPrototyped, spFlags: 0)
!117 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !118, file: !47, line: 116)
!118 = !DISubprogram(name: "fseek", scope: !50, file: !50, line: 779, type: !119, flags: DIFlagPrototyped, spFlags: 0)
!119 = !DISubroutineType(types: !120)
!120 = !{!22, !58, !121, !22}
!121 = !DIBasicType(name: "long", size: 64, encoding: DW_ATE_signed)
!122 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !123, file: !47, line: 117)
!123 = !DISubprogram(name: "fsetpos", scope: !50, file: !50, line: 835, type: !124, flags: DIFlagPrototyped, spFlags: 0)
!124 = !DISubroutineType(types: !125)
!125 = !{!22, !58, !126}
!126 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !127, size: 64)
!127 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !49)
!128 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !129, file: !47, line: 118)
!129 = !DISubprogram(name: "ftell", scope: !50, file: !50, line: 785, type: !130, flags: DIFlagPrototyped, spFlags: 0)
!130 = !DISubroutineType(types: !131)
!131 = !{!121, !58}
!132 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !133, file: !47, line: 119)
!133 = !DISubprogram(name: "fwrite", scope: !50, file: !50, line: 745, type: !134, flags: DIFlagPrototyped, spFlags: 0)
!134 = !DISubroutineType(types: !135)
!135 = !{!106, !136, !106, !106, !75}
!136 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !137)
!137 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !138, size: 64)
!138 = !DIDerivedType(tag: DW_TAG_const_type, baseType: null)
!139 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !140, file: !47, line: 120)
!140 = !DISubprogram(name: "getc", scope: !50, file: !50, line: 576, type: !61, flags: DIFlagPrototyped, spFlags: 0)
!141 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !142, file: !47, line: 121)
!142 = !DISubprogram(name: "getchar", scope: !50, file: !50, line: 582, type: !20, flags: DIFlagPrototyped, spFlags: 0)
!143 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !144, file: !47, line: 126)
!144 = !DISubprogram(name: "perror", scope: !50, file: !50, line: 878, type: !145, flags: DIFlagPrototyped, spFlags: 0)
!145 = !DISubroutineType(types: !146)
!146 = !{null, !89}
!147 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !148, file: !47, line: 127)
!148 = !DISubprogram(name: "printf", scope: !50, file: !50, line: 363, type: !149, flags: DIFlagPrototyped, spFlags: 0)
!149 = !DISubroutineType(types: !150)
!150 = !{!22, !88, null}
!151 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !152, file: !47, line: 128)
!152 = !DISubprogram(name: "putc", scope: !50, file: !50, line: 612, type: !96, flags: DIFlagPrototyped, spFlags: 0)
!153 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !154, file: !47, line: 129)
!154 = !DISubprogram(name: "putchar", scope: !50, file: !50, line: 618, type: !155, flags: DIFlagPrototyped, spFlags: 0)
!155 = !DISubroutineType(types: !156)
!156 = !{!22, !22}
!157 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !158, file: !47, line: 130)
!158 = !DISubprogram(name: "puts", scope: !50, file: !50, line: 724, type: !159, flags: DIFlagPrototyped, spFlags: 0)
!159 = !DISubroutineType(types: !160)
!160 = !{!22, !89}
!161 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !162, file: !47, line: 131)
!162 = !DISubprogram(name: "remove", scope: !50, file: !50, line: 158, type: !159, flags: DIFlagPrototyped, spFlags: 0)
!163 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !164, file: !47, line: 132)
!164 = !DISubprogram(name: "rename", scope: !50, file: !50, line: 160, type: !165, flags: DIFlagPrototyped, spFlags: 0)
!165 = !DISubroutineType(types: !166)
!166 = !{!22, !89, !89}
!167 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !168, file: !47, line: 133)
!168 = !DISubprogram(name: "rewind", scope: !50, file: !50, line: 790, type: !56, flags: DIFlagPrototyped, spFlags: 0)
!169 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !170, file: !47, line: 134)
!170 = !DISubprogram(name: "scanf", linkageName: "__isoc23_scanf", scope: !50, file: !50, line: 445, type: !149, flags: DIFlagPrototyped, spFlags: 0)
!171 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !172, file: !47, line: 135)
!172 = !DISubprogram(name: "setbuf", scope: !50, file: !50, line: 334, type: !173, flags: DIFlagPrototyped, spFlags: 0)
!173 = !DISubroutineType(types: !174)
!174 = !{null, !75, !83}
!175 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !176, file: !47, line: 136)
!176 = !DISubprogram(name: "setvbuf", scope: !50, file: !50, line: 339, type: !177, flags: DIFlagPrototyped, spFlags: 0)
!177 = !DISubroutineType(types: !178)
!178 = !{!22, !75, !83, !22, !106}
!179 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !180, file: !47, line: 137)
!180 = !DISubprogram(name: "sprintf", scope: !50, file: !50, line: 365, type: !181, flags: DIFlagPrototyped, spFlags: 0)
!181 = !DISubroutineType(types: !182)
!182 = !{!22, !83, !88, null}
!183 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !184, file: !47, line: 138)
!184 = !DISubprogram(name: "sscanf", linkageName: "__isoc23_sscanf", scope: !50, file: !50, line: 447, type: !185, flags: DIFlagPrototyped, spFlags: 0)
!185 = !DISubroutineType(types: !186)
!186 = !{!22, !88, !88, null}
!187 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !188, file: !47, line: 139)
!188 = !DISubprogram(name: "tmpfile", scope: !50, file: !50, line: 194, type: !189, flags: DIFlagPrototyped, spFlags: 0)
!189 = !DISubroutineType(types: !190)
!190 = !{!58}
!191 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !192, file: !47, line: 141)
!192 = !DISubprogram(name: "tmpnam", scope: !50, file: !50, line: 211, type: !193, flags: DIFlagPrototyped, spFlags: 0)
!193 = !DISubroutineType(types: !194)
!194 = !{!82, !82}
!195 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !196, file: !47, line: 143)
!196 = !DISubprogram(name: "ungetc", scope: !50, file: !50, line: 731, type: !96, flags: DIFlagPrototyped, spFlags: 0)
!197 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !198, file: !47, line: 144)
!198 = !DISubprogram(name: "vfprintf", scope: !50, file: !50, line: 372, type: !199, flags: DIFlagPrototyped, spFlags: 0)
!199 = !DISubroutineType(types: !200)
!200 = !{!22, !75, !88, !201}
!201 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !202, size: 64)
!202 = !DICompositeType(tag: DW_TAG_structure_type, name: "__va_list_tag", file: !203, size: 192, flags: DIFlagFwdDecl, identifier: "_ZTS13__va_list_tag")
!203 = !DIFile(filename: "class_hierarchy.cpp", directory: "phasar/examples/llvm-hello-world/target")
!204 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !205, file: !47, line: 145)
!205 = !DISubprogram(name: "vprintf", scope: !50, file: !50, line: 378, type: !206, flags: DIFlagPrototyped, spFlags: 0)
!206 = !DISubroutineType(types: !207)
!207 = !{!22, !88, !201}
!208 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !209, file: !47, line: 146)
!209 = !DISubprogram(name: "vsprintf", scope: !50, file: !50, line: 380, type: !210, flags: DIFlagPrototyped, spFlags: 0)
!210 = !DISubroutineType(types: !211)
!211 = !{!22, !83, !88, !201}
!212 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !213, entity: !214, file: !47, line: 175)
!213 = !DINamespace(name: "__gnu_cxx", scope: null)
!214 = !DISubprogram(name: "snprintf", scope: !50, file: !50, line: 385, type: !215, flags: DIFlagPrototyped, spFlags: 0)
!215 = !DISubroutineType(types: !216)
!216 = !{!22, !83, !106, !88, null}
!217 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !213, entity: !218, file: !47, line: 176)
!218 = !DISubprogram(name: "vfscanf", linkageName: "__isoc23_vfscanf", scope: !50, file: !50, line: 511, type: !199, flags: DIFlagPrototyped, spFlags: 0)
!219 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !213, entity: !220, file: !47, line: 177)
!220 = !DISubprogram(name: "vscanf", linkageName: "__isoc23_vscanf", scope: !50, file: !50, line: 516, type: !206, flags: DIFlagPrototyped, spFlags: 0)
!221 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !213, entity: !222, file: !47, line: 178)
!222 = !DISubprogram(name: "vsnprintf", scope: !50, file: !50, line: 389, type: !223, flags: DIFlagPrototyped, spFlags: 0)
!223 = !DISubroutineType(types: !224)
!224 = !{!22, !83, !106, !88, !201}
!225 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !213, entity: !226, file: !47, line: 179)
!226 = !DISubprogram(name: "vsscanf", linkageName: "__isoc23_vsscanf", scope: !50, file: !50, line: 519, type: !227, flags: DIFlagPrototyped, spFlags: 0)
!227 = !DISubroutineType(types: !228)
!228 = !{!22, !88, !88, !201}
!229 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !214, file: !47, line: 185)
!230 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !218, file: !47, line: 186)
!231 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !220, file: !47, line: 187)
!232 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !222, file: !47, line: 188)
!233 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !42, entity: !226, file: !47, line: 189)
!234 = !{i32 7, !"Dwarf Version", i32 5}
!235 = !{i32 2, !"Debug Info Version", i32 3}
!236 = !{i32 1, !"wchar_size", i32 4}
!237 = !{i32 7, !"PIC Level", i32 2}
!238 = !{i32 7, !"PIE Level", i32 2}
!239 = !{i32 7, !"uwtable", i32 2}
!240 = !{i32 7, !"frame-pointer", i32 2}
!241 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!242 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 23, type: !20, scopeLine: 23, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !10, retainedNodes: !243)
!243 = !{}
!244 = !DILocalVariable(name: "b", scope: !242, file: !2, line: 24, type: !12)
!245 = !DILocation(line: 24, column: 5, scope: !242)
!246 = !DILocation(line: 25, column: 5, scope: !242)
!247 = !DILocalVariable(name: "c", scope: !242, file: !2, line: 27, type: !32)
!248 = !DILocation(line: 27, column: 5, scope: !242)
!249 = !DILocation(line: 28, column: 5, scope: !242)
!250 = !DILocalVariable(name: "a", scope: !242, file: !2, line: 30, type: !251)
!251 = !DIDerivedType(tag: DW_TAG_reference_type, baseType: !15, size: 64)
!252 = !DILocation(line: 30, column: 6, scope: !242)
!253 = !DILocation(line: 31, column: 3, scope: !242)
!254 = !DILocation(line: 31, column: 5, scope: !242)
!255 = !DILocation(line: 32, column: 1, scope: !242)
!256 = distinct !DISubprogram(name: "B", linkageName: "_ZN1BC2Ev", scope: !12, file: !2, line: 9, type: !29, scopeLine: 9, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !10, declaration: !257, retainedNodes: !243)
!257 = !DISubprogram(name: "B", scope: !12, type: !29, flags: DIFlagPublic | DIFlagArtificial | DIFlagPrototyped, spFlags: 0)
!258 = !DILocalVariable(name: "this", arg: 1, scope: !256, type: !259, flags: DIFlagArtificial | DIFlagObjectPointer)
!259 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !12, size: 64)
!260 = !DILocation(line: 0, scope: !256)
!261 = !DILocation(line: 9, column: 7, scope: !256)
!262 = distinct !DISubprogram(name: "foo", linkageName: "_ZN1B3fooEv", scope: !12, file: !2, line: 11, type: !29, scopeLine: 11, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !10, declaration: !28, retainedNodes: !243)
!263 = !DILocalVariable(name: "this", arg: 1, scope: !262, type: !259, flags: DIFlagArtificial | DIFlagObjectPointer)
!264 = !DILocation(line: 0, scope: !262)
!265 = !DILocation(line: 12, column: 5, scope: !262)
!266 = !DILocation(line: 13, column: 3, scope: !262)
!267 = distinct !DISubprogram(name: "C", linkageName: "_ZN1CC2Ev", scope: !32, file: !2, line: 16, type: !36, scopeLine: 16, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !10, declaration: !268, retainedNodes: !243)
!268 = !DISubprogram(name: "C", scope: !32, type: !36, flags: DIFlagPublic | DIFlagArtificial | DIFlagPrototyped, spFlags: 0)
!269 = !DILocalVariable(name: "this", arg: 1, scope: !267, type: !270, flags: DIFlagArtificial | DIFlagObjectPointer)
!270 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !32, size: 64)
!271 = !DILocation(line: 0, scope: !267)
!272 = !DILocation(line: 16, column: 7, scope: !267)
!273 = distinct !DISubprogram(name: "foo", linkageName: "_ZN1C3fooEv", scope: !32, file: !2, line: 18, type: !36, scopeLine: 18, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !10, declaration: !35, retainedNodes: !243)
!274 = !DILocalVariable(name: "this", arg: 1, scope: !273, type: !270, flags: DIFlagArtificial | DIFlagObjectPointer)
!275 = !DILocation(line: 0, scope: !273)
!276 = !DILocation(line: 19, column: 5, scope: !273)
!277 = !DILocation(line: 20, column: 3, scope: !273)
!278 = distinct !DISubprogram(name: "~C", linkageName: "_ZN1CD2Ev", scope: !32, file: !2, line: 16, type: !36, scopeLine: 16, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !10, declaration: !279, retainedNodes: !243)
!279 = !DISubprogram(name: "~C", scope: !32, type: !36, containingType: !32, virtualIndex: 0, flags: DIFlagPublic | DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagVirtual)
!280 = !DILocalVariable(name: "this", arg: 1, scope: !278, type: !270, flags: DIFlagArtificial | DIFlagObjectPointer)
!281 = !DILocation(line: 0, scope: !278)
!282 = !DILocation(line: 16, column: 7, scope: !283)
!283 = distinct !DILexicalBlock(scope: !278, file: !2, line: 16, column: 7)
!284 = !DILocation(line: 16, column: 7, scope: !278)
!285 = distinct !DISubprogram(name: "~B", linkageName: "_ZN1BD2Ev", scope: !12, file: !2, line: 9, type: !29, scopeLine: 9, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !10, declaration: !286, retainedNodes: !243)
!286 = !DISubprogram(name: "~B", scope: !12, type: !29, containingType: !12, virtualIndex: 0, flags: DIFlagPublic | DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagVirtual)
!287 = !DILocalVariable(name: "this", arg: 1, scope: !285, type: !259, flags: DIFlagArtificial | DIFlagObjectPointer)
!288 = !DILocation(line: 0, scope: !285)
!289 = !DILocation(line: 9, column: 7, scope: !290)
!290 = distinct !DILexicalBlock(scope: !285, file: !2, line: 9, column: 7)
!291 = !DILocation(line: 9, column: 7, scope: !285)
!292 = distinct !DISubprogram(name: "A", linkageName: "_ZN1AC2Ev", scope: !15, file: !2, line: 2, type: !24, scopeLine: 2, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !10, declaration: !293, retainedNodes: !243)
!293 = !DISubprogram(name: "A", scope: !15, type: !24, flags: DIFlagPublic | DIFlagArtificial | DIFlagPrototyped, spFlags: 0)
!294 = !DILocalVariable(name: "this", arg: 1, scope: !292, type: !295, flags: DIFlagArtificial | DIFlagObjectPointer)
!295 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !15, size: 64)
!296 = !DILocation(line: 0, scope: !292)
!297 = !DILocation(line: 2, column: 7, scope: !292)
!298 = distinct !DISubprogram(name: "~B", linkageName: "_ZN1BD0Ev", scope: !12, file: !2, line: 9, type: !29, scopeLine: 9, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !10, declaration: !286, retainedNodes: !243)
!299 = !DILocalVariable(name: "this", arg: 1, scope: !298, type: !259, flags: DIFlagArtificial | DIFlagObjectPointer)
!300 = !DILocation(line: 0, scope: !298)
!301 = !DILocation(line: 9, column: 7, scope: !298)
!302 = distinct !DISubprogram(name: "~A", linkageName: "_ZN1AD2Ev", scope: !15, file: !2, line: 4, type: !24, scopeLine: 4, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !10, declaration: !23, retainedNodes: !243)
!303 = !DILocalVariable(name: "this", arg: 1, scope: !302, type: !295, flags: DIFlagArtificial | DIFlagObjectPointer)
!304 = !DILocation(line: 0, scope: !302)
!305 = !DILocation(line: 4, column: 24, scope: !302)
!306 = distinct !DISubprogram(name: "~A", linkageName: "_ZN1AD0Ev", scope: !15, file: !2, line: 4, type: !24, scopeLine: 4, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !10, declaration: !23, retainedNodes: !243)
!307 = !DILocalVariable(name: "this", arg: 1, scope: !306, type: !295, flags: DIFlagArtificial | DIFlagObjectPointer)
!308 = !DILocation(line: 0, scope: !306)
!309 = !DILocation(line: 4, column: 24, scope: !306)
!310 = distinct !DISubprogram(name: "~C", linkageName: "_ZN1CD0Ev", scope: !32, file: !2, line: 16, type: !36, scopeLine: 16, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !10, declaration: !279, retainedNodes: !243)
!311 = !DILocalVariable(name: "this", arg: 1, scope: !310, type: !270, flags: DIFlagArtificial | DIFlagObjectPointer)
!312 = !DILocation(line: 0, scope: !310)
!313 = !DILocation(line: 16, column: 7, scope: !310)

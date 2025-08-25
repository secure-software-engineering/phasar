; ModuleID = 'struct3.cpp'
source_filename = "struct3.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.S = type { i32, i32 }

$_ZN1SC2Eii = comdat any

; Function Attrs: mustprogress noinline norecurse optnone uwtable
define dso_local noundef i32 @main() #0 personality ptr @__gxx_personality_v0 !dbg !20 {
entry:
  %retval = alloca i32, align 4
  %s = alloca ptr, align 8
  %exn.slot = alloca ptr, align 8
  %ehselector.slot = alloca i32, align 4
  %sum = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  call void @llvm.dbg.declare(metadata ptr %s, metadata !24, metadata !DIExpression()), !dbg !26
  %call = call noalias noundef nonnull ptr @_Znwm(i64 noundef 8) #5, !dbg !27, !heapallocsite !3
  invoke void @_ZN1SC2Eii(ptr noundef nonnull align 4 dereferenceable(8) %call, i32 noundef 10, i32 noundef 20)
          to label %invoke.cont unwind label %lpad, !dbg !28

invoke.cont:                                      ; preds = %entry
  store ptr %call, ptr %s, align 8, !dbg !26
  call void @llvm.dbg.declare(metadata ptr %sum, metadata !29, metadata !DIExpression()), !dbg !30
  %0 = load ptr, ptr %s, align 8, !dbg !31
  %x = getelementptr inbounds %struct.S, ptr %0, i32 0, i32 0, !dbg !32
  %1 = load i32, ptr %x, align 4, !dbg !32
  %2 = load ptr, ptr %s, align 8, !dbg !33
  %y = getelementptr inbounds %struct.S, ptr %2, i32 0, i32 1, !dbg !34
  %3 = load i32, ptr %y, align 4, !dbg !34
  %add = add nsw i32 %1, %3, !dbg !35
  store i32 %add, ptr %sum, align 4, !dbg !30
  %4 = load ptr, ptr %s, align 8, !dbg !36
  %isnull = icmp eq ptr %4, null, !dbg !37
  br i1 %isnull, label %delete.end, label %delete.notnull, !dbg !37

delete.notnull:                                   ; preds = %invoke.cont
  call void @_ZdlPv(ptr noundef %4) #6, !dbg !37
  br label %delete.end, !dbg !37

delete.end:                                       ; preds = %delete.notnull, %invoke.cont
  %5 = load i32, ptr %sum, align 4, !dbg !38
  ret i32 %5, !dbg !39

lpad:                                             ; preds = %entry
  %6 = landingpad { ptr, i32 }
          cleanup, !dbg !40
  %7 = extractvalue { ptr, i32 } %6, 0, !dbg !40
  store ptr %7, ptr %exn.slot, align 8, !dbg !40
  %8 = extractvalue { ptr, i32 } %6, 1, !dbg !40
  store i32 %8, ptr %ehselector.slot, align 4, !dbg !40
  call void @_ZdlPv(ptr noundef %call) #6, !dbg !27
  br label %eh.resume, !dbg !27

eh.resume:                                        ; preds = %lpad
  %exn = load ptr, ptr %exn.slot, align 8, !dbg !27
  %sel = load i32, ptr %ehselector.slot, align 4, !dbg !27
  %lpad.val = insertvalue { ptr, i32 } undef, ptr %exn, 0, !dbg !27
  %lpad.val1 = insertvalue { ptr, i32 } %lpad.val, i32 %sel, 1, !dbg !27
  resume { ptr, i32 } %lpad.val1, !dbg !27
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nobuiltin allocsize(0)
declare noundef nonnull ptr @_Znwm(i64 noundef) #2

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN1SC2Eii(ptr noundef nonnull align 4 dereferenceable(8) %this, i32 noundef %i, i32 noundef %j) unnamed_addr #3 comdat align 2 !dbg !41 {
entry:
  %this.addr = alloca ptr, align 8
  %i.addr = alloca i32, align 4
  %j.addr = alloca i32, align 4
  store ptr %this, ptr %this.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %this.addr, metadata !42, metadata !DIExpression()), !dbg !43
  store i32 %i, ptr %i.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %i.addr, metadata !44, metadata !DIExpression()), !dbg !45
  store i32 %j, ptr %j.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %j.addr, metadata !46, metadata !DIExpression()), !dbg !47
  %this1 = load ptr, ptr %this.addr, align 8
  %x = getelementptr inbounds %struct.S, ptr %this1, i32 0, i32 0, !dbg !48
  %0 = load i32, ptr %i.addr, align 4, !dbg !49
  store i32 %0, ptr %x, align 4, !dbg !48
  %y = getelementptr inbounds %struct.S, ptr %this1, i32 0, i32 1, !dbg !50
  %1 = load i32, ptr %j.addr, align 4, !dbg !51
  store i32 %1, ptr %y, align 4, !dbg !50
  ret void, !dbg !52
}

declare i32 @__gxx_personality_v0(...)

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(ptr noundef) #4

attributes #0 = { mustprogress noinline norecurse optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { nobuiltin allocsize(0) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nobuiltin nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #5 = { builtin allocsize(0) }
attributes #6 = { builtin nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!12, !13, !14, !15, !16, !17, !18}
!llvm.ident = !{!19}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !2, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "struct3.cpp", directory: "phasar/examples/llvm-hello-world/target", checksumkind: CSK_MD5, checksum: "c7fb26640a9fc424b32be72824c46ca7")
!2 = !{!3}
!3 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "S", file: !1, line: 1, size: 64, flags: DIFlagTypePassByValue | DIFlagNonTrivial, elements: !4, identifier: "_ZTS1S")
!4 = !{!5, !7, !8}
!5 = !DIDerivedType(tag: DW_TAG_member, name: "x", scope: !3, file: !1, line: 2, baseType: !6, size: 32)
!6 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!7 = !DIDerivedType(tag: DW_TAG_member, name: "y", scope: !3, file: !1, line: 3, baseType: !6, size: 32, offset: 32)
!8 = !DISubprogram(name: "S", scope: !3, file: !1, line: 4, type: !9, scopeLine: 4, flags: DIFlagPrototyped, spFlags: 0)
!9 = !DISubroutineType(types: !10)
!10 = !{null, !11, !6, !6}
!11 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !3, size: 64, flags: DIFlagArtificial | DIFlagObjectPointer)
!12 = !{i32 7, !"Dwarf Version", i32 5}
!13 = !{i32 2, !"Debug Info Version", i32 3}
!14 = !{i32 1, !"wchar_size", i32 4}
!15 = !{i32 7, !"PIC Level", i32 2}
!16 = !{i32 7, !"PIE Level", i32 2}
!17 = !{i32 7, !"uwtable", i32 2}
!18 = !{i32 7, !"frame-pointer", i32 2}
!19 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!20 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 7, type: !21, scopeLine: 7, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !23)
!21 = !DISubroutineType(types: !22)
!22 = !{!6}
!23 = !{}
!24 = !DILocalVariable(name: "s", scope: !20, file: !1, line: 8, type: !25)
!25 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !3, size: 64)
!26 = !DILocation(line: 8, column: 6, scope: !20)
!27 = !DILocation(line: 8, column: 10, scope: !20)
!28 = !DILocation(line: 8, column: 14, scope: !20)
!29 = !DILocalVariable(name: "sum", scope: !20, file: !1, line: 9, type: !6)
!30 = !DILocation(line: 9, column: 7, scope: !20)
!31 = !DILocation(line: 9, column: 13, scope: !20)
!32 = !DILocation(line: 9, column: 16, scope: !20)
!33 = !DILocation(line: 9, column: 20, scope: !20)
!34 = !DILocation(line: 9, column: 23, scope: !20)
!35 = !DILocation(line: 9, column: 18, scope: !20)
!36 = !DILocation(line: 10, column: 10, scope: !20)
!37 = !DILocation(line: 10, column: 3, scope: !20)
!38 = !DILocation(line: 11, column: 10, scope: !20)
!39 = !DILocation(line: 11, column: 3, scope: !20)
!40 = !DILocation(line: 12, column: 1, scope: !20)
!41 = distinct !DISubprogram(name: "S", linkageName: "_ZN1SC2Eii", scope: !3, file: !1, line: 4, type: !9, scopeLine: 4, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, declaration: !8, retainedNodes: !23)
!42 = !DILocalVariable(name: "this", arg: 1, scope: !41, type: !25, flags: DIFlagArtificial | DIFlagObjectPointer)
!43 = !DILocation(line: 0, scope: !41)
!44 = !DILocalVariable(name: "i", arg: 2, scope: !41, file: !1, line: 4, type: !6)
!45 = !DILocation(line: 4, column: 9, scope: !41)
!46 = !DILocalVariable(name: "j", arg: 3, scope: !41, file: !1, line: 4, type: !6)
!47 = !DILocation(line: 4, column: 16, scope: !41)
!48 = !DILocation(line: 4, column: 21, scope: !41)
!49 = !DILocation(line: 4, column: 23, scope: !41)
!50 = !DILocation(line: 4, column: 27, scope: !41)
!51 = !DILocation(line: 4, column: 29, scope: !41)
!52 = !DILocation(line: 4, column: 33, scope: !41)

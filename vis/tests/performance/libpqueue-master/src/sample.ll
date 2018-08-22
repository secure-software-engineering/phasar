; ModuleID = 'sample.c'
source_filename = "sample.c"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.13.0"

%struct.pqueue_t = type { i64, i64, i64, i32 (i64, i64)*, i64 (i8*)*, void (i8*, i64)*, i64 (i8*)*, void (i8*, i64)*, i8** }
%struct.node_t = type { i64, i32, i64 }

@.str = private unnamed_addr constant [17 x i8] c"peek: %lld [%d]\0A\00", align 1
@.str.1 = private unnamed_addr constant [16 x i8] c"pop: %lld [%d]\0A\00", align 1

; Function Attrs: nounwind ssp uwtable
define i32 @main() #0 !dbg !24 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.pqueue_t*, align 8
  %3 = alloca %struct.node_t*, align 8
  %4 = alloca %struct.node_t*, align 8
  store i32 0, i32* %1, align 4
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %2, metadata !27, metadata !63), !dbg !64
  call void @llvm.dbg.declare(metadata %struct.node_t** %3, metadata !65, metadata !63), !dbg !66
  call void @llvm.dbg.declare(metadata %struct.node_t** %4, metadata !67, metadata !63), !dbg !68
  %5 = call i8* @malloc(i64 240), !dbg !69
  %6 = bitcast i8* %5 to %struct.node_t*, !dbg !69
  store %struct.node_t* %6, %struct.node_t** %3, align 8, !dbg !70
  %7 = call %struct.pqueue_t* @pqueue_init(i64 10, i32 (i64, i64)* @cmp_pri, i64 (i8*)* @get_pri, void (i8*, i64)* @set_pri, i64 (i8*)* @get_pos, void (i8*, i64)* @set_pos), !dbg !71
  store %struct.pqueue_t* %7, %struct.pqueue_t** %2, align 8, !dbg !72
  %8 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !73
  %9 = icmp ne %struct.node_t* %8, null, !dbg !73
  br i1 %9, label %10, label %13, !dbg !75

; <label>:10:                                     ; preds = %0
  %11 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !76
  %12 = icmp ne %struct.pqueue_t* %11, null, !dbg !76
  br i1 %12, label %14, label %13, !dbg !77

; <label>:13:                                     ; preds = %10, %0
  store i32 1, i32* %1, align 4, !dbg !78
  br label %105, !dbg !78

; <label>:14:                                     ; preds = %10
  %15 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !79
  %16 = getelementptr inbounds %struct.node_t, %struct.node_t* %15, i64 0, !dbg !79
  %17 = getelementptr inbounds %struct.node_t, %struct.node_t* %16, i32 0, i32 0, !dbg !80
  store i64 5, i64* %17, align 8, !dbg !81
  %18 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !82
  %19 = getelementptr inbounds %struct.node_t, %struct.node_t* %18, i64 0, !dbg !82
  %20 = getelementptr inbounds %struct.node_t, %struct.node_t* %19, i32 0, i32 1, !dbg !83
  store i32 -5, i32* %20, align 8, !dbg !84
  %21 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !85
  %22 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !86
  %23 = getelementptr inbounds %struct.node_t, %struct.node_t* %22, i64 0, !dbg !86
  %24 = bitcast %struct.node_t* %23 to i8*, !dbg !87
  %25 = call i32 @pqueue_insert(%struct.pqueue_t* %21, i8* %24), !dbg !88
  %26 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !89
  %27 = getelementptr inbounds %struct.node_t, %struct.node_t* %26, i64 1, !dbg !89
  %28 = getelementptr inbounds %struct.node_t, %struct.node_t* %27, i32 0, i32 0, !dbg !90
  store i64 4, i64* %28, align 8, !dbg !91
  %29 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !92
  %30 = getelementptr inbounds %struct.node_t, %struct.node_t* %29, i64 1, !dbg !92
  %31 = getelementptr inbounds %struct.node_t, %struct.node_t* %30, i32 0, i32 1, !dbg !93
  store i32 -4, i32* %31, align 8, !dbg !94
  %32 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !95
  %33 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !96
  %34 = getelementptr inbounds %struct.node_t, %struct.node_t* %33, i64 1, !dbg !96
  %35 = bitcast %struct.node_t* %34 to i8*, !dbg !97
  %36 = call i32 @pqueue_insert(%struct.pqueue_t* %32, i8* %35), !dbg !98
  %37 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !99
  %38 = getelementptr inbounds %struct.node_t, %struct.node_t* %37, i64 2, !dbg !99
  %39 = getelementptr inbounds %struct.node_t, %struct.node_t* %38, i32 0, i32 0, !dbg !100
  store i64 2, i64* %39, align 8, !dbg !101
  %40 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !102
  %41 = getelementptr inbounds %struct.node_t, %struct.node_t* %40, i64 2, !dbg !102
  %42 = getelementptr inbounds %struct.node_t, %struct.node_t* %41, i32 0, i32 1, !dbg !103
  store i32 -2, i32* %42, align 8, !dbg !104
  %43 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !105
  %44 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !106
  %45 = getelementptr inbounds %struct.node_t, %struct.node_t* %44, i64 2, !dbg !106
  %46 = bitcast %struct.node_t* %45 to i8*, !dbg !107
  %47 = call i32 @pqueue_insert(%struct.pqueue_t* %43, i8* %46), !dbg !108
  %48 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !109
  %49 = getelementptr inbounds %struct.node_t, %struct.node_t* %48, i64 3, !dbg !109
  %50 = getelementptr inbounds %struct.node_t, %struct.node_t* %49, i32 0, i32 0, !dbg !110
  store i64 6, i64* %50, align 8, !dbg !111
  %51 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !112
  %52 = getelementptr inbounds %struct.node_t, %struct.node_t* %51, i64 3, !dbg !112
  %53 = getelementptr inbounds %struct.node_t, %struct.node_t* %52, i32 0, i32 1, !dbg !113
  store i32 -6, i32* %53, align 8, !dbg !114
  %54 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !115
  %55 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !116
  %56 = getelementptr inbounds %struct.node_t, %struct.node_t* %55, i64 3, !dbg !116
  %57 = bitcast %struct.node_t* %56 to i8*, !dbg !117
  %58 = call i32 @pqueue_insert(%struct.pqueue_t* %54, i8* %57), !dbg !118
  %59 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !119
  %60 = getelementptr inbounds %struct.node_t, %struct.node_t* %59, i64 4, !dbg !119
  %61 = getelementptr inbounds %struct.node_t, %struct.node_t* %60, i32 0, i32 0, !dbg !120
  store i64 1, i64* %61, align 8, !dbg !121
  %62 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !122
  %63 = getelementptr inbounds %struct.node_t, %struct.node_t* %62, i64 4, !dbg !122
  %64 = getelementptr inbounds %struct.node_t, %struct.node_t* %63, i32 0, i32 1, !dbg !123
  store i32 -1, i32* %64, align 8, !dbg !124
  %65 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !125
  %66 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !126
  %67 = getelementptr inbounds %struct.node_t, %struct.node_t* %66, i64 4, !dbg !126
  %68 = bitcast %struct.node_t* %67 to i8*, !dbg !127
  %69 = call i32 @pqueue_insert(%struct.pqueue_t* %65, i8* %68), !dbg !128
  %70 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !129
  %71 = call i8* @pqueue_peek(%struct.pqueue_t* %70), !dbg !130
  %72 = bitcast i8* %71 to %struct.node_t*, !dbg !130
  store %struct.node_t* %72, %struct.node_t** %4, align 8, !dbg !131
  %73 = load %struct.node_t*, %struct.node_t** %4, align 8, !dbg !132
  %74 = getelementptr inbounds %struct.node_t, %struct.node_t* %73, i32 0, i32 0, !dbg !133
  %75 = load i64, i64* %74, align 8, !dbg !133
  %76 = load %struct.node_t*, %struct.node_t** %4, align 8, !dbg !134
  %77 = getelementptr inbounds %struct.node_t, %struct.node_t* %76, i32 0, i32 1, !dbg !135
  %78 = load i32, i32* %77, align 8, !dbg !135
  %79 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @.str, i32 0, i32 0), i64 %75, i32 %78), !dbg !136
  %80 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !137
  %81 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !138
  %82 = getelementptr inbounds %struct.node_t, %struct.node_t* %81, i64 4, !dbg !138
  %83 = bitcast %struct.node_t* %82 to i8*, !dbg !139
  call void @pqueue_change_priority(%struct.pqueue_t* %80, i64 8, i8* %83), !dbg !140
  %84 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !141
  %85 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !142
  %86 = getelementptr inbounds %struct.node_t, %struct.node_t* %85, i64 2, !dbg !142
  %87 = bitcast %struct.node_t* %86 to i8*, !dbg !143
  call void @pqueue_change_priority(%struct.pqueue_t* %84, i64 7, i8* %87), !dbg !144
  br label %88, !dbg !145

; <label>:88:                                     ; preds = %93, %14
  %89 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !146
  %90 = call i8* @pqueue_pop(%struct.pqueue_t* %89), !dbg !147
  %91 = bitcast i8* %90 to %struct.node_t*, !dbg !147
  store %struct.node_t* %91, %struct.node_t** %4, align 8, !dbg !148
  %92 = icmp ne %struct.node_t* %91, null, !dbg !145
  br i1 %92, label %93, label %101, !dbg !145

; <label>:93:                                     ; preds = %88
  %94 = load %struct.node_t*, %struct.node_t** %4, align 8, !dbg !149
  %95 = getelementptr inbounds %struct.node_t, %struct.node_t* %94, i32 0, i32 0, !dbg !150
  %96 = load i64, i64* %95, align 8, !dbg !150
  %97 = load %struct.node_t*, %struct.node_t** %4, align 8, !dbg !151
  %98 = getelementptr inbounds %struct.node_t, %struct.node_t* %97, i32 0, i32 1, !dbg !152
  %99 = load i32, i32* %98, align 8, !dbg !152
  %100 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([16 x i8], [16 x i8]* @.str.1, i32 0, i32 0), i64 %96, i32 %99), !dbg !153
  br label %88, !dbg !145, !llvm.loop !154

; <label>:101:                                    ; preds = %88
  %102 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !155
  call void @pqueue_free(%struct.pqueue_t* %102), !dbg !156
  %103 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !157
  %104 = bitcast %struct.node_t* %103 to i8*, !dbg !157
  call void @free(i8* %104), !dbg !158
  store i32 0, i32* %1, align 4, !dbg !159
  br label %105, !dbg !159

; <label>:105:                                    ; preds = %101, %13
  %106 = load i32, i32* %1, align 4, !dbg !160
  ret i32 %106, !dbg !160
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare i8* @malloc(i64) #2

declare %struct.pqueue_t* @pqueue_init(i64, i32 (i64, i64)*, i64 (i8*)*, void (i8*, i64)*, i64 (i8*)*, void (i8*, i64)*) #2

; Function Attrs: nounwind ssp uwtable
define internal i32 @cmp_pri(i64, i64) #0 !dbg !161 {
  %3 = alloca i64, align 8
  %4 = alloca i64, align 8
  store i64 %0, i64* %3, align 8
  call void @llvm.dbg.declare(metadata i64* %3, metadata !162, metadata !63), !dbg !163
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !164, metadata !63), !dbg !165
  %5 = load i64, i64* %3, align 8, !dbg !166
  %6 = load i64, i64* %4, align 8, !dbg !167
  %7 = icmp ult i64 %5, %6, !dbg !168
  %8 = zext i1 %7 to i32, !dbg !168
  ret i32 %8, !dbg !169
}

; Function Attrs: nounwind ssp uwtable
define internal i64 @get_pri(i8*) #0 !dbg !170 {
  %2 = alloca i8*, align 8
  store i8* %0, i8** %2, align 8
  call void @llvm.dbg.declare(metadata i8** %2, metadata !171, metadata !63), !dbg !172
  %3 = load i8*, i8** %2, align 8, !dbg !173
  %4 = bitcast i8* %3 to %struct.node_t*, !dbg !174
  %5 = getelementptr inbounds %struct.node_t, %struct.node_t* %4, i32 0, i32 0, !dbg !175
  %6 = load i64, i64* %5, align 8, !dbg !175
  ret i64 %6, !dbg !176
}

; Function Attrs: nounwind ssp uwtable
define internal void @set_pri(i8*, i64) #0 !dbg !177 {
  %3 = alloca i8*, align 8
  %4 = alloca i64, align 8
  store i8* %0, i8** %3, align 8
  call void @llvm.dbg.declare(metadata i8** %3, metadata !178, metadata !63), !dbg !179
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !180, metadata !63), !dbg !181
  %5 = load i64, i64* %4, align 8, !dbg !182
  %6 = load i8*, i8** %3, align 8, !dbg !183
  %7 = bitcast i8* %6 to %struct.node_t*, !dbg !184
  %8 = getelementptr inbounds %struct.node_t, %struct.node_t* %7, i32 0, i32 0, !dbg !185
  store i64 %5, i64* %8, align 8, !dbg !186
  ret void, !dbg !187
}

; Function Attrs: nounwind ssp uwtable
define internal i64 @get_pos(i8*) #0 !dbg !188 {
  %2 = alloca i8*, align 8
  store i8* %0, i8** %2, align 8
  call void @llvm.dbg.declare(metadata i8** %2, metadata !189, metadata !63), !dbg !190
  %3 = load i8*, i8** %2, align 8, !dbg !191
  %4 = bitcast i8* %3 to %struct.node_t*, !dbg !192
  %5 = getelementptr inbounds %struct.node_t, %struct.node_t* %4, i32 0, i32 2, !dbg !193
  %6 = load i64, i64* %5, align 8, !dbg !193
  ret i64 %6, !dbg !194
}

; Function Attrs: nounwind ssp uwtable
define internal void @set_pos(i8*, i64) #0 !dbg !195 {
  %3 = alloca i8*, align 8
  %4 = alloca i64, align 8
  store i8* %0, i8** %3, align 8
  call void @llvm.dbg.declare(metadata i8** %3, metadata !196, metadata !63), !dbg !197
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !198, metadata !63), !dbg !199
  %5 = load i64, i64* %4, align 8, !dbg !200
  %6 = load i8*, i8** %3, align 8, !dbg !201
  %7 = bitcast i8* %6 to %struct.node_t*, !dbg !202
  %8 = getelementptr inbounds %struct.node_t, %struct.node_t* %7, i32 0, i32 2, !dbg !203
  store i64 %5, i64* %8, align 8, !dbg !204
  ret void, !dbg !205
}

declare i32 @pqueue_insert(%struct.pqueue_t*, i8*) #2

declare i8* @pqueue_peek(%struct.pqueue_t*) #2

declare i32 @printf(i8*, ...) #2

declare void @pqueue_change_priority(%struct.pqueue_t*, i64, i8*) #2

declare i8* @pqueue_pop(%struct.pqueue_t*) #2

declare void @pqueue_free(%struct.pqueue_t*) #2

declare void @free(i8*) #2

attributes #0 = { nounwind ssp uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }
attributes #2 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!20, !21, !22}
!llvm.ident = !{!23}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 3.9.1 (tags/RELEASE_391/final)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !3)
!1 = !DIFile(filename: "sample.c", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!2 = !{}
!3 = !{!4}
!4 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !5, size: 64, align: 64)
!5 = !DIDerivedType(tag: DW_TAG_typedef, name: "node_t", file: !1, line: 43, baseType: !6)
!6 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "node_t", file: !1, line: 38, size: 192, align: 64, elements: !7)
!7 = !{!8, !12, !14}
!8 = !DIDerivedType(tag: DW_TAG_member, name: "pri", scope: !6, file: !1, line: 40, baseType: !9, size: 64, align: 64)
!9 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_pri_t", file: !10, line: 39, baseType: !11)
!10 = !DIFile(filename: "./pqueue.h", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!11 = !DIBasicType(name: "long long unsigned int", size: 64, align: 64, encoding: DW_ATE_unsigned)
!12 = !DIDerivedType(tag: DW_TAG_member, name: "val", scope: !6, file: !1, line: 41, baseType: !13, size: 32, align: 32, offset: 64)
!13 = !DIBasicType(name: "int", size: 32, align: 32, encoding: DW_ATE_signed)
!14 = !DIDerivedType(tag: DW_TAG_member, name: "pos", scope: !6, file: !1, line: 42, baseType: !15, size: 64, align: 64, offset: 128)
!15 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !16, line: 31, baseType: !17)
!16 = !DIFile(filename: "/usr/include/sys/_types/_size_t.h", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!17 = !DIDerivedType(tag: DW_TAG_typedef, name: "__darwin_size_t", file: !18, line: 92, baseType: !19)
!18 = !DIFile(filename: "/usr/include/i386/_types.h", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!19 = !DIBasicType(name: "long unsigned int", size: 64, align: 64, encoding: DW_ATE_unsigned)
!20 = !{i32 2, !"Dwarf Version", i32 2}
!21 = !{i32 2, !"Debug Info Version", i32 3}
!22 = !{i32 1, !"PIC Level", i32 2}
!23 = !{!"clang version 3.9.1 (tags/RELEASE_391/final)"}
!24 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 82, type: !25, isLocal: false, isDefinition: true, scopeLine: 83, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!25 = !DISubroutineType(types: !26)
!26 = !{!13}
!27 = !DILocalVariable(name: "pq", scope: !24, file: !1, line: 84, type: !28)
!28 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !29, size: 64, align: 64)
!29 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_t", file: !10, line: 68, baseType: !30)
!30 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "pqueue_t", file: !10, line: 57, size: 576, align: 64, elements: !31)
!31 = !{!32, !33, !34, !35, !40, !46, !51, !56, !61}
!32 = !DIDerivedType(tag: DW_TAG_member, name: "size", scope: !30, file: !10, line: 59, baseType: !15, size: 64, align: 64)
!33 = !DIDerivedType(tag: DW_TAG_member, name: "avail", scope: !30, file: !10, line: 60, baseType: !15, size: 64, align: 64, offset: 64)
!34 = !DIDerivedType(tag: DW_TAG_member, name: "step", scope: !30, file: !10, line: 61, baseType: !15, size: 64, align: 64, offset: 128)
!35 = !DIDerivedType(tag: DW_TAG_member, name: "cmppri", scope: !30, file: !10, line: 62, baseType: !36, size: 64, align: 64, offset: 192)
!36 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_cmp_pri_f", file: !10, line: 44, baseType: !37)
!37 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !38, size: 64, align: 64)
!38 = !DISubroutineType(types: !39)
!39 = !{!13, !9, !9}
!40 = !DIDerivedType(tag: DW_TAG_member, name: "getpri", scope: !30, file: !10, line: 63, baseType: !41, size: 64, align: 64, offset: 256)
!41 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_get_pri_f", file: !10, line: 42, baseType: !42)
!42 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !43, size: 64, align: 64)
!43 = !DISubroutineType(types: !44)
!44 = !{!9, !45}
!45 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64, align: 64)
!46 = !DIDerivedType(tag: DW_TAG_member, name: "setpri", scope: !30, file: !10, line: 64, baseType: !47, size: 64, align: 64, offset: 320)
!47 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_set_pri_f", file: !10, line: 43, baseType: !48)
!48 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !49, size: 64, align: 64)
!49 = !DISubroutineType(types: !50)
!50 = !{null, !45, !9}
!51 = !DIDerivedType(tag: DW_TAG_member, name: "getpos", scope: !30, file: !10, line: 65, baseType: !52, size: 64, align: 64, offset: 384)
!52 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_get_pos_f", file: !10, line: 48, baseType: !53)
!53 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !54, size: 64, align: 64)
!54 = !DISubroutineType(types: !55)
!55 = !{!15, !45}
!56 = !DIDerivedType(tag: DW_TAG_member, name: "setpos", scope: !30, file: !10, line: 66, baseType: !57, size: 64, align: 64, offset: 448)
!57 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_set_pos_f", file: !10, line: 49, baseType: !58)
!58 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !59, size: 64, align: 64)
!59 = !DISubroutineType(types: !60)
!60 = !{null, !45, !15}
!61 = !DIDerivedType(tag: DW_TAG_member, name: "d", scope: !30, file: !10, line: 67, baseType: !62, size: 64, align: 64, offset: 512)
!62 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !45, size: 64, align: 64)
!63 = !DIExpression()
!64 = !DILocation(line: 84, column: 12, scope: !24)
!65 = !DILocalVariable(name: "ns", scope: !24, file: !1, line: 85, type: !4)
!66 = !DILocation(line: 85, column: 12, scope: !24)
!67 = !DILocalVariable(name: "n", scope: !24, file: !1, line: 86, type: !4)
!68 = !DILocation(line: 86, column: 12, scope: !24)
!69 = !DILocation(line: 88, column: 7, scope: !24)
!70 = !DILocation(line: 88, column: 5, scope: !24)
!71 = !DILocation(line: 89, column: 7, scope: !24)
!72 = !DILocation(line: 89, column: 5, scope: !24)
!73 = !DILocation(line: 90, column: 8, scope: !74)
!74 = distinct !DILexicalBlock(scope: !24, file: !1, line: 90, column: 6)
!75 = !DILocation(line: 90, column: 11, scope: !74)
!76 = !DILocation(line: 90, column: 14, scope: !74)
!77 = !DILocation(line: 90, column: 6, scope: !24)
!78 = !DILocation(line: 90, column: 19, scope: !74)
!79 = !DILocation(line: 92, column: 2, scope: !24)
!80 = !DILocation(line: 92, column: 8, scope: !24)
!81 = !DILocation(line: 92, column: 12, scope: !24)
!82 = !DILocation(line: 92, column: 17, scope: !24)
!83 = !DILocation(line: 92, column: 23, scope: !24)
!84 = !DILocation(line: 92, column: 27, scope: !24)
!85 = !DILocation(line: 92, column: 47, scope: !24)
!86 = !DILocation(line: 92, column: 52, scope: !24)
!87 = !DILocation(line: 92, column: 51, scope: !24)
!88 = !DILocation(line: 92, column: 33, scope: !24)
!89 = !DILocation(line: 93, column: 2, scope: !24)
!90 = !DILocation(line: 93, column: 8, scope: !24)
!91 = !DILocation(line: 93, column: 12, scope: !24)
!92 = !DILocation(line: 93, column: 17, scope: !24)
!93 = !DILocation(line: 93, column: 23, scope: !24)
!94 = !DILocation(line: 93, column: 27, scope: !24)
!95 = !DILocation(line: 93, column: 47, scope: !24)
!96 = !DILocation(line: 93, column: 52, scope: !24)
!97 = !DILocation(line: 93, column: 51, scope: !24)
!98 = !DILocation(line: 93, column: 33, scope: !24)
!99 = !DILocation(line: 94, column: 2, scope: !24)
!100 = !DILocation(line: 94, column: 8, scope: !24)
!101 = !DILocation(line: 94, column: 12, scope: !24)
!102 = !DILocation(line: 94, column: 17, scope: !24)
!103 = !DILocation(line: 94, column: 23, scope: !24)
!104 = !DILocation(line: 94, column: 27, scope: !24)
!105 = !DILocation(line: 94, column: 47, scope: !24)
!106 = !DILocation(line: 94, column: 52, scope: !24)
!107 = !DILocation(line: 94, column: 51, scope: !24)
!108 = !DILocation(line: 94, column: 33, scope: !24)
!109 = !DILocation(line: 95, column: 2, scope: !24)
!110 = !DILocation(line: 95, column: 8, scope: !24)
!111 = !DILocation(line: 95, column: 12, scope: !24)
!112 = !DILocation(line: 95, column: 17, scope: !24)
!113 = !DILocation(line: 95, column: 23, scope: !24)
!114 = !DILocation(line: 95, column: 27, scope: !24)
!115 = !DILocation(line: 95, column: 47, scope: !24)
!116 = !DILocation(line: 95, column: 52, scope: !24)
!117 = !DILocation(line: 95, column: 51, scope: !24)
!118 = !DILocation(line: 95, column: 33, scope: !24)
!119 = !DILocation(line: 96, column: 2, scope: !24)
!120 = !DILocation(line: 96, column: 8, scope: !24)
!121 = !DILocation(line: 96, column: 12, scope: !24)
!122 = !DILocation(line: 96, column: 17, scope: !24)
!123 = !DILocation(line: 96, column: 23, scope: !24)
!124 = !DILocation(line: 96, column: 27, scope: !24)
!125 = !DILocation(line: 96, column: 47, scope: !24)
!126 = !DILocation(line: 96, column: 52, scope: !24)
!127 = !DILocation(line: 96, column: 51, scope: !24)
!128 = !DILocation(line: 96, column: 33, scope: !24)
!129 = !DILocation(line: 98, column: 18, scope: !24)
!130 = !DILocation(line: 98, column: 6, scope: !24)
!131 = !DILocation(line: 98, column: 4, scope: !24)
!132 = !DILocation(line: 99, column: 30, scope: !24)
!133 = !DILocation(line: 99, column: 33, scope: !24)
!134 = !DILocation(line: 99, column: 38, scope: !24)
!135 = !DILocation(line: 99, column: 41, scope: !24)
!136 = !DILocation(line: 99, column: 2, scope: !24)
!137 = !DILocation(line: 101, column: 25, scope: !24)
!138 = !DILocation(line: 101, column: 33, scope: !24)
!139 = !DILocation(line: 101, column: 32, scope: !24)
!140 = !DILocation(line: 101, column: 2, scope: !24)
!141 = !DILocation(line: 102, column: 25, scope: !24)
!142 = !DILocation(line: 102, column: 33, scope: !24)
!143 = !DILocation(line: 102, column: 32, scope: !24)
!144 = !DILocation(line: 102, column: 2, scope: !24)
!145 = !DILocation(line: 104, column: 5, scope: !24)
!146 = !DILocation(line: 104, column: 28, scope: !24)
!147 = !DILocation(line: 104, column: 17, scope: !24)
!148 = !DILocation(line: 104, column: 15, scope: !24)
!149 = !DILocation(line: 105, column: 30, scope: !24)
!150 = !DILocation(line: 105, column: 33, scope: !24)
!151 = !DILocation(line: 105, column: 38, scope: !24)
!152 = !DILocation(line: 105, column: 41, scope: !24)
!153 = !DILocation(line: 105, column: 3, scope: !24)
!154 = distinct !{!154, !145}
!155 = !DILocation(line: 107, column: 14, scope: !24)
!156 = !DILocation(line: 107, column: 2, scope: !24)
!157 = !DILocation(line: 108, column: 7, scope: !24)
!158 = !DILocation(line: 108, column: 2, scope: !24)
!159 = !DILocation(line: 110, column: 2, scope: !24)
!160 = !DILocation(line: 111, column: 1, scope: !24)
!161 = distinct !DISubprogram(name: "cmp_pri", scope: !1, file: !1, line: 47, type: !38, isLocal: true, isDefinition: true, scopeLine: 48, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!162 = !DILocalVariable(name: "next", arg: 1, scope: !161, file: !1, line: 47, type: !9)
!163 = !DILocation(line: 47, column: 22, scope: !161)
!164 = !DILocalVariable(name: "curr", arg: 2, scope: !161, file: !1, line: 47, type: !9)
!165 = !DILocation(line: 47, column: 41, scope: !161)
!166 = !DILocation(line: 49, column: 10, scope: !161)
!167 = !DILocation(line: 49, column: 17, scope: !161)
!168 = !DILocation(line: 49, column: 15, scope: !161)
!169 = !DILocation(line: 49, column: 2, scope: !161)
!170 = distinct !DISubprogram(name: "get_pri", scope: !1, file: !1, line: 54, type: !43, isLocal: true, isDefinition: true, scopeLine: 55, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!171 = !DILocalVariable(name: "a", arg: 1, scope: !170, file: !1, line: 54, type: !45)
!172 = !DILocation(line: 54, column: 15, scope: !170)
!173 = !DILocation(line: 56, column: 21, scope: !170)
!174 = !DILocation(line: 56, column: 10, scope: !170)
!175 = !DILocation(line: 56, column: 25, scope: !170)
!176 = !DILocation(line: 56, column: 2, scope: !170)
!177 = distinct !DISubprogram(name: "set_pri", scope: !1, file: !1, line: 61, type: !49, isLocal: true, isDefinition: true, scopeLine: 62, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!178 = !DILocalVariable(name: "a", arg: 1, scope: !177, file: !1, line: 61, type: !45)
!179 = !DILocation(line: 61, column: 15, scope: !177)
!180 = !DILocalVariable(name: "pri", arg: 2, scope: !177, file: !1, line: 61, type: !9)
!181 = !DILocation(line: 61, column: 31, scope: !177)
!182 = !DILocation(line: 63, column: 24, scope: !177)
!183 = !DILocation(line: 63, column: 14, scope: !177)
!184 = !DILocation(line: 63, column: 3, scope: !177)
!185 = !DILocation(line: 63, column: 18, scope: !177)
!186 = !DILocation(line: 63, column: 22, scope: !177)
!187 = !DILocation(line: 64, column: 1, scope: !177)
!188 = distinct !DISubprogram(name: "get_pos", scope: !1, file: !1, line: 68, type: !54, isLocal: true, isDefinition: true, scopeLine: 69, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!189 = !DILocalVariable(name: "a", arg: 1, scope: !188, file: !1, line: 68, type: !45)
!190 = !DILocation(line: 68, column: 15, scope: !188)
!191 = !DILocation(line: 70, column: 21, scope: !188)
!192 = !DILocation(line: 70, column: 10, scope: !188)
!193 = !DILocation(line: 70, column: 25, scope: !188)
!194 = !DILocation(line: 70, column: 2, scope: !188)
!195 = distinct !DISubprogram(name: "set_pos", scope: !1, file: !1, line: 75, type: !59, isLocal: true, isDefinition: true, scopeLine: 76, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!196 = !DILocalVariable(name: "a", arg: 1, scope: !195, file: !1, line: 75, type: !45)
!197 = !DILocation(line: 75, column: 15, scope: !195)
!198 = !DILocalVariable(name: "pos", arg: 2, scope: !195, file: !1, line: 75, type: !15)
!199 = !DILocation(line: 75, column: 25, scope: !195)
!200 = !DILocation(line: 77, column: 24, scope: !195)
!201 = !DILocation(line: 77, column: 14, scope: !195)
!202 = !DILocation(line: 77, column: 3, scope: !195)
!203 = !DILocation(line: 77, column: 18, scope: !195)
!204 = !DILocation(line: 77, column: 22, scope: !195)
!205 = !DILocation(line: 78, column: 1, scope: !195)

; ModuleID = 'pqueue.c'
source_filename = "pqueue.c"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.13.0"

%struct.__sFILE = type { i8*, i32, i32, i16, i16, %struct.__sbuf, i32, i8*, i32 (i8*)*, i32 (i8*, i8*, i32)*, i64 (i8*, i64, i32)*, i32 (i8*, i8*, i32)*, %struct.__sbuf, %struct.__sFILEX*, i32, [3 x i8], [1 x i8], %struct.__sbuf, i32, i64 }
%struct.__sFILEX = type opaque
%struct.__sbuf = type { i8*, i32 }
%struct.pqueue_t = type { i64, i64, i64, i32 (i64, i64)*, i64 (i8*)*, void (i8*, i64)*, i64 (i8*)*, void (i8*, i64)*, i8** }

@__stdoutp = external global %struct.__sFILE*, align 8
@.str = private unnamed_addr constant [37 x i8] c"posn\09left\09right\09parent\09maxchild\09...\0A\00", align 1
@.str.1 = private unnamed_addr constant [17 x i8] c"%d\09%d\09%d\09%d\09%ul\09\00", align 1

; Function Attrs: nounwind ssp uwtable
define %struct.pqueue_t* @pqueue_init(i64, i32 (i64, i64)*, i64 (i8*)*, void (i8*, i64)*, i64 (i8*)*, void (i8*, i64)*) #0 !dbg !10 {
  %7 = alloca %struct.pqueue_t*, align 8
  %8 = alloca i64, align 8
  %9 = alloca i32 (i64, i64)*, align 8
  %10 = alloca i64 (i8*)*, align 8
  %11 = alloca void (i8*, i64)*, align 8
  %12 = alloca i64 (i8*)*, align 8
  %13 = alloca void (i8*, i64)*, align 8
  %14 = alloca %struct.pqueue_t*, align 8
  store i64 %0, i64* %8, align 8
  call void @llvm.dbg.declare(metadata i64* %8, metadata !56, metadata !57), !dbg !58
  store i32 (i64, i64)* %1, i32 (i64, i64)** %9, align 8
  call void @llvm.dbg.declare(metadata i32 (i64, i64)** %9, metadata !59, metadata !57), !dbg !60
  store i64 (i8*)* %2, i64 (i8*)** %10, align 8
  call void @llvm.dbg.declare(metadata i64 (i8*)** %10, metadata !61, metadata !57), !dbg !62
  store void (i8*, i64)* %3, void (i8*, i64)** %11, align 8
  call void @llvm.dbg.declare(metadata void (i8*, i64)** %11, metadata !63, metadata !57), !dbg !64
  store i64 (i8*)* %4, i64 (i8*)** %12, align 8
  call void @llvm.dbg.declare(metadata i64 (i8*)** %12, metadata !65, metadata !57), !dbg !66
  store void (i8*, i64)* %5, void (i8*, i64)** %13, align 8
  call void @llvm.dbg.declare(metadata void (i8*, i64)** %13, metadata !67, metadata !57), !dbg !68
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %14, metadata !69, metadata !57), !dbg !70
  %15 = call i8* @malloc(i64 72), !dbg !71
  %16 = bitcast i8* %15 to %struct.pqueue_t*, !dbg !71
  store %struct.pqueue_t* %16, %struct.pqueue_t** %14, align 8, !dbg !73
  %17 = icmp ne %struct.pqueue_t* %16, null, !dbg !73
  br i1 %17, label %19, label %18, !dbg !74

; <label>:18:                                     ; preds = %6
  store %struct.pqueue_t* null, %struct.pqueue_t** %7, align 8, !dbg !75
  br label %56, !dbg !75

; <label>:19:                                     ; preds = %6
  %20 = load i64, i64* %8, align 8, !dbg !76
  %21 = add i64 %20, 1, !dbg !78
  %22 = mul i64 %21, 8, !dbg !79
  %23 = call i8* @malloc(i64 %22), !dbg !80
  %24 = bitcast i8* %23 to i8**, !dbg !80
  %25 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !81
  %26 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %25, i32 0, i32 8, !dbg !82
  store i8** %24, i8*** %26, align 8, !dbg !83
  %27 = icmp ne i8** %24, null, !dbg !83
  br i1 %27, label %31, label %28, !dbg !84

; <label>:28:                                     ; preds = %19
  %29 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !85
  %30 = bitcast %struct.pqueue_t* %29 to i8*, !dbg !85
  call void @free(i8* %30), !dbg !87
  store %struct.pqueue_t* null, %struct.pqueue_t** %7, align 8, !dbg !88
  br label %56, !dbg !88

; <label>:31:                                     ; preds = %19
  %32 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !89
  %33 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %32, i32 0, i32 0, !dbg !90
  store i64 1, i64* %33, align 8, !dbg !91
  %34 = load i64, i64* %8, align 8, !dbg !92
  %35 = add i64 %34, 1, !dbg !93
  %36 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !94
  %37 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %36, i32 0, i32 2, !dbg !95
  store i64 %35, i64* %37, align 8, !dbg !96
  %38 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !97
  %39 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %38, i32 0, i32 1, !dbg !98
  store i64 %35, i64* %39, align 8, !dbg !99
  %40 = load i32 (i64, i64)*, i32 (i64, i64)** %9, align 8, !dbg !100
  %41 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !101
  %42 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %41, i32 0, i32 3, !dbg !102
  store i32 (i64, i64)* %40, i32 (i64, i64)** %42, align 8, !dbg !103
  %43 = load void (i8*, i64)*, void (i8*, i64)** %11, align 8, !dbg !104
  %44 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !105
  %45 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %44, i32 0, i32 5, !dbg !106
  store void (i8*, i64)* %43, void (i8*, i64)** %45, align 8, !dbg !107
  %46 = load i64 (i8*)*, i64 (i8*)** %10, align 8, !dbg !108
  %47 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !109
  %48 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %47, i32 0, i32 4, !dbg !110
  store i64 (i8*)* %46, i64 (i8*)** %48, align 8, !dbg !111
  %49 = load i64 (i8*)*, i64 (i8*)** %12, align 8, !dbg !112
  %50 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !113
  %51 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %50, i32 0, i32 6, !dbg !114
  store i64 (i8*)* %49, i64 (i8*)** %51, align 8, !dbg !115
  %52 = load void (i8*, i64)*, void (i8*, i64)** %13, align 8, !dbg !116
  %53 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !117
  %54 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %53, i32 0, i32 7, !dbg !118
  store void (i8*, i64)* %52, void (i8*, i64)** %54, align 8, !dbg !119
  %55 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !120
  store %struct.pqueue_t* %55, %struct.pqueue_t** %7, align 8, !dbg !121
  br label %56, !dbg !121

; <label>:56:                                     ; preds = %31, %28, %18
  %57 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !122
  ret %struct.pqueue_t* %57, !dbg !122
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare i8* @malloc(i64) #2

declare void @free(i8*) #2

; Function Attrs: nounwind ssp uwtable
define void @pqueue_free(%struct.pqueue_t*) #0 !dbg !123 {
  %2 = alloca %struct.pqueue_t*, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %2, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %2, metadata !126, metadata !57), !dbg !127
  %3 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !128
  %4 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %3, i32 0, i32 8, !dbg !129
  %5 = load i8**, i8*** %4, align 8, !dbg !129
  %6 = bitcast i8** %5 to i8*, !dbg !128
  call void @free(i8* %6), !dbg !130
  %7 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !131
  %8 = bitcast %struct.pqueue_t* %7 to i8*, !dbg !131
  call void @free(i8* %8), !dbg !132
  ret void, !dbg !133
}

; Function Attrs: nounwind ssp uwtable
define i64 @pqueue_size(%struct.pqueue_t*) #0 !dbg !134 {
  %2 = alloca %struct.pqueue_t*, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %2, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %2, metadata !137, metadata !57), !dbg !138
  %3 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !139
  %4 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %3, i32 0, i32 0, !dbg !140
  %5 = load i64, i64* %4, align 8, !dbg !140
  %6 = sub i64 %5, 1, !dbg !141
  ret i64 %6, !dbg !142
}

; Function Attrs: nounwind ssp uwtable
define i32 @pqueue_insert(%struct.pqueue_t*, i8*) #0 !dbg !143 {
  %3 = alloca i32, align 4
  %4 = alloca %struct.pqueue_t*, align 8
  %5 = alloca i8*, align 8
  %6 = alloca i8*, align 8
  %7 = alloca i64, align 8
  %8 = alloca i64, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %4, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %4, metadata !146, metadata !57), !dbg !147
  store i8* %1, i8** %5, align 8
  call void @llvm.dbg.declare(metadata i8** %5, metadata !148, metadata !57), !dbg !149
  call void @llvm.dbg.declare(metadata i8** %6, metadata !150, metadata !57), !dbg !151
  call void @llvm.dbg.declare(metadata i64* %7, metadata !152, metadata !57), !dbg !153
  call void @llvm.dbg.declare(metadata i64* %8, metadata !154, metadata !57), !dbg !155
  %9 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !156
  %10 = icmp ne %struct.pqueue_t* %9, null, !dbg !156
  br i1 %10, label %12, label %11, !dbg !158

; <label>:11:                                     ; preds = %2
  store i32 1, i32* %3, align 4, !dbg !159
  br label %58, !dbg !159

; <label>:12:                                     ; preds = %2
  %13 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !160
  %14 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %13, i32 0, i32 0, !dbg !162
  %15 = load i64, i64* %14, align 8, !dbg !162
  %16 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !163
  %17 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %16, i32 0, i32 1, !dbg !164
  %18 = load i64, i64* %17, align 8, !dbg !164
  %19 = icmp uge i64 %15, %18, !dbg !165
  br i1 %19, label %20, label %45, !dbg !166

; <label>:20:                                     ; preds = %12
  %21 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !167
  %22 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %21, i32 0, i32 0, !dbg !169
  %23 = load i64, i64* %22, align 8, !dbg !169
  %24 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !170
  %25 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %24, i32 0, i32 2, !dbg !171
  %26 = load i64, i64* %25, align 8, !dbg !171
  %27 = add i64 %23, %26, !dbg !172
  store i64 %27, i64* %8, align 8, !dbg !173
  %28 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !174
  %29 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %28, i32 0, i32 8, !dbg !176
  %30 = load i8**, i8*** %29, align 8, !dbg !176
  %31 = bitcast i8** %30 to i8*, !dbg !174
  %32 = load i64, i64* %8, align 8, !dbg !177
  %33 = mul i64 8, %32, !dbg !178
  %34 = call i8* @realloc(i8* %31, i64 %33), !dbg !179
  store i8* %34, i8** %6, align 8, !dbg !180
  %35 = icmp ne i8* %34, null, !dbg !180
  br i1 %35, label %37, label %36, !dbg !181

; <label>:36:                                     ; preds = %20
  store i32 1, i32* %3, align 4, !dbg !182
  br label %58, !dbg !182

; <label>:37:                                     ; preds = %20
  %38 = load i8*, i8** %6, align 8, !dbg !183
  %39 = bitcast i8* %38 to i8**, !dbg !183
  %40 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !184
  %41 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %40, i32 0, i32 8, !dbg !185
  store i8** %39, i8*** %41, align 8, !dbg !186
  %42 = load i64, i64* %8, align 8, !dbg !187
  %43 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !188
  %44 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %43, i32 0, i32 1, !dbg !189
  store i64 %42, i64* %44, align 8, !dbg !190
  br label %45, !dbg !191

; <label>:45:                                     ; preds = %37, %12
  %46 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !192
  %47 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %46, i32 0, i32 0, !dbg !193
  %48 = load i64, i64* %47, align 8, !dbg !194
  %49 = add i64 %48, 1, !dbg !194
  store i64 %49, i64* %47, align 8, !dbg !194
  store i64 %48, i64* %7, align 8, !dbg !195
  %50 = load i8*, i8** %5, align 8, !dbg !196
  %51 = load i64, i64* %7, align 8, !dbg !197
  %52 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !198
  %53 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %52, i32 0, i32 8, !dbg !199
  %54 = load i8**, i8*** %53, align 8, !dbg !199
  %55 = getelementptr inbounds i8*, i8** %54, i64 %51, !dbg !198
  store i8* %50, i8** %55, align 8, !dbg !200
  %56 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !201
  %57 = load i64, i64* %7, align 8, !dbg !202
  call void @bubble_up(%struct.pqueue_t* %56, i64 %57), !dbg !203
  store i32 0, i32* %3, align 4, !dbg !204
  br label %58, !dbg !204

; <label>:58:                                     ; preds = %45, %36, %11
  %59 = load i32, i32* %3, align 4, !dbg !205
  ret i32 %59, !dbg !205
}

declare i8* @realloc(i8*, i64) #2

; Function Attrs: nounwind ssp uwtable
define internal void @bubble_up(%struct.pqueue_t*, i64) #0 !dbg !206 {
  %3 = alloca %struct.pqueue_t*, align 8
  %4 = alloca i64, align 8
  %5 = alloca i64, align 8
  %6 = alloca i8*, align 8
  %7 = alloca i64, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %3, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %3, metadata !209, metadata !57), !dbg !210
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !211, metadata !57), !dbg !212
  call void @llvm.dbg.declare(metadata i64* %5, metadata !213, metadata !57), !dbg !214
  call void @llvm.dbg.declare(metadata i8** %6, metadata !215, metadata !57), !dbg !216
  %8 = load i64, i64* %4, align 8, !dbg !217
  %9 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !218
  %10 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %9, i32 0, i32 8, !dbg !219
  %11 = load i8**, i8*** %10, align 8, !dbg !219
  %12 = getelementptr inbounds i8*, i8** %11, i64 %8, !dbg !218
  %13 = load i8*, i8** %12, align 8, !dbg !218
  store i8* %13, i8** %6, align 8, !dbg !216
  call void @llvm.dbg.declare(metadata i64* %7, metadata !220, metadata !57), !dbg !221
  %14 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !222
  %15 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %14, i32 0, i32 4, !dbg !223
  %16 = load i64 (i8*)*, i64 (i8*)** %15, align 8, !dbg !223
  %17 = load i8*, i8** %6, align 8, !dbg !224
  %18 = call i64 %16(i8* %17), !dbg !222
  store i64 %18, i64* %7, align 8, !dbg !221
  %19 = load i64, i64* %4, align 8, !dbg !225
  %20 = lshr i64 %19, 1, !dbg !225
  store i64 %20, i64* %5, align 8, !dbg !227
  br label %21, !dbg !228

; <label>:21:                                     ; preds = %65, %2
  %22 = load i64, i64* %4, align 8, !dbg !229
  %23 = icmp ugt i64 %22, 1, !dbg !231
  br i1 %23, label %24, label %41, !dbg !232

; <label>:24:                                     ; preds = %21
  %25 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !233
  %26 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %25, i32 0, i32 3, !dbg !234
  %27 = load i32 (i64, i64)*, i32 (i64, i64)** %26, align 8, !dbg !234
  %28 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !235
  %29 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %28, i32 0, i32 4, !dbg !236
  %30 = load i64 (i8*)*, i64 (i8*)** %29, align 8, !dbg !236
  %31 = load i64, i64* %5, align 8, !dbg !237
  %32 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !238
  %33 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %32, i32 0, i32 8, !dbg !239
  %34 = load i8**, i8*** %33, align 8, !dbg !239
  %35 = getelementptr inbounds i8*, i8** %34, i64 %31, !dbg !238
  %36 = load i8*, i8** %35, align 8, !dbg !238
  %37 = call i64 %30(i8* %36), !dbg !235
  %38 = load i64, i64* %7, align 8, !dbg !240
  %39 = call i32 %27(i64 %37, i64 %38), !dbg !233
  %40 = icmp ne i32 %39, 0, !dbg !232
  br label %41

; <label>:41:                                     ; preds = %24, %21
  %42 = phi i1 [ false, %21 ], [ %40, %24 ]
  br i1 %42, label %43, label %69, !dbg !241

; <label>:43:                                     ; preds = %41
  %44 = load i64, i64* %5, align 8, !dbg !242
  %45 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !244
  %46 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %45, i32 0, i32 8, !dbg !245
  %47 = load i8**, i8*** %46, align 8, !dbg !245
  %48 = getelementptr inbounds i8*, i8** %47, i64 %44, !dbg !244
  %49 = load i8*, i8** %48, align 8, !dbg !244
  %50 = load i64, i64* %4, align 8, !dbg !246
  %51 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !247
  %52 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %51, i32 0, i32 8, !dbg !248
  %53 = load i8**, i8*** %52, align 8, !dbg !248
  %54 = getelementptr inbounds i8*, i8** %53, i64 %50, !dbg !247
  store i8* %49, i8** %54, align 8, !dbg !249
  %55 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !250
  %56 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %55, i32 0, i32 7, !dbg !251
  %57 = load void (i8*, i64)*, void (i8*, i64)** %56, align 8, !dbg !251
  %58 = load i64, i64* %4, align 8, !dbg !252
  %59 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !253
  %60 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %59, i32 0, i32 8, !dbg !254
  %61 = load i8**, i8*** %60, align 8, !dbg !254
  %62 = getelementptr inbounds i8*, i8** %61, i64 %58, !dbg !253
  %63 = load i8*, i8** %62, align 8, !dbg !253
  %64 = load i64, i64* %4, align 8, !dbg !255
  call void %57(i8* %63, i64 %64), !dbg !250
  br label %65, !dbg !256

; <label>:65:                                     ; preds = %43
  %66 = load i64, i64* %5, align 8, !dbg !257
  store i64 %66, i64* %4, align 8, !dbg !258
  %67 = load i64, i64* %4, align 8, !dbg !259
  %68 = lshr i64 %67, 1, !dbg !259
  store i64 %68, i64* %5, align 8, !dbg !260
  br label %21, !dbg !261, !llvm.loop !262

; <label>:69:                                     ; preds = %41
  %70 = load i8*, i8** %6, align 8, !dbg !264
  %71 = load i64, i64* %4, align 8, !dbg !265
  %72 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !266
  %73 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %72, i32 0, i32 8, !dbg !267
  %74 = load i8**, i8*** %73, align 8, !dbg !267
  %75 = getelementptr inbounds i8*, i8** %74, i64 %71, !dbg !266
  store i8* %70, i8** %75, align 8, !dbg !268
  %76 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !269
  %77 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %76, i32 0, i32 7, !dbg !270
  %78 = load void (i8*, i64)*, void (i8*, i64)** %77, align 8, !dbg !270
  %79 = load i8*, i8** %6, align 8, !dbg !271
  %80 = load i64, i64* %4, align 8, !dbg !272
  call void %78(i8* %79, i64 %80), !dbg !269
  ret void, !dbg !273
}

; Function Attrs: nounwind ssp uwtable
define void @pqueue_change_priority(%struct.pqueue_t*, i64, i8*) #0 !dbg !274 {
  %4 = alloca %struct.pqueue_t*, align 8
  %5 = alloca i64, align 8
  %6 = alloca i8*, align 8
  %7 = alloca i64, align 8
  %8 = alloca i64, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %4, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %4, metadata !277, metadata !57), !dbg !278
  store i64 %1, i64* %5, align 8
  call void @llvm.dbg.declare(metadata i64* %5, metadata !279, metadata !57), !dbg !280
  store i8* %2, i8** %6, align 8
  call void @llvm.dbg.declare(metadata i8** %6, metadata !281, metadata !57), !dbg !282
  call void @llvm.dbg.declare(metadata i64* %7, metadata !283, metadata !57), !dbg !284
  call void @llvm.dbg.declare(metadata i64* %8, metadata !285, metadata !57), !dbg !286
  %9 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !287
  %10 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %9, i32 0, i32 4, !dbg !288
  %11 = load i64 (i8*)*, i64 (i8*)** %10, align 8, !dbg !288
  %12 = load i8*, i8** %6, align 8, !dbg !289
  %13 = call i64 %11(i8* %12), !dbg !287
  store i64 %13, i64* %8, align 8, !dbg !286
  %14 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !290
  %15 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %14, i32 0, i32 5, !dbg !291
  %16 = load void (i8*, i64)*, void (i8*, i64)** %15, align 8, !dbg !291
  %17 = load i8*, i8** %6, align 8, !dbg !292
  %18 = load i64, i64* %5, align 8, !dbg !293
  call void %16(i8* %17, i64 %18), !dbg !290
  %19 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !294
  %20 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %19, i32 0, i32 6, !dbg !295
  %21 = load i64 (i8*)*, i64 (i8*)** %20, align 8, !dbg !295
  %22 = load i8*, i8** %6, align 8, !dbg !296
  %23 = call i64 %21(i8* %22), !dbg !294
  store i64 %23, i64* %7, align 8, !dbg !297
  %24 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !298
  %25 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %24, i32 0, i32 3, !dbg !300
  %26 = load i32 (i64, i64)*, i32 (i64, i64)** %25, align 8, !dbg !300
  %27 = load i64, i64* %8, align 8, !dbg !301
  %28 = load i64, i64* %5, align 8, !dbg !302
  %29 = call i32 %26(i64 %27, i64 %28), !dbg !298
  %30 = icmp ne i32 %29, 0, !dbg !298
  br i1 %30, label %31, label %34, !dbg !303

; <label>:31:                                     ; preds = %3
  %32 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !304
  %33 = load i64, i64* %7, align 8, !dbg !305
  call void @bubble_up(%struct.pqueue_t* %32, i64 %33), !dbg !306
  br label %37, !dbg !306

; <label>:34:                                     ; preds = %3
  %35 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !307
  %36 = load i64, i64* %7, align 8, !dbg !308
  call void @percolate_down(%struct.pqueue_t* %35, i64 %36), !dbg !309
  br label %37

; <label>:37:                                     ; preds = %34, %31
  ret void, !dbg !310
}

; Function Attrs: nounwind ssp uwtable
define internal void @percolate_down(%struct.pqueue_t*, i64) #0 !dbg !311 {
  %3 = alloca %struct.pqueue_t*, align 8
  %4 = alloca i64, align 8
  %5 = alloca i64, align 8
  %6 = alloca i8*, align 8
  %7 = alloca i64, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %3, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %3, metadata !312, metadata !57), !dbg !313
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !314, metadata !57), !dbg !315
  call void @llvm.dbg.declare(metadata i64* %5, metadata !316, metadata !57), !dbg !317
  call void @llvm.dbg.declare(metadata i8** %6, metadata !318, metadata !57), !dbg !319
  %8 = load i64, i64* %4, align 8, !dbg !320
  %9 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !321
  %10 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %9, i32 0, i32 8, !dbg !322
  %11 = load i8**, i8*** %10, align 8, !dbg !322
  %12 = getelementptr inbounds i8*, i8** %11, i64 %8, !dbg !321
  %13 = load i8*, i8** %12, align 8, !dbg !321
  store i8* %13, i8** %6, align 8, !dbg !319
  call void @llvm.dbg.declare(metadata i64* %7, metadata !323, metadata !57), !dbg !324
  %14 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !325
  %15 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %14, i32 0, i32 4, !dbg !326
  %16 = load i64 (i8*)*, i64 (i8*)** %15, align 8, !dbg !326
  %17 = load i8*, i8** %6, align 8, !dbg !327
  %18 = call i64 %16(i8* %17), !dbg !325
  store i64 %18, i64* %7, align 8, !dbg !324
  br label %19, !dbg !328

; <label>:19:                                     ; preds = %43, %2
  %20 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !329
  %21 = load i64, i64* %4, align 8, !dbg !330
  %22 = call i64 @maxchild(%struct.pqueue_t* %20, i64 %21), !dbg !331
  store i64 %22, i64* %5, align 8, !dbg !332
  %23 = icmp ne i64 %22, 0, !dbg !332
  br i1 %23, label %24, label %41, !dbg !333

; <label>:24:                                     ; preds = %19
  %25 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !334
  %26 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %25, i32 0, i32 3, !dbg !335
  %27 = load i32 (i64, i64)*, i32 (i64, i64)** %26, align 8, !dbg !335
  %28 = load i64, i64* %7, align 8, !dbg !336
  %29 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !337
  %30 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %29, i32 0, i32 4, !dbg !338
  %31 = load i64 (i8*)*, i64 (i8*)** %30, align 8, !dbg !338
  %32 = load i64, i64* %5, align 8, !dbg !339
  %33 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !340
  %34 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %33, i32 0, i32 8, !dbg !341
  %35 = load i8**, i8*** %34, align 8, !dbg !341
  %36 = getelementptr inbounds i8*, i8** %35, i64 %32, !dbg !340
  %37 = load i8*, i8** %36, align 8, !dbg !340
  %38 = call i64 %31(i8* %37), !dbg !337
  %39 = call i32 %27(i64 %28, i64 %38), !dbg !334
  %40 = icmp ne i32 %39, 0, !dbg !333
  br label %41

; <label>:41:                                     ; preds = %24, %19
  %42 = phi i1 [ false, %19 ], [ %40, %24 ]
  br i1 %42, label %43, label %66, !dbg !328

; <label>:43:                                     ; preds = %41
  %44 = load i64, i64* %5, align 8, !dbg !342
  %45 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !344
  %46 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %45, i32 0, i32 8, !dbg !345
  %47 = load i8**, i8*** %46, align 8, !dbg !345
  %48 = getelementptr inbounds i8*, i8** %47, i64 %44, !dbg !344
  %49 = load i8*, i8** %48, align 8, !dbg !344
  %50 = load i64, i64* %4, align 8, !dbg !346
  %51 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !347
  %52 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %51, i32 0, i32 8, !dbg !348
  %53 = load i8**, i8*** %52, align 8, !dbg !348
  %54 = getelementptr inbounds i8*, i8** %53, i64 %50, !dbg !347
  store i8* %49, i8** %54, align 8, !dbg !349
  %55 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !350
  %56 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %55, i32 0, i32 7, !dbg !351
  %57 = load void (i8*, i64)*, void (i8*, i64)** %56, align 8, !dbg !351
  %58 = load i64, i64* %4, align 8, !dbg !352
  %59 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !353
  %60 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %59, i32 0, i32 8, !dbg !354
  %61 = load i8**, i8*** %60, align 8, !dbg !354
  %62 = getelementptr inbounds i8*, i8** %61, i64 %58, !dbg !353
  %63 = load i8*, i8** %62, align 8, !dbg !353
  %64 = load i64, i64* %4, align 8, !dbg !355
  call void %57(i8* %63, i64 %64), !dbg !350
  %65 = load i64, i64* %5, align 8, !dbg !356
  store i64 %65, i64* %4, align 8, !dbg !357
  br label %19, !dbg !328, !llvm.loop !358

; <label>:66:                                     ; preds = %41
  %67 = load i8*, i8** %6, align 8, !dbg !359
  %68 = load i64, i64* %4, align 8, !dbg !360
  %69 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !361
  %70 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %69, i32 0, i32 8, !dbg !362
  %71 = load i8**, i8*** %70, align 8, !dbg !362
  %72 = getelementptr inbounds i8*, i8** %71, i64 %68, !dbg !361
  store i8* %67, i8** %72, align 8, !dbg !363
  %73 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !364
  %74 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %73, i32 0, i32 7, !dbg !365
  %75 = load void (i8*, i64)*, void (i8*, i64)** %74, align 8, !dbg !365
  %76 = load i8*, i8** %6, align 8, !dbg !366
  %77 = load i64, i64* %4, align 8, !dbg !367
  call void %75(i8* %76, i64 %77), !dbg !364
  ret void, !dbg !368
}

; Function Attrs: nounwind ssp uwtable
define i32 @pqueue_remove(%struct.pqueue_t*, i8*) #0 !dbg !369 {
  %3 = alloca %struct.pqueue_t*, align 8
  %4 = alloca i8*, align 8
  %5 = alloca i64, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %3, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %3, metadata !370, metadata !57), !dbg !371
  store i8* %1, i8** %4, align 8
  call void @llvm.dbg.declare(metadata i8** %4, metadata !372, metadata !57), !dbg !373
  call void @llvm.dbg.declare(metadata i64* %5, metadata !374, metadata !57), !dbg !375
  %6 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !376
  %7 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %6, i32 0, i32 6, !dbg !377
  %8 = load i64 (i8*)*, i64 (i8*)** %7, align 8, !dbg !377
  %9 = load i8*, i8** %4, align 8, !dbg !378
  %10 = call i64 %8(i8* %9), !dbg !376
  store i64 %10, i64* %5, align 8, !dbg !375
  %11 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !379
  %12 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %11, i32 0, i32 0, !dbg !380
  %13 = load i64, i64* %12, align 8, !dbg !381
  %14 = add i64 %13, -1, !dbg !381
  store i64 %14, i64* %12, align 8, !dbg !381
  %15 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !382
  %16 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %15, i32 0, i32 8, !dbg !383
  %17 = load i8**, i8*** %16, align 8, !dbg !383
  %18 = getelementptr inbounds i8*, i8** %17, i64 %14, !dbg !382
  %19 = load i8*, i8** %18, align 8, !dbg !382
  %20 = load i64, i64* %5, align 8, !dbg !384
  %21 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !385
  %22 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %21, i32 0, i32 8, !dbg !386
  %23 = load i8**, i8*** %22, align 8, !dbg !386
  %24 = getelementptr inbounds i8*, i8** %23, i64 %20, !dbg !385
  store i8* %19, i8** %24, align 8, !dbg !387
  %25 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !388
  %26 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %25, i32 0, i32 3, !dbg !390
  %27 = load i32 (i64, i64)*, i32 (i64, i64)** %26, align 8, !dbg !390
  %28 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !391
  %29 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %28, i32 0, i32 4, !dbg !392
  %30 = load i64 (i8*)*, i64 (i8*)** %29, align 8, !dbg !392
  %31 = load i8*, i8** %4, align 8, !dbg !393
  %32 = call i64 %30(i8* %31), !dbg !391
  %33 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !394
  %34 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %33, i32 0, i32 4, !dbg !395
  %35 = load i64 (i8*)*, i64 (i8*)** %34, align 8, !dbg !395
  %36 = load i64, i64* %5, align 8, !dbg !396
  %37 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !397
  %38 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %37, i32 0, i32 8, !dbg !398
  %39 = load i8**, i8*** %38, align 8, !dbg !398
  %40 = getelementptr inbounds i8*, i8** %39, i64 %36, !dbg !397
  %41 = load i8*, i8** %40, align 8, !dbg !397
  %42 = call i64 %35(i8* %41), !dbg !394
  %43 = call i32 %27(i64 %32, i64 %42), !dbg !388
  %44 = icmp ne i32 %43, 0, !dbg !388
  br i1 %44, label %45, label %48, !dbg !399

; <label>:45:                                     ; preds = %2
  %46 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !400
  %47 = load i64, i64* %5, align 8, !dbg !401
  call void @bubble_up(%struct.pqueue_t* %46, i64 %47), !dbg !402
  br label %51, !dbg !402

; <label>:48:                                     ; preds = %2
  %49 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !403
  %50 = load i64, i64* %5, align 8, !dbg !404
  call void @percolate_down(%struct.pqueue_t* %49, i64 %50), !dbg !405
  br label %51

; <label>:51:                                     ; preds = %48, %45
  ret i32 0, !dbg !406
}

; Function Attrs: nounwind ssp uwtable
define i8* @pqueue_pop(%struct.pqueue_t*) #0 !dbg !407 {
  %2 = alloca i8*, align 8
  %3 = alloca %struct.pqueue_t*, align 8
  %4 = alloca i8*, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %3, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %3, metadata !410, metadata !57), !dbg !411
  call void @llvm.dbg.declare(metadata i8** %4, metadata !412, metadata !57), !dbg !413
  %5 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !414
  %6 = icmp ne %struct.pqueue_t* %5, null, !dbg !414
  br i1 %6, label %7, label %12, !dbg !416

; <label>:7:                                      ; preds = %1
  %8 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !417
  %9 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %8, i32 0, i32 0, !dbg !418
  %10 = load i64, i64* %9, align 8, !dbg !418
  %11 = icmp eq i64 %10, 1, !dbg !419
  br i1 %11, label %12, label %13, !dbg !420

; <label>:12:                                     ; preds = %7, %1
  store i8* null, i8** %2, align 8, !dbg !421
  br label %34, !dbg !421

; <label>:13:                                     ; preds = %7
  %14 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !422
  %15 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %14, i32 0, i32 8, !dbg !423
  %16 = load i8**, i8*** %15, align 8, !dbg !423
  %17 = getelementptr inbounds i8*, i8** %16, i64 1, !dbg !422
  %18 = load i8*, i8** %17, align 8, !dbg !422
  store i8* %18, i8** %4, align 8, !dbg !424
  %19 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !425
  %20 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %19, i32 0, i32 0, !dbg !426
  %21 = load i64, i64* %20, align 8, !dbg !427
  %22 = add i64 %21, -1, !dbg !427
  store i64 %22, i64* %20, align 8, !dbg !427
  %23 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !428
  %24 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %23, i32 0, i32 8, !dbg !429
  %25 = load i8**, i8*** %24, align 8, !dbg !429
  %26 = getelementptr inbounds i8*, i8** %25, i64 %22, !dbg !428
  %27 = load i8*, i8** %26, align 8, !dbg !428
  %28 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !430
  %29 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %28, i32 0, i32 8, !dbg !431
  %30 = load i8**, i8*** %29, align 8, !dbg !431
  %31 = getelementptr inbounds i8*, i8** %30, i64 1, !dbg !430
  store i8* %27, i8** %31, align 8, !dbg !432
  %32 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !433
  call void @percolate_down(%struct.pqueue_t* %32, i64 1), !dbg !434
  %33 = load i8*, i8** %4, align 8, !dbg !435
  store i8* %33, i8** %2, align 8, !dbg !436
  br label %34, !dbg !436

; <label>:34:                                     ; preds = %13, %12
  %35 = load i8*, i8** %2, align 8, !dbg !437
  ret i8* %35, !dbg !437
}

; Function Attrs: nounwind ssp uwtable
define i8* @pqueue_peek(%struct.pqueue_t*) #0 !dbg !438 {
  %2 = alloca i8*, align 8
  %3 = alloca %struct.pqueue_t*, align 8
  %4 = alloca i8*, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %3, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %3, metadata !439, metadata !57), !dbg !440
  call void @llvm.dbg.declare(metadata i8** %4, metadata !441, metadata !57), !dbg !442
  %5 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !443
  %6 = icmp ne %struct.pqueue_t* %5, null, !dbg !443
  br i1 %6, label %7, label %12, !dbg !445

; <label>:7:                                      ; preds = %1
  %8 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !446
  %9 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %8, i32 0, i32 0, !dbg !447
  %10 = load i64, i64* %9, align 8, !dbg !447
  %11 = icmp eq i64 %10, 1, !dbg !448
  br i1 %11, label %12, label %13, !dbg !449

; <label>:12:                                     ; preds = %7, %1
  store i8* null, i8** %2, align 8, !dbg !450
  br label %20, !dbg !450

; <label>:13:                                     ; preds = %7
  %14 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !451
  %15 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %14, i32 0, i32 8, !dbg !452
  %16 = load i8**, i8*** %15, align 8, !dbg !452
  %17 = getelementptr inbounds i8*, i8** %16, i64 1, !dbg !451
  %18 = load i8*, i8** %17, align 8, !dbg !451
  store i8* %18, i8** %4, align 8, !dbg !453
  %19 = load i8*, i8** %4, align 8, !dbg !454
  store i8* %19, i8** %2, align 8, !dbg !455
  br label %20, !dbg !455

; <label>:20:                                     ; preds = %13, %12
  %21 = load i8*, i8** %2, align 8, !dbg !456
  ret i8* %21, !dbg !456
}

; Function Attrs: nounwind ssp uwtable
define void @pqueue_dump(%struct.pqueue_t*, %struct.__sFILE*, void (%struct.__sFILE*, i8*)*) #0 !dbg !457 {
  %4 = alloca %struct.pqueue_t*, align 8
  %5 = alloca %struct.__sFILE*, align 8
  %6 = alloca void (%struct.__sFILE*, i8*)*, align 8
  %7 = alloca i32, align 4
  store %struct.pqueue_t* %0, %struct.pqueue_t** %4, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %4, metadata !525, metadata !57), !dbg !526
  store %struct.__sFILE* %1, %struct.__sFILE** %5, align 8
  call void @llvm.dbg.declare(metadata %struct.__sFILE** %5, metadata !527, metadata !57), !dbg !528
  store void (%struct.__sFILE*, i8*)* %2, void (%struct.__sFILE*, i8*)** %6, align 8
  call void @llvm.dbg.declare(metadata void (%struct.__sFILE*, i8*)** %6, metadata !529, metadata !57), !dbg !530
  call void @llvm.dbg.declare(metadata i32* %7, metadata !531, metadata !57), !dbg !532
  %8 = load %struct.__sFILE*, %struct.__sFILE** @__stdoutp, align 8, !dbg !533
  %9 = call i32 (%struct.__sFILE*, i8*, ...) @fprintf(%struct.__sFILE* %8, i8* getelementptr inbounds ([37 x i8], [37 x i8]* @.str, i32 0, i32 0)), !dbg !534
  store i32 1, i32* %7, align 4, !dbg !535
  br label %10, !dbg !537

; <label>:10:                                     ; preds = %42, %3
  %11 = load i32, i32* %7, align 4, !dbg !538
  %12 = sext i32 %11 to i64, !dbg !538
  %13 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !540
  %14 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %13, i32 0, i32 0, !dbg !541
  %15 = load i64, i64* %14, align 8, !dbg !541
  %16 = icmp ult i64 %12, %15, !dbg !542
  br i1 %16, label %17, label %45, !dbg !543

; <label>:17:                                     ; preds = %10
  %18 = load %struct.__sFILE*, %struct.__sFILE** @__stdoutp, align 8, !dbg !544
  %19 = load i32, i32* %7, align 4, !dbg !546
  %20 = load i32, i32* %7, align 4, !dbg !547
  %21 = shl i32 %20, 1, !dbg !547
  %22 = load i32, i32* %7, align 4, !dbg !548
  %23 = shl i32 %22, 1, !dbg !548
  %24 = add nsw i32 %23, 1, !dbg !548
  %25 = load i32, i32* %7, align 4, !dbg !549
  %26 = ashr i32 %25, 1, !dbg !549
  %27 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !550
  %28 = load i32, i32* %7, align 4, !dbg !551
  %29 = sext i32 %28 to i64, !dbg !551
  %30 = call i64 @maxchild(%struct.pqueue_t* %27, i64 %29), !dbg !552
  %31 = trunc i64 %30 to i32, !dbg !553
  %32 = call i32 (%struct.__sFILE*, i8*, ...) @fprintf(%struct.__sFILE* %18, i8* getelementptr inbounds ([17 x i8], [17 x i8]* @.str.1, i32 0, i32 0), i32 %19, i32 %21, i32 %24, i32 %26, i32 %31), !dbg !554
  %33 = load void (%struct.__sFILE*, i8*)*, void (%struct.__sFILE*, i8*)** %6, align 8, !dbg !555
  %34 = load %struct.__sFILE*, %struct.__sFILE** %5, align 8, !dbg !556
  %35 = load i32, i32* %7, align 4, !dbg !557
  %36 = sext i32 %35 to i64, !dbg !558
  %37 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !558
  %38 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %37, i32 0, i32 8, !dbg !559
  %39 = load i8**, i8*** %38, align 8, !dbg !559
  %40 = getelementptr inbounds i8*, i8** %39, i64 %36, !dbg !558
  %41 = load i8*, i8** %40, align 8, !dbg !558
  call void %33(%struct.__sFILE* %34, i8* %41), !dbg !555
  br label %42, !dbg !560

; <label>:42:                                     ; preds = %17
  %43 = load i32, i32* %7, align 4, !dbg !561
  %44 = add nsw i32 %43, 1, !dbg !561
  store i32 %44, i32* %7, align 4, !dbg !561
  br label %10, !dbg !562, !llvm.loop !563

; <label>:45:                                     ; preds = %10
  ret void, !dbg !565
}

declare i32 @fprintf(%struct.__sFILE*, i8*, ...) #2

; Function Attrs: nounwind ssp uwtable
define internal i64 @maxchild(%struct.pqueue_t*, i64) #0 !dbg !566 {
  %3 = alloca i64, align 8
  %4 = alloca %struct.pqueue_t*, align 8
  %5 = alloca i64, align 8
  %6 = alloca i64, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %4, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %4, metadata !569, metadata !57), !dbg !570
  store i64 %1, i64* %5, align 8
  call void @llvm.dbg.declare(metadata i64* %5, metadata !571, metadata !57), !dbg !572
  call void @llvm.dbg.declare(metadata i64* %6, metadata !573, metadata !57), !dbg !574
  %7 = load i64, i64* %5, align 8, !dbg !575
  %8 = shl i64 %7, 1, !dbg !575
  store i64 %8, i64* %6, align 8, !dbg !574
  %9 = load i64, i64* %6, align 8, !dbg !576
  %10 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !578
  %11 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %10, i32 0, i32 0, !dbg !579
  %12 = load i64, i64* %11, align 8, !dbg !579
  %13 = icmp uge i64 %9, %12, !dbg !580
  br i1 %13, label %14, label %15, !dbg !581

; <label>:14:                                     ; preds = %2
  store i64 0, i64* %3, align 8, !dbg !582
  br label %54, !dbg !582

; <label>:15:                                     ; preds = %2
  %16 = load i64, i64* %6, align 8, !dbg !583
  %17 = add i64 %16, 1, !dbg !585
  %18 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !586
  %19 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %18, i32 0, i32 0, !dbg !587
  %20 = load i64, i64* %19, align 8, !dbg !587
  %21 = icmp ult i64 %17, %20, !dbg !588
  br i1 %21, label %22, label %52, !dbg !589

; <label>:22:                                     ; preds = %15
  %23 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !590
  %24 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %23, i32 0, i32 3, !dbg !591
  %25 = load i32 (i64, i64)*, i32 (i64, i64)** %24, align 8, !dbg !591
  %26 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !592
  %27 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %26, i32 0, i32 4, !dbg !593
  %28 = load i64 (i8*)*, i64 (i8*)** %27, align 8, !dbg !593
  %29 = load i64, i64* %6, align 8, !dbg !594
  %30 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !595
  %31 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %30, i32 0, i32 8, !dbg !596
  %32 = load i8**, i8*** %31, align 8, !dbg !596
  %33 = getelementptr inbounds i8*, i8** %32, i64 %29, !dbg !595
  %34 = load i8*, i8** %33, align 8, !dbg !595
  %35 = call i64 %28(i8* %34), !dbg !592
  %36 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !597
  %37 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %36, i32 0, i32 4, !dbg !598
  %38 = load i64 (i8*)*, i64 (i8*)** %37, align 8, !dbg !598
  %39 = load i64, i64* %6, align 8, !dbg !599
  %40 = add i64 %39, 1, !dbg !600
  %41 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !601
  %42 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %41, i32 0, i32 8, !dbg !602
  %43 = load i8**, i8*** %42, align 8, !dbg !602
  %44 = getelementptr inbounds i8*, i8** %43, i64 %40, !dbg !601
  %45 = load i8*, i8** %44, align 8, !dbg !601
  %46 = call i64 %38(i8* %45), !dbg !597
  %47 = call i32 %25(i64 %35, i64 %46), !dbg !590
  %48 = icmp ne i32 %47, 0, !dbg !590
  br i1 %48, label %49, label %52, !dbg !603

; <label>:49:                                     ; preds = %22
  %50 = load i64, i64* %6, align 8, !dbg !604
  %51 = add i64 %50, 1, !dbg !604
  store i64 %51, i64* %6, align 8, !dbg !604
  br label %52, !dbg !605

; <label>:52:                                     ; preds = %49, %22, %15
  %53 = load i64, i64* %6, align 8, !dbg !606
  store i64 %53, i64* %3, align 8, !dbg !607
  br label %54, !dbg !607

; <label>:54:                                     ; preds = %52, %14
  %55 = load i64, i64* %3, align 8, !dbg !608
  ret i64 %55, !dbg !608
}

; Function Attrs: nounwind ssp uwtable
define void @pqueue_print(%struct.pqueue_t*, %struct.__sFILE*, void (%struct.__sFILE*, i8*)*) #0 !dbg !609 {
  %4 = alloca %struct.pqueue_t*, align 8
  %5 = alloca %struct.__sFILE*, align 8
  %6 = alloca void (%struct.__sFILE*, i8*)*, align 8
  %7 = alloca %struct.pqueue_t*, align 8
  %8 = alloca i8*, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %4, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %4, metadata !610, metadata !57), !dbg !611
  store %struct.__sFILE* %1, %struct.__sFILE** %5, align 8
  call void @llvm.dbg.declare(metadata %struct.__sFILE** %5, metadata !612, metadata !57), !dbg !613
  store void (%struct.__sFILE*, i8*)* %2, void (%struct.__sFILE*, i8*)** %6, align 8
  call void @llvm.dbg.declare(metadata void (%struct.__sFILE*, i8*)** %6, metadata !614, metadata !57), !dbg !615
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %7, metadata !616, metadata !57), !dbg !617
  call void @llvm.dbg.declare(metadata i8** %8, metadata !618, metadata !57), !dbg !619
  %9 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !620
  %10 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %9, i32 0, i32 0, !dbg !621
  %11 = load i64, i64* %10, align 8, !dbg !621
  %12 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !622
  %13 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %12, i32 0, i32 3, !dbg !623
  %14 = load i32 (i64, i64)*, i32 (i64, i64)** %13, align 8, !dbg !623
  %15 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !624
  %16 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %15, i32 0, i32 4, !dbg !625
  %17 = load i64 (i8*)*, i64 (i8*)** %16, align 8, !dbg !625
  %18 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !626
  %19 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %18, i32 0, i32 6, !dbg !627
  %20 = load i64 (i8*)*, i64 (i8*)** %19, align 8, !dbg !627
  %21 = call %struct.pqueue_t* @pqueue_init(i64 %11, i32 (i64, i64)* %14, i64 (i8*)* %17, void (i8*, i64)* @set_pri, i64 (i8*)* %20, void (i8*, i64)* @set_pos), !dbg !628
  store %struct.pqueue_t* %21, %struct.pqueue_t** %7, align 8, !dbg !629
  %22 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !630
  %23 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %22, i32 0, i32 0, !dbg !631
  %24 = load i64, i64* %23, align 8, !dbg !631
  %25 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !632
  %26 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %25, i32 0, i32 0, !dbg !633
  store i64 %24, i64* %26, align 8, !dbg !634
  %27 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !635
  %28 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %27, i32 0, i32 1, !dbg !636
  %29 = load i64, i64* %28, align 8, !dbg !636
  %30 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !637
  %31 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %30, i32 0, i32 1, !dbg !638
  store i64 %29, i64* %31, align 8, !dbg !639
  %32 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !640
  %33 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %32, i32 0, i32 2, !dbg !641
  %34 = load i64, i64* %33, align 8, !dbg !641
  %35 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !642
  %36 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %35, i32 0, i32 2, !dbg !643
  store i64 %34, i64* %36, align 8, !dbg !644
  %37 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !645
  %38 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %37, i32 0, i32 8, !dbg !645
  %39 = load i8**, i8*** %38, align 8, !dbg !645
  %40 = bitcast i8** %39 to i8*, !dbg !645
  %41 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !645
  %42 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %41, i32 0, i32 8, !dbg !645
  %43 = load i8**, i8*** %42, align 8, !dbg !645
  %44 = bitcast i8** %43 to i8*, !dbg !645
  %45 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !645
  %46 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %45, i32 0, i32 0, !dbg !645
  %47 = load i64, i64* %46, align 8, !dbg !645
  %48 = mul i64 %47, 8, !dbg !645
  %49 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !645
  %50 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %49, i32 0, i32 8, !dbg !645
  %51 = load i8**, i8*** %50, align 8, !dbg !645
  %52 = bitcast i8** %51 to i8*, !dbg !645
  %53 = call i64 @llvm.objectsize.i64.p0i8(i8* %52, i1 false), !dbg !645
  %54 = call i8* @__memcpy_chk(i8* %40, i8* %44, i64 %48, i64 %53) #4, !dbg !645
  br label %55, !dbg !646

; <label>:55:                                     ; preds = %59, %3
  %56 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !647
  %57 = call i8* @pqueue_pop(%struct.pqueue_t* %56), !dbg !648
  store i8* %57, i8** %8, align 8, !dbg !649
  %58 = icmp ne i8* %57, null, !dbg !646
  br i1 %58, label %59, label %63, !dbg !646

; <label>:59:                                     ; preds = %55
  %60 = load void (%struct.__sFILE*, i8*)*, void (%struct.__sFILE*, i8*)** %6, align 8, !dbg !650
  %61 = load %struct.__sFILE*, %struct.__sFILE** %5, align 8, !dbg !651
  %62 = load i8*, i8** %8, align 8, !dbg !652
  call void %60(%struct.__sFILE* %61, i8* %62), !dbg !650
  br label %55, !dbg !646, !llvm.loop !653

; <label>:63:                                     ; preds = %55
  %64 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !654
  call void @pqueue_free(%struct.pqueue_t* %64), !dbg !655
  ret void, !dbg !656
}

; Function Attrs: nounwind ssp uwtable
define internal void @set_pri(i8*, i64) #0 !dbg !657 {
  %3 = alloca i8*, align 8
  %4 = alloca i64, align 8
  store i8* %0, i8** %3, align 8
  call void @llvm.dbg.declare(metadata i8** %3, metadata !658, metadata !57), !dbg !659
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !660, metadata !57), !dbg !661
  ret void, !dbg !662
}

; Function Attrs: nounwind ssp uwtable
define internal void @set_pos(i8*, i64) #0 !dbg !663 {
  %3 = alloca i8*, align 8
  %4 = alloca i64, align 8
  store i8* %0, i8** %3, align 8
  call void @llvm.dbg.declare(metadata i8** %3, metadata !664, metadata !57), !dbg !665
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !666, metadata !57), !dbg !667
  ret void, !dbg !668
}

; Function Attrs: nounwind
declare i8* @__memcpy_chk(i8*, i8*, i64, i64) #3

; Function Attrs: nounwind readnone
declare i64 @llvm.objectsize.i64.p0i8(i8*, i1) #1

; Function Attrs: nounwind ssp uwtable
define i32 @pqueue_is_valid(%struct.pqueue_t*) #0 !dbg !669 {
  %2 = alloca %struct.pqueue_t*, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %2, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %2, metadata !672, metadata !57), !dbg !673
  %3 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !674
  %4 = call i32 @subtree_is_valid(%struct.pqueue_t* %3, i32 1), !dbg !675
  ret i32 %4, !dbg !676
}

; Function Attrs: nounwind ssp uwtable
define internal i32 @subtree_is_valid(%struct.pqueue_t*, i32) #0 !dbg !677 {
  %3 = alloca i32, align 4
  %4 = alloca %struct.pqueue_t*, align 8
  %5 = alloca i32, align 4
  store %struct.pqueue_t* %0, %struct.pqueue_t** %4, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %4, metadata !680, metadata !57), !dbg !681
  store i32 %1, i32* %5, align 4
  call void @llvm.dbg.declare(metadata i32* %5, metadata !682, metadata !57), !dbg !683
  %6 = load i32, i32* %5, align 4, !dbg !684
  %7 = shl i32 %6, 1, !dbg !684
  %8 = sext i32 %7 to i64, !dbg !684
  %9 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !686
  %10 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %9, i32 0, i32 0, !dbg !687
  %11 = load i64, i64* %10, align 8, !dbg !687
  %12 = icmp ult i64 %8, %11, !dbg !688
  br i1 %12, label %13, label %51, !dbg !689

; <label>:13:                                     ; preds = %2
  %14 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !690
  %15 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %14, i32 0, i32 3, !dbg !693
  %16 = load i32 (i64, i64)*, i32 (i64, i64)** %15, align 8, !dbg !693
  %17 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !694
  %18 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %17, i32 0, i32 4, !dbg !695
  %19 = load i64 (i8*)*, i64 (i8*)** %18, align 8, !dbg !695
  %20 = load i32, i32* %5, align 4, !dbg !696
  %21 = sext i32 %20 to i64, !dbg !697
  %22 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !697
  %23 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %22, i32 0, i32 8, !dbg !698
  %24 = load i8**, i8*** %23, align 8, !dbg !698
  %25 = getelementptr inbounds i8*, i8** %24, i64 %21, !dbg !697
  %26 = load i8*, i8** %25, align 8, !dbg !697
  %27 = call i64 %19(i8* %26), !dbg !694
  %28 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !699
  %29 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %28, i32 0, i32 4, !dbg !700
  %30 = load i64 (i8*)*, i64 (i8*)** %29, align 8, !dbg !700
  %31 = load i32, i32* %5, align 4, !dbg !701
  %32 = shl i32 %31, 1, !dbg !701
  %33 = sext i32 %32 to i64, !dbg !702
  %34 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !702
  %35 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %34, i32 0, i32 8, !dbg !703
  %36 = load i8**, i8*** %35, align 8, !dbg !703
  %37 = getelementptr inbounds i8*, i8** %36, i64 %33, !dbg !702
  %38 = load i8*, i8** %37, align 8, !dbg !702
  %39 = call i64 %30(i8* %38), !dbg !699
  %40 = call i32 %16(i64 %27, i64 %39), !dbg !690
  %41 = icmp ne i32 %40, 0, !dbg !690
  br i1 %41, label %42, label %43, !dbg !704

; <label>:42:                                     ; preds = %13
  store i32 0, i32* %3, align 4, !dbg !705
  br label %101, !dbg !705

; <label>:43:                                     ; preds = %13
  %44 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !706
  %45 = load i32, i32* %5, align 4, !dbg !708
  %46 = shl i32 %45, 1, !dbg !708
  %47 = call i32 @subtree_is_valid(%struct.pqueue_t* %44, i32 %46), !dbg !709
  %48 = icmp ne i32 %47, 0, !dbg !709
  br i1 %48, label %50, label %49, !dbg !710

; <label>:49:                                     ; preds = %43
  store i32 0, i32* %3, align 4, !dbg !711
  br label %101, !dbg !711

; <label>:50:                                     ; preds = %43
  br label %51, !dbg !712

; <label>:51:                                     ; preds = %50, %2
  %52 = load i32, i32* %5, align 4, !dbg !713
  %53 = shl i32 %52, 1, !dbg !713
  %54 = add nsw i32 %53, 1, !dbg !713
  %55 = sext i32 %54 to i64, !dbg !713
  %56 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !715
  %57 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %56, i32 0, i32 0, !dbg !716
  %58 = load i64, i64* %57, align 8, !dbg !716
  %59 = icmp ult i64 %55, %58, !dbg !717
  br i1 %59, label %60, label %100, !dbg !718

; <label>:60:                                     ; preds = %51
  %61 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !719
  %62 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %61, i32 0, i32 3, !dbg !722
  %63 = load i32 (i64, i64)*, i32 (i64, i64)** %62, align 8, !dbg !722
  %64 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !723
  %65 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %64, i32 0, i32 4, !dbg !724
  %66 = load i64 (i8*)*, i64 (i8*)** %65, align 8, !dbg !724
  %67 = load i32, i32* %5, align 4, !dbg !725
  %68 = sext i32 %67 to i64, !dbg !726
  %69 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !726
  %70 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %69, i32 0, i32 8, !dbg !727
  %71 = load i8**, i8*** %70, align 8, !dbg !727
  %72 = getelementptr inbounds i8*, i8** %71, i64 %68, !dbg !726
  %73 = load i8*, i8** %72, align 8, !dbg !726
  %74 = call i64 %66(i8* %73), !dbg !723
  %75 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !728
  %76 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %75, i32 0, i32 4, !dbg !729
  %77 = load i64 (i8*)*, i64 (i8*)** %76, align 8, !dbg !729
  %78 = load i32, i32* %5, align 4, !dbg !730
  %79 = shl i32 %78, 1, !dbg !730
  %80 = add nsw i32 %79, 1, !dbg !730
  %81 = sext i32 %80 to i64, !dbg !731
  %82 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !731
  %83 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %82, i32 0, i32 8, !dbg !732
  %84 = load i8**, i8*** %83, align 8, !dbg !732
  %85 = getelementptr inbounds i8*, i8** %84, i64 %81, !dbg !731
  %86 = load i8*, i8** %85, align 8, !dbg !731
  %87 = call i64 %77(i8* %86), !dbg !728
  %88 = call i32 %63(i64 %74, i64 %87), !dbg !719
  %89 = icmp ne i32 %88, 0, !dbg !719
  br i1 %89, label %90, label %91, !dbg !733

; <label>:90:                                     ; preds = %60
  store i32 0, i32* %3, align 4, !dbg !734
  br label %101, !dbg !734

; <label>:91:                                     ; preds = %60
  %92 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !735
  %93 = load i32, i32* %5, align 4, !dbg !737
  %94 = shl i32 %93, 1, !dbg !737
  %95 = add nsw i32 %94, 1, !dbg !737
  %96 = call i32 @subtree_is_valid(%struct.pqueue_t* %92, i32 %95), !dbg !738
  %97 = icmp ne i32 %96, 0, !dbg !738
  br i1 %97, label %99, label %98, !dbg !739

; <label>:98:                                     ; preds = %91
  store i32 0, i32* %3, align 4, !dbg !740
  br label %101, !dbg !740

; <label>:99:                                     ; preds = %91
  br label %100, !dbg !741

; <label>:100:                                    ; preds = %99, %51
  store i32 1, i32* %3, align 4, !dbg !742
  br label %101, !dbg !742

; <label>:101:                                    ; preds = %100, %98, %90, %49, %42
  %102 = load i32, i32* %3, align 4, !dbg !743
  ret i32 %102, !dbg !743
}

attributes #0 = { nounwind ssp uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }
attributes #2 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!6, !7, !8}
!llvm.ident = !{!9}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 3.9.1 (tags/RELEASE_391/final)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !3)
!1 = !DIFile(filename: "pqueue.c", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!2 = !{}
!3 = !{!4, !5}
!4 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64, align: 64)
!5 = !DIBasicType(name: "unsigned int", size: 32, align: 32, encoding: DW_ATE_unsigned)
!6 = !{i32 2, !"Dwarf Version", i32 2}
!7 = !{i32 2, !"Debug Info Version", i32 3}
!8 = !{i32 1, !"PIC Level", i32 2}
!9 = !{!"clang version 3.9.1 (tags/RELEASE_391/final)"}
!10 = distinct !DISubprogram(name: "pqueue_init", scope: !1, file: !1, line: 40, type: !11, isLocal: false, isDefinition: true, scopeLine: 46, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!11 = !DISubroutineType(types: !12)
!12 = !{!13, !19, !27, !35, !40, !45, !50}
!13 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !14, size: 64, align: 64)
!14 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_t", file: !15, line: 68, baseType: !16)
!15 = !DIFile(filename: "./pqueue.h", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!16 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "pqueue_t", file: !15, line: 57, size: 576, align: 64, elements: !17)
!17 = !{!18, !24, !25, !26, !34, !39, !44, !49, !54}
!18 = !DIDerivedType(tag: DW_TAG_member, name: "size", scope: !16, file: !15, line: 59, baseType: !19, size: 64, align: 64)
!19 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !20, line: 31, baseType: !21)
!20 = !DIFile(filename: "/usr/include/sys/_types/_size_t.h", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!21 = !DIDerivedType(tag: DW_TAG_typedef, name: "__darwin_size_t", file: !22, line: 92, baseType: !23)
!22 = !DIFile(filename: "/usr/include/i386/_types.h", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!23 = !DIBasicType(name: "long unsigned int", size: 64, align: 64, encoding: DW_ATE_unsigned)
!24 = !DIDerivedType(tag: DW_TAG_member, name: "avail", scope: !16, file: !15, line: 60, baseType: !19, size: 64, align: 64, offset: 64)
!25 = !DIDerivedType(tag: DW_TAG_member, name: "step", scope: !16, file: !15, line: 61, baseType: !19, size: 64, align: 64, offset: 128)
!26 = !DIDerivedType(tag: DW_TAG_member, name: "cmppri", scope: !16, file: !15, line: 62, baseType: !27, size: 64, align: 64, offset: 192)
!27 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_cmp_pri_f", file: !15, line: 44, baseType: !28)
!28 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !29, size: 64, align: 64)
!29 = !DISubroutineType(types: !30)
!30 = !{!31, !32, !32}
!31 = !DIBasicType(name: "int", size: 32, align: 32, encoding: DW_ATE_signed)
!32 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_pri_t", file: !15, line: 39, baseType: !33)
!33 = !DIBasicType(name: "long long unsigned int", size: 64, align: 64, encoding: DW_ATE_unsigned)
!34 = !DIDerivedType(tag: DW_TAG_member, name: "getpri", scope: !16, file: !15, line: 63, baseType: !35, size: 64, align: 64, offset: 256)
!35 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_get_pri_f", file: !15, line: 42, baseType: !36)
!36 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !37, size: 64, align: 64)
!37 = !DISubroutineType(types: !38)
!38 = !{!32, !4}
!39 = !DIDerivedType(tag: DW_TAG_member, name: "setpri", scope: !16, file: !15, line: 64, baseType: !40, size: 64, align: 64, offset: 320)
!40 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_set_pri_f", file: !15, line: 43, baseType: !41)
!41 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !42, size: 64, align: 64)
!42 = !DISubroutineType(types: !43)
!43 = !{null, !4, !32}
!44 = !DIDerivedType(tag: DW_TAG_member, name: "getpos", scope: !16, file: !15, line: 65, baseType: !45, size: 64, align: 64, offset: 384)
!45 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_get_pos_f", file: !15, line: 48, baseType: !46)
!46 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !47, size: 64, align: 64)
!47 = !DISubroutineType(types: !48)
!48 = !{!19, !4}
!49 = !DIDerivedType(tag: DW_TAG_member, name: "setpos", scope: !16, file: !15, line: 66, baseType: !50, size: 64, align: 64, offset: 448)
!50 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_set_pos_f", file: !15, line: 49, baseType: !51)
!51 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !52, size: 64, align: 64)
!52 = !DISubroutineType(types: !53)
!53 = !{null, !4, !19}
!54 = !DIDerivedType(tag: DW_TAG_member, name: "d", scope: !16, file: !15, line: 67, baseType: !55, size: 64, align: 64, offset: 512)
!55 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 64, align: 64)
!56 = !DILocalVariable(name: "n", arg: 1, scope: !10, file: !1, line: 40, type: !19)
!57 = !DIExpression()
!58 = !DILocation(line: 40, column: 20, scope: !10)
!59 = !DILocalVariable(name: "cmppri", arg: 2, scope: !10, file: !1, line: 41, type: !27)
!60 = !DILocation(line: 41, column: 30, scope: !10)
!61 = !DILocalVariable(name: "getpri", arg: 3, scope: !10, file: !1, line: 42, type: !35)
!62 = !DILocation(line: 42, column: 30, scope: !10)
!63 = !DILocalVariable(name: "setpri", arg: 4, scope: !10, file: !1, line: 43, type: !40)
!64 = !DILocation(line: 43, column: 30, scope: !10)
!65 = !DILocalVariable(name: "getpos", arg: 5, scope: !10, file: !1, line: 44, type: !45)
!66 = !DILocation(line: 44, column: 30, scope: !10)
!67 = !DILocalVariable(name: "setpos", arg: 6, scope: !10, file: !1, line: 45, type: !50)
!68 = !DILocation(line: 45, column: 30, scope: !10)
!69 = !DILocalVariable(name: "q", scope: !10, file: !1, line: 47, type: !13)
!70 = !DILocation(line: 47, column: 15, scope: !10)
!71 = !DILocation(line: 49, column: 15, scope: !72)
!72 = distinct !DILexicalBlock(scope: !10, file: !1, line: 49, column: 9)
!73 = !DILocation(line: 49, column: 13, scope: !72)
!74 = !DILocation(line: 49, column: 9, scope: !10)
!75 = !DILocation(line: 50, column: 9, scope: !72)
!76 = !DILocation(line: 53, column: 26, scope: !77)
!77 = distinct !DILexicalBlock(scope: !10, file: !1, line: 53, column: 9)
!78 = !DILocation(line: 53, column: 28, scope: !77)
!79 = !DILocation(line: 53, column: 33, scope: !77)
!80 = !DILocation(line: 53, column: 18, scope: !77)
!81 = !DILocation(line: 53, column: 11, scope: !77)
!82 = !DILocation(line: 53, column: 14, scope: !77)
!83 = !DILocation(line: 53, column: 16, scope: !77)
!84 = !DILocation(line: 53, column: 9, scope: !10)
!85 = !DILocation(line: 54, column: 14, scope: !86)
!86 = distinct !DILexicalBlock(scope: !77, file: !1, line: 53, column: 53)
!87 = !DILocation(line: 54, column: 9, scope: !86)
!88 = !DILocation(line: 55, column: 9, scope: !86)
!89 = !DILocation(line: 58, column: 5, scope: !10)
!90 = !DILocation(line: 58, column: 8, scope: !10)
!91 = !DILocation(line: 58, column: 13, scope: !10)
!92 = !DILocation(line: 59, column: 27, scope: !10)
!93 = !DILocation(line: 59, column: 28, scope: !10)
!94 = !DILocation(line: 59, column: 16, scope: !10)
!95 = !DILocation(line: 59, column: 19, scope: !10)
!96 = !DILocation(line: 59, column: 24, scope: !10)
!97 = !DILocation(line: 59, column: 5, scope: !10)
!98 = !DILocation(line: 59, column: 8, scope: !10)
!99 = !DILocation(line: 59, column: 14, scope: !10)
!100 = !DILocation(line: 60, column: 17, scope: !10)
!101 = !DILocation(line: 60, column: 5, scope: !10)
!102 = !DILocation(line: 60, column: 8, scope: !10)
!103 = !DILocation(line: 60, column: 15, scope: !10)
!104 = !DILocation(line: 61, column: 17, scope: !10)
!105 = !DILocation(line: 61, column: 5, scope: !10)
!106 = !DILocation(line: 61, column: 8, scope: !10)
!107 = !DILocation(line: 61, column: 15, scope: !10)
!108 = !DILocation(line: 62, column: 17, scope: !10)
!109 = !DILocation(line: 62, column: 5, scope: !10)
!110 = !DILocation(line: 62, column: 8, scope: !10)
!111 = !DILocation(line: 62, column: 15, scope: !10)
!112 = !DILocation(line: 63, column: 17, scope: !10)
!113 = !DILocation(line: 63, column: 5, scope: !10)
!114 = !DILocation(line: 63, column: 8, scope: !10)
!115 = !DILocation(line: 63, column: 15, scope: !10)
!116 = !DILocation(line: 64, column: 17, scope: !10)
!117 = !DILocation(line: 64, column: 5, scope: !10)
!118 = !DILocation(line: 64, column: 8, scope: !10)
!119 = !DILocation(line: 64, column: 15, scope: !10)
!120 = !DILocation(line: 66, column: 12, scope: !10)
!121 = !DILocation(line: 66, column: 5, scope: !10)
!122 = !DILocation(line: 67, column: 1, scope: !10)
!123 = distinct !DISubprogram(name: "pqueue_free", scope: !1, file: !1, line: 71, type: !124, isLocal: false, isDefinition: true, scopeLine: 72, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!124 = !DISubroutineType(types: !125)
!125 = !{null, !13}
!126 = !DILocalVariable(name: "q", arg: 1, scope: !123, file: !1, line: 71, type: !13)
!127 = !DILocation(line: 71, column: 23, scope: !123)
!128 = !DILocation(line: 73, column: 10, scope: !123)
!129 = !DILocation(line: 73, column: 13, scope: !123)
!130 = !DILocation(line: 73, column: 5, scope: !123)
!131 = !DILocation(line: 74, column: 10, scope: !123)
!132 = !DILocation(line: 74, column: 5, scope: !123)
!133 = !DILocation(line: 75, column: 1, scope: !123)
!134 = distinct !DISubprogram(name: "pqueue_size", scope: !1, file: !1, line: 79, type: !135, isLocal: false, isDefinition: true, scopeLine: 80, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!135 = !DISubroutineType(types: !136)
!136 = !{!19, !13}
!137 = !DILocalVariable(name: "q", arg: 1, scope: !134, file: !1, line: 79, type: !13)
!138 = !DILocation(line: 79, column: 23, scope: !134)
!139 = !DILocation(line: 82, column: 13, scope: !134)
!140 = !DILocation(line: 82, column: 16, scope: !134)
!141 = !DILocation(line: 82, column: 21, scope: !134)
!142 = !DILocation(line: 82, column: 5, scope: !134)
!143 = distinct !DISubprogram(name: "pqueue_insert", scope: !1, file: !1, line: 143, type: !144, isLocal: false, isDefinition: true, scopeLine: 144, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!144 = !DISubroutineType(types: !145)
!145 = !{!31, !13, !4}
!146 = !DILocalVariable(name: "q", arg: 1, scope: !143, file: !1, line: 143, type: !13)
!147 = !DILocation(line: 143, column: 25, scope: !143)
!148 = !DILocalVariable(name: "d", arg: 2, scope: !143, file: !1, line: 143, type: !4)
!149 = !DILocation(line: 143, column: 34, scope: !143)
!150 = !DILocalVariable(name: "tmp", scope: !143, file: !1, line: 145, type: !4)
!151 = !DILocation(line: 145, column: 11, scope: !143)
!152 = !DILocalVariable(name: "i", scope: !143, file: !1, line: 146, type: !19)
!153 = !DILocation(line: 146, column: 12, scope: !143)
!154 = !DILocalVariable(name: "newsize", scope: !143, file: !1, line: 147, type: !19)
!155 = !DILocation(line: 147, column: 12, scope: !143)
!156 = !DILocation(line: 149, column: 10, scope: !157)
!157 = distinct !DILexicalBlock(scope: !143, file: !1, line: 149, column: 9)
!158 = !DILocation(line: 149, column: 9, scope: !143)
!159 = !DILocation(line: 149, column: 13, scope: !157)
!160 = !DILocation(line: 152, column: 9, scope: !161)
!161 = distinct !DILexicalBlock(scope: !143, file: !1, line: 152, column: 9)
!162 = !DILocation(line: 152, column: 12, scope: !161)
!163 = !DILocation(line: 152, column: 20, scope: !161)
!164 = !DILocation(line: 152, column: 23, scope: !161)
!165 = !DILocation(line: 152, column: 17, scope: !161)
!166 = !DILocation(line: 152, column: 9, scope: !143)
!167 = !DILocation(line: 153, column: 19, scope: !168)
!168 = distinct !DILexicalBlock(scope: !161, file: !1, line: 152, column: 30)
!169 = !DILocation(line: 153, column: 22, scope: !168)
!170 = !DILocation(line: 153, column: 29, scope: !168)
!171 = !DILocation(line: 153, column: 32, scope: !168)
!172 = !DILocation(line: 153, column: 27, scope: !168)
!173 = !DILocation(line: 153, column: 17, scope: !168)
!174 = !DILocation(line: 154, column: 29, scope: !175)
!175 = distinct !DILexicalBlock(scope: !168, file: !1, line: 154, column: 13)
!176 = !DILocation(line: 154, column: 32, scope: !175)
!177 = !DILocation(line: 154, column: 52, scope: !175)
!178 = !DILocation(line: 154, column: 50, scope: !175)
!179 = !DILocation(line: 154, column: 21, scope: !175)
!180 = !DILocation(line: 154, column: 19, scope: !175)
!181 = !DILocation(line: 154, column: 13, scope: !168)
!182 = !DILocation(line: 155, column: 13, scope: !175)
!183 = !DILocation(line: 156, column: 16, scope: !168)
!184 = !DILocation(line: 156, column: 9, scope: !168)
!185 = !DILocation(line: 156, column: 12, scope: !168)
!186 = !DILocation(line: 156, column: 14, scope: !168)
!187 = !DILocation(line: 157, column: 20, scope: !168)
!188 = !DILocation(line: 157, column: 9, scope: !168)
!189 = !DILocation(line: 157, column: 12, scope: !168)
!190 = !DILocation(line: 157, column: 18, scope: !168)
!191 = !DILocation(line: 158, column: 5, scope: !168)
!192 = !DILocation(line: 161, column: 9, scope: !143)
!193 = !DILocation(line: 161, column: 12, scope: !143)
!194 = !DILocation(line: 161, column: 16, scope: !143)
!195 = !DILocation(line: 161, column: 7, scope: !143)
!196 = !DILocation(line: 162, column: 15, scope: !143)
!197 = !DILocation(line: 162, column: 10, scope: !143)
!198 = !DILocation(line: 162, column: 5, scope: !143)
!199 = !DILocation(line: 162, column: 8, scope: !143)
!200 = !DILocation(line: 162, column: 13, scope: !143)
!201 = !DILocation(line: 163, column: 15, scope: !143)
!202 = !DILocation(line: 163, column: 18, scope: !143)
!203 = !DILocation(line: 163, column: 5, scope: !143)
!204 = !DILocation(line: 165, column: 5, scope: !143)
!205 = !DILocation(line: 166, column: 1, scope: !143)
!206 = distinct !DISubprogram(name: "bubble_up", scope: !1, file: !1, line: 87, type: !207, isLocal: true, isDefinition: true, scopeLine: 88, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!207 = !DISubroutineType(types: !208)
!208 = !{null, !13, !19}
!209 = !DILocalVariable(name: "q", arg: 1, scope: !206, file: !1, line: 87, type: !13)
!210 = !DILocation(line: 87, column: 21, scope: !206)
!211 = !DILocalVariable(name: "i", arg: 2, scope: !206, file: !1, line: 87, type: !19)
!212 = !DILocation(line: 87, column: 31, scope: !206)
!213 = !DILocalVariable(name: "parent_node", scope: !206, file: !1, line: 89, type: !19)
!214 = !DILocation(line: 89, column: 12, scope: !206)
!215 = !DILocalVariable(name: "moving_node", scope: !206, file: !1, line: 90, type: !4)
!216 = !DILocation(line: 90, column: 11, scope: !206)
!217 = !DILocation(line: 90, column: 30, scope: !206)
!218 = !DILocation(line: 90, column: 25, scope: !206)
!219 = !DILocation(line: 90, column: 28, scope: !206)
!220 = !DILocalVariable(name: "moving_pri", scope: !206, file: !1, line: 91, type: !32)
!221 = !DILocation(line: 91, column: 18, scope: !206)
!222 = !DILocation(line: 91, column: 31, scope: !206)
!223 = !DILocation(line: 91, column: 34, scope: !206)
!224 = !DILocation(line: 91, column: 41, scope: !206)
!225 = !DILocation(line: 93, column: 24, scope: !226)
!226 = distinct !DILexicalBlock(scope: !206, file: !1, line: 93, column: 5)
!227 = !DILocation(line: 93, column: 22, scope: !226)
!228 = !DILocation(line: 93, column: 10, scope: !226)
!229 = !DILocation(line: 94, column: 12, scope: !230)
!230 = distinct !DILexicalBlock(scope: !226, file: !1, line: 93, column: 5)
!231 = !DILocation(line: 94, column: 14, scope: !230)
!232 = !DILocation(line: 94, column: 19, scope: !230)
!233 = !DILocation(line: 94, column: 22, scope: !230)
!234 = !DILocation(line: 94, column: 25, scope: !230)
!235 = !DILocation(line: 94, column: 32, scope: !230)
!236 = !DILocation(line: 94, column: 35, scope: !230)
!237 = !DILocation(line: 94, column: 47, scope: !230)
!238 = !DILocation(line: 94, column: 42, scope: !230)
!239 = !DILocation(line: 94, column: 45, scope: !230)
!240 = !DILocation(line: 94, column: 62, scope: !230)
!241 = !DILocation(line: 93, column: 5, scope: !226)
!242 = !DILocation(line: 97, column: 24, scope: !243)
!243 = distinct !DILexicalBlock(scope: !230, file: !1, line: 96, column: 5)
!244 = !DILocation(line: 97, column: 19, scope: !243)
!245 = !DILocation(line: 97, column: 22, scope: !243)
!246 = !DILocation(line: 97, column: 14, scope: !243)
!247 = !DILocation(line: 97, column: 9, scope: !243)
!248 = !DILocation(line: 97, column: 12, scope: !243)
!249 = !DILocation(line: 97, column: 17, scope: !243)
!250 = !DILocation(line: 98, column: 9, scope: !243)
!251 = !DILocation(line: 98, column: 12, scope: !243)
!252 = !DILocation(line: 98, column: 24, scope: !243)
!253 = !DILocation(line: 98, column: 19, scope: !243)
!254 = !DILocation(line: 98, column: 22, scope: !243)
!255 = !DILocation(line: 98, column: 28, scope: !243)
!256 = !DILocation(line: 99, column: 5, scope: !243)
!257 = !DILocation(line: 95, column: 14, scope: !230)
!258 = !DILocation(line: 95, column: 12, scope: !230)
!259 = !DILocation(line: 95, column: 41, scope: !230)
!260 = !DILocation(line: 95, column: 39, scope: !230)
!261 = !DILocation(line: 93, column: 5, scope: !230)
!262 = distinct !{!262, !263}
!263 = !DILocation(line: 93, column: 5, scope: !206)
!264 = !DILocation(line: 101, column: 15, scope: !206)
!265 = !DILocation(line: 101, column: 10, scope: !206)
!266 = !DILocation(line: 101, column: 5, scope: !206)
!267 = !DILocation(line: 101, column: 8, scope: !206)
!268 = !DILocation(line: 101, column: 13, scope: !206)
!269 = !DILocation(line: 102, column: 5, scope: !206)
!270 = !DILocation(line: 102, column: 8, scope: !206)
!271 = !DILocation(line: 102, column: 15, scope: !206)
!272 = !DILocation(line: 102, column: 28, scope: !206)
!273 = !DILocation(line: 103, column: 1, scope: !206)
!274 = distinct !DISubprogram(name: "pqueue_change_priority", scope: !1, file: !1, line: 170, type: !275, isLocal: false, isDefinition: true, scopeLine: 173, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!275 = !DISubroutineType(types: !276)
!276 = !{null, !13, !32, !4}
!277 = !DILocalVariable(name: "q", arg: 1, scope: !274, file: !1, line: 170, type: !13)
!278 = !DILocation(line: 170, column: 34, scope: !274)
!279 = !DILocalVariable(name: "new_pri", arg: 2, scope: !274, file: !1, line: 171, type: !32)
!280 = !DILocation(line: 171, column: 37, scope: !274)
!281 = !DILocalVariable(name: "d", arg: 3, scope: !274, file: !1, line: 172, type: !4)
!282 = !DILocation(line: 172, column: 30, scope: !274)
!283 = !DILocalVariable(name: "posn", scope: !274, file: !1, line: 174, type: !19)
!284 = !DILocation(line: 174, column: 12, scope: !274)
!285 = !DILocalVariable(name: "old_pri", scope: !274, file: !1, line: 175, type: !32)
!286 = !DILocation(line: 175, column: 18, scope: !274)
!287 = !DILocation(line: 175, column: 28, scope: !274)
!288 = !DILocation(line: 175, column: 31, scope: !274)
!289 = !DILocation(line: 175, column: 38, scope: !274)
!290 = !DILocation(line: 177, column: 5, scope: !274)
!291 = !DILocation(line: 177, column: 8, scope: !274)
!292 = !DILocation(line: 177, column: 15, scope: !274)
!293 = !DILocation(line: 177, column: 18, scope: !274)
!294 = !DILocation(line: 178, column: 12, scope: !274)
!295 = !DILocation(line: 178, column: 15, scope: !274)
!296 = !DILocation(line: 178, column: 22, scope: !274)
!297 = !DILocation(line: 178, column: 10, scope: !274)
!298 = !DILocation(line: 179, column: 9, scope: !299)
!299 = distinct !DILexicalBlock(scope: !274, file: !1, line: 179, column: 9)
!300 = !DILocation(line: 179, column: 12, scope: !299)
!301 = !DILocation(line: 179, column: 19, scope: !299)
!302 = !DILocation(line: 179, column: 28, scope: !299)
!303 = !DILocation(line: 179, column: 9, scope: !274)
!304 = !DILocation(line: 180, column: 19, scope: !299)
!305 = !DILocation(line: 180, column: 22, scope: !299)
!306 = !DILocation(line: 180, column: 9, scope: !299)
!307 = !DILocation(line: 182, column: 24, scope: !299)
!308 = !DILocation(line: 182, column: 27, scope: !299)
!309 = !DILocation(line: 182, column: 9, scope: !299)
!310 = !DILocation(line: 183, column: 1, scope: !274)
!311 = distinct !DISubprogram(name: "percolate_down", scope: !1, file: !1, line: 123, type: !207, isLocal: true, isDefinition: true, scopeLine: 124, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!312 = !DILocalVariable(name: "q", arg: 1, scope: !311, file: !1, line: 123, type: !13)
!313 = !DILocation(line: 123, column: 26, scope: !311)
!314 = !DILocalVariable(name: "i", arg: 2, scope: !311, file: !1, line: 123, type: !19)
!315 = !DILocation(line: 123, column: 36, scope: !311)
!316 = !DILocalVariable(name: "child_node", scope: !311, file: !1, line: 125, type: !19)
!317 = !DILocation(line: 125, column: 12, scope: !311)
!318 = !DILocalVariable(name: "moving_node", scope: !311, file: !1, line: 126, type: !4)
!319 = !DILocation(line: 126, column: 11, scope: !311)
!320 = !DILocation(line: 126, column: 30, scope: !311)
!321 = !DILocation(line: 126, column: 25, scope: !311)
!322 = !DILocation(line: 126, column: 28, scope: !311)
!323 = !DILocalVariable(name: "moving_pri", scope: !311, file: !1, line: 127, type: !32)
!324 = !DILocation(line: 127, column: 18, scope: !311)
!325 = !DILocation(line: 127, column: 31, scope: !311)
!326 = !DILocation(line: 127, column: 34, scope: !311)
!327 = !DILocation(line: 127, column: 41, scope: !311)
!328 = !DILocation(line: 129, column: 5, scope: !311)
!329 = !DILocation(line: 129, column: 35, scope: !311)
!330 = !DILocation(line: 129, column: 38, scope: !311)
!331 = !DILocation(line: 129, column: 26, scope: !311)
!332 = !DILocation(line: 129, column: 24, scope: !311)
!333 = !DILocation(line: 129, column: 42, scope: !311)
!334 = !DILocation(line: 130, column: 12, scope: !311)
!335 = !DILocation(line: 130, column: 15, scope: !311)
!336 = !DILocation(line: 130, column: 22, scope: !311)
!337 = !DILocation(line: 130, column: 34, scope: !311)
!338 = !DILocation(line: 130, column: 37, scope: !311)
!339 = !DILocation(line: 130, column: 49, scope: !311)
!340 = !DILocation(line: 130, column: 44, scope: !311)
!341 = !DILocation(line: 130, column: 47, scope: !311)
!342 = !DILocation(line: 132, column: 24, scope: !343)
!343 = distinct !DILexicalBlock(scope: !311, file: !1, line: 131, column: 5)
!344 = !DILocation(line: 132, column: 19, scope: !343)
!345 = !DILocation(line: 132, column: 22, scope: !343)
!346 = !DILocation(line: 132, column: 14, scope: !343)
!347 = !DILocation(line: 132, column: 9, scope: !343)
!348 = !DILocation(line: 132, column: 12, scope: !343)
!349 = !DILocation(line: 132, column: 17, scope: !343)
!350 = !DILocation(line: 133, column: 9, scope: !343)
!351 = !DILocation(line: 133, column: 12, scope: !343)
!352 = !DILocation(line: 133, column: 24, scope: !343)
!353 = !DILocation(line: 133, column: 19, scope: !343)
!354 = !DILocation(line: 133, column: 22, scope: !343)
!355 = !DILocation(line: 133, column: 28, scope: !343)
!356 = !DILocation(line: 134, column: 13, scope: !343)
!357 = !DILocation(line: 134, column: 11, scope: !343)
!358 = distinct !{!358, !328}
!359 = !DILocation(line: 137, column: 15, scope: !311)
!360 = !DILocation(line: 137, column: 10, scope: !311)
!361 = !DILocation(line: 137, column: 5, scope: !311)
!362 = !DILocation(line: 137, column: 8, scope: !311)
!363 = !DILocation(line: 137, column: 13, scope: !311)
!364 = !DILocation(line: 138, column: 5, scope: !311)
!365 = !DILocation(line: 138, column: 8, scope: !311)
!366 = !DILocation(line: 138, column: 15, scope: !311)
!367 = !DILocation(line: 138, column: 28, scope: !311)
!368 = !DILocation(line: 139, column: 1, scope: !311)
!369 = distinct !DISubprogram(name: "pqueue_remove", scope: !1, file: !1, line: 187, type: !144, isLocal: false, isDefinition: true, scopeLine: 188, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!370 = !DILocalVariable(name: "q", arg: 1, scope: !369, file: !1, line: 187, type: !13)
!371 = !DILocation(line: 187, column: 25, scope: !369)
!372 = !DILocalVariable(name: "d", arg: 2, scope: !369, file: !1, line: 187, type: !4)
!373 = !DILocation(line: 187, column: 34, scope: !369)
!374 = !DILocalVariable(name: "posn", scope: !369, file: !1, line: 189, type: !19)
!375 = !DILocation(line: 189, column: 12, scope: !369)
!376 = !DILocation(line: 189, column: 19, scope: !369)
!377 = !DILocation(line: 189, column: 22, scope: !369)
!378 = !DILocation(line: 189, column: 29, scope: !369)
!379 = !DILocation(line: 190, column: 25, scope: !369)
!380 = !DILocation(line: 190, column: 28, scope: !369)
!381 = !DILocation(line: 190, column: 23, scope: !369)
!382 = !DILocation(line: 190, column: 18, scope: !369)
!383 = !DILocation(line: 190, column: 21, scope: !369)
!384 = !DILocation(line: 190, column: 10, scope: !369)
!385 = !DILocation(line: 190, column: 5, scope: !369)
!386 = !DILocation(line: 190, column: 8, scope: !369)
!387 = !DILocation(line: 190, column: 16, scope: !369)
!388 = !DILocation(line: 191, column: 9, scope: !389)
!389 = distinct !DILexicalBlock(scope: !369, file: !1, line: 191, column: 9)
!390 = !DILocation(line: 191, column: 12, scope: !389)
!391 = !DILocation(line: 191, column: 19, scope: !389)
!392 = !DILocation(line: 191, column: 22, scope: !389)
!393 = !DILocation(line: 191, column: 29, scope: !389)
!394 = !DILocation(line: 191, column: 33, scope: !389)
!395 = !DILocation(line: 191, column: 36, scope: !389)
!396 = !DILocation(line: 191, column: 48, scope: !389)
!397 = !DILocation(line: 191, column: 43, scope: !389)
!398 = !DILocation(line: 191, column: 46, scope: !389)
!399 = !DILocation(line: 191, column: 9, scope: !369)
!400 = !DILocation(line: 192, column: 19, scope: !389)
!401 = !DILocation(line: 192, column: 22, scope: !389)
!402 = !DILocation(line: 192, column: 9, scope: !389)
!403 = !DILocation(line: 194, column: 24, scope: !389)
!404 = !DILocation(line: 194, column: 27, scope: !389)
!405 = !DILocation(line: 194, column: 9, scope: !389)
!406 = !DILocation(line: 196, column: 5, scope: !369)
!407 = distinct !DISubprogram(name: "pqueue_pop", scope: !1, file: !1, line: 201, type: !408, isLocal: false, isDefinition: true, scopeLine: 202, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!408 = !DISubroutineType(types: !409)
!409 = !{!4, !13}
!410 = !DILocalVariable(name: "q", arg: 1, scope: !407, file: !1, line: 201, type: !13)
!411 = !DILocation(line: 201, column: 22, scope: !407)
!412 = !DILocalVariable(name: "head", scope: !407, file: !1, line: 203, type: !4)
!413 = !DILocation(line: 203, column: 11, scope: !407)
!414 = !DILocation(line: 205, column: 10, scope: !415)
!415 = distinct !DILexicalBlock(scope: !407, file: !1, line: 205, column: 9)
!416 = !DILocation(line: 205, column: 12, scope: !415)
!417 = !DILocation(line: 205, column: 15, scope: !415)
!418 = !DILocation(line: 205, column: 18, scope: !415)
!419 = !DILocation(line: 205, column: 23, scope: !415)
!420 = !DILocation(line: 205, column: 9, scope: !407)
!421 = !DILocation(line: 206, column: 9, scope: !415)
!422 = !DILocation(line: 208, column: 12, scope: !407)
!423 = !DILocation(line: 208, column: 15, scope: !407)
!424 = !DILocation(line: 208, column: 10, scope: !407)
!425 = !DILocation(line: 209, column: 22, scope: !407)
!426 = !DILocation(line: 209, column: 25, scope: !407)
!427 = !DILocation(line: 209, column: 20, scope: !407)
!428 = !DILocation(line: 209, column: 15, scope: !407)
!429 = !DILocation(line: 209, column: 18, scope: !407)
!430 = !DILocation(line: 209, column: 5, scope: !407)
!431 = !DILocation(line: 209, column: 8, scope: !407)
!432 = !DILocation(line: 209, column: 13, scope: !407)
!433 = !DILocation(line: 210, column: 20, scope: !407)
!434 = !DILocation(line: 210, column: 5, scope: !407)
!435 = !DILocation(line: 212, column: 12, scope: !407)
!436 = !DILocation(line: 212, column: 5, scope: !407)
!437 = !DILocation(line: 213, column: 1, scope: !407)
!438 = distinct !DISubprogram(name: "pqueue_peek", scope: !1, file: !1, line: 217, type: !408, isLocal: false, isDefinition: true, scopeLine: 218, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!439 = !DILocalVariable(name: "q", arg: 1, scope: !438, file: !1, line: 217, type: !13)
!440 = !DILocation(line: 217, column: 23, scope: !438)
!441 = !DILocalVariable(name: "d", scope: !438, file: !1, line: 219, type: !4)
!442 = !DILocation(line: 219, column: 11, scope: !438)
!443 = !DILocation(line: 220, column: 10, scope: !444)
!444 = distinct !DILexicalBlock(scope: !438, file: !1, line: 220, column: 9)
!445 = !DILocation(line: 220, column: 12, scope: !444)
!446 = !DILocation(line: 220, column: 15, scope: !444)
!447 = !DILocation(line: 220, column: 18, scope: !444)
!448 = !DILocation(line: 220, column: 23, scope: !444)
!449 = !DILocation(line: 220, column: 9, scope: !438)
!450 = !DILocation(line: 221, column: 9, scope: !444)
!451 = !DILocation(line: 222, column: 9, scope: !438)
!452 = !DILocation(line: 222, column: 12, scope: !438)
!453 = !DILocation(line: 222, column: 7, scope: !438)
!454 = !DILocation(line: 223, column: 12, scope: !438)
!455 = !DILocation(line: 223, column: 5, scope: !438)
!456 = !DILocation(line: 224, column: 1, scope: !438)
!457 = distinct !DISubprogram(name: "pqueue_dump", scope: !1, file: !1, line: 228, type: !458, isLocal: false, isDefinition: true, scopeLine: 231, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!458 = !DISubroutineType(types: !459)
!459 = !{null, !13, !460, !521}
!460 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !461, size: 64, align: 64)
!461 = !DIDerivedType(tag: DW_TAG_typedef, name: "FILE", file: !462, line: 153, baseType: !463)
!462 = !DIFile(filename: "/usr/include/stdio.h", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!463 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "__sFILE", file: !462, line: 122, size: 1216, align: 64, elements: !464)
!464 = !{!465, !468, !469, !470, !472, !473, !478, !479, !480, !484, !490, !499, !505, !506, !509, !510, !514, !518, !519, !520}
!465 = !DIDerivedType(tag: DW_TAG_member, name: "_p", scope: !463, file: !462, line: 123, baseType: !466, size: 64, align: 64)
!466 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !467, size: 64, align: 64)
!467 = !DIBasicType(name: "unsigned char", size: 8, align: 8, encoding: DW_ATE_unsigned_char)
!468 = !DIDerivedType(tag: DW_TAG_member, name: "_r", scope: !463, file: !462, line: 124, baseType: !31, size: 32, align: 32, offset: 64)
!469 = !DIDerivedType(tag: DW_TAG_member, name: "_w", scope: !463, file: !462, line: 125, baseType: !31, size: 32, align: 32, offset: 96)
!470 = !DIDerivedType(tag: DW_TAG_member, name: "_flags", scope: !463, file: !462, line: 126, baseType: !471, size: 16, align: 16, offset: 128)
!471 = !DIBasicType(name: "short", size: 16, align: 16, encoding: DW_ATE_signed)
!472 = !DIDerivedType(tag: DW_TAG_member, name: "_file", scope: !463, file: !462, line: 127, baseType: !471, size: 16, align: 16, offset: 144)
!473 = !DIDerivedType(tag: DW_TAG_member, name: "_bf", scope: !463, file: !462, line: 128, baseType: !474, size: 128, align: 64, offset: 192)
!474 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "__sbuf", file: !462, line: 88, size: 128, align: 64, elements: !475)
!475 = !{!476, !477}
!476 = !DIDerivedType(tag: DW_TAG_member, name: "_base", scope: !474, file: !462, line: 89, baseType: !466, size: 64, align: 64)
!477 = !DIDerivedType(tag: DW_TAG_member, name: "_size", scope: !474, file: !462, line: 90, baseType: !31, size: 32, align: 32, offset: 64)
!478 = !DIDerivedType(tag: DW_TAG_member, name: "_lbfsize", scope: !463, file: !462, line: 129, baseType: !31, size: 32, align: 32, offset: 320)
!479 = !DIDerivedType(tag: DW_TAG_member, name: "_cookie", scope: !463, file: !462, line: 132, baseType: !4, size: 64, align: 64, offset: 384)
!480 = !DIDerivedType(tag: DW_TAG_member, name: "_close", scope: !463, file: !462, line: 133, baseType: !481, size: 64, align: 64, offset: 448)
!481 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !482, size: 64, align: 64)
!482 = !DISubroutineType(types: !483)
!483 = !{!31, !4}
!484 = !DIDerivedType(tag: DW_TAG_member, name: "_read", scope: !463, file: !462, line: 134, baseType: !485, size: 64, align: 64, offset: 512)
!485 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !486, size: 64, align: 64)
!486 = !DISubroutineType(types: !487)
!487 = !{!31, !4, !488, !31}
!488 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !489, size: 64, align: 64)
!489 = !DIBasicType(name: "char", size: 8, align: 8, encoding: DW_ATE_signed_char)
!490 = !DIDerivedType(tag: DW_TAG_member, name: "_seek", scope: !463, file: !462, line: 135, baseType: !491, size: 64, align: 64, offset: 576)
!491 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !492, size: 64, align: 64)
!492 = !DISubroutineType(types: !493)
!493 = !{!494, !4, !494, !31}
!494 = !DIDerivedType(tag: DW_TAG_typedef, name: "fpos_t", file: !462, line: 77, baseType: !495)
!495 = !DIDerivedType(tag: DW_TAG_typedef, name: "__darwin_off_t", file: !496, line: 71, baseType: !497)
!496 = !DIFile(filename: "/usr/include/sys/_types.h", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!497 = !DIDerivedType(tag: DW_TAG_typedef, name: "__int64_t", file: !22, line: 46, baseType: !498)
!498 = !DIBasicType(name: "long long int", size: 64, align: 64, encoding: DW_ATE_signed)
!499 = !DIDerivedType(tag: DW_TAG_member, name: "_write", scope: !463, file: !462, line: 136, baseType: !500, size: 64, align: 64, offset: 640)
!500 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !501, size: 64, align: 64)
!501 = !DISubroutineType(types: !502)
!502 = !{!31, !4, !503, !31}
!503 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !504, size: 64, align: 64)
!504 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !489)
!505 = !DIDerivedType(tag: DW_TAG_member, name: "_ub", scope: !463, file: !462, line: 139, baseType: !474, size: 128, align: 64, offset: 704)
!506 = !DIDerivedType(tag: DW_TAG_member, name: "_extra", scope: !463, file: !462, line: 140, baseType: !507, size: 64, align: 64, offset: 832)
!507 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !508, size: 64, align: 64)
!508 = !DICompositeType(tag: DW_TAG_structure_type, name: "__sFILEX", file: !462, line: 94, flags: DIFlagFwdDecl)
!509 = !DIDerivedType(tag: DW_TAG_member, name: "_ur", scope: !463, file: !462, line: 141, baseType: !31, size: 32, align: 32, offset: 896)
!510 = !DIDerivedType(tag: DW_TAG_member, name: "_ubuf", scope: !463, file: !462, line: 144, baseType: !511, size: 24, align: 8, offset: 928)
!511 = !DICompositeType(tag: DW_TAG_array_type, baseType: !467, size: 24, align: 8, elements: !512)
!512 = !{!513}
!513 = !DISubrange(count: 3)
!514 = !DIDerivedType(tag: DW_TAG_member, name: "_nbuf", scope: !463, file: !462, line: 145, baseType: !515, size: 8, align: 8, offset: 952)
!515 = !DICompositeType(tag: DW_TAG_array_type, baseType: !467, size: 8, align: 8, elements: !516)
!516 = !{!517}
!517 = !DISubrange(count: 1)
!518 = !DIDerivedType(tag: DW_TAG_member, name: "_lb", scope: !463, file: !462, line: 148, baseType: !474, size: 128, align: 64, offset: 960)
!519 = !DIDerivedType(tag: DW_TAG_member, name: "_blksize", scope: !463, file: !462, line: 151, baseType: !31, size: 32, align: 32, offset: 1088)
!520 = !DIDerivedType(tag: DW_TAG_member, name: "_offset", scope: !463, file: !462, line: 152, baseType: !494, size: 64, align: 64, offset: 1152)
!521 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_print_entry_f", file: !15, line: 53, baseType: !522)
!522 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !523, size: 64, align: 64)
!523 = !DISubroutineType(types: !524)
!524 = !{null, !460, !4}
!525 = !DILocalVariable(name: "q", arg: 1, scope: !457, file: !1, line: 228, type: !13)
!526 = !DILocation(line: 228, column: 23, scope: !457)
!527 = !DILocalVariable(name: "out", arg: 2, scope: !457, file: !1, line: 229, type: !460)
!528 = !DILocation(line: 229, column: 19, scope: !457)
!529 = !DILocalVariable(name: "print", arg: 3, scope: !457, file: !1, line: 230, type: !521)
!530 = !DILocation(line: 230, column: 34, scope: !457)
!531 = !DILocalVariable(name: "i", scope: !457, file: !1, line: 232, type: !31)
!532 = !DILocation(line: 232, column: 9, scope: !457)
!533 = !DILocation(line: 234, column: 13, scope: !457)
!534 = !DILocation(line: 234, column: 5, scope: !457)
!535 = !DILocation(line: 235, column: 12, scope: !536)
!536 = distinct !DILexicalBlock(scope: !457, file: !1, line: 235, column: 5)
!537 = !DILocation(line: 235, column: 10, scope: !536)
!538 = !DILocation(line: 235, column: 17, scope: !539)
!539 = distinct !DILexicalBlock(scope: !536, file: !1, line: 235, column: 5)
!540 = !DILocation(line: 235, column: 21, scope: !539)
!541 = !DILocation(line: 235, column: 24, scope: !539)
!542 = !DILocation(line: 235, column: 19, scope: !539)
!543 = !DILocation(line: 235, column: 5, scope: !536)
!544 = !DILocation(line: 236, column: 17, scope: !545)
!545 = distinct !DILexicalBlock(scope: !539, file: !1, line: 235, column: 35)
!546 = !DILocation(line: 238, column: 17, scope: !545)
!547 = !DILocation(line: 239, column: 17, scope: !545)
!548 = !DILocation(line: 239, column: 26, scope: !545)
!549 = !DILocation(line: 239, column: 36, scope: !545)
!550 = !DILocation(line: 240, column: 40, scope: !545)
!551 = !DILocation(line: 240, column: 43, scope: !545)
!552 = !DILocation(line: 240, column: 31, scope: !545)
!553 = !DILocation(line: 240, column: 17, scope: !545)
!554 = !DILocation(line: 236, column: 9, scope: !545)
!555 = !DILocation(line: 241, column: 9, scope: !545)
!556 = !DILocation(line: 241, column: 15, scope: !545)
!557 = !DILocation(line: 241, column: 25, scope: !545)
!558 = !DILocation(line: 241, column: 20, scope: !545)
!559 = !DILocation(line: 241, column: 23, scope: !545)
!560 = !DILocation(line: 242, column: 5, scope: !545)
!561 = !DILocation(line: 235, column: 31, scope: !539)
!562 = !DILocation(line: 235, column: 5, scope: !539)
!563 = distinct !{!563, !564}
!564 = !DILocation(line: 235, column: 5, scope: !457)
!565 = !DILocation(line: 243, column: 1, scope: !457)
!566 = distinct !DISubprogram(name: "maxchild", scope: !1, file: !1, line: 107, type: !567, isLocal: true, isDefinition: true, scopeLine: 108, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!567 = !DISubroutineType(types: !568)
!568 = !{!19, !13, !19}
!569 = !DILocalVariable(name: "q", arg: 1, scope: !566, file: !1, line: 107, type: !13)
!570 = !DILocation(line: 107, column: 20, scope: !566)
!571 = !DILocalVariable(name: "i", arg: 2, scope: !566, file: !1, line: 107, type: !19)
!572 = !DILocation(line: 107, column: 30, scope: !566)
!573 = !DILocalVariable(name: "child_node", scope: !566, file: !1, line: 109, type: !19)
!574 = !DILocation(line: 109, column: 12, scope: !566)
!575 = !DILocation(line: 109, column: 25, scope: !566)
!576 = !DILocation(line: 111, column: 9, scope: !577)
!577 = distinct !DILexicalBlock(scope: !566, file: !1, line: 111, column: 9)
!578 = !DILocation(line: 111, column: 23, scope: !577)
!579 = !DILocation(line: 111, column: 26, scope: !577)
!580 = !DILocation(line: 111, column: 20, scope: !577)
!581 = !DILocation(line: 111, column: 9, scope: !566)
!582 = !DILocation(line: 112, column: 9, scope: !577)
!583 = !DILocation(line: 114, column: 10, scope: !584)
!584 = distinct !DILexicalBlock(scope: !566, file: !1, line: 114, column: 9)
!585 = !DILocation(line: 114, column: 20, scope: !584)
!586 = !DILocation(line: 114, column: 26, scope: !584)
!587 = !DILocation(line: 114, column: 29, scope: !584)
!588 = !DILocation(line: 114, column: 24, scope: !584)
!589 = !DILocation(line: 114, column: 34, scope: !584)
!590 = !DILocation(line: 115, column: 9, scope: !584)
!591 = !DILocation(line: 115, column: 12, scope: !584)
!592 = !DILocation(line: 115, column: 19, scope: !584)
!593 = !DILocation(line: 115, column: 22, scope: !584)
!594 = !DILocation(line: 115, column: 34, scope: !584)
!595 = !DILocation(line: 115, column: 29, scope: !584)
!596 = !DILocation(line: 115, column: 32, scope: !584)
!597 = !DILocation(line: 115, column: 48, scope: !584)
!598 = !DILocation(line: 115, column: 51, scope: !584)
!599 = !DILocation(line: 115, column: 63, scope: !584)
!600 = !DILocation(line: 115, column: 73, scope: !584)
!601 = !DILocation(line: 115, column: 58, scope: !584)
!602 = !DILocation(line: 115, column: 61, scope: !584)
!603 = !DILocation(line: 114, column: 9, scope: !566)
!604 = !DILocation(line: 116, column: 19, scope: !584)
!605 = !DILocation(line: 116, column: 9, scope: !584)
!606 = !DILocation(line: 118, column: 12, scope: !566)
!607 = !DILocation(line: 118, column: 5, scope: !566)
!608 = !DILocation(line: 119, column: 1, scope: !566)
!609 = distinct !DISubprogram(name: "pqueue_print", scope: !1, file: !1, line: 261, type: !458, isLocal: false, isDefinition: true, scopeLine: 264, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!610 = !DILocalVariable(name: "q", arg: 1, scope: !609, file: !1, line: 261, type: !13)
!611 = !DILocation(line: 261, column: 24, scope: !609)
!612 = !DILocalVariable(name: "out", arg: 2, scope: !609, file: !1, line: 262, type: !460)
!613 = !DILocation(line: 262, column: 20, scope: !609)
!614 = !DILocalVariable(name: "print", arg: 3, scope: !609, file: !1, line: 263, type: !521)
!615 = !DILocation(line: 263, column: 35, scope: !609)
!616 = !DILocalVariable(name: "dup", scope: !609, file: !1, line: 265, type: !13)
!617 = !DILocation(line: 265, column: 15, scope: !609)
!618 = !DILocalVariable(name: "e", scope: !609, file: !1, line: 266, type: !4)
!619 = !DILocation(line: 266, column: 8, scope: !609)
!620 = !DILocation(line: 268, column: 23, scope: !609)
!621 = !DILocation(line: 268, column: 26, scope: !609)
!622 = !DILocation(line: 269, column: 23, scope: !609)
!623 = !DILocation(line: 269, column: 26, scope: !609)
!624 = !DILocation(line: 269, column: 34, scope: !609)
!625 = !DILocation(line: 269, column: 37, scope: !609)
!626 = !DILocation(line: 270, column: 23, scope: !609)
!627 = !DILocation(line: 270, column: 26, scope: !609)
!628 = !DILocation(line: 268, column: 11, scope: !609)
!629 = !DILocation(line: 268, column: 9, scope: !609)
!630 = !DILocation(line: 271, column: 17, scope: !609)
!631 = !DILocation(line: 271, column: 20, scope: !609)
!632 = !DILocation(line: 271, column: 5, scope: !609)
!633 = !DILocation(line: 271, column: 10, scope: !609)
!634 = !DILocation(line: 271, column: 15, scope: !609)
!635 = !DILocation(line: 272, column: 18, scope: !609)
!636 = !DILocation(line: 272, column: 21, scope: !609)
!637 = !DILocation(line: 272, column: 5, scope: !609)
!638 = !DILocation(line: 272, column: 10, scope: !609)
!639 = !DILocation(line: 272, column: 16, scope: !609)
!640 = !DILocation(line: 273, column: 17, scope: !609)
!641 = !DILocation(line: 273, column: 20, scope: !609)
!642 = !DILocation(line: 273, column: 5, scope: !609)
!643 = !DILocation(line: 273, column: 10, scope: !609)
!644 = !DILocation(line: 273, column: 15, scope: !609)
!645 = !DILocation(line: 275, column: 5, scope: !609)
!646 = !DILocation(line: 277, column: 5, scope: !609)
!647 = !DILocation(line: 277, column: 28, scope: !609)
!648 = !DILocation(line: 277, column: 17, scope: !609)
!649 = !DILocation(line: 277, column: 15, scope: !609)
!650 = !DILocation(line: 278, column: 3, scope: !609)
!651 = !DILocation(line: 278, column: 9, scope: !609)
!652 = !DILocation(line: 278, column: 14, scope: !609)
!653 = distinct !{!653, !646}
!654 = !DILocation(line: 280, column: 17, scope: !609)
!655 = !DILocation(line: 280, column: 5, scope: !609)
!656 = !DILocation(line: 281, column: 1, scope: !609)
!657 = distinct !DISubprogram(name: "set_pri", scope: !1, file: !1, line: 254, type: !42, isLocal: true, isDefinition: true, scopeLine: 255, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!658 = !DILocalVariable(name: "d", arg: 1, scope: !657, file: !1, line: 254, type: !4)
!659 = !DILocation(line: 254, column: 15, scope: !657)
!660 = !DILocalVariable(name: "pri", arg: 2, scope: !657, file: !1, line: 254, type: !32)
!661 = !DILocation(line: 254, column: 31, scope: !657)
!662 = !DILocation(line: 257, column: 1, scope: !657)
!663 = distinct !DISubprogram(name: "set_pos", scope: !1, file: !1, line: 247, type: !52, isLocal: true, isDefinition: true, scopeLine: 248, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!664 = !DILocalVariable(name: "d", arg: 1, scope: !663, file: !1, line: 247, type: !4)
!665 = !DILocation(line: 247, column: 15, scope: !663)
!666 = !DILocalVariable(name: "val", arg: 2, scope: !663, file: !1, line: 247, type: !19)
!667 = !DILocation(line: 247, column: 25, scope: !663)
!668 = !DILocation(line: 250, column: 1, scope: !663)
!669 = distinct !DISubprogram(name: "pqueue_is_valid", scope: !1, file: !1, line: 306, type: !670, isLocal: false, isDefinition: true, scopeLine: 307, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!670 = !DISubroutineType(types: !671)
!671 = !{!31, !13}
!672 = !DILocalVariable(name: "q", arg: 1, scope: !669, file: !1, line: 306, type: !13)
!673 = !DILocation(line: 306, column: 27, scope: !669)
!674 = !DILocation(line: 308, column: 29, scope: !669)
!675 = !DILocation(line: 308, column: 12, scope: !669)
!676 = !DILocation(line: 308, column: 5, scope: !669)
!677 = distinct !DISubprogram(name: "subtree_is_valid", scope: !1, file: !1, line: 285, type: !678, isLocal: true, isDefinition: true, scopeLine: 286, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!678 = !DISubroutineType(types: !679)
!679 = !{!31, !13, !31}
!680 = !DILocalVariable(name: "q", arg: 1, scope: !677, file: !1, line: 285, type: !13)
!681 = !DILocation(line: 285, column: 28, scope: !677)
!682 = !DILocalVariable(name: "pos", arg: 2, scope: !677, file: !1, line: 285, type: !31)
!683 = !DILocation(line: 285, column: 35, scope: !677)
!684 = !DILocation(line: 287, column: 9, scope: !685)
!685 = distinct !DILexicalBlock(scope: !677, file: !1, line: 287, column: 9)
!686 = !DILocation(line: 287, column: 21, scope: !685)
!687 = !DILocation(line: 287, column: 24, scope: !685)
!688 = !DILocation(line: 287, column: 19, scope: !685)
!689 = !DILocation(line: 287, column: 9, scope: !677)
!690 = !DILocation(line: 289, column: 13, scope: !691)
!691 = distinct !DILexicalBlock(scope: !692, file: !1, line: 289, column: 13)
!692 = distinct !DILexicalBlock(scope: !685, file: !1, line: 287, column: 30)
!693 = !DILocation(line: 289, column: 16, scope: !691)
!694 = !DILocation(line: 289, column: 23, scope: !691)
!695 = !DILocation(line: 289, column: 26, scope: !691)
!696 = !DILocation(line: 289, column: 38, scope: !691)
!697 = !DILocation(line: 289, column: 33, scope: !691)
!698 = !DILocation(line: 289, column: 36, scope: !691)
!699 = !DILocation(line: 289, column: 45, scope: !691)
!700 = !DILocation(line: 289, column: 48, scope: !691)
!701 = !DILocation(line: 289, column: 60, scope: !691)
!702 = !DILocation(line: 289, column: 55, scope: !691)
!703 = !DILocation(line: 289, column: 58, scope: !691)
!704 = !DILocation(line: 289, column: 13, scope: !692)
!705 = !DILocation(line: 290, column: 13, scope: !691)
!706 = !DILocation(line: 291, column: 31, scope: !707)
!707 = distinct !DILexicalBlock(scope: !692, file: !1, line: 291, column: 13)
!708 = !DILocation(line: 291, column: 34, scope: !707)
!709 = !DILocation(line: 291, column: 14, scope: !707)
!710 = !DILocation(line: 291, column: 13, scope: !692)
!711 = !DILocation(line: 292, column: 13, scope: !707)
!712 = !DILocation(line: 293, column: 5, scope: !692)
!713 = !DILocation(line: 294, column: 9, scope: !714)
!714 = distinct !DILexicalBlock(scope: !677, file: !1, line: 294, column: 9)
!715 = !DILocation(line: 294, column: 22, scope: !714)
!716 = !DILocation(line: 294, column: 25, scope: !714)
!717 = !DILocation(line: 294, column: 20, scope: !714)
!718 = !DILocation(line: 294, column: 9, scope: !677)
!719 = !DILocation(line: 296, column: 13, scope: !720)
!720 = distinct !DILexicalBlock(scope: !721, file: !1, line: 296, column: 13)
!721 = distinct !DILexicalBlock(scope: !714, file: !1, line: 294, column: 31)
!722 = !DILocation(line: 296, column: 16, scope: !720)
!723 = !DILocation(line: 296, column: 23, scope: !720)
!724 = !DILocation(line: 296, column: 26, scope: !720)
!725 = !DILocation(line: 296, column: 38, scope: !720)
!726 = !DILocation(line: 296, column: 33, scope: !720)
!727 = !DILocation(line: 296, column: 36, scope: !720)
!728 = !DILocation(line: 296, column: 45, scope: !720)
!729 = !DILocation(line: 296, column: 48, scope: !720)
!730 = !DILocation(line: 296, column: 60, scope: !720)
!731 = !DILocation(line: 296, column: 55, scope: !720)
!732 = !DILocation(line: 296, column: 58, scope: !720)
!733 = !DILocation(line: 296, column: 13, scope: !721)
!734 = !DILocation(line: 297, column: 13, scope: !720)
!735 = !DILocation(line: 298, column: 31, scope: !736)
!736 = distinct !DILexicalBlock(scope: !721, file: !1, line: 298, column: 13)
!737 = !DILocation(line: 298, column: 34, scope: !736)
!738 = !DILocation(line: 298, column: 14, scope: !736)
!739 = !DILocation(line: 298, column: 13, scope: !721)
!740 = !DILocation(line: 299, column: 13, scope: !736)
!741 = !DILocation(line: 300, column: 5, scope: !721)
!742 = !DILocation(line: 301, column: 5, scope: !677)
!743 = !DILocation(line: 302, column: 1, scope: !677)

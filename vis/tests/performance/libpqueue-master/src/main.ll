; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.13.0"

%struct.__sFILE = type { i8*, i32, i32, i16, i16, %struct.__sbuf, i32, i8*, i32 (i8*)*, i32 (i8*, i8*, i32)*, i64 (i8*, i64, i32)*, i32 (i8*, i8*, i32)*, %struct.__sbuf, %struct.__sFILEX*, i32, [3 x i8], [1 x i8], %struct.__sbuf, i32, i64 }
%struct.__sFILEX = type opaque
%struct.__sbuf = type { i8*, i32 }
%struct.pqueue_t = type { i64, i64, i64, i32 (i64, i64)*, i64 (i8*)*, void (i8*, i64)*, i64 (i8*)*, void (i8*, i64)*, i8** }
%struct.node_t = type { i64, i32, i64 }

@__stdoutp = external global %struct.__sFILE*, align 8
@.str = private unnamed_addr constant [37 x i8] c"posn\09left\09right\09parent\09maxchild\09...\0A\00", align 1
@.str.1 = private unnamed_addr constant [17 x i8] c"%d\09%d\09%d\09%d\09%ul\09\00", align 1
@.str.3 = private unnamed_addr constant [17 x i8] c"peek: %lld [%d]\0A\00", align 1
@.str.1.4 = private unnamed_addr constant [16 x i8] c"pop: %lld [%d]\0A\00", align 1

; Function Attrs: nounwind ssp uwtable
define %struct.pqueue_t* @pqueue_init(i64, i32 (i64, i64)*, i64 (i8*)*, void (i8*, i64)*, i64 (i8*)*, void (i8*, i64)*) #0 !dbg !29 {
  %7 = alloca %struct.pqueue_t*, align 8
  %8 = alloca i64, align 8
  %9 = alloca i32 (i64, i64)*, align 8
  %10 = alloca i64 (i8*)*, align 8
  %11 = alloca void (i8*, i64)*, align 8
  %12 = alloca i64 (i8*)*, align 8
  %13 = alloca void (i8*, i64)*, align 8
  %14 = alloca %struct.pqueue_t*, align 8
  store i64 %0, i64* %8, align 8
  call void @llvm.dbg.declare(metadata i64* %8, metadata !66, metadata !67), !dbg !68
  store i32 (i64, i64)* %1, i32 (i64, i64)** %9, align 8
  call void @llvm.dbg.declare(metadata i32 (i64, i64)** %9, metadata !69, metadata !67), !dbg !70
  store i64 (i8*)* %2, i64 (i8*)** %10, align 8
  call void @llvm.dbg.declare(metadata i64 (i8*)** %10, metadata !71, metadata !67), !dbg !72
  store void (i8*, i64)* %3, void (i8*, i64)** %11, align 8
  call void @llvm.dbg.declare(metadata void (i8*, i64)** %11, metadata !73, metadata !67), !dbg !74
  store i64 (i8*)* %4, i64 (i8*)** %12, align 8
  call void @llvm.dbg.declare(metadata i64 (i8*)** %12, metadata !75, metadata !67), !dbg !76
  store void (i8*, i64)* %5, void (i8*, i64)** %13, align 8
  call void @llvm.dbg.declare(metadata void (i8*, i64)** %13, metadata !77, metadata !67), !dbg !78
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %14, metadata !79, metadata !67), !dbg !80
  %15 = call i8* @malloc(i64 72), !dbg !81
  %16 = bitcast i8* %15 to %struct.pqueue_t*, !dbg !81
  store %struct.pqueue_t* %16, %struct.pqueue_t** %14, align 8, !dbg !83
  %17 = icmp ne %struct.pqueue_t* %16, null, !dbg !83
  br i1 %17, label %19, label %18, !dbg !84

; <label>:18:                                     ; preds = %6
  store %struct.pqueue_t* null, %struct.pqueue_t** %7, align 8, !dbg !85
  br label %56, !dbg !85

; <label>:19:                                     ; preds = %6
  %20 = load i64, i64* %8, align 8, !dbg !86
  %21 = add i64 %20, 1, !dbg !88
  %22 = mul i64 %21, 8, !dbg !89
  %23 = call i8* @malloc(i64 %22), !dbg !90
  %24 = bitcast i8* %23 to i8**, !dbg !90
  %25 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !91
  %26 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %25, i32 0, i32 8, !dbg !92
  store i8** %24, i8*** %26, align 8, !dbg !93
  %27 = icmp ne i8** %24, null, !dbg !93
  br i1 %27, label %31, label %28, !dbg !94

; <label>:28:                                     ; preds = %19
  %29 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !95
  %30 = bitcast %struct.pqueue_t* %29 to i8*, !dbg !95
  call void @free(i8* %30), !dbg !97
  store %struct.pqueue_t* null, %struct.pqueue_t** %7, align 8, !dbg !98
  br label %56, !dbg !98

; <label>:31:                                     ; preds = %19
  %32 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !99
  %33 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %32, i32 0, i32 0, !dbg !100
  store i64 1, i64* %33, align 8, !dbg !101
  %34 = load i64, i64* %8, align 8, !dbg !102
  %35 = add i64 %34, 1, !dbg !103
  %36 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !104
  %37 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %36, i32 0, i32 2, !dbg !105
  store i64 %35, i64* %37, align 8, !dbg !106
  %38 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !107
  %39 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %38, i32 0, i32 1, !dbg !108
  store i64 %35, i64* %39, align 8, !dbg !109
  %40 = load i32 (i64, i64)*, i32 (i64, i64)** %9, align 8, !dbg !110
  %41 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !111
  %42 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %41, i32 0, i32 3, !dbg !112
  store i32 (i64, i64)* %40, i32 (i64, i64)** %42, align 8, !dbg !113
  %43 = load void (i8*, i64)*, void (i8*, i64)** %11, align 8, !dbg !114
  %44 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !115
  %45 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %44, i32 0, i32 5, !dbg !116
  store void (i8*, i64)* %43, void (i8*, i64)** %45, align 8, !dbg !117
  %46 = load i64 (i8*)*, i64 (i8*)** %10, align 8, !dbg !118
  %47 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !119
  %48 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %47, i32 0, i32 4, !dbg !120
  store i64 (i8*)* %46, i64 (i8*)** %48, align 8, !dbg !121
  %49 = load i64 (i8*)*, i64 (i8*)** %12, align 8, !dbg !122
  %50 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !123
  %51 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %50, i32 0, i32 6, !dbg !124
  store i64 (i8*)* %49, i64 (i8*)** %51, align 8, !dbg !125
  %52 = load void (i8*, i64)*, void (i8*, i64)** %13, align 8, !dbg !126
  %53 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !127
  %54 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %53, i32 0, i32 7, !dbg !128
  store void (i8*, i64)* %52, void (i8*, i64)** %54, align 8, !dbg !129
  %55 = load %struct.pqueue_t*, %struct.pqueue_t** %14, align 8, !dbg !130
  store %struct.pqueue_t* %55, %struct.pqueue_t** %7, align 8, !dbg !131
  br label %56, !dbg !131

; <label>:56:                                     ; preds = %31, %28, %18
  %57 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !132
  ret %struct.pqueue_t* %57, !dbg !132
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare i8* @malloc(i64) #2

declare void @free(i8*) #2

; Function Attrs: nounwind ssp uwtable
define void @pqueue_free(%struct.pqueue_t*) #0 !dbg !133 {
  %2 = alloca %struct.pqueue_t*, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %2, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %2, metadata !136, metadata !67), !dbg !137
  %3 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !138
  %4 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %3, i32 0, i32 8, !dbg !139
  %5 = load i8**, i8*** %4, align 8, !dbg !139
  %6 = bitcast i8** %5 to i8*, !dbg !138
  call void @free(i8* %6), !dbg !140
  %7 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !141
  %8 = bitcast %struct.pqueue_t* %7 to i8*, !dbg !141
  call void @free(i8* %8), !dbg !142
  ret void, !dbg !143
}

; Function Attrs: nounwind ssp uwtable
define i64 @pqueue_size(%struct.pqueue_t*) #0 !dbg !144 {
  %2 = alloca %struct.pqueue_t*, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %2, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %2, metadata !147, metadata !67), !dbg !148
  %3 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !149
  %4 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %3, i32 0, i32 0, !dbg !150
  %5 = load i64, i64* %4, align 8, !dbg !150
  %6 = sub i64 %5, 1, !dbg !151
  ret i64 %6, !dbg !152
}

; Function Attrs: nounwind ssp uwtable
define i32 @pqueue_insert(%struct.pqueue_t*, i8*) #0 !dbg !153 {
  %3 = alloca i32, align 4
  %4 = alloca %struct.pqueue_t*, align 8
  %5 = alloca i8*, align 8
  %6 = alloca i8*, align 8
  %7 = alloca i64, align 8
  %8 = alloca i64, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %4, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %4, metadata !156, metadata !67), !dbg !157
  store i8* %1, i8** %5, align 8
  call void @llvm.dbg.declare(metadata i8** %5, metadata !158, metadata !67), !dbg !159
  call void @llvm.dbg.declare(metadata i8** %6, metadata !160, metadata !67), !dbg !161
  call void @llvm.dbg.declare(metadata i64* %7, metadata !162, metadata !67), !dbg !163
  call void @llvm.dbg.declare(metadata i64* %8, metadata !164, metadata !67), !dbg !165
  %9 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !166
  %10 = icmp ne %struct.pqueue_t* %9, null, !dbg !166
  br i1 %10, label %12, label %11, !dbg !168

; <label>:11:                                     ; preds = %2
  store i32 1, i32* %3, align 4, !dbg !169
  br label %58, !dbg !169

; <label>:12:                                     ; preds = %2
  %13 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !170
  %14 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %13, i32 0, i32 0, !dbg !172
  %15 = load i64, i64* %14, align 8, !dbg !172
  %16 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !173
  %17 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %16, i32 0, i32 1, !dbg !174
  %18 = load i64, i64* %17, align 8, !dbg !174
  %19 = icmp uge i64 %15, %18, !dbg !175
  br i1 %19, label %20, label %45, !dbg !176

; <label>:20:                                     ; preds = %12
  %21 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !177
  %22 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %21, i32 0, i32 0, !dbg !179
  %23 = load i64, i64* %22, align 8, !dbg !179
  %24 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !180
  %25 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %24, i32 0, i32 2, !dbg !181
  %26 = load i64, i64* %25, align 8, !dbg !181
  %27 = add i64 %23, %26, !dbg !182
  store i64 %27, i64* %8, align 8, !dbg !183
  %28 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !184
  %29 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %28, i32 0, i32 8, !dbg !186
  %30 = load i8**, i8*** %29, align 8, !dbg !186
  %31 = bitcast i8** %30 to i8*, !dbg !184
  %32 = load i64, i64* %8, align 8, !dbg !187
  %33 = mul i64 8, %32, !dbg !188
  %34 = call i8* @realloc(i8* %31, i64 %33), !dbg !189
  store i8* %34, i8** %6, align 8, !dbg !190
  %35 = icmp ne i8* %34, null, !dbg !190
  br i1 %35, label %37, label %36, !dbg !191

; <label>:36:                                     ; preds = %20
  store i32 1, i32* %3, align 4, !dbg !192
  br label %58, !dbg !192

; <label>:37:                                     ; preds = %20
  %38 = load i8*, i8** %6, align 8, !dbg !193
  %39 = bitcast i8* %38 to i8**, !dbg !193
  %40 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !194
  %41 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %40, i32 0, i32 8, !dbg !195
  store i8** %39, i8*** %41, align 8, !dbg !196
  %42 = load i64, i64* %8, align 8, !dbg !197
  %43 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !198
  %44 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %43, i32 0, i32 1, !dbg !199
  store i64 %42, i64* %44, align 8, !dbg !200
  br label %45, !dbg !201

; <label>:45:                                     ; preds = %37, %12
  %46 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !202
  %47 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %46, i32 0, i32 0, !dbg !203
  %48 = load i64, i64* %47, align 8, !dbg !204
  %49 = add i64 %48, 1, !dbg !204
  store i64 %49, i64* %47, align 8, !dbg !204
  store i64 %48, i64* %7, align 8, !dbg !205
  %50 = load i8*, i8** %5, align 8, !dbg !206
  %51 = load i64, i64* %7, align 8, !dbg !207
  %52 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !208
  %53 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %52, i32 0, i32 8, !dbg !209
  %54 = load i8**, i8*** %53, align 8, !dbg !209
  %55 = getelementptr inbounds i8*, i8** %54, i64 %51, !dbg !208
  store i8* %50, i8** %55, align 8, !dbg !210
  %56 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !211
  %57 = load i64, i64* %7, align 8, !dbg !212
  call void @bubble_up(%struct.pqueue_t* %56, i64 %57), !dbg !213
  store i32 0, i32* %3, align 4, !dbg !214
  br label %58, !dbg !214

; <label>:58:                                     ; preds = %45, %36, %11
  %59 = load i32, i32* %3, align 4, !dbg !215
  ret i32 %59, !dbg !215
}

declare i8* @realloc(i8*, i64) #2

; Function Attrs: nounwind ssp uwtable
define internal void @bubble_up(%struct.pqueue_t*, i64) #0 !dbg !216 {
  %3 = alloca %struct.pqueue_t*, align 8
  %4 = alloca i64, align 8
  %5 = alloca i64, align 8
  %6 = alloca i8*, align 8
  %7 = alloca i64, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %3, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %3, metadata !219, metadata !67), !dbg !220
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !221, metadata !67), !dbg !222
  call void @llvm.dbg.declare(metadata i64* %5, metadata !223, metadata !67), !dbg !224
  call void @llvm.dbg.declare(metadata i8** %6, metadata !225, metadata !67), !dbg !226
  %8 = load i64, i64* %4, align 8, !dbg !227
  %9 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !228
  %10 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %9, i32 0, i32 8, !dbg !229
  %11 = load i8**, i8*** %10, align 8, !dbg !229
  %12 = getelementptr inbounds i8*, i8** %11, i64 %8, !dbg !228
  %13 = load i8*, i8** %12, align 8, !dbg !228
  store i8* %13, i8** %6, align 8, !dbg !226
  call void @llvm.dbg.declare(metadata i64* %7, metadata !230, metadata !67), !dbg !231
  %14 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !232
  %15 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %14, i32 0, i32 4, !dbg !233
  %16 = load i64 (i8*)*, i64 (i8*)** %15, align 8, !dbg !233
  %17 = load i8*, i8** %6, align 8, !dbg !234
  %18 = call i64 %16(i8* %17), !dbg !232
  store i64 %18, i64* %7, align 8, !dbg !231
  %19 = load i64, i64* %4, align 8, !dbg !235
  %20 = lshr i64 %19, 1, !dbg !235
  store i64 %20, i64* %5, align 8, !dbg !237
  br label %21, !dbg !238

; <label>:21:                                     ; preds = %65, %2
  %22 = load i64, i64* %4, align 8, !dbg !239
  %23 = icmp ugt i64 %22, 1, !dbg !241
  br i1 %23, label %24, label %41, !dbg !242

; <label>:24:                                     ; preds = %21
  %25 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !243
  %26 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %25, i32 0, i32 3, !dbg !244
  %27 = load i32 (i64, i64)*, i32 (i64, i64)** %26, align 8, !dbg !244
  %28 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !245
  %29 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %28, i32 0, i32 4, !dbg !246
  %30 = load i64 (i8*)*, i64 (i8*)** %29, align 8, !dbg !246
  %31 = load i64, i64* %5, align 8, !dbg !247
  %32 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !248
  %33 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %32, i32 0, i32 8, !dbg !249
  %34 = load i8**, i8*** %33, align 8, !dbg !249
  %35 = getelementptr inbounds i8*, i8** %34, i64 %31, !dbg !248
  %36 = load i8*, i8** %35, align 8, !dbg !248
  %37 = call i64 %30(i8* %36), !dbg !245
  %38 = load i64, i64* %7, align 8, !dbg !250
  %39 = call i32 %27(i64 %37, i64 %38), !dbg !243
  %40 = icmp ne i32 %39, 0, !dbg !242
  br label %41

; <label>:41:                                     ; preds = %24, %21
  %42 = phi i1 [ false, %21 ], [ %40, %24 ]
  br i1 %42, label %43, label %69, !dbg !251

; <label>:43:                                     ; preds = %41
  %44 = load i64, i64* %5, align 8, !dbg !252
  %45 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !254
  %46 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %45, i32 0, i32 8, !dbg !255
  %47 = load i8**, i8*** %46, align 8, !dbg !255
  %48 = getelementptr inbounds i8*, i8** %47, i64 %44, !dbg !254
  %49 = load i8*, i8** %48, align 8, !dbg !254
  %50 = load i64, i64* %4, align 8, !dbg !256
  %51 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !257
  %52 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %51, i32 0, i32 8, !dbg !258
  %53 = load i8**, i8*** %52, align 8, !dbg !258
  %54 = getelementptr inbounds i8*, i8** %53, i64 %50, !dbg !257
  store i8* %49, i8** %54, align 8, !dbg !259
  %55 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !260
  %56 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %55, i32 0, i32 7, !dbg !261
  %57 = load void (i8*, i64)*, void (i8*, i64)** %56, align 8, !dbg !261
  %58 = load i64, i64* %4, align 8, !dbg !262
  %59 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !263
  %60 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %59, i32 0, i32 8, !dbg !264
  %61 = load i8**, i8*** %60, align 8, !dbg !264
  %62 = getelementptr inbounds i8*, i8** %61, i64 %58, !dbg !263
  %63 = load i8*, i8** %62, align 8, !dbg !263
  %64 = load i64, i64* %4, align 8, !dbg !265
  call void %57(i8* %63, i64 %64), !dbg !260
  br label %65, !dbg !266

; <label>:65:                                     ; preds = %43
  %66 = load i64, i64* %5, align 8, !dbg !267
  store i64 %66, i64* %4, align 8, !dbg !268
  %67 = load i64, i64* %4, align 8, !dbg !269
  %68 = lshr i64 %67, 1, !dbg !269
  store i64 %68, i64* %5, align 8, !dbg !270
  br label %21, !dbg !271, !llvm.loop !272

; <label>:69:                                     ; preds = %41
  %70 = load i8*, i8** %6, align 8, !dbg !274
  %71 = load i64, i64* %4, align 8, !dbg !275
  %72 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !276
  %73 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %72, i32 0, i32 8, !dbg !277
  %74 = load i8**, i8*** %73, align 8, !dbg !277
  %75 = getelementptr inbounds i8*, i8** %74, i64 %71, !dbg !276
  store i8* %70, i8** %75, align 8, !dbg !278
  %76 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !279
  %77 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %76, i32 0, i32 7, !dbg !280
  %78 = load void (i8*, i64)*, void (i8*, i64)** %77, align 8, !dbg !280
  %79 = load i8*, i8** %6, align 8, !dbg !281
  %80 = load i64, i64* %4, align 8, !dbg !282
  call void %78(i8* %79, i64 %80), !dbg !279
  ret void, !dbg !283
}

; Function Attrs: nounwind ssp uwtable
define void @pqueue_change_priority(%struct.pqueue_t*, i64, i8*) #0 !dbg !284 {
  %4 = alloca %struct.pqueue_t*, align 8
  %5 = alloca i64, align 8
  %6 = alloca i8*, align 8
  %7 = alloca i64, align 8
  %8 = alloca i64, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %4, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %4, metadata !287, metadata !67), !dbg !288
  store i64 %1, i64* %5, align 8
  call void @llvm.dbg.declare(metadata i64* %5, metadata !289, metadata !67), !dbg !290
  store i8* %2, i8** %6, align 8
  call void @llvm.dbg.declare(metadata i8** %6, metadata !291, metadata !67), !dbg !292
  call void @llvm.dbg.declare(metadata i64* %7, metadata !293, metadata !67), !dbg !294
  call void @llvm.dbg.declare(metadata i64* %8, metadata !295, metadata !67), !dbg !296
  %9 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !297
  %10 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %9, i32 0, i32 4, !dbg !298
  %11 = load i64 (i8*)*, i64 (i8*)** %10, align 8, !dbg !298
  %12 = load i8*, i8** %6, align 8, !dbg !299
  %13 = call i64 %11(i8* %12), !dbg !297
  store i64 %13, i64* %8, align 8, !dbg !296
  %14 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !300
  %15 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %14, i32 0, i32 5, !dbg !301
  %16 = load void (i8*, i64)*, void (i8*, i64)** %15, align 8, !dbg !301
  %17 = load i8*, i8** %6, align 8, !dbg !302
  %18 = load i64, i64* %5, align 8, !dbg !303
  call void %16(i8* %17, i64 %18), !dbg !300
  %19 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !304
  %20 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %19, i32 0, i32 6, !dbg !305
  %21 = load i64 (i8*)*, i64 (i8*)** %20, align 8, !dbg !305
  %22 = load i8*, i8** %6, align 8, !dbg !306
  %23 = call i64 %21(i8* %22), !dbg !304
  store i64 %23, i64* %7, align 8, !dbg !307
  %24 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !308
  %25 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %24, i32 0, i32 3, !dbg !310
  %26 = load i32 (i64, i64)*, i32 (i64, i64)** %25, align 8, !dbg !310
  %27 = load i64, i64* %8, align 8, !dbg !311
  %28 = load i64, i64* %5, align 8, !dbg !312
  %29 = call i32 %26(i64 %27, i64 %28), !dbg !308
  %30 = icmp ne i32 %29, 0, !dbg !308
  br i1 %30, label %31, label %34, !dbg !313

; <label>:31:                                     ; preds = %3
  %32 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !314
  %33 = load i64, i64* %7, align 8, !dbg !315
  call void @bubble_up(%struct.pqueue_t* %32, i64 %33), !dbg !316
  br label %37, !dbg !316

; <label>:34:                                     ; preds = %3
  %35 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !317
  %36 = load i64, i64* %7, align 8, !dbg !318
  call void @percolate_down(%struct.pqueue_t* %35, i64 %36), !dbg !319
  br label %37

; <label>:37:                                     ; preds = %34, %31
  ret void, !dbg !320
}

; Function Attrs: nounwind ssp uwtable
define internal void @percolate_down(%struct.pqueue_t*, i64) #0 !dbg !321 {
  %3 = alloca %struct.pqueue_t*, align 8
  %4 = alloca i64, align 8
  %5 = alloca i64, align 8
  %6 = alloca i8*, align 8
  %7 = alloca i64, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %3, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %3, metadata !322, metadata !67), !dbg !323
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !324, metadata !67), !dbg !325
  call void @llvm.dbg.declare(metadata i64* %5, metadata !326, metadata !67), !dbg !327
  call void @llvm.dbg.declare(metadata i8** %6, metadata !328, metadata !67), !dbg !329
  %8 = load i64, i64* %4, align 8, !dbg !330
  %9 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !331
  %10 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %9, i32 0, i32 8, !dbg !332
  %11 = load i8**, i8*** %10, align 8, !dbg !332
  %12 = getelementptr inbounds i8*, i8** %11, i64 %8, !dbg !331
  %13 = load i8*, i8** %12, align 8, !dbg !331
  store i8* %13, i8** %6, align 8, !dbg !329
  call void @llvm.dbg.declare(metadata i64* %7, metadata !333, metadata !67), !dbg !334
  %14 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !335
  %15 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %14, i32 0, i32 4, !dbg !336
  %16 = load i64 (i8*)*, i64 (i8*)** %15, align 8, !dbg !336
  %17 = load i8*, i8** %6, align 8, !dbg !337
  %18 = call i64 %16(i8* %17), !dbg !335
  store i64 %18, i64* %7, align 8, !dbg !334
  br label %19, !dbg !338

; <label>:19:                                     ; preds = %43, %2
  %20 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !339
  %21 = load i64, i64* %4, align 8, !dbg !340
  %22 = call i64 @maxchild(%struct.pqueue_t* %20, i64 %21), !dbg !341
  store i64 %22, i64* %5, align 8, !dbg !342
  %23 = icmp ne i64 %22, 0, !dbg !342
  br i1 %23, label %24, label %41, !dbg !343

; <label>:24:                                     ; preds = %19
  %25 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !344
  %26 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %25, i32 0, i32 3, !dbg !345
  %27 = load i32 (i64, i64)*, i32 (i64, i64)** %26, align 8, !dbg !345
  %28 = load i64, i64* %7, align 8, !dbg !346
  %29 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !347
  %30 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %29, i32 0, i32 4, !dbg !348
  %31 = load i64 (i8*)*, i64 (i8*)** %30, align 8, !dbg !348
  %32 = load i64, i64* %5, align 8, !dbg !349
  %33 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !350
  %34 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %33, i32 0, i32 8, !dbg !351
  %35 = load i8**, i8*** %34, align 8, !dbg !351
  %36 = getelementptr inbounds i8*, i8** %35, i64 %32, !dbg !350
  %37 = load i8*, i8** %36, align 8, !dbg !350
  %38 = call i64 %31(i8* %37), !dbg !347
  %39 = call i32 %27(i64 %28, i64 %38), !dbg !344
  %40 = icmp ne i32 %39, 0, !dbg !343
  br label %41

; <label>:41:                                     ; preds = %24, %19
  %42 = phi i1 [ false, %19 ], [ %40, %24 ]
  br i1 %42, label %43, label %66, !dbg !338

; <label>:43:                                     ; preds = %41
  %44 = load i64, i64* %5, align 8, !dbg !352
  %45 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !354
  %46 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %45, i32 0, i32 8, !dbg !355
  %47 = load i8**, i8*** %46, align 8, !dbg !355
  %48 = getelementptr inbounds i8*, i8** %47, i64 %44, !dbg !354
  %49 = load i8*, i8** %48, align 8, !dbg !354
  %50 = load i64, i64* %4, align 8, !dbg !356
  %51 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !357
  %52 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %51, i32 0, i32 8, !dbg !358
  %53 = load i8**, i8*** %52, align 8, !dbg !358
  %54 = getelementptr inbounds i8*, i8** %53, i64 %50, !dbg !357
  store i8* %49, i8** %54, align 8, !dbg !359
  %55 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !360
  %56 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %55, i32 0, i32 7, !dbg !361
  %57 = load void (i8*, i64)*, void (i8*, i64)** %56, align 8, !dbg !361
  %58 = load i64, i64* %4, align 8, !dbg !362
  %59 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !363
  %60 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %59, i32 0, i32 8, !dbg !364
  %61 = load i8**, i8*** %60, align 8, !dbg !364
  %62 = getelementptr inbounds i8*, i8** %61, i64 %58, !dbg !363
  %63 = load i8*, i8** %62, align 8, !dbg !363
  %64 = load i64, i64* %4, align 8, !dbg !365
  call void %57(i8* %63, i64 %64), !dbg !360
  %65 = load i64, i64* %5, align 8, !dbg !366
  store i64 %65, i64* %4, align 8, !dbg !367
  br label %19, !dbg !338, !llvm.loop !368

; <label>:66:                                     ; preds = %41
  %67 = load i8*, i8** %6, align 8, !dbg !369
  %68 = load i64, i64* %4, align 8, !dbg !370
  %69 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !371
  %70 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %69, i32 0, i32 8, !dbg !372
  %71 = load i8**, i8*** %70, align 8, !dbg !372
  %72 = getelementptr inbounds i8*, i8** %71, i64 %68, !dbg !371
  store i8* %67, i8** %72, align 8, !dbg !373
  %73 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !374
  %74 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %73, i32 0, i32 7, !dbg !375
  %75 = load void (i8*, i64)*, void (i8*, i64)** %74, align 8, !dbg !375
  %76 = load i8*, i8** %6, align 8, !dbg !376
  %77 = load i64, i64* %4, align 8, !dbg !377
  call void %75(i8* %76, i64 %77), !dbg !374
  ret void, !dbg !378
}

; Function Attrs: nounwind ssp uwtable
define internal i64 @maxchild(%struct.pqueue_t*, i64) #0 !dbg !379 {
  %3 = alloca i64, align 8
  %4 = alloca %struct.pqueue_t*, align 8
  %5 = alloca i64, align 8
  %6 = alloca i64, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %4, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %4, metadata !382, metadata !67), !dbg !383
  store i64 %1, i64* %5, align 8
  call void @llvm.dbg.declare(metadata i64* %5, metadata !384, metadata !67), !dbg !385
  call void @llvm.dbg.declare(metadata i64* %6, metadata !386, metadata !67), !dbg !387
  %7 = load i64, i64* %5, align 8, !dbg !388
  %8 = shl i64 %7, 1, !dbg !388
  store i64 %8, i64* %6, align 8, !dbg !387
  %9 = load i64, i64* %6, align 8, !dbg !389
  %10 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !391
  %11 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %10, i32 0, i32 0, !dbg !392
  %12 = load i64, i64* %11, align 8, !dbg !392
  %13 = icmp uge i64 %9, %12, !dbg !393
  br i1 %13, label %14, label %15, !dbg !394

; <label>:14:                                     ; preds = %2
  store i64 0, i64* %3, align 8, !dbg !395
  br label %54, !dbg !395

; <label>:15:                                     ; preds = %2
  %16 = load i64, i64* %6, align 8, !dbg !396
  %17 = add i64 %16, 1, !dbg !398
  %18 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !399
  %19 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %18, i32 0, i32 0, !dbg !400
  %20 = load i64, i64* %19, align 8, !dbg !400
  %21 = icmp ult i64 %17, %20, !dbg !401
  br i1 %21, label %22, label %52, !dbg !402

; <label>:22:                                     ; preds = %15
  %23 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !403
  %24 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %23, i32 0, i32 3, !dbg !404
  %25 = load i32 (i64, i64)*, i32 (i64, i64)** %24, align 8, !dbg !404
  %26 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !405
  %27 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %26, i32 0, i32 4, !dbg !406
  %28 = load i64 (i8*)*, i64 (i8*)** %27, align 8, !dbg !406
  %29 = load i64, i64* %6, align 8, !dbg !407
  %30 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !408
  %31 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %30, i32 0, i32 8, !dbg !409
  %32 = load i8**, i8*** %31, align 8, !dbg !409
  %33 = getelementptr inbounds i8*, i8** %32, i64 %29, !dbg !408
  %34 = load i8*, i8** %33, align 8, !dbg !408
  %35 = call i64 %28(i8* %34), !dbg !405
  %36 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !410
  %37 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %36, i32 0, i32 4, !dbg !411
  %38 = load i64 (i8*)*, i64 (i8*)** %37, align 8, !dbg !411
  %39 = load i64, i64* %6, align 8, !dbg !412
  %40 = add i64 %39, 1, !dbg !413
  %41 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !414
  %42 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %41, i32 0, i32 8, !dbg !415
  %43 = load i8**, i8*** %42, align 8, !dbg !415
  %44 = getelementptr inbounds i8*, i8** %43, i64 %40, !dbg !414
  %45 = load i8*, i8** %44, align 8, !dbg !414
  %46 = call i64 %38(i8* %45), !dbg !410
  %47 = call i32 %25(i64 %35, i64 %46), !dbg !403
  %48 = icmp ne i32 %47, 0, !dbg !403
  br i1 %48, label %49, label %52, !dbg !416

; <label>:49:                                     ; preds = %22
  %50 = load i64, i64* %6, align 8, !dbg !417
  %51 = add i64 %50, 1, !dbg !417
  store i64 %51, i64* %6, align 8, !dbg !417
  br label %52, !dbg !418

; <label>:52:                                     ; preds = %49, %22, %15
  %53 = load i64, i64* %6, align 8, !dbg !419
  store i64 %53, i64* %3, align 8, !dbg !420
  br label %54, !dbg !420

; <label>:54:                                     ; preds = %52, %14
  %55 = load i64, i64* %3, align 8, !dbg !421
  ret i64 %55, !dbg !421
}

; Function Attrs: nounwind ssp uwtable
define i32 @pqueue_remove(%struct.pqueue_t*, i8*) #0 !dbg !422 {
  %3 = alloca %struct.pqueue_t*, align 8
  %4 = alloca i8*, align 8
  %5 = alloca i64, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %3, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %3, metadata !423, metadata !67), !dbg !424
  store i8* %1, i8** %4, align 8
  call void @llvm.dbg.declare(metadata i8** %4, metadata !425, metadata !67), !dbg !426
  call void @llvm.dbg.declare(metadata i64* %5, metadata !427, metadata !67), !dbg !428
  %6 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !429
  %7 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %6, i32 0, i32 6, !dbg !430
  %8 = load i64 (i8*)*, i64 (i8*)** %7, align 8, !dbg !430
  %9 = load i8*, i8** %4, align 8, !dbg !431
  %10 = call i64 %8(i8* %9), !dbg !429
  store i64 %10, i64* %5, align 8, !dbg !428
  %11 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !432
  %12 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %11, i32 0, i32 0, !dbg !433
  %13 = load i64, i64* %12, align 8, !dbg !434
  %14 = add i64 %13, -1, !dbg !434
  store i64 %14, i64* %12, align 8, !dbg !434
  %15 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !435
  %16 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %15, i32 0, i32 8, !dbg !436
  %17 = load i8**, i8*** %16, align 8, !dbg !436
  %18 = getelementptr inbounds i8*, i8** %17, i64 %14, !dbg !435
  %19 = load i8*, i8** %18, align 8, !dbg !435
  %20 = load i64, i64* %5, align 8, !dbg !437
  %21 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !438
  %22 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %21, i32 0, i32 8, !dbg !439
  %23 = load i8**, i8*** %22, align 8, !dbg !439
  %24 = getelementptr inbounds i8*, i8** %23, i64 %20, !dbg !438
  store i8* %19, i8** %24, align 8, !dbg !440
  %25 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !441
  %26 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %25, i32 0, i32 3, !dbg !443
  %27 = load i32 (i64, i64)*, i32 (i64, i64)** %26, align 8, !dbg !443
  %28 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !444
  %29 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %28, i32 0, i32 4, !dbg !445
  %30 = load i64 (i8*)*, i64 (i8*)** %29, align 8, !dbg !445
  %31 = load i8*, i8** %4, align 8, !dbg !446
  %32 = call i64 %30(i8* %31), !dbg !444
  %33 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !447
  %34 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %33, i32 0, i32 4, !dbg !448
  %35 = load i64 (i8*)*, i64 (i8*)** %34, align 8, !dbg !448
  %36 = load i64, i64* %5, align 8, !dbg !449
  %37 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !450
  %38 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %37, i32 0, i32 8, !dbg !451
  %39 = load i8**, i8*** %38, align 8, !dbg !451
  %40 = getelementptr inbounds i8*, i8** %39, i64 %36, !dbg !450
  %41 = load i8*, i8** %40, align 8, !dbg !450
  %42 = call i64 %35(i8* %41), !dbg !447
  %43 = call i32 %27(i64 %32, i64 %42), !dbg !441
  %44 = icmp ne i32 %43, 0, !dbg !441
  br i1 %44, label %45, label %48, !dbg !452

; <label>:45:                                     ; preds = %2
  %46 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !453
  %47 = load i64, i64* %5, align 8, !dbg !454
  call void @bubble_up(%struct.pqueue_t* %46, i64 %47), !dbg !455
  br label %51, !dbg !455

; <label>:48:                                     ; preds = %2
  %49 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !456
  %50 = load i64, i64* %5, align 8, !dbg !457
  call void @percolate_down(%struct.pqueue_t* %49, i64 %50), !dbg !458
  br label %51

; <label>:51:                                     ; preds = %48, %45
  ret i32 0, !dbg !459
}

; Function Attrs: nounwind ssp uwtable
define i8* @pqueue_pop(%struct.pqueue_t*) #0 !dbg !460 {
  %2 = alloca i8*, align 8
  %3 = alloca %struct.pqueue_t*, align 8
  %4 = alloca i8*, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %3, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %3, metadata !463, metadata !67), !dbg !464
  call void @llvm.dbg.declare(metadata i8** %4, metadata !465, metadata !67), !dbg !466
  %5 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !467
  %6 = icmp ne %struct.pqueue_t* %5, null, !dbg !467
  br i1 %6, label %7, label %12, !dbg !469

; <label>:7:                                      ; preds = %1
  %8 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !470
  %9 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %8, i32 0, i32 0, !dbg !471
  %10 = load i64, i64* %9, align 8, !dbg !471
  %11 = icmp eq i64 %10, 1, !dbg !472
  br i1 %11, label %12, label %13, !dbg !473

; <label>:12:                                     ; preds = %7, %1
  store i8* null, i8** %2, align 8, !dbg !474
  br label %34, !dbg !474

; <label>:13:                                     ; preds = %7
  %14 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !475
  %15 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %14, i32 0, i32 8, !dbg !476
  %16 = load i8**, i8*** %15, align 8, !dbg !476
  %17 = getelementptr inbounds i8*, i8** %16, i64 1, !dbg !475
  %18 = load i8*, i8** %17, align 8, !dbg !475
  store i8* %18, i8** %4, align 8, !dbg !477
  %19 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !478
  %20 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %19, i32 0, i32 0, !dbg !479
  %21 = load i64, i64* %20, align 8, !dbg !480
  %22 = add i64 %21, -1, !dbg !480
  store i64 %22, i64* %20, align 8, !dbg !480
  %23 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !481
  %24 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %23, i32 0, i32 8, !dbg !482
  %25 = load i8**, i8*** %24, align 8, !dbg !482
  %26 = getelementptr inbounds i8*, i8** %25, i64 %22, !dbg !481
  %27 = load i8*, i8** %26, align 8, !dbg !481
  %28 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !483
  %29 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %28, i32 0, i32 8, !dbg !484
  %30 = load i8**, i8*** %29, align 8, !dbg !484
  %31 = getelementptr inbounds i8*, i8** %30, i64 1, !dbg !483
  store i8* %27, i8** %31, align 8, !dbg !485
  %32 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !486
  call void @percolate_down(%struct.pqueue_t* %32, i64 1), !dbg !487
  %33 = load i8*, i8** %4, align 8, !dbg !488
  store i8* %33, i8** %2, align 8, !dbg !489
  br label %34, !dbg !489

; <label>:34:                                     ; preds = %13, %12
  %35 = load i8*, i8** %2, align 8, !dbg !490
  ret i8* %35, !dbg !490
}

; Function Attrs: nounwind ssp uwtable
define i8* @pqueue_peek(%struct.pqueue_t*) #0 !dbg !491 {
  %2 = alloca i8*, align 8
  %3 = alloca %struct.pqueue_t*, align 8
  %4 = alloca i8*, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %3, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %3, metadata !492, metadata !67), !dbg !493
  call void @llvm.dbg.declare(metadata i8** %4, metadata !494, metadata !67), !dbg !495
  %5 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !496
  %6 = icmp ne %struct.pqueue_t* %5, null, !dbg !496
  br i1 %6, label %7, label %12, !dbg !498

; <label>:7:                                      ; preds = %1
  %8 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !499
  %9 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %8, i32 0, i32 0, !dbg !500
  %10 = load i64, i64* %9, align 8, !dbg !500
  %11 = icmp eq i64 %10, 1, !dbg !501
  br i1 %11, label %12, label %13, !dbg !502

; <label>:12:                                     ; preds = %7, %1
  store i8* null, i8** %2, align 8, !dbg !503
  br label %20, !dbg !503

; <label>:13:                                     ; preds = %7
  %14 = load %struct.pqueue_t*, %struct.pqueue_t** %3, align 8, !dbg !504
  %15 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %14, i32 0, i32 8, !dbg !505
  %16 = load i8**, i8*** %15, align 8, !dbg !505
  %17 = getelementptr inbounds i8*, i8** %16, i64 1, !dbg !504
  %18 = load i8*, i8** %17, align 8, !dbg !504
  store i8* %18, i8** %4, align 8, !dbg !506
  %19 = load i8*, i8** %4, align 8, !dbg !507
  store i8* %19, i8** %2, align 8, !dbg !508
  br label %20, !dbg !508

; <label>:20:                                     ; preds = %13, %12
  %21 = load i8*, i8** %2, align 8, !dbg !509
  ret i8* %21, !dbg !509
}

; Function Attrs: nounwind ssp uwtable
define void @pqueue_dump(%struct.pqueue_t*, %struct.__sFILE*, void (%struct.__sFILE*, i8*)*) #0 !dbg !510 {
  %4 = alloca %struct.pqueue_t*, align 8
  %5 = alloca %struct.__sFILE*, align 8
  %6 = alloca void (%struct.__sFILE*, i8*)*, align 8
  %7 = alloca i32, align 4
  store %struct.pqueue_t* %0, %struct.pqueue_t** %4, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %4, metadata !578, metadata !67), !dbg !579
  store %struct.__sFILE* %1, %struct.__sFILE** %5, align 8
  call void @llvm.dbg.declare(metadata %struct.__sFILE** %5, metadata !580, metadata !67), !dbg !581
  store void (%struct.__sFILE*, i8*)* %2, void (%struct.__sFILE*, i8*)** %6, align 8
  call void @llvm.dbg.declare(metadata void (%struct.__sFILE*, i8*)** %6, metadata !582, metadata !67), !dbg !583
  call void @llvm.dbg.declare(metadata i32* %7, metadata !584, metadata !67), !dbg !585
  %8 = load %struct.__sFILE*, %struct.__sFILE** @__stdoutp, align 8, !dbg !586
  %9 = call i32 (%struct.__sFILE*, i8*, ...) @fprintf(%struct.__sFILE* %8, i8* getelementptr inbounds ([37 x i8], [37 x i8]* @.str, i32 0, i32 0)), !dbg !587
  store i32 1, i32* %7, align 4, !dbg !588
  br label %10, !dbg !590

; <label>:10:                                     ; preds = %42, %3
  %11 = load i32, i32* %7, align 4, !dbg !591
  %12 = sext i32 %11 to i64, !dbg !591
  %13 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !593
  %14 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %13, i32 0, i32 0, !dbg !594
  %15 = load i64, i64* %14, align 8, !dbg !594
  %16 = icmp ult i64 %12, %15, !dbg !595
  br i1 %16, label %17, label %45, !dbg !596

; <label>:17:                                     ; preds = %10
  %18 = load %struct.__sFILE*, %struct.__sFILE** @__stdoutp, align 8, !dbg !597
  %19 = load i32, i32* %7, align 4, !dbg !599
  %20 = load i32, i32* %7, align 4, !dbg !600
  %21 = shl i32 %20, 1, !dbg !600
  %22 = load i32, i32* %7, align 4, !dbg !601
  %23 = shl i32 %22, 1, !dbg !601
  %24 = add nsw i32 %23, 1, !dbg !601
  %25 = load i32, i32* %7, align 4, !dbg !602
  %26 = ashr i32 %25, 1, !dbg !602
  %27 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !603
  %28 = load i32, i32* %7, align 4, !dbg !604
  %29 = sext i32 %28 to i64, !dbg !604
  %30 = call i64 @maxchild(%struct.pqueue_t* %27, i64 %29), !dbg !605
  %31 = trunc i64 %30 to i32, !dbg !606
  %32 = call i32 (%struct.__sFILE*, i8*, ...) @fprintf(%struct.__sFILE* %18, i8* getelementptr inbounds ([17 x i8], [17 x i8]* @.str.1, i32 0, i32 0), i32 %19, i32 %21, i32 %24, i32 %26, i32 %31), !dbg !607
  %33 = load void (%struct.__sFILE*, i8*)*, void (%struct.__sFILE*, i8*)** %6, align 8, !dbg !608
  %34 = load %struct.__sFILE*, %struct.__sFILE** %5, align 8, !dbg !609
  %35 = load i32, i32* %7, align 4, !dbg !610
  %36 = sext i32 %35 to i64, !dbg !611
  %37 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !611
  %38 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %37, i32 0, i32 8, !dbg !612
  %39 = load i8**, i8*** %38, align 8, !dbg !612
  %40 = getelementptr inbounds i8*, i8** %39, i64 %36, !dbg !611
  %41 = load i8*, i8** %40, align 8, !dbg !611
  call void %33(%struct.__sFILE* %34, i8* %41), !dbg !608
  br label %42, !dbg !613

; <label>:42:                                     ; preds = %17
  %43 = load i32, i32* %7, align 4, !dbg !614
  %44 = add nsw i32 %43, 1, !dbg !614
  store i32 %44, i32* %7, align 4, !dbg !614
  br label %10, !dbg !615, !llvm.loop !616

; <label>:45:                                     ; preds = %10
  ret void, !dbg !618
}

declare i32 @fprintf(%struct.__sFILE*, i8*, ...) #2

; Function Attrs: nounwind ssp uwtable
define void @pqueue_print(%struct.pqueue_t*, %struct.__sFILE*, void (%struct.__sFILE*, i8*)*) #0 !dbg !619 {
  %4 = alloca %struct.pqueue_t*, align 8
  %5 = alloca %struct.__sFILE*, align 8
  %6 = alloca void (%struct.__sFILE*, i8*)*, align 8
  %7 = alloca %struct.pqueue_t*, align 8
  %8 = alloca i8*, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %4, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %4, metadata !620, metadata !67), !dbg !621
  store %struct.__sFILE* %1, %struct.__sFILE** %5, align 8
  call void @llvm.dbg.declare(metadata %struct.__sFILE** %5, metadata !622, metadata !67), !dbg !623
  store void (%struct.__sFILE*, i8*)* %2, void (%struct.__sFILE*, i8*)** %6, align 8
  call void @llvm.dbg.declare(metadata void (%struct.__sFILE*, i8*)** %6, metadata !624, metadata !67), !dbg !625
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %7, metadata !626, metadata !67), !dbg !627
  call void @llvm.dbg.declare(metadata i8** %8, metadata !628, metadata !67), !dbg !629
  %9 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !630
  %10 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %9, i32 0, i32 0, !dbg !631
  %11 = load i64, i64* %10, align 8, !dbg !631
  %12 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !632
  %13 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %12, i32 0, i32 3, !dbg !633
  %14 = load i32 (i64, i64)*, i32 (i64, i64)** %13, align 8, !dbg !633
  %15 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !634
  %16 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %15, i32 0, i32 4, !dbg !635
  %17 = load i64 (i8*)*, i64 (i8*)** %16, align 8, !dbg !635
  %18 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !636
  %19 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %18, i32 0, i32 6, !dbg !637
  %20 = load i64 (i8*)*, i64 (i8*)** %19, align 8, !dbg !637
  %21 = call %struct.pqueue_t* @pqueue_init(i64 %11, i32 (i64, i64)* %14, i64 (i8*)* %17, void (i8*, i64)* @set_pri, i64 (i8*)* %20, void (i8*, i64)* @set_pos), !dbg !638
  store %struct.pqueue_t* %21, %struct.pqueue_t** %7, align 8, !dbg !639
  %22 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !640
  %23 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %22, i32 0, i32 0, !dbg !641
  %24 = load i64, i64* %23, align 8, !dbg !641
  %25 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !642
  %26 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %25, i32 0, i32 0, !dbg !643
  store i64 %24, i64* %26, align 8, !dbg !644
  %27 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !645
  %28 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %27, i32 0, i32 1, !dbg !646
  %29 = load i64, i64* %28, align 8, !dbg !646
  %30 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !647
  %31 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %30, i32 0, i32 1, !dbg !648
  store i64 %29, i64* %31, align 8, !dbg !649
  %32 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !650
  %33 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %32, i32 0, i32 2, !dbg !651
  %34 = load i64, i64* %33, align 8, !dbg !651
  %35 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !652
  %36 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %35, i32 0, i32 2, !dbg !653
  store i64 %34, i64* %36, align 8, !dbg !654
  %37 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !655
  %38 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %37, i32 0, i32 8, !dbg !655
  %39 = load i8**, i8*** %38, align 8, !dbg !655
  %40 = bitcast i8** %39 to i8*, !dbg !655
  %41 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !655
  %42 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %41, i32 0, i32 8, !dbg !655
  %43 = load i8**, i8*** %42, align 8, !dbg !655
  %44 = bitcast i8** %43 to i8*, !dbg !655
  %45 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !655
  %46 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %45, i32 0, i32 0, !dbg !655
  %47 = load i64, i64* %46, align 8, !dbg !655
  %48 = mul i64 %47, 8, !dbg !655
  %49 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !655
  %50 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %49, i32 0, i32 8, !dbg !655
  %51 = load i8**, i8*** %50, align 8, !dbg !655
  %52 = bitcast i8** %51 to i8*, !dbg !655
  %53 = call i64 @llvm.objectsize.i64.p0i8(i8* %52, i1 false), !dbg !655
  %54 = call i8* @__memcpy_chk(i8* %40, i8* %44, i64 %48, i64 %53) #4, !dbg !655
  br label %55, !dbg !656

; <label>:55:                                     ; preds = %59, %3
  %56 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !657
  %57 = call i8* @pqueue_pop(%struct.pqueue_t* %56), !dbg !658
  store i8* %57, i8** %8, align 8, !dbg !659
  %58 = icmp ne i8* %57, null, !dbg !656
  br i1 %58, label %59, label %63, !dbg !656

; <label>:59:                                     ; preds = %55
  %60 = load void (%struct.__sFILE*, i8*)*, void (%struct.__sFILE*, i8*)** %6, align 8, !dbg !660
  %61 = load %struct.__sFILE*, %struct.__sFILE** %5, align 8, !dbg !661
  %62 = load i8*, i8** %8, align 8, !dbg !662
  call void %60(%struct.__sFILE* %61, i8* %62), !dbg !660
  br label %55, !dbg !656, !llvm.loop !663

; <label>:63:                                     ; preds = %55
  %64 = load %struct.pqueue_t*, %struct.pqueue_t** %7, align 8, !dbg !664
  call void @pqueue_free(%struct.pqueue_t* %64), !dbg !665
  ret void, !dbg !666
}

; Function Attrs: nounwind ssp uwtable
define internal void @set_pri(i8*, i64) #0 !dbg !667 {
  %3 = alloca i8*, align 8
  %4 = alloca i64, align 8
  store i8* %0, i8** %3, align 8
  call void @llvm.dbg.declare(metadata i8** %3, metadata !668, metadata !67), !dbg !669
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !670, metadata !67), !dbg !671
  ret void, !dbg !672
}

; Function Attrs: nounwind ssp uwtable
define internal void @set_pos(i8*, i64) #0 !dbg !673 {
  %3 = alloca i8*, align 8
  %4 = alloca i64, align 8
  store i8* %0, i8** %3, align 8
  call void @llvm.dbg.declare(metadata i8** %3, metadata !674, metadata !67), !dbg !675
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !676, metadata !67), !dbg !677
  ret void, !dbg !678
}

; Function Attrs: nounwind readnone
declare i64 @llvm.objectsize.i64.p0i8(i8*, i1) #1

; Function Attrs: nounwind
declare i8* @__memcpy_chk(i8*, i8*, i64, i64) #3

; Function Attrs: nounwind ssp uwtable
define i32 @pqueue_is_valid(%struct.pqueue_t*) #0 !dbg !679 {
  %2 = alloca %struct.pqueue_t*, align 8
  store %struct.pqueue_t* %0, %struct.pqueue_t** %2, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %2, metadata !682, metadata !67), !dbg !683
  %3 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !684
  %4 = call i32 @subtree_is_valid(%struct.pqueue_t* %3, i32 1), !dbg !685
  ret i32 %4, !dbg !686
}

; Function Attrs: nounwind ssp uwtable
define internal i32 @subtree_is_valid(%struct.pqueue_t*, i32) #0 !dbg !687 {
  %3 = alloca i32, align 4
  %4 = alloca %struct.pqueue_t*, align 8
  %5 = alloca i32, align 4
  store %struct.pqueue_t* %0, %struct.pqueue_t** %4, align 8
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %4, metadata !690, metadata !67), !dbg !691
  store i32 %1, i32* %5, align 4
  call void @llvm.dbg.declare(metadata i32* %5, metadata !692, metadata !67), !dbg !693
  %6 = load i32, i32* %5, align 4, !dbg !694
  %7 = shl i32 %6, 1, !dbg !694
  %8 = sext i32 %7 to i64, !dbg !694
  %9 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !696
  %10 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %9, i32 0, i32 0, !dbg !697
  %11 = load i64, i64* %10, align 8, !dbg !697
  %12 = icmp ult i64 %8, %11, !dbg !698
  br i1 %12, label %13, label %51, !dbg !699

; <label>:13:                                     ; preds = %2
  %14 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !700
  %15 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %14, i32 0, i32 3, !dbg !703
  %16 = load i32 (i64, i64)*, i32 (i64, i64)** %15, align 8, !dbg !703
  %17 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !704
  %18 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %17, i32 0, i32 4, !dbg !705
  %19 = load i64 (i8*)*, i64 (i8*)** %18, align 8, !dbg !705
  %20 = load i32, i32* %5, align 4, !dbg !706
  %21 = sext i32 %20 to i64, !dbg !707
  %22 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !707
  %23 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %22, i32 0, i32 8, !dbg !708
  %24 = load i8**, i8*** %23, align 8, !dbg !708
  %25 = getelementptr inbounds i8*, i8** %24, i64 %21, !dbg !707
  %26 = load i8*, i8** %25, align 8, !dbg !707
  %27 = call i64 %19(i8* %26), !dbg !704
  %28 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !709
  %29 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %28, i32 0, i32 4, !dbg !710
  %30 = load i64 (i8*)*, i64 (i8*)** %29, align 8, !dbg !710
  %31 = load i32, i32* %5, align 4, !dbg !711
  %32 = shl i32 %31, 1, !dbg !711
  %33 = sext i32 %32 to i64, !dbg !712
  %34 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !712
  %35 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %34, i32 0, i32 8, !dbg !713
  %36 = load i8**, i8*** %35, align 8, !dbg !713
  %37 = getelementptr inbounds i8*, i8** %36, i64 %33, !dbg !712
  %38 = load i8*, i8** %37, align 8, !dbg !712
  %39 = call i64 %30(i8* %38), !dbg !709
  %40 = call i32 %16(i64 %27, i64 %39), !dbg !700
  %41 = icmp ne i32 %40, 0, !dbg !700
  br i1 %41, label %42, label %43, !dbg !714

; <label>:42:                                     ; preds = %13
  store i32 0, i32* %3, align 4, !dbg !715
  br label %101, !dbg !715

; <label>:43:                                     ; preds = %13
  %44 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !716
  %45 = load i32, i32* %5, align 4, !dbg !718
  %46 = shl i32 %45, 1, !dbg !718
  %47 = call i32 @subtree_is_valid(%struct.pqueue_t* %44, i32 %46), !dbg !719
  %48 = icmp ne i32 %47, 0, !dbg !719
  br i1 %48, label %50, label %49, !dbg !720

; <label>:49:                                     ; preds = %43
  store i32 0, i32* %3, align 4, !dbg !721
  br label %101, !dbg !721

; <label>:50:                                     ; preds = %43
  br label %51, !dbg !722

; <label>:51:                                     ; preds = %50, %2
  %52 = load i32, i32* %5, align 4, !dbg !723
  %53 = shl i32 %52, 1, !dbg !723
  %54 = add nsw i32 %53, 1, !dbg !723
  %55 = sext i32 %54 to i64, !dbg !723
  %56 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !725
  %57 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %56, i32 0, i32 0, !dbg !726
  %58 = load i64, i64* %57, align 8, !dbg !726
  %59 = icmp ult i64 %55, %58, !dbg !727
  br i1 %59, label %60, label %100, !dbg !728

; <label>:60:                                     ; preds = %51
  %61 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !729
  %62 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %61, i32 0, i32 3, !dbg !732
  %63 = load i32 (i64, i64)*, i32 (i64, i64)** %62, align 8, !dbg !732
  %64 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !733
  %65 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %64, i32 0, i32 4, !dbg !734
  %66 = load i64 (i8*)*, i64 (i8*)** %65, align 8, !dbg !734
  %67 = load i32, i32* %5, align 4, !dbg !735
  %68 = sext i32 %67 to i64, !dbg !736
  %69 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !736
  %70 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %69, i32 0, i32 8, !dbg !737
  %71 = load i8**, i8*** %70, align 8, !dbg !737
  %72 = getelementptr inbounds i8*, i8** %71, i64 %68, !dbg !736
  %73 = load i8*, i8** %72, align 8, !dbg !736
  %74 = call i64 %66(i8* %73), !dbg !733
  %75 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !738
  %76 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %75, i32 0, i32 4, !dbg !739
  %77 = load i64 (i8*)*, i64 (i8*)** %76, align 8, !dbg !739
  %78 = load i32, i32* %5, align 4, !dbg !740
  %79 = shl i32 %78, 1, !dbg !740
  %80 = add nsw i32 %79, 1, !dbg !740
  %81 = sext i32 %80 to i64, !dbg !741
  %82 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !741
  %83 = getelementptr inbounds %struct.pqueue_t, %struct.pqueue_t* %82, i32 0, i32 8, !dbg !742
  %84 = load i8**, i8*** %83, align 8, !dbg !742
  %85 = getelementptr inbounds i8*, i8** %84, i64 %81, !dbg !741
  %86 = load i8*, i8** %85, align 8, !dbg !741
  %87 = call i64 %77(i8* %86), !dbg !738
  %88 = call i32 %63(i64 %74, i64 %87), !dbg !729
  %89 = icmp ne i32 %88, 0, !dbg !729
  br i1 %89, label %90, label %91, !dbg !743

; <label>:90:                                     ; preds = %60
  store i32 0, i32* %3, align 4, !dbg !744
  br label %101, !dbg !744

; <label>:91:                                     ; preds = %60
  %92 = load %struct.pqueue_t*, %struct.pqueue_t** %4, align 8, !dbg !745
  %93 = load i32, i32* %5, align 4, !dbg !747
  %94 = shl i32 %93, 1, !dbg !747
  %95 = add nsw i32 %94, 1, !dbg !747
  %96 = call i32 @subtree_is_valid(%struct.pqueue_t* %92, i32 %95), !dbg !748
  %97 = icmp ne i32 %96, 0, !dbg !748
  br i1 %97, label %99, label %98, !dbg !749

; <label>:98:                                     ; preds = %91
  store i32 0, i32* %3, align 4, !dbg !750
  br label %101, !dbg !750

; <label>:99:                                     ; preds = %91
  br label %100, !dbg !751

; <label>:100:                                    ; preds = %99, %51
  store i32 1, i32* %3, align 4, !dbg !752
  br label %101, !dbg !752

; <label>:101:                                    ; preds = %100, %98, %90, %49, %42
  %102 = load i32, i32* %3, align 4, !dbg !753
  ret i32 %102, !dbg !753
}

; Function Attrs: nounwind ssp uwtable
define i32 @main() #0 !dbg !754 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.pqueue_t*, align 8
  %3 = alloca %struct.node_t*, align 8
  %4 = alloca %struct.node_t*, align 8
  store i32 0, i32* %1, align 4
  call void @llvm.dbg.declare(metadata %struct.pqueue_t** %2, metadata !757, metadata !67), !dbg !771
  call void @llvm.dbg.declare(metadata %struct.node_t** %3, metadata !772, metadata !67), !dbg !773
  call void @llvm.dbg.declare(metadata %struct.node_t** %4, metadata !774, metadata !67), !dbg !775
  %5 = call i8* @malloc(i64 240), !dbg !776
  %6 = bitcast i8* %5 to %struct.node_t*, !dbg !776
  store %struct.node_t* %6, %struct.node_t** %3, align 8, !dbg !777
  %7 = call %struct.pqueue_t* @pqueue_init(i64 10, i32 (i64, i64)* @cmp_pri, i64 (i8*)* @get_pri, void (i8*, i64)* @set_pri.1, i64 (i8*)* @get_pos, void (i8*, i64)* @set_pos.2), !dbg !778
  store %struct.pqueue_t* %7, %struct.pqueue_t** %2, align 8, !dbg !779
  %8 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !780
  %9 = icmp ne %struct.node_t* %8, null, !dbg !780
  br i1 %9, label %10, label %13, !dbg !782

; <label>:10:                                     ; preds = %0
  %11 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !783
  %12 = icmp ne %struct.pqueue_t* %11, null, !dbg !783
  br i1 %12, label %14, label %13, !dbg !784

; <label>:13:                                     ; preds = %10, %0
  store i32 1, i32* %1, align 4, !dbg !785
  br label %105, !dbg !785

; <label>:14:                                     ; preds = %10
  %15 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !786
  %16 = getelementptr inbounds %struct.node_t, %struct.node_t* %15, i64 0, !dbg !786
  %17 = getelementptr inbounds %struct.node_t, %struct.node_t* %16, i32 0, i32 0, !dbg !787
  store i64 5, i64* %17, align 8, !dbg !788
  %18 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !789
  %19 = getelementptr inbounds %struct.node_t, %struct.node_t* %18, i64 0, !dbg !789
  %20 = getelementptr inbounds %struct.node_t, %struct.node_t* %19, i32 0, i32 1, !dbg !790
  store i32 -5, i32* %20, align 8, !dbg !791
  %21 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !792
  %22 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !793
  %23 = getelementptr inbounds %struct.node_t, %struct.node_t* %22, i64 0, !dbg !793
  %24 = bitcast %struct.node_t* %23 to i8*, !dbg !794
  %25 = call i32 @pqueue_insert(%struct.pqueue_t* %21, i8* %24), !dbg !795
  %26 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !796
  %27 = getelementptr inbounds %struct.node_t, %struct.node_t* %26, i64 1, !dbg !796
  %28 = getelementptr inbounds %struct.node_t, %struct.node_t* %27, i32 0, i32 0, !dbg !797
  store i64 4, i64* %28, align 8, !dbg !798
  %29 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !799
  %30 = getelementptr inbounds %struct.node_t, %struct.node_t* %29, i64 1, !dbg !799
  %31 = getelementptr inbounds %struct.node_t, %struct.node_t* %30, i32 0, i32 1, !dbg !800
  store i32 -4, i32* %31, align 8, !dbg !801
  %32 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !802
  %33 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !803
  %34 = getelementptr inbounds %struct.node_t, %struct.node_t* %33, i64 1, !dbg !803
  %35 = bitcast %struct.node_t* %34 to i8*, !dbg !804
  %36 = call i32 @pqueue_insert(%struct.pqueue_t* %32, i8* %35), !dbg !805
  %37 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !806
  %38 = getelementptr inbounds %struct.node_t, %struct.node_t* %37, i64 2, !dbg !806
  %39 = getelementptr inbounds %struct.node_t, %struct.node_t* %38, i32 0, i32 0, !dbg !807
  store i64 2, i64* %39, align 8, !dbg !808
  %40 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !809
  %41 = getelementptr inbounds %struct.node_t, %struct.node_t* %40, i64 2, !dbg !809
  %42 = getelementptr inbounds %struct.node_t, %struct.node_t* %41, i32 0, i32 1, !dbg !810
  store i32 -2, i32* %42, align 8, !dbg !811
  %43 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !812
  %44 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !813
  %45 = getelementptr inbounds %struct.node_t, %struct.node_t* %44, i64 2, !dbg !813
  %46 = bitcast %struct.node_t* %45 to i8*, !dbg !814
  %47 = call i32 @pqueue_insert(%struct.pqueue_t* %43, i8* %46), !dbg !815
  %48 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !816
  %49 = getelementptr inbounds %struct.node_t, %struct.node_t* %48, i64 3, !dbg !816
  %50 = getelementptr inbounds %struct.node_t, %struct.node_t* %49, i32 0, i32 0, !dbg !817
  store i64 6, i64* %50, align 8, !dbg !818
  %51 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !819
  %52 = getelementptr inbounds %struct.node_t, %struct.node_t* %51, i64 3, !dbg !819
  %53 = getelementptr inbounds %struct.node_t, %struct.node_t* %52, i32 0, i32 1, !dbg !820
  store i32 -6, i32* %53, align 8, !dbg !821
  %54 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !822
  %55 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !823
  %56 = getelementptr inbounds %struct.node_t, %struct.node_t* %55, i64 3, !dbg !823
  %57 = bitcast %struct.node_t* %56 to i8*, !dbg !824
  %58 = call i32 @pqueue_insert(%struct.pqueue_t* %54, i8* %57), !dbg !825
  %59 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !826
  %60 = getelementptr inbounds %struct.node_t, %struct.node_t* %59, i64 4, !dbg !826
  %61 = getelementptr inbounds %struct.node_t, %struct.node_t* %60, i32 0, i32 0, !dbg !827
  store i64 1, i64* %61, align 8, !dbg !828
  %62 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !829
  %63 = getelementptr inbounds %struct.node_t, %struct.node_t* %62, i64 4, !dbg !829
  %64 = getelementptr inbounds %struct.node_t, %struct.node_t* %63, i32 0, i32 1, !dbg !830
  store i32 -1, i32* %64, align 8, !dbg !831
  %65 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !832
  %66 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !833
  %67 = getelementptr inbounds %struct.node_t, %struct.node_t* %66, i64 4, !dbg !833
  %68 = bitcast %struct.node_t* %67 to i8*, !dbg !834
  %69 = call i32 @pqueue_insert(%struct.pqueue_t* %65, i8* %68), !dbg !835
  %70 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !836
  %71 = call i8* @pqueue_peek(%struct.pqueue_t* %70), !dbg !837
  %72 = bitcast i8* %71 to %struct.node_t*, !dbg !837
  store %struct.node_t* %72, %struct.node_t** %4, align 8, !dbg !838
  %73 = load %struct.node_t*, %struct.node_t** %4, align 8, !dbg !839
  %74 = getelementptr inbounds %struct.node_t, %struct.node_t* %73, i32 0, i32 0, !dbg !840
  %75 = load i64, i64* %74, align 8, !dbg !840
  %76 = load %struct.node_t*, %struct.node_t** %4, align 8, !dbg !841
  %77 = getelementptr inbounds %struct.node_t, %struct.node_t* %76, i32 0, i32 1, !dbg !842
  %78 = load i32, i32* %77, align 8, !dbg !842
  %79 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @.str.3, i32 0, i32 0), i64 %75, i32 %78), !dbg !843
  %80 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !844
  %81 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !845
  %82 = getelementptr inbounds %struct.node_t, %struct.node_t* %81, i64 4, !dbg !845
  %83 = bitcast %struct.node_t* %82 to i8*, !dbg !846
  call void @pqueue_change_priority(%struct.pqueue_t* %80, i64 8, i8* %83), !dbg !847
  %84 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !848
  %85 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !849
  %86 = getelementptr inbounds %struct.node_t, %struct.node_t* %85, i64 2, !dbg !849
  %87 = bitcast %struct.node_t* %86 to i8*, !dbg !850
  call void @pqueue_change_priority(%struct.pqueue_t* %84, i64 7, i8* %87), !dbg !851
  br label %88, !dbg !852

; <label>:88:                                     ; preds = %93, %14
  %89 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !853
  %90 = call i8* @pqueue_pop(%struct.pqueue_t* %89), !dbg !854
  %91 = bitcast i8* %90 to %struct.node_t*, !dbg !854
  store %struct.node_t* %91, %struct.node_t** %4, align 8, !dbg !855
  %92 = icmp ne %struct.node_t* %91, null, !dbg !852
  br i1 %92, label %93, label %101, !dbg !852

; <label>:93:                                     ; preds = %88
  %94 = load %struct.node_t*, %struct.node_t** %4, align 8, !dbg !856
  %95 = getelementptr inbounds %struct.node_t, %struct.node_t* %94, i32 0, i32 0, !dbg !857
  %96 = load i64, i64* %95, align 8, !dbg !857
  %97 = load %struct.node_t*, %struct.node_t** %4, align 8, !dbg !858
  %98 = getelementptr inbounds %struct.node_t, %struct.node_t* %97, i32 0, i32 1, !dbg !859
  %99 = load i32, i32* %98, align 8, !dbg !859
  %100 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([16 x i8], [16 x i8]* @.str.1.4, i32 0, i32 0), i64 %96, i32 %99), !dbg !860
  br label %88, !dbg !852, !llvm.loop !861

; <label>:101:                                    ; preds = %88
  %102 = load %struct.pqueue_t*, %struct.pqueue_t** %2, align 8, !dbg !862
  call void @pqueue_free(%struct.pqueue_t* %102), !dbg !863
  %103 = load %struct.node_t*, %struct.node_t** %3, align 8, !dbg !864
  %104 = bitcast %struct.node_t* %103 to i8*, !dbg !864
  call void @free(i8* %104), !dbg !865
  store i32 0, i32* %1, align 4, !dbg !866
  br label %105, !dbg !866

; <label>:105:                                    ; preds = %101, %13
  %106 = load i32, i32* %1, align 4, !dbg !867
  ret i32 %106, !dbg !867
}

; Function Attrs: nounwind ssp uwtable
define internal i32 @cmp_pri(i64, i64) #0 !dbg !868 {
  %3 = alloca i64, align 8
  %4 = alloca i64, align 8
  store i64 %0, i64* %3, align 8
  call void @llvm.dbg.declare(metadata i64* %3, metadata !869, metadata !67), !dbg !870
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !871, metadata !67), !dbg !872
  %5 = load i64, i64* %3, align 8, !dbg !873
  %6 = load i64, i64* %4, align 8, !dbg !874
  %7 = icmp ult i64 %5, %6, !dbg !875
  %8 = zext i1 %7 to i32, !dbg !875
  ret i32 %8, !dbg !876
}

; Function Attrs: nounwind ssp uwtable
define internal i64 @get_pri(i8*) #0 !dbg !877 {
  %2 = alloca i8*, align 8
  store i8* %0, i8** %2, align 8
  call void @llvm.dbg.declare(metadata i8** %2, metadata !878, metadata !67), !dbg !879
  %3 = load i8*, i8** %2, align 8, !dbg !880
  %4 = bitcast i8* %3 to %struct.node_t*, !dbg !881
  %5 = getelementptr inbounds %struct.node_t, %struct.node_t* %4, i32 0, i32 0, !dbg !882
  %6 = load i64, i64* %5, align 8, !dbg !882
  ret i64 %6, !dbg !883
}

; Function Attrs: nounwind ssp uwtable
define internal void @set_pri.1(i8*, i64) #0 !dbg !884 {
  %3 = alloca i8*, align 8
  %4 = alloca i64, align 8
  store i8* %0, i8** %3, align 8
  call void @llvm.dbg.declare(metadata i8** %3, metadata !885, metadata !67), !dbg !886
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !887, metadata !67), !dbg !888
  %5 = load i64, i64* %4, align 8, !dbg !889
  %6 = load i8*, i8** %3, align 8, !dbg !890
  %7 = bitcast i8* %6 to %struct.node_t*, !dbg !891
  %8 = getelementptr inbounds %struct.node_t, %struct.node_t* %7, i32 0, i32 0, !dbg !892
  store i64 %5, i64* %8, align 8, !dbg !893
  ret void, !dbg !894
}

; Function Attrs: nounwind ssp uwtable
define internal i64 @get_pos(i8*) #0 !dbg !895 {
  %2 = alloca i8*, align 8
  store i8* %0, i8** %2, align 8
  call void @llvm.dbg.declare(metadata i8** %2, metadata !896, metadata !67), !dbg !897
  %3 = load i8*, i8** %2, align 8, !dbg !898
  %4 = bitcast i8* %3 to %struct.node_t*, !dbg !899
  %5 = getelementptr inbounds %struct.node_t, %struct.node_t* %4, i32 0, i32 2, !dbg !900
  %6 = load i64, i64* %5, align 8, !dbg !900
  ret i64 %6, !dbg !901
}

; Function Attrs: nounwind ssp uwtable
define internal void @set_pos.2(i8*, i64) #0 !dbg !902 {
  %3 = alloca i8*, align 8
  %4 = alloca i64, align 8
  store i8* %0, i8** %3, align 8
  call void @llvm.dbg.declare(metadata i8** %3, metadata !903, metadata !67), !dbg !904
  store i64 %1, i64* %4, align 8
  call void @llvm.dbg.declare(metadata i64* %4, metadata !905, metadata !67), !dbg !906
  %5 = load i64, i64* %4, align 8, !dbg !907
  %6 = load i8*, i8** %3, align 8, !dbg !908
  %7 = bitcast i8* %6 to %struct.node_t*, !dbg !909
  %8 = getelementptr inbounds %struct.node_t, %struct.node_t* %7, i32 0, i32 2, !dbg !910
  store i64 %5, i64* %8, align 8, !dbg !911
  ret void, !dbg !912
}

declare i32 @printf(i8*, ...) #2

attributes #0 = { nounwind ssp uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }
attributes #2 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.dbg.cu = !{!0, !6}
!llvm.ident = !{!25, !25}
!llvm.module.flags = !{!26, !27, !28}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 3.9.1 (tags/RELEASE_391/final)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !3)
!1 = !DIFile(filename: "pqueue.c", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!2 = !{}
!3 = !{!4, !5}
!4 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64, align: 64)
!5 = !DIBasicType(name: "unsigned int", size: 32, align: 32, encoding: DW_ATE_unsigned)
!6 = distinct !DICompileUnit(language: DW_LANG_C99, file: !7, producer: "clang version 3.9.1 (tags/RELEASE_391/final)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !8)
!7 = !DIFile(filename: "sample.c", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!8 = !{!9}
!9 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !10, size: 64, align: 64)
!10 = !DIDerivedType(tag: DW_TAG_typedef, name: "node_t", file: !7, line: 43, baseType: !11)
!11 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "node_t", file: !7, line: 38, size: 192, align: 64, elements: !12)
!12 = !{!13, !17, !19}
!13 = !DIDerivedType(tag: DW_TAG_member, name: "pri", scope: !11, file: !7, line: 40, baseType: !14, size: 64, align: 64)
!14 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_pri_t", file: !15, line: 39, baseType: !16)
!15 = !DIFile(filename: "./pqueue.h", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!16 = !DIBasicType(name: "long long unsigned int", size: 64, align: 64, encoding: DW_ATE_unsigned)
!17 = !DIDerivedType(tag: DW_TAG_member, name: "val", scope: !11, file: !7, line: 41, baseType: !18, size: 32, align: 32, offset: 64)
!18 = !DIBasicType(name: "int", size: 32, align: 32, encoding: DW_ATE_signed)
!19 = !DIDerivedType(tag: DW_TAG_member, name: "pos", scope: !11, file: !7, line: 42, baseType: !20, size: 64, align: 64, offset: 128)
!20 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !21, line: 31, baseType: !22)
!21 = !DIFile(filename: "/usr/include/sys/_types/_size_t.h", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!22 = !DIDerivedType(tag: DW_TAG_typedef, name: "__darwin_size_t", file: !23, line: 92, baseType: !24)
!23 = !DIFile(filename: "/usr/include/i386/_types.h", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!24 = !DIBasicType(name: "long unsigned int", size: 64, align: 64, encoding: DW_ATE_unsigned)
!25 = !{!"clang version 3.9.1 (tags/RELEASE_391/final)"}
!26 = !{i32 2, !"Dwarf Version", i32 2}
!27 = !{i32 2, !"Debug Info Version", i32 3}
!28 = !{i32 1, !"PIC Level", i32 2}
!29 = distinct !DISubprogram(name: "pqueue_init", scope: !1, file: !1, line: 40, type: !30, isLocal: false, isDefinition: true, scopeLine: 46, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!30 = !DISubroutineType(types: !31)
!31 = !{!32, !20, !40, !45, !50, !55, !60}
!32 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !33, size: 64, align: 64)
!33 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_t", file: !15, line: 68, baseType: !34)
!34 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "pqueue_t", file: !15, line: 57, size: 576, align: 64, elements: !35)
!35 = !{!36, !37, !38, !39, !44, !49, !54, !59, !64}
!36 = !DIDerivedType(tag: DW_TAG_member, name: "size", scope: !34, file: !15, line: 59, baseType: !20, size: 64, align: 64)
!37 = !DIDerivedType(tag: DW_TAG_member, name: "avail", scope: !34, file: !15, line: 60, baseType: !20, size: 64, align: 64, offset: 64)
!38 = !DIDerivedType(tag: DW_TAG_member, name: "step", scope: !34, file: !15, line: 61, baseType: !20, size: 64, align: 64, offset: 128)
!39 = !DIDerivedType(tag: DW_TAG_member, name: "cmppri", scope: !34, file: !15, line: 62, baseType: !40, size: 64, align: 64, offset: 192)
!40 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_cmp_pri_f", file: !15, line: 44, baseType: !41)
!41 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !42, size: 64, align: 64)
!42 = !DISubroutineType(types: !43)
!43 = !{!18, !14, !14}
!44 = !DIDerivedType(tag: DW_TAG_member, name: "getpri", scope: !34, file: !15, line: 63, baseType: !45, size: 64, align: 64, offset: 256)
!45 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_get_pri_f", file: !15, line: 42, baseType: !46)
!46 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !47, size: 64, align: 64)
!47 = !DISubroutineType(types: !48)
!48 = !{!14, !4}
!49 = !DIDerivedType(tag: DW_TAG_member, name: "setpri", scope: !34, file: !15, line: 64, baseType: !50, size: 64, align: 64, offset: 320)
!50 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_set_pri_f", file: !15, line: 43, baseType: !51)
!51 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !52, size: 64, align: 64)
!52 = !DISubroutineType(types: !53)
!53 = !{null, !4, !14}
!54 = !DIDerivedType(tag: DW_TAG_member, name: "getpos", scope: !34, file: !15, line: 65, baseType: !55, size: 64, align: 64, offset: 384)
!55 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_get_pos_f", file: !15, line: 48, baseType: !56)
!56 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !57, size: 64, align: 64)
!57 = !DISubroutineType(types: !58)
!58 = !{!20, !4}
!59 = !DIDerivedType(tag: DW_TAG_member, name: "setpos", scope: !34, file: !15, line: 66, baseType: !60, size: 64, align: 64, offset: 448)
!60 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_set_pos_f", file: !15, line: 49, baseType: !61)
!61 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !62, size: 64, align: 64)
!62 = !DISubroutineType(types: !63)
!63 = !{null, !4, !20}
!64 = !DIDerivedType(tag: DW_TAG_member, name: "d", scope: !34, file: !15, line: 67, baseType: !65, size: 64, align: 64, offset: 512)
!65 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 64, align: 64)
!66 = !DILocalVariable(name: "n", arg: 1, scope: !29, file: !1, line: 40, type: !20)
!67 = !DIExpression()
!68 = !DILocation(line: 40, column: 20, scope: !29)
!69 = !DILocalVariable(name: "cmppri", arg: 2, scope: !29, file: !1, line: 41, type: !40)
!70 = !DILocation(line: 41, column: 30, scope: !29)
!71 = !DILocalVariable(name: "getpri", arg: 3, scope: !29, file: !1, line: 42, type: !45)
!72 = !DILocation(line: 42, column: 30, scope: !29)
!73 = !DILocalVariable(name: "setpri", arg: 4, scope: !29, file: !1, line: 43, type: !50)
!74 = !DILocation(line: 43, column: 30, scope: !29)
!75 = !DILocalVariable(name: "getpos", arg: 5, scope: !29, file: !1, line: 44, type: !55)
!76 = !DILocation(line: 44, column: 30, scope: !29)
!77 = !DILocalVariable(name: "setpos", arg: 6, scope: !29, file: !1, line: 45, type: !60)
!78 = !DILocation(line: 45, column: 30, scope: !29)
!79 = !DILocalVariable(name: "q", scope: !29, file: !1, line: 47, type: !32)
!80 = !DILocation(line: 47, column: 15, scope: !29)
!81 = !DILocation(line: 49, column: 15, scope: !82)
!82 = distinct !DILexicalBlock(scope: !29, file: !1, line: 49, column: 9)
!83 = !DILocation(line: 49, column: 13, scope: !82)
!84 = !DILocation(line: 49, column: 9, scope: !29)
!85 = !DILocation(line: 50, column: 9, scope: !82)
!86 = !DILocation(line: 53, column: 26, scope: !87)
!87 = distinct !DILexicalBlock(scope: !29, file: !1, line: 53, column: 9)
!88 = !DILocation(line: 53, column: 28, scope: !87)
!89 = !DILocation(line: 53, column: 33, scope: !87)
!90 = !DILocation(line: 53, column: 18, scope: !87)
!91 = !DILocation(line: 53, column: 11, scope: !87)
!92 = !DILocation(line: 53, column: 14, scope: !87)
!93 = !DILocation(line: 53, column: 16, scope: !87)
!94 = !DILocation(line: 53, column: 9, scope: !29)
!95 = !DILocation(line: 54, column: 14, scope: !96)
!96 = distinct !DILexicalBlock(scope: !87, file: !1, line: 53, column: 53)
!97 = !DILocation(line: 54, column: 9, scope: !96)
!98 = !DILocation(line: 55, column: 9, scope: !96)
!99 = !DILocation(line: 58, column: 5, scope: !29)
!100 = !DILocation(line: 58, column: 8, scope: !29)
!101 = !DILocation(line: 58, column: 13, scope: !29)
!102 = !DILocation(line: 59, column: 27, scope: !29)
!103 = !DILocation(line: 59, column: 28, scope: !29)
!104 = !DILocation(line: 59, column: 16, scope: !29)
!105 = !DILocation(line: 59, column: 19, scope: !29)
!106 = !DILocation(line: 59, column: 24, scope: !29)
!107 = !DILocation(line: 59, column: 5, scope: !29)
!108 = !DILocation(line: 59, column: 8, scope: !29)
!109 = !DILocation(line: 59, column: 14, scope: !29)
!110 = !DILocation(line: 60, column: 17, scope: !29)
!111 = !DILocation(line: 60, column: 5, scope: !29)
!112 = !DILocation(line: 60, column: 8, scope: !29)
!113 = !DILocation(line: 60, column: 15, scope: !29)
!114 = !DILocation(line: 61, column: 17, scope: !29)
!115 = !DILocation(line: 61, column: 5, scope: !29)
!116 = !DILocation(line: 61, column: 8, scope: !29)
!117 = !DILocation(line: 61, column: 15, scope: !29)
!118 = !DILocation(line: 62, column: 17, scope: !29)
!119 = !DILocation(line: 62, column: 5, scope: !29)
!120 = !DILocation(line: 62, column: 8, scope: !29)
!121 = !DILocation(line: 62, column: 15, scope: !29)
!122 = !DILocation(line: 63, column: 17, scope: !29)
!123 = !DILocation(line: 63, column: 5, scope: !29)
!124 = !DILocation(line: 63, column: 8, scope: !29)
!125 = !DILocation(line: 63, column: 15, scope: !29)
!126 = !DILocation(line: 64, column: 17, scope: !29)
!127 = !DILocation(line: 64, column: 5, scope: !29)
!128 = !DILocation(line: 64, column: 8, scope: !29)
!129 = !DILocation(line: 64, column: 15, scope: !29)
!130 = !DILocation(line: 66, column: 12, scope: !29)
!131 = !DILocation(line: 66, column: 5, scope: !29)
!132 = !DILocation(line: 67, column: 1, scope: !29)
!133 = distinct !DISubprogram(name: "pqueue_free", scope: !1, file: !1, line: 71, type: !134, isLocal: false, isDefinition: true, scopeLine: 72, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!134 = !DISubroutineType(types: !135)
!135 = !{null, !32}
!136 = !DILocalVariable(name: "q", arg: 1, scope: !133, file: !1, line: 71, type: !32)
!137 = !DILocation(line: 71, column: 23, scope: !133)
!138 = !DILocation(line: 73, column: 10, scope: !133)
!139 = !DILocation(line: 73, column: 13, scope: !133)
!140 = !DILocation(line: 73, column: 5, scope: !133)
!141 = !DILocation(line: 74, column: 10, scope: !133)
!142 = !DILocation(line: 74, column: 5, scope: !133)
!143 = !DILocation(line: 75, column: 1, scope: !133)
!144 = distinct !DISubprogram(name: "pqueue_size", scope: !1, file: !1, line: 79, type: !145, isLocal: false, isDefinition: true, scopeLine: 80, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!145 = !DISubroutineType(types: !146)
!146 = !{!20, !32}
!147 = !DILocalVariable(name: "q", arg: 1, scope: !144, file: !1, line: 79, type: !32)
!148 = !DILocation(line: 79, column: 23, scope: !144)
!149 = !DILocation(line: 82, column: 13, scope: !144)
!150 = !DILocation(line: 82, column: 16, scope: !144)
!151 = !DILocation(line: 82, column: 21, scope: !144)
!152 = !DILocation(line: 82, column: 5, scope: !144)
!153 = distinct !DISubprogram(name: "pqueue_insert", scope: !1, file: !1, line: 143, type: !154, isLocal: false, isDefinition: true, scopeLine: 144, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!154 = !DISubroutineType(types: !155)
!155 = !{!18, !32, !4}
!156 = !DILocalVariable(name: "q", arg: 1, scope: !153, file: !1, line: 143, type: !32)
!157 = !DILocation(line: 143, column: 25, scope: !153)
!158 = !DILocalVariable(name: "d", arg: 2, scope: !153, file: !1, line: 143, type: !4)
!159 = !DILocation(line: 143, column: 34, scope: !153)
!160 = !DILocalVariable(name: "tmp", scope: !153, file: !1, line: 145, type: !4)
!161 = !DILocation(line: 145, column: 11, scope: !153)
!162 = !DILocalVariable(name: "i", scope: !153, file: !1, line: 146, type: !20)
!163 = !DILocation(line: 146, column: 12, scope: !153)
!164 = !DILocalVariable(name: "newsize", scope: !153, file: !1, line: 147, type: !20)
!165 = !DILocation(line: 147, column: 12, scope: !153)
!166 = !DILocation(line: 149, column: 10, scope: !167)
!167 = distinct !DILexicalBlock(scope: !153, file: !1, line: 149, column: 9)
!168 = !DILocation(line: 149, column: 9, scope: !153)
!169 = !DILocation(line: 149, column: 13, scope: !167)
!170 = !DILocation(line: 152, column: 9, scope: !171)
!171 = distinct !DILexicalBlock(scope: !153, file: !1, line: 152, column: 9)
!172 = !DILocation(line: 152, column: 12, scope: !171)
!173 = !DILocation(line: 152, column: 20, scope: !171)
!174 = !DILocation(line: 152, column: 23, scope: !171)
!175 = !DILocation(line: 152, column: 17, scope: !171)
!176 = !DILocation(line: 152, column: 9, scope: !153)
!177 = !DILocation(line: 153, column: 19, scope: !178)
!178 = distinct !DILexicalBlock(scope: !171, file: !1, line: 152, column: 30)
!179 = !DILocation(line: 153, column: 22, scope: !178)
!180 = !DILocation(line: 153, column: 29, scope: !178)
!181 = !DILocation(line: 153, column: 32, scope: !178)
!182 = !DILocation(line: 153, column: 27, scope: !178)
!183 = !DILocation(line: 153, column: 17, scope: !178)
!184 = !DILocation(line: 154, column: 29, scope: !185)
!185 = distinct !DILexicalBlock(scope: !178, file: !1, line: 154, column: 13)
!186 = !DILocation(line: 154, column: 32, scope: !185)
!187 = !DILocation(line: 154, column: 52, scope: !185)
!188 = !DILocation(line: 154, column: 50, scope: !185)
!189 = !DILocation(line: 154, column: 21, scope: !185)
!190 = !DILocation(line: 154, column: 19, scope: !185)
!191 = !DILocation(line: 154, column: 13, scope: !178)
!192 = !DILocation(line: 155, column: 13, scope: !185)
!193 = !DILocation(line: 156, column: 16, scope: !178)
!194 = !DILocation(line: 156, column: 9, scope: !178)
!195 = !DILocation(line: 156, column: 12, scope: !178)
!196 = !DILocation(line: 156, column: 14, scope: !178)
!197 = !DILocation(line: 157, column: 20, scope: !178)
!198 = !DILocation(line: 157, column: 9, scope: !178)
!199 = !DILocation(line: 157, column: 12, scope: !178)
!200 = !DILocation(line: 157, column: 18, scope: !178)
!201 = !DILocation(line: 158, column: 5, scope: !178)
!202 = !DILocation(line: 161, column: 9, scope: !153)
!203 = !DILocation(line: 161, column: 12, scope: !153)
!204 = !DILocation(line: 161, column: 16, scope: !153)
!205 = !DILocation(line: 161, column: 7, scope: !153)
!206 = !DILocation(line: 162, column: 15, scope: !153)
!207 = !DILocation(line: 162, column: 10, scope: !153)
!208 = !DILocation(line: 162, column: 5, scope: !153)
!209 = !DILocation(line: 162, column: 8, scope: !153)
!210 = !DILocation(line: 162, column: 13, scope: !153)
!211 = !DILocation(line: 163, column: 15, scope: !153)
!212 = !DILocation(line: 163, column: 18, scope: !153)
!213 = !DILocation(line: 163, column: 5, scope: !153)
!214 = !DILocation(line: 165, column: 5, scope: !153)
!215 = !DILocation(line: 166, column: 1, scope: !153)
!216 = distinct !DISubprogram(name: "bubble_up", scope: !1, file: !1, line: 87, type: !217, isLocal: true, isDefinition: true, scopeLine: 88, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!217 = !DISubroutineType(types: !218)
!218 = !{null, !32, !20}
!219 = !DILocalVariable(name: "q", arg: 1, scope: !216, file: !1, line: 87, type: !32)
!220 = !DILocation(line: 87, column: 21, scope: !216)
!221 = !DILocalVariable(name: "i", arg: 2, scope: !216, file: !1, line: 87, type: !20)
!222 = !DILocation(line: 87, column: 31, scope: !216)
!223 = !DILocalVariable(name: "parent_node", scope: !216, file: !1, line: 89, type: !20)
!224 = !DILocation(line: 89, column: 12, scope: !216)
!225 = !DILocalVariable(name: "moving_node", scope: !216, file: !1, line: 90, type: !4)
!226 = !DILocation(line: 90, column: 11, scope: !216)
!227 = !DILocation(line: 90, column: 30, scope: !216)
!228 = !DILocation(line: 90, column: 25, scope: !216)
!229 = !DILocation(line: 90, column: 28, scope: !216)
!230 = !DILocalVariable(name: "moving_pri", scope: !216, file: !1, line: 91, type: !14)
!231 = !DILocation(line: 91, column: 18, scope: !216)
!232 = !DILocation(line: 91, column: 31, scope: !216)
!233 = !DILocation(line: 91, column: 34, scope: !216)
!234 = !DILocation(line: 91, column: 41, scope: !216)
!235 = !DILocation(line: 93, column: 24, scope: !236)
!236 = distinct !DILexicalBlock(scope: !216, file: !1, line: 93, column: 5)
!237 = !DILocation(line: 93, column: 22, scope: !236)
!238 = !DILocation(line: 93, column: 10, scope: !236)
!239 = !DILocation(line: 94, column: 12, scope: !240)
!240 = distinct !DILexicalBlock(scope: !236, file: !1, line: 93, column: 5)
!241 = !DILocation(line: 94, column: 14, scope: !240)
!242 = !DILocation(line: 94, column: 19, scope: !240)
!243 = !DILocation(line: 94, column: 22, scope: !240)
!244 = !DILocation(line: 94, column: 25, scope: !240)
!245 = !DILocation(line: 94, column: 32, scope: !240)
!246 = !DILocation(line: 94, column: 35, scope: !240)
!247 = !DILocation(line: 94, column: 47, scope: !240)
!248 = !DILocation(line: 94, column: 42, scope: !240)
!249 = !DILocation(line: 94, column: 45, scope: !240)
!250 = !DILocation(line: 94, column: 62, scope: !240)
!251 = !DILocation(line: 93, column: 5, scope: !236)
!252 = !DILocation(line: 97, column: 24, scope: !253)
!253 = distinct !DILexicalBlock(scope: !240, file: !1, line: 96, column: 5)
!254 = !DILocation(line: 97, column: 19, scope: !253)
!255 = !DILocation(line: 97, column: 22, scope: !253)
!256 = !DILocation(line: 97, column: 14, scope: !253)
!257 = !DILocation(line: 97, column: 9, scope: !253)
!258 = !DILocation(line: 97, column: 12, scope: !253)
!259 = !DILocation(line: 97, column: 17, scope: !253)
!260 = !DILocation(line: 98, column: 9, scope: !253)
!261 = !DILocation(line: 98, column: 12, scope: !253)
!262 = !DILocation(line: 98, column: 24, scope: !253)
!263 = !DILocation(line: 98, column: 19, scope: !253)
!264 = !DILocation(line: 98, column: 22, scope: !253)
!265 = !DILocation(line: 98, column: 28, scope: !253)
!266 = !DILocation(line: 99, column: 5, scope: !253)
!267 = !DILocation(line: 95, column: 14, scope: !240)
!268 = !DILocation(line: 95, column: 12, scope: !240)
!269 = !DILocation(line: 95, column: 41, scope: !240)
!270 = !DILocation(line: 95, column: 39, scope: !240)
!271 = !DILocation(line: 93, column: 5, scope: !240)
!272 = distinct !{!272, !273}
!273 = !DILocation(line: 93, column: 5, scope: !216)
!274 = !DILocation(line: 101, column: 15, scope: !216)
!275 = !DILocation(line: 101, column: 10, scope: !216)
!276 = !DILocation(line: 101, column: 5, scope: !216)
!277 = !DILocation(line: 101, column: 8, scope: !216)
!278 = !DILocation(line: 101, column: 13, scope: !216)
!279 = !DILocation(line: 102, column: 5, scope: !216)
!280 = !DILocation(line: 102, column: 8, scope: !216)
!281 = !DILocation(line: 102, column: 15, scope: !216)
!282 = !DILocation(line: 102, column: 28, scope: !216)
!283 = !DILocation(line: 103, column: 1, scope: !216)
!284 = distinct !DISubprogram(name: "pqueue_change_priority", scope: !1, file: !1, line: 170, type: !285, isLocal: false, isDefinition: true, scopeLine: 173, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!285 = !DISubroutineType(types: !286)
!286 = !{null, !32, !14, !4}
!287 = !DILocalVariable(name: "q", arg: 1, scope: !284, file: !1, line: 170, type: !32)
!288 = !DILocation(line: 170, column: 34, scope: !284)
!289 = !DILocalVariable(name: "new_pri", arg: 2, scope: !284, file: !1, line: 171, type: !14)
!290 = !DILocation(line: 171, column: 37, scope: !284)
!291 = !DILocalVariable(name: "d", arg: 3, scope: !284, file: !1, line: 172, type: !4)
!292 = !DILocation(line: 172, column: 30, scope: !284)
!293 = !DILocalVariable(name: "posn", scope: !284, file: !1, line: 174, type: !20)
!294 = !DILocation(line: 174, column: 12, scope: !284)
!295 = !DILocalVariable(name: "old_pri", scope: !284, file: !1, line: 175, type: !14)
!296 = !DILocation(line: 175, column: 18, scope: !284)
!297 = !DILocation(line: 175, column: 28, scope: !284)
!298 = !DILocation(line: 175, column: 31, scope: !284)
!299 = !DILocation(line: 175, column: 38, scope: !284)
!300 = !DILocation(line: 177, column: 5, scope: !284)
!301 = !DILocation(line: 177, column: 8, scope: !284)
!302 = !DILocation(line: 177, column: 15, scope: !284)
!303 = !DILocation(line: 177, column: 18, scope: !284)
!304 = !DILocation(line: 178, column: 12, scope: !284)
!305 = !DILocation(line: 178, column: 15, scope: !284)
!306 = !DILocation(line: 178, column: 22, scope: !284)
!307 = !DILocation(line: 178, column: 10, scope: !284)
!308 = !DILocation(line: 179, column: 9, scope: !309)
!309 = distinct !DILexicalBlock(scope: !284, file: !1, line: 179, column: 9)
!310 = !DILocation(line: 179, column: 12, scope: !309)
!311 = !DILocation(line: 179, column: 19, scope: !309)
!312 = !DILocation(line: 179, column: 28, scope: !309)
!313 = !DILocation(line: 179, column: 9, scope: !284)
!314 = !DILocation(line: 180, column: 19, scope: !309)
!315 = !DILocation(line: 180, column: 22, scope: !309)
!316 = !DILocation(line: 180, column: 9, scope: !309)
!317 = !DILocation(line: 182, column: 24, scope: !309)
!318 = !DILocation(line: 182, column: 27, scope: !309)
!319 = !DILocation(line: 182, column: 9, scope: !309)
!320 = !DILocation(line: 183, column: 1, scope: !284)
!321 = distinct !DISubprogram(name: "percolate_down", scope: !1, file: !1, line: 123, type: !217, isLocal: true, isDefinition: true, scopeLine: 124, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!322 = !DILocalVariable(name: "q", arg: 1, scope: !321, file: !1, line: 123, type: !32)
!323 = !DILocation(line: 123, column: 26, scope: !321)
!324 = !DILocalVariable(name: "i", arg: 2, scope: !321, file: !1, line: 123, type: !20)
!325 = !DILocation(line: 123, column: 36, scope: !321)
!326 = !DILocalVariable(name: "child_node", scope: !321, file: !1, line: 125, type: !20)
!327 = !DILocation(line: 125, column: 12, scope: !321)
!328 = !DILocalVariable(name: "moving_node", scope: !321, file: !1, line: 126, type: !4)
!329 = !DILocation(line: 126, column: 11, scope: !321)
!330 = !DILocation(line: 126, column: 30, scope: !321)
!331 = !DILocation(line: 126, column: 25, scope: !321)
!332 = !DILocation(line: 126, column: 28, scope: !321)
!333 = !DILocalVariable(name: "moving_pri", scope: !321, file: !1, line: 127, type: !14)
!334 = !DILocation(line: 127, column: 18, scope: !321)
!335 = !DILocation(line: 127, column: 31, scope: !321)
!336 = !DILocation(line: 127, column: 34, scope: !321)
!337 = !DILocation(line: 127, column: 41, scope: !321)
!338 = !DILocation(line: 129, column: 5, scope: !321)
!339 = !DILocation(line: 129, column: 35, scope: !321)
!340 = !DILocation(line: 129, column: 38, scope: !321)
!341 = !DILocation(line: 129, column: 26, scope: !321)
!342 = !DILocation(line: 129, column: 24, scope: !321)
!343 = !DILocation(line: 129, column: 42, scope: !321)
!344 = !DILocation(line: 130, column: 12, scope: !321)
!345 = !DILocation(line: 130, column: 15, scope: !321)
!346 = !DILocation(line: 130, column: 22, scope: !321)
!347 = !DILocation(line: 130, column: 34, scope: !321)
!348 = !DILocation(line: 130, column: 37, scope: !321)
!349 = !DILocation(line: 130, column: 49, scope: !321)
!350 = !DILocation(line: 130, column: 44, scope: !321)
!351 = !DILocation(line: 130, column: 47, scope: !321)
!352 = !DILocation(line: 132, column: 24, scope: !353)
!353 = distinct !DILexicalBlock(scope: !321, file: !1, line: 131, column: 5)
!354 = !DILocation(line: 132, column: 19, scope: !353)
!355 = !DILocation(line: 132, column: 22, scope: !353)
!356 = !DILocation(line: 132, column: 14, scope: !353)
!357 = !DILocation(line: 132, column: 9, scope: !353)
!358 = !DILocation(line: 132, column: 12, scope: !353)
!359 = !DILocation(line: 132, column: 17, scope: !353)
!360 = !DILocation(line: 133, column: 9, scope: !353)
!361 = !DILocation(line: 133, column: 12, scope: !353)
!362 = !DILocation(line: 133, column: 24, scope: !353)
!363 = !DILocation(line: 133, column: 19, scope: !353)
!364 = !DILocation(line: 133, column: 22, scope: !353)
!365 = !DILocation(line: 133, column: 28, scope: !353)
!366 = !DILocation(line: 134, column: 13, scope: !353)
!367 = !DILocation(line: 134, column: 11, scope: !353)
!368 = distinct !{!368, !338}
!369 = !DILocation(line: 137, column: 15, scope: !321)
!370 = !DILocation(line: 137, column: 10, scope: !321)
!371 = !DILocation(line: 137, column: 5, scope: !321)
!372 = !DILocation(line: 137, column: 8, scope: !321)
!373 = !DILocation(line: 137, column: 13, scope: !321)
!374 = !DILocation(line: 138, column: 5, scope: !321)
!375 = !DILocation(line: 138, column: 8, scope: !321)
!376 = !DILocation(line: 138, column: 15, scope: !321)
!377 = !DILocation(line: 138, column: 28, scope: !321)
!378 = !DILocation(line: 139, column: 1, scope: !321)
!379 = distinct !DISubprogram(name: "maxchild", scope: !1, file: !1, line: 107, type: !380, isLocal: true, isDefinition: true, scopeLine: 108, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!380 = !DISubroutineType(types: !381)
!381 = !{!20, !32, !20}
!382 = !DILocalVariable(name: "q", arg: 1, scope: !379, file: !1, line: 107, type: !32)
!383 = !DILocation(line: 107, column: 20, scope: !379)
!384 = !DILocalVariable(name: "i", arg: 2, scope: !379, file: !1, line: 107, type: !20)
!385 = !DILocation(line: 107, column: 30, scope: !379)
!386 = !DILocalVariable(name: "child_node", scope: !379, file: !1, line: 109, type: !20)
!387 = !DILocation(line: 109, column: 12, scope: !379)
!388 = !DILocation(line: 109, column: 25, scope: !379)
!389 = !DILocation(line: 111, column: 9, scope: !390)
!390 = distinct !DILexicalBlock(scope: !379, file: !1, line: 111, column: 9)
!391 = !DILocation(line: 111, column: 23, scope: !390)
!392 = !DILocation(line: 111, column: 26, scope: !390)
!393 = !DILocation(line: 111, column: 20, scope: !390)
!394 = !DILocation(line: 111, column: 9, scope: !379)
!395 = !DILocation(line: 112, column: 9, scope: !390)
!396 = !DILocation(line: 114, column: 10, scope: !397)
!397 = distinct !DILexicalBlock(scope: !379, file: !1, line: 114, column: 9)
!398 = !DILocation(line: 114, column: 20, scope: !397)
!399 = !DILocation(line: 114, column: 26, scope: !397)
!400 = !DILocation(line: 114, column: 29, scope: !397)
!401 = !DILocation(line: 114, column: 24, scope: !397)
!402 = !DILocation(line: 114, column: 34, scope: !397)
!403 = !DILocation(line: 115, column: 9, scope: !397)
!404 = !DILocation(line: 115, column: 12, scope: !397)
!405 = !DILocation(line: 115, column: 19, scope: !397)
!406 = !DILocation(line: 115, column: 22, scope: !397)
!407 = !DILocation(line: 115, column: 34, scope: !397)
!408 = !DILocation(line: 115, column: 29, scope: !397)
!409 = !DILocation(line: 115, column: 32, scope: !397)
!410 = !DILocation(line: 115, column: 48, scope: !397)
!411 = !DILocation(line: 115, column: 51, scope: !397)
!412 = !DILocation(line: 115, column: 63, scope: !397)
!413 = !DILocation(line: 115, column: 73, scope: !397)
!414 = !DILocation(line: 115, column: 58, scope: !397)
!415 = !DILocation(line: 115, column: 61, scope: !397)
!416 = !DILocation(line: 114, column: 9, scope: !379)
!417 = !DILocation(line: 116, column: 19, scope: !397)
!418 = !DILocation(line: 116, column: 9, scope: !397)
!419 = !DILocation(line: 118, column: 12, scope: !379)
!420 = !DILocation(line: 118, column: 5, scope: !379)
!421 = !DILocation(line: 119, column: 1, scope: !379)
!422 = distinct !DISubprogram(name: "pqueue_remove", scope: !1, file: !1, line: 187, type: !154, isLocal: false, isDefinition: true, scopeLine: 188, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!423 = !DILocalVariable(name: "q", arg: 1, scope: !422, file: !1, line: 187, type: !32)
!424 = !DILocation(line: 187, column: 25, scope: !422)
!425 = !DILocalVariable(name: "d", arg: 2, scope: !422, file: !1, line: 187, type: !4)
!426 = !DILocation(line: 187, column: 34, scope: !422)
!427 = !DILocalVariable(name: "posn", scope: !422, file: !1, line: 189, type: !20)
!428 = !DILocation(line: 189, column: 12, scope: !422)
!429 = !DILocation(line: 189, column: 19, scope: !422)
!430 = !DILocation(line: 189, column: 22, scope: !422)
!431 = !DILocation(line: 189, column: 29, scope: !422)
!432 = !DILocation(line: 190, column: 25, scope: !422)
!433 = !DILocation(line: 190, column: 28, scope: !422)
!434 = !DILocation(line: 190, column: 23, scope: !422)
!435 = !DILocation(line: 190, column: 18, scope: !422)
!436 = !DILocation(line: 190, column: 21, scope: !422)
!437 = !DILocation(line: 190, column: 10, scope: !422)
!438 = !DILocation(line: 190, column: 5, scope: !422)
!439 = !DILocation(line: 190, column: 8, scope: !422)
!440 = !DILocation(line: 190, column: 16, scope: !422)
!441 = !DILocation(line: 191, column: 9, scope: !442)
!442 = distinct !DILexicalBlock(scope: !422, file: !1, line: 191, column: 9)
!443 = !DILocation(line: 191, column: 12, scope: !442)
!444 = !DILocation(line: 191, column: 19, scope: !442)
!445 = !DILocation(line: 191, column: 22, scope: !442)
!446 = !DILocation(line: 191, column: 29, scope: !442)
!447 = !DILocation(line: 191, column: 33, scope: !442)
!448 = !DILocation(line: 191, column: 36, scope: !442)
!449 = !DILocation(line: 191, column: 48, scope: !442)
!450 = !DILocation(line: 191, column: 43, scope: !442)
!451 = !DILocation(line: 191, column: 46, scope: !442)
!452 = !DILocation(line: 191, column: 9, scope: !422)
!453 = !DILocation(line: 192, column: 19, scope: !442)
!454 = !DILocation(line: 192, column: 22, scope: !442)
!455 = !DILocation(line: 192, column: 9, scope: !442)
!456 = !DILocation(line: 194, column: 24, scope: !442)
!457 = !DILocation(line: 194, column: 27, scope: !442)
!458 = !DILocation(line: 194, column: 9, scope: !442)
!459 = !DILocation(line: 196, column: 5, scope: !422)
!460 = distinct !DISubprogram(name: "pqueue_pop", scope: !1, file: !1, line: 201, type: !461, isLocal: false, isDefinition: true, scopeLine: 202, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!461 = !DISubroutineType(types: !462)
!462 = !{!4, !32}
!463 = !DILocalVariable(name: "q", arg: 1, scope: !460, file: !1, line: 201, type: !32)
!464 = !DILocation(line: 201, column: 22, scope: !460)
!465 = !DILocalVariable(name: "head", scope: !460, file: !1, line: 203, type: !4)
!466 = !DILocation(line: 203, column: 11, scope: !460)
!467 = !DILocation(line: 205, column: 10, scope: !468)
!468 = distinct !DILexicalBlock(scope: !460, file: !1, line: 205, column: 9)
!469 = !DILocation(line: 205, column: 12, scope: !468)
!470 = !DILocation(line: 205, column: 15, scope: !468)
!471 = !DILocation(line: 205, column: 18, scope: !468)
!472 = !DILocation(line: 205, column: 23, scope: !468)
!473 = !DILocation(line: 205, column: 9, scope: !460)
!474 = !DILocation(line: 206, column: 9, scope: !468)
!475 = !DILocation(line: 208, column: 12, scope: !460)
!476 = !DILocation(line: 208, column: 15, scope: !460)
!477 = !DILocation(line: 208, column: 10, scope: !460)
!478 = !DILocation(line: 209, column: 22, scope: !460)
!479 = !DILocation(line: 209, column: 25, scope: !460)
!480 = !DILocation(line: 209, column: 20, scope: !460)
!481 = !DILocation(line: 209, column: 15, scope: !460)
!482 = !DILocation(line: 209, column: 18, scope: !460)
!483 = !DILocation(line: 209, column: 5, scope: !460)
!484 = !DILocation(line: 209, column: 8, scope: !460)
!485 = !DILocation(line: 209, column: 13, scope: !460)
!486 = !DILocation(line: 210, column: 20, scope: !460)
!487 = !DILocation(line: 210, column: 5, scope: !460)
!488 = !DILocation(line: 212, column: 12, scope: !460)
!489 = !DILocation(line: 212, column: 5, scope: !460)
!490 = !DILocation(line: 213, column: 1, scope: !460)
!491 = distinct !DISubprogram(name: "pqueue_peek", scope: !1, file: !1, line: 217, type: !461, isLocal: false, isDefinition: true, scopeLine: 218, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!492 = !DILocalVariable(name: "q", arg: 1, scope: !491, file: !1, line: 217, type: !32)
!493 = !DILocation(line: 217, column: 23, scope: !491)
!494 = !DILocalVariable(name: "d", scope: !491, file: !1, line: 219, type: !4)
!495 = !DILocation(line: 219, column: 11, scope: !491)
!496 = !DILocation(line: 220, column: 10, scope: !497)
!497 = distinct !DILexicalBlock(scope: !491, file: !1, line: 220, column: 9)
!498 = !DILocation(line: 220, column: 12, scope: !497)
!499 = !DILocation(line: 220, column: 15, scope: !497)
!500 = !DILocation(line: 220, column: 18, scope: !497)
!501 = !DILocation(line: 220, column: 23, scope: !497)
!502 = !DILocation(line: 220, column: 9, scope: !491)
!503 = !DILocation(line: 221, column: 9, scope: !497)
!504 = !DILocation(line: 222, column: 9, scope: !491)
!505 = !DILocation(line: 222, column: 12, scope: !491)
!506 = !DILocation(line: 222, column: 7, scope: !491)
!507 = !DILocation(line: 223, column: 12, scope: !491)
!508 = !DILocation(line: 223, column: 5, scope: !491)
!509 = !DILocation(line: 224, column: 1, scope: !491)
!510 = distinct !DISubprogram(name: "pqueue_dump", scope: !1, file: !1, line: 228, type: !511, isLocal: false, isDefinition: true, scopeLine: 231, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!511 = !DISubroutineType(types: !512)
!512 = !{null, !32, !513, !574}
!513 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !514, size: 64, align: 64)
!514 = !DIDerivedType(tag: DW_TAG_typedef, name: "FILE", file: !515, line: 153, baseType: !516)
!515 = !DIFile(filename: "/usr/include/stdio.h", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!516 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "__sFILE", file: !515, line: 122, size: 1216, align: 64, elements: !517)
!517 = !{!518, !521, !522, !523, !525, !526, !531, !532, !533, !537, !543, !552, !558, !559, !562, !563, !567, !571, !572, !573}
!518 = !DIDerivedType(tag: DW_TAG_member, name: "_p", scope: !516, file: !515, line: 123, baseType: !519, size: 64, align: 64)
!519 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !520, size: 64, align: 64)
!520 = !DIBasicType(name: "unsigned char", size: 8, align: 8, encoding: DW_ATE_unsigned_char)
!521 = !DIDerivedType(tag: DW_TAG_member, name: "_r", scope: !516, file: !515, line: 124, baseType: !18, size: 32, align: 32, offset: 64)
!522 = !DIDerivedType(tag: DW_TAG_member, name: "_w", scope: !516, file: !515, line: 125, baseType: !18, size: 32, align: 32, offset: 96)
!523 = !DIDerivedType(tag: DW_TAG_member, name: "_flags", scope: !516, file: !515, line: 126, baseType: !524, size: 16, align: 16, offset: 128)
!524 = !DIBasicType(name: "short", size: 16, align: 16, encoding: DW_ATE_signed)
!525 = !DIDerivedType(tag: DW_TAG_member, name: "_file", scope: !516, file: !515, line: 127, baseType: !524, size: 16, align: 16, offset: 144)
!526 = !DIDerivedType(tag: DW_TAG_member, name: "_bf", scope: !516, file: !515, line: 128, baseType: !527, size: 128, align: 64, offset: 192)
!527 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "__sbuf", file: !515, line: 88, size: 128, align: 64, elements: !528)
!528 = !{!529, !530}
!529 = !DIDerivedType(tag: DW_TAG_member, name: "_base", scope: !527, file: !515, line: 89, baseType: !519, size: 64, align: 64)
!530 = !DIDerivedType(tag: DW_TAG_member, name: "_size", scope: !527, file: !515, line: 90, baseType: !18, size: 32, align: 32, offset: 64)
!531 = !DIDerivedType(tag: DW_TAG_member, name: "_lbfsize", scope: !516, file: !515, line: 129, baseType: !18, size: 32, align: 32, offset: 320)
!532 = !DIDerivedType(tag: DW_TAG_member, name: "_cookie", scope: !516, file: !515, line: 132, baseType: !4, size: 64, align: 64, offset: 384)
!533 = !DIDerivedType(tag: DW_TAG_member, name: "_close", scope: !516, file: !515, line: 133, baseType: !534, size: 64, align: 64, offset: 448)
!534 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !535, size: 64, align: 64)
!535 = !DISubroutineType(types: !536)
!536 = !{!18, !4}
!537 = !DIDerivedType(tag: DW_TAG_member, name: "_read", scope: !516, file: !515, line: 134, baseType: !538, size: 64, align: 64, offset: 512)
!538 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !539, size: 64, align: 64)
!539 = !DISubroutineType(types: !540)
!540 = !{!18, !4, !541, !18}
!541 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !542, size: 64, align: 64)
!542 = !DIBasicType(name: "char", size: 8, align: 8, encoding: DW_ATE_signed_char)
!543 = !DIDerivedType(tag: DW_TAG_member, name: "_seek", scope: !516, file: !515, line: 135, baseType: !544, size: 64, align: 64, offset: 576)
!544 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !545, size: 64, align: 64)
!545 = !DISubroutineType(types: !546)
!546 = !{!547, !4, !547, !18}
!547 = !DIDerivedType(tag: DW_TAG_typedef, name: "fpos_t", file: !515, line: 77, baseType: !548)
!548 = !DIDerivedType(tag: DW_TAG_typedef, name: "__darwin_off_t", file: !549, line: 71, baseType: !550)
!549 = !DIFile(filename: "/usr/include/sys/_types.h", directory: "/Users/struewer/Downloads/libpqueue-master/src")
!550 = !DIDerivedType(tag: DW_TAG_typedef, name: "__int64_t", file: !23, line: 46, baseType: !551)
!551 = !DIBasicType(name: "long long int", size: 64, align: 64, encoding: DW_ATE_signed)
!552 = !DIDerivedType(tag: DW_TAG_member, name: "_write", scope: !516, file: !515, line: 136, baseType: !553, size: 64, align: 64, offset: 640)
!553 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !554, size: 64, align: 64)
!554 = !DISubroutineType(types: !555)
!555 = !{!18, !4, !556, !18}
!556 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !557, size: 64, align: 64)
!557 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !542)
!558 = !DIDerivedType(tag: DW_TAG_member, name: "_ub", scope: !516, file: !515, line: 139, baseType: !527, size: 128, align: 64, offset: 704)
!559 = !DIDerivedType(tag: DW_TAG_member, name: "_extra", scope: !516, file: !515, line: 140, baseType: !560, size: 64, align: 64, offset: 832)
!560 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !561, size: 64, align: 64)
!561 = !DICompositeType(tag: DW_TAG_structure_type, name: "__sFILEX", file: !515, line: 94, flags: DIFlagFwdDecl)
!562 = !DIDerivedType(tag: DW_TAG_member, name: "_ur", scope: !516, file: !515, line: 141, baseType: !18, size: 32, align: 32, offset: 896)
!563 = !DIDerivedType(tag: DW_TAG_member, name: "_ubuf", scope: !516, file: !515, line: 144, baseType: !564, size: 24, align: 8, offset: 928)
!564 = !DICompositeType(tag: DW_TAG_array_type, baseType: !520, size: 24, align: 8, elements: !565)
!565 = !{!566}
!566 = !DISubrange(count: 3)
!567 = !DIDerivedType(tag: DW_TAG_member, name: "_nbuf", scope: !516, file: !515, line: 145, baseType: !568, size: 8, align: 8, offset: 952)
!568 = !DICompositeType(tag: DW_TAG_array_type, baseType: !520, size: 8, align: 8, elements: !569)
!569 = !{!570}
!570 = !DISubrange(count: 1)
!571 = !DIDerivedType(tag: DW_TAG_member, name: "_lb", scope: !516, file: !515, line: 148, baseType: !527, size: 128, align: 64, offset: 960)
!572 = !DIDerivedType(tag: DW_TAG_member, name: "_blksize", scope: !516, file: !515, line: 151, baseType: !18, size: 32, align: 32, offset: 1088)
!573 = !DIDerivedType(tag: DW_TAG_member, name: "_offset", scope: !516, file: !515, line: 152, baseType: !547, size: 64, align: 64, offset: 1152)
!574 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_print_entry_f", file: !15, line: 53, baseType: !575)
!575 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !576, size: 64, align: 64)
!576 = !DISubroutineType(types: !577)
!577 = !{null, !513, !4}
!578 = !DILocalVariable(name: "q", arg: 1, scope: !510, file: !1, line: 228, type: !32)
!579 = !DILocation(line: 228, column: 23, scope: !510)
!580 = !DILocalVariable(name: "out", arg: 2, scope: !510, file: !1, line: 229, type: !513)
!581 = !DILocation(line: 229, column: 19, scope: !510)
!582 = !DILocalVariable(name: "print", arg: 3, scope: !510, file: !1, line: 230, type: !574)
!583 = !DILocation(line: 230, column: 34, scope: !510)
!584 = !DILocalVariable(name: "i", scope: !510, file: !1, line: 232, type: !18)
!585 = !DILocation(line: 232, column: 9, scope: !510)
!586 = !DILocation(line: 234, column: 13, scope: !510)
!587 = !DILocation(line: 234, column: 5, scope: !510)
!588 = !DILocation(line: 235, column: 12, scope: !589)
!589 = distinct !DILexicalBlock(scope: !510, file: !1, line: 235, column: 5)
!590 = !DILocation(line: 235, column: 10, scope: !589)
!591 = !DILocation(line: 235, column: 17, scope: !592)
!592 = distinct !DILexicalBlock(scope: !589, file: !1, line: 235, column: 5)
!593 = !DILocation(line: 235, column: 21, scope: !592)
!594 = !DILocation(line: 235, column: 24, scope: !592)
!595 = !DILocation(line: 235, column: 19, scope: !592)
!596 = !DILocation(line: 235, column: 5, scope: !589)
!597 = !DILocation(line: 236, column: 17, scope: !598)
!598 = distinct !DILexicalBlock(scope: !592, file: !1, line: 235, column: 35)
!599 = !DILocation(line: 238, column: 17, scope: !598)
!600 = !DILocation(line: 239, column: 17, scope: !598)
!601 = !DILocation(line: 239, column: 26, scope: !598)
!602 = !DILocation(line: 239, column: 36, scope: !598)
!603 = !DILocation(line: 240, column: 40, scope: !598)
!604 = !DILocation(line: 240, column: 43, scope: !598)
!605 = !DILocation(line: 240, column: 31, scope: !598)
!606 = !DILocation(line: 240, column: 17, scope: !598)
!607 = !DILocation(line: 236, column: 9, scope: !598)
!608 = !DILocation(line: 241, column: 9, scope: !598)
!609 = !DILocation(line: 241, column: 15, scope: !598)
!610 = !DILocation(line: 241, column: 25, scope: !598)
!611 = !DILocation(line: 241, column: 20, scope: !598)
!612 = !DILocation(line: 241, column: 23, scope: !598)
!613 = !DILocation(line: 242, column: 5, scope: !598)
!614 = !DILocation(line: 235, column: 31, scope: !592)
!615 = !DILocation(line: 235, column: 5, scope: !592)
!616 = distinct !{!616, !617}
!617 = !DILocation(line: 235, column: 5, scope: !510)
!618 = !DILocation(line: 243, column: 1, scope: !510)
!619 = distinct !DISubprogram(name: "pqueue_print", scope: !1, file: !1, line: 261, type: !511, isLocal: false, isDefinition: true, scopeLine: 264, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!620 = !DILocalVariable(name: "q", arg: 1, scope: !619, file: !1, line: 261, type: !32)
!621 = !DILocation(line: 261, column: 24, scope: !619)
!622 = !DILocalVariable(name: "out", arg: 2, scope: !619, file: !1, line: 262, type: !513)
!623 = !DILocation(line: 262, column: 20, scope: !619)
!624 = !DILocalVariable(name: "print", arg: 3, scope: !619, file: !1, line: 263, type: !574)
!625 = !DILocation(line: 263, column: 35, scope: !619)
!626 = !DILocalVariable(name: "dup", scope: !619, file: !1, line: 265, type: !32)
!627 = !DILocation(line: 265, column: 15, scope: !619)
!628 = !DILocalVariable(name: "e", scope: !619, file: !1, line: 266, type: !4)
!629 = !DILocation(line: 266, column: 8, scope: !619)
!630 = !DILocation(line: 268, column: 23, scope: !619)
!631 = !DILocation(line: 268, column: 26, scope: !619)
!632 = !DILocation(line: 269, column: 23, scope: !619)
!633 = !DILocation(line: 269, column: 26, scope: !619)
!634 = !DILocation(line: 269, column: 34, scope: !619)
!635 = !DILocation(line: 269, column: 37, scope: !619)
!636 = !DILocation(line: 270, column: 23, scope: !619)
!637 = !DILocation(line: 270, column: 26, scope: !619)
!638 = !DILocation(line: 268, column: 11, scope: !619)
!639 = !DILocation(line: 268, column: 9, scope: !619)
!640 = !DILocation(line: 271, column: 17, scope: !619)
!641 = !DILocation(line: 271, column: 20, scope: !619)
!642 = !DILocation(line: 271, column: 5, scope: !619)
!643 = !DILocation(line: 271, column: 10, scope: !619)
!644 = !DILocation(line: 271, column: 15, scope: !619)
!645 = !DILocation(line: 272, column: 18, scope: !619)
!646 = !DILocation(line: 272, column: 21, scope: !619)
!647 = !DILocation(line: 272, column: 5, scope: !619)
!648 = !DILocation(line: 272, column: 10, scope: !619)
!649 = !DILocation(line: 272, column: 16, scope: !619)
!650 = !DILocation(line: 273, column: 17, scope: !619)
!651 = !DILocation(line: 273, column: 20, scope: !619)
!652 = !DILocation(line: 273, column: 5, scope: !619)
!653 = !DILocation(line: 273, column: 10, scope: !619)
!654 = !DILocation(line: 273, column: 15, scope: !619)
!655 = !DILocation(line: 275, column: 5, scope: !619)
!656 = !DILocation(line: 277, column: 5, scope: !619)
!657 = !DILocation(line: 277, column: 28, scope: !619)
!658 = !DILocation(line: 277, column: 17, scope: !619)
!659 = !DILocation(line: 277, column: 15, scope: !619)
!660 = !DILocation(line: 278, column: 3, scope: !619)
!661 = !DILocation(line: 278, column: 9, scope: !619)
!662 = !DILocation(line: 278, column: 14, scope: !619)
!663 = distinct !{!663, !656}
!664 = !DILocation(line: 280, column: 17, scope: !619)
!665 = !DILocation(line: 280, column: 5, scope: !619)
!666 = !DILocation(line: 281, column: 1, scope: !619)
!667 = distinct !DISubprogram(name: "set_pri", scope: !1, file: !1, line: 254, type: !52, isLocal: true, isDefinition: true, scopeLine: 255, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!668 = !DILocalVariable(name: "d", arg: 1, scope: !667, file: !1, line: 254, type: !4)
!669 = !DILocation(line: 254, column: 15, scope: !667)
!670 = !DILocalVariable(name: "pri", arg: 2, scope: !667, file: !1, line: 254, type: !14)
!671 = !DILocation(line: 254, column: 31, scope: !667)
!672 = !DILocation(line: 257, column: 1, scope: !667)
!673 = distinct !DISubprogram(name: "set_pos", scope: !1, file: !1, line: 247, type: !62, isLocal: true, isDefinition: true, scopeLine: 248, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!674 = !DILocalVariable(name: "d", arg: 1, scope: !673, file: !1, line: 247, type: !4)
!675 = !DILocation(line: 247, column: 15, scope: !673)
!676 = !DILocalVariable(name: "val", arg: 2, scope: !673, file: !1, line: 247, type: !20)
!677 = !DILocation(line: 247, column: 25, scope: !673)
!678 = !DILocation(line: 250, column: 1, scope: !673)
!679 = distinct !DISubprogram(name: "pqueue_is_valid", scope: !1, file: !1, line: 306, type: !680, isLocal: false, isDefinition: true, scopeLine: 307, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!680 = !DISubroutineType(types: !681)
!681 = !{!18, !32}
!682 = !DILocalVariable(name: "q", arg: 1, scope: !679, file: !1, line: 306, type: !32)
!683 = !DILocation(line: 306, column: 27, scope: !679)
!684 = !DILocation(line: 308, column: 29, scope: !679)
!685 = !DILocation(line: 308, column: 12, scope: !679)
!686 = !DILocation(line: 308, column: 5, scope: !679)
!687 = distinct !DISubprogram(name: "subtree_is_valid", scope: !1, file: !1, line: 285, type: !688, isLocal: true, isDefinition: true, scopeLine: 286, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!688 = !DISubroutineType(types: !689)
!689 = !{!18, !32, !18}
!690 = !DILocalVariable(name: "q", arg: 1, scope: !687, file: !1, line: 285, type: !32)
!691 = !DILocation(line: 285, column: 28, scope: !687)
!692 = !DILocalVariable(name: "pos", arg: 2, scope: !687, file: !1, line: 285, type: !18)
!693 = !DILocation(line: 285, column: 35, scope: !687)
!694 = !DILocation(line: 287, column: 9, scope: !695)
!695 = distinct !DILexicalBlock(scope: !687, file: !1, line: 287, column: 9)
!696 = !DILocation(line: 287, column: 21, scope: !695)
!697 = !DILocation(line: 287, column: 24, scope: !695)
!698 = !DILocation(line: 287, column: 19, scope: !695)
!699 = !DILocation(line: 287, column: 9, scope: !687)
!700 = !DILocation(line: 289, column: 13, scope: !701)
!701 = distinct !DILexicalBlock(scope: !702, file: !1, line: 289, column: 13)
!702 = distinct !DILexicalBlock(scope: !695, file: !1, line: 287, column: 30)
!703 = !DILocation(line: 289, column: 16, scope: !701)
!704 = !DILocation(line: 289, column: 23, scope: !701)
!705 = !DILocation(line: 289, column: 26, scope: !701)
!706 = !DILocation(line: 289, column: 38, scope: !701)
!707 = !DILocation(line: 289, column: 33, scope: !701)
!708 = !DILocation(line: 289, column: 36, scope: !701)
!709 = !DILocation(line: 289, column: 45, scope: !701)
!710 = !DILocation(line: 289, column: 48, scope: !701)
!711 = !DILocation(line: 289, column: 60, scope: !701)
!712 = !DILocation(line: 289, column: 55, scope: !701)
!713 = !DILocation(line: 289, column: 58, scope: !701)
!714 = !DILocation(line: 289, column: 13, scope: !702)
!715 = !DILocation(line: 290, column: 13, scope: !701)
!716 = !DILocation(line: 291, column: 31, scope: !717)
!717 = distinct !DILexicalBlock(scope: !702, file: !1, line: 291, column: 13)
!718 = !DILocation(line: 291, column: 34, scope: !717)
!719 = !DILocation(line: 291, column: 14, scope: !717)
!720 = !DILocation(line: 291, column: 13, scope: !702)
!721 = !DILocation(line: 292, column: 13, scope: !717)
!722 = !DILocation(line: 293, column: 5, scope: !702)
!723 = !DILocation(line: 294, column: 9, scope: !724)
!724 = distinct !DILexicalBlock(scope: !687, file: !1, line: 294, column: 9)
!725 = !DILocation(line: 294, column: 22, scope: !724)
!726 = !DILocation(line: 294, column: 25, scope: !724)
!727 = !DILocation(line: 294, column: 20, scope: !724)
!728 = !DILocation(line: 294, column: 9, scope: !687)
!729 = !DILocation(line: 296, column: 13, scope: !730)
!730 = distinct !DILexicalBlock(scope: !731, file: !1, line: 296, column: 13)
!731 = distinct !DILexicalBlock(scope: !724, file: !1, line: 294, column: 31)
!732 = !DILocation(line: 296, column: 16, scope: !730)
!733 = !DILocation(line: 296, column: 23, scope: !730)
!734 = !DILocation(line: 296, column: 26, scope: !730)
!735 = !DILocation(line: 296, column: 38, scope: !730)
!736 = !DILocation(line: 296, column: 33, scope: !730)
!737 = !DILocation(line: 296, column: 36, scope: !730)
!738 = !DILocation(line: 296, column: 45, scope: !730)
!739 = !DILocation(line: 296, column: 48, scope: !730)
!740 = !DILocation(line: 296, column: 60, scope: !730)
!741 = !DILocation(line: 296, column: 55, scope: !730)
!742 = !DILocation(line: 296, column: 58, scope: !730)
!743 = !DILocation(line: 296, column: 13, scope: !731)
!744 = !DILocation(line: 297, column: 13, scope: !730)
!745 = !DILocation(line: 298, column: 31, scope: !746)
!746 = distinct !DILexicalBlock(scope: !731, file: !1, line: 298, column: 13)
!747 = !DILocation(line: 298, column: 34, scope: !746)
!748 = !DILocation(line: 298, column: 14, scope: !746)
!749 = !DILocation(line: 298, column: 13, scope: !731)
!750 = !DILocation(line: 299, column: 13, scope: !746)
!751 = !DILocation(line: 300, column: 5, scope: !731)
!752 = !DILocation(line: 301, column: 5, scope: !687)
!753 = !DILocation(line: 302, column: 1, scope: !687)
!754 = distinct !DISubprogram(name: "main", scope: !7, file: !7, line: 82, type: !755, isLocal: false, isDefinition: true, scopeLine: 83, flags: DIFlagPrototyped, isOptimized: false, unit: !6, variables: !2)
!755 = !DISubroutineType(types: !756)
!756 = !{!18}
!757 = !DILocalVariable(name: "pq", scope: !754, file: !7, line: 84, type: !758)
!758 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !759, size: 64, align: 64)
!759 = !DIDerivedType(tag: DW_TAG_typedef, name: "pqueue_t", file: !15, line: 68, baseType: !760)
!760 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "pqueue_t", file: !15, line: 57, size: 576, align: 64, elements: !761)
!761 = !{!762, !763, !764, !765, !766, !767, !768, !769, !770}
!762 = !DIDerivedType(tag: DW_TAG_member, name: "size", scope: !760, file: !15, line: 59, baseType: !20, size: 64, align: 64)
!763 = !DIDerivedType(tag: DW_TAG_member, name: "avail", scope: !760, file: !15, line: 60, baseType: !20, size: 64, align: 64, offset: 64)
!764 = !DIDerivedType(tag: DW_TAG_member, name: "step", scope: !760, file: !15, line: 61, baseType: !20, size: 64, align: 64, offset: 128)
!765 = !DIDerivedType(tag: DW_TAG_member, name: "cmppri", scope: !760, file: !15, line: 62, baseType: !40, size: 64, align: 64, offset: 192)
!766 = !DIDerivedType(tag: DW_TAG_member, name: "getpri", scope: !760, file: !15, line: 63, baseType: !45, size: 64, align: 64, offset: 256)
!767 = !DIDerivedType(tag: DW_TAG_member, name: "setpri", scope: !760, file: !15, line: 64, baseType: !50, size: 64, align: 64, offset: 320)
!768 = !DIDerivedType(tag: DW_TAG_member, name: "getpos", scope: !760, file: !15, line: 65, baseType: !55, size: 64, align: 64, offset: 384)
!769 = !DIDerivedType(tag: DW_TAG_member, name: "setpos", scope: !760, file: !15, line: 66, baseType: !60, size: 64, align: 64, offset: 448)
!770 = !DIDerivedType(tag: DW_TAG_member, name: "d", scope: !760, file: !15, line: 67, baseType: !65, size: 64, align: 64, offset: 512)
!771 = !DILocation(line: 84, column: 12, scope: !754)
!772 = !DILocalVariable(name: "ns", scope: !754, file: !7, line: 85, type: !9)
!773 = !DILocation(line: 85, column: 12, scope: !754)
!774 = !DILocalVariable(name: "n", scope: !754, file: !7, line: 86, type: !9)
!775 = !DILocation(line: 86, column: 12, scope: !754)
!776 = !DILocation(line: 88, column: 7, scope: !754)
!777 = !DILocation(line: 88, column: 5, scope: !754)
!778 = !DILocation(line: 89, column: 7, scope: !754)
!779 = !DILocation(line: 89, column: 5, scope: !754)
!780 = !DILocation(line: 90, column: 8, scope: !781)
!781 = distinct !DILexicalBlock(scope: !754, file: !7, line: 90, column: 6)
!782 = !DILocation(line: 90, column: 11, scope: !781)
!783 = !DILocation(line: 90, column: 14, scope: !781)
!784 = !DILocation(line: 90, column: 6, scope: !754)
!785 = !DILocation(line: 90, column: 19, scope: !781)
!786 = !DILocation(line: 92, column: 2, scope: !754)
!787 = !DILocation(line: 92, column: 8, scope: !754)
!788 = !DILocation(line: 92, column: 12, scope: !754)
!789 = !DILocation(line: 92, column: 17, scope: !754)
!790 = !DILocation(line: 92, column: 23, scope: !754)
!791 = !DILocation(line: 92, column: 27, scope: !754)
!792 = !DILocation(line: 92, column: 47, scope: !754)
!793 = !DILocation(line: 92, column: 52, scope: !754)
!794 = !DILocation(line: 92, column: 51, scope: !754)
!795 = !DILocation(line: 92, column: 33, scope: !754)
!796 = !DILocation(line: 93, column: 2, scope: !754)
!797 = !DILocation(line: 93, column: 8, scope: !754)
!798 = !DILocation(line: 93, column: 12, scope: !754)
!799 = !DILocation(line: 93, column: 17, scope: !754)
!800 = !DILocation(line: 93, column: 23, scope: !754)
!801 = !DILocation(line: 93, column: 27, scope: !754)
!802 = !DILocation(line: 93, column: 47, scope: !754)
!803 = !DILocation(line: 93, column: 52, scope: !754)
!804 = !DILocation(line: 93, column: 51, scope: !754)
!805 = !DILocation(line: 93, column: 33, scope: !754)
!806 = !DILocation(line: 94, column: 2, scope: !754)
!807 = !DILocation(line: 94, column: 8, scope: !754)
!808 = !DILocation(line: 94, column: 12, scope: !754)
!809 = !DILocation(line: 94, column: 17, scope: !754)
!810 = !DILocation(line: 94, column: 23, scope: !754)
!811 = !DILocation(line: 94, column: 27, scope: !754)
!812 = !DILocation(line: 94, column: 47, scope: !754)
!813 = !DILocation(line: 94, column: 52, scope: !754)
!814 = !DILocation(line: 94, column: 51, scope: !754)
!815 = !DILocation(line: 94, column: 33, scope: !754)
!816 = !DILocation(line: 95, column: 2, scope: !754)
!817 = !DILocation(line: 95, column: 8, scope: !754)
!818 = !DILocation(line: 95, column: 12, scope: !754)
!819 = !DILocation(line: 95, column: 17, scope: !754)
!820 = !DILocation(line: 95, column: 23, scope: !754)
!821 = !DILocation(line: 95, column: 27, scope: !754)
!822 = !DILocation(line: 95, column: 47, scope: !754)
!823 = !DILocation(line: 95, column: 52, scope: !754)
!824 = !DILocation(line: 95, column: 51, scope: !754)
!825 = !DILocation(line: 95, column: 33, scope: !754)
!826 = !DILocation(line: 96, column: 2, scope: !754)
!827 = !DILocation(line: 96, column: 8, scope: !754)
!828 = !DILocation(line: 96, column: 12, scope: !754)
!829 = !DILocation(line: 96, column: 17, scope: !754)
!830 = !DILocation(line: 96, column: 23, scope: !754)
!831 = !DILocation(line: 96, column: 27, scope: !754)
!832 = !DILocation(line: 96, column: 47, scope: !754)
!833 = !DILocation(line: 96, column: 52, scope: !754)
!834 = !DILocation(line: 96, column: 51, scope: !754)
!835 = !DILocation(line: 96, column: 33, scope: !754)
!836 = !DILocation(line: 98, column: 18, scope: !754)
!837 = !DILocation(line: 98, column: 6, scope: !754)
!838 = !DILocation(line: 98, column: 4, scope: !754)
!839 = !DILocation(line: 99, column: 30, scope: !754)
!840 = !DILocation(line: 99, column: 33, scope: !754)
!841 = !DILocation(line: 99, column: 38, scope: !754)
!842 = !DILocation(line: 99, column: 41, scope: !754)
!843 = !DILocation(line: 99, column: 2, scope: !754)
!844 = !DILocation(line: 101, column: 25, scope: !754)
!845 = !DILocation(line: 101, column: 33, scope: !754)
!846 = !DILocation(line: 101, column: 32, scope: !754)
!847 = !DILocation(line: 101, column: 2, scope: !754)
!848 = !DILocation(line: 102, column: 25, scope: !754)
!849 = !DILocation(line: 102, column: 33, scope: !754)
!850 = !DILocation(line: 102, column: 32, scope: !754)
!851 = !DILocation(line: 102, column: 2, scope: !754)
!852 = !DILocation(line: 104, column: 5, scope: !754)
!853 = !DILocation(line: 104, column: 28, scope: !754)
!854 = !DILocation(line: 104, column: 17, scope: !754)
!855 = !DILocation(line: 104, column: 15, scope: !754)
!856 = !DILocation(line: 105, column: 30, scope: !754)
!857 = !DILocation(line: 105, column: 33, scope: !754)
!858 = !DILocation(line: 105, column: 38, scope: !754)
!859 = !DILocation(line: 105, column: 41, scope: !754)
!860 = !DILocation(line: 105, column: 3, scope: !754)
!861 = distinct !{!861, !852}
!862 = !DILocation(line: 107, column: 14, scope: !754)
!863 = !DILocation(line: 107, column: 2, scope: !754)
!864 = !DILocation(line: 108, column: 7, scope: !754)
!865 = !DILocation(line: 108, column: 2, scope: !754)
!866 = !DILocation(line: 110, column: 2, scope: !754)
!867 = !DILocation(line: 111, column: 1, scope: !754)
!868 = distinct !DISubprogram(name: "cmp_pri", scope: !7, file: !7, line: 47, type: !42, isLocal: true, isDefinition: true, scopeLine: 48, flags: DIFlagPrototyped, isOptimized: false, unit: !6, variables: !2)
!869 = !DILocalVariable(name: "next", arg: 1, scope: !868, file: !7, line: 47, type: !14)
!870 = !DILocation(line: 47, column: 22, scope: !868)
!871 = !DILocalVariable(name: "curr", arg: 2, scope: !868, file: !7, line: 47, type: !14)
!872 = !DILocation(line: 47, column: 41, scope: !868)
!873 = !DILocation(line: 49, column: 10, scope: !868)
!874 = !DILocation(line: 49, column: 17, scope: !868)
!875 = !DILocation(line: 49, column: 15, scope: !868)
!876 = !DILocation(line: 49, column: 2, scope: !868)
!877 = distinct !DISubprogram(name: "get_pri", scope: !7, file: !7, line: 54, type: !47, isLocal: true, isDefinition: true, scopeLine: 55, flags: DIFlagPrototyped, isOptimized: false, unit: !6, variables: !2)
!878 = !DILocalVariable(name: "a", arg: 1, scope: !877, file: !7, line: 54, type: !4)
!879 = !DILocation(line: 54, column: 15, scope: !877)
!880 = !DILocation(line: 56, column: 21, scope: !877)
!881 = !DILocation(line: 56, column: 10, scope: !877)
!882 = !DILocation(line: 56, column: 25, scope: !877)
!883 = !DILocation(line: 56, column: 2, scope: !877)
!884 = distinct !DISubprogram(name: "set_pri", scope: !7, file: !7, line: 61, type: !52, isLocal: true, isDefinition: true, scopeLine: 62, flags: DIFlagPrototyped, isOptimized: false, unit: !6, variables: !2)
!885 = !DILocalVariable(name: "a", arg: 1, scope: !884, file: !7, line: 61, type: !4)
!886 = !DILocation(line: 61, column: 15, scope: !884)
!887 = !DILocalVariable(name: "pri", arg: 2, scope: !884, file: !7, line: 61, type: !14)
!888 = !DILocation(line: 61, column: 31, scope: !884)
!889 = !DILocation(line: 63, column: 24, scope: !884)
!890 = !DILocation(line: 63, column: 14, scope: !884)
!891 = !DILocation(line: 63, column: 3, scope: !884)
!892 = !DILocation(line: 63, column: 18, scope: !884)
!893 = !DILocation(line: 63, column: 22, scope: !884)
!894 = !DILocation(line: 64, column: 1, scope: !884)
!895 = distinct !DISubprogram(name: "get_pos", scope: !7, file: !7, line: 68, type: !57, isLocal: true, isDefinition: true, scopeLine: 69, flags: DIFlagPrototyped, isOptimized: false, unit: !6, variables: !2)
!896 = !DILocalVariable(name: "a", arg: 1, scope: !895, file: !7, line: 68, type: !4)
!897 = !DILocation(line: 68, column: 15, scope: !895)
!898 = !DILocation(line: 70, column: 21, scope: !895)
!899 = !DILocation(line: 70, column: 10, scope: !895)
!900 = !DILocation(line: 70, column: 25, scope: !895)
!901 = !DILocation(line: 70, column: 2, scope: !895)
!902 = distinct !DISubprogram(name: "set_pos", scope: !7, file: !7, line: 75, type: !62, isLocal: true, isDefinition: true, scopeLine: 76, flags: DIFlagPrototyped, isOptimized: false, unit: !6, variables: !2)
!903 = !DILocalVariable(name: "a", arg: 1, scope: !902, file: !7, line: 75, type: !4)
!904 = !DILocation(line: 75, column: 15, scope: !902)
!905 = !DILocalVariable(name: "pos", arg: 2, scope: !902, file: !7, line: 75, type: !20)
!906 = !DILocation(line: 75, column: 25, scope: !902)
!907 = !DILocation(line: 77, column: 24, scope: !902)
!908 = !DILocation(line: 77, column: 14, scope: !902)
!909 = !DILocation(line: 77, column: 3, scope: !902)
!910 = !DILocation(line: 77, column: 18, scope: !902)
!911 = !DILocation(line: 77, column: 22, scope: !902)
!912 = !DILocation(line: 78, column: 1, scope: !902)

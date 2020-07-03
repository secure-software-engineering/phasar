; ModuleID = 'functions2.cpp'
source_filename = "functions2.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.Adder = type { i32 (...)** }
%struct.S = type { %struct.Adder }
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

@F = global i32 (i32, i32)* @_Z3addii, align 8
@A = global %struct.Adder* null, align 8
@M = global { i64, i64 } { i64 ptrtoint (i32 (%struct.R*, i32, i32)* @_ZN1R4multEii to i64), i64 0 }, align 8
@_ZTV1S = linkonce_odr unnamed_addr constant { [3 x i8*] } { [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI1S to i8*), i8* bitcast (i32 (%struct.S*, i32, i32)* @_ZN1S3addEii to i8*)] }, comdat, align 8
@_ZTVN10__cxxabiv120__si_class_type_infoE = external global i8*
@_ZTS1S = linkonce_odr constant [3 x i8] c"1S\00", comdat
@_ZTVN10__cxxabiv117__class_type_infoE = external global i8*
@_ZTS5Adder = linkonce_odr constant [7 x i8] c"5Adder\00", comdat
@_ZTI5Adder = linkonce_odr constant { i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv117__class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([7 x i8], [7 x i8]* @_ZTS5Adder, i32 0, i32 0) }, comdat
@_ZTI1S = linkonce_odr constant { i8*, i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @_ZTS1S, i32 0, i32 0), i8* bitcast ({ i8*, i8* }* @_ZTI5Adder to i8*) }, comdat
@_ZTV5Adder = linkonce_odr unnamed_addr constant { [3 x i8*] } { [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8* }* @_ZTI5Adder to i8*), i8* bitcast (void ()* @__cxa_pure_virtual to i8*)] }, comdat, align 8
@llvm.global_ctors = appending global [1 x { i32, void ()*, i8* }] [{ i32, void ()*, i8* } { i32 65535, void ()* @_GLOBAL__sub_I_functions2.cpp, i8* null }]

; Function Attrs: noinline optnone uwtable
define %struct.Adder* @_Z9makeAdderv() #0 {
  %1 = call i8* @_Znwm(i64 8) #5
  %2 = bitcast i8* %1 to %struct.S*
  call void @_ZN1SC2Ev(%struct.S* %2) #6
  %3 = bitcast %struct.S* %2 to %struct.Adder*
  ret %struct.Adder* %3
}

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #1

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr void @_ZN1SC2Ev(%struct.S*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.S*, align 8
  store %struct.S* %0, %struct.S** %2, align 8
  %3 = load %struct.S*, %struct.S** %2, align 8
  %4 = bitcast %struct.S* %3 to %struct.Adder*
  call void @_ZN5AdderC2Ev(%struct.Adder* %4) #6
  %5 = bitcast %struct.S* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ({ [3 x i8*] }, { [3 x i8*] }* @_ZTV1S, i32 0, inrange i32 0, i32 2) to i32 (...)**), i32 (...)*** %5, align 8
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define i32 @_Z3addii(i32, i32) #2 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  store i32 %1, i32* %4, align 4
  %5 = load i32, i32* %3, align 4
  %6 = load i32, i32* %4, align 4
  %7 = add nsw i32 %5, %6
  ret i32 %7
}

; Function Attrs: noinline uwtable
define internal void @__cxx_global_var_init() #3 section ".text.startup" {
  %1 = call %struct.Adder* @_Z9makeAdderv()
  store %struct.Adder* %1, %struct.Adder** @A, align 8
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr i32 @_ZN1R4multEii(%struct.R*, i32, i32) #2 comdat align 2 {
  %4 = alloca %struct.R*, align 8
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  store %struct.R* %0, %struct.R** %4, align 8
  store i32 %1, i32* %5, align 4
  store i32 %2, i32* %6, align 4
  %7 = load %struct.R*, %struct.R** %4, align 8
  %8 = load i32, i32* %5, align 4
  %9 = load i32, i32* %6, align 4
  %10 = mul nsw i32 %8, %9
  ret i32 %10
}

; Function Attrs: noinline norecurse optnone uwtable
define i32 @main() #4 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca %struct.R, align 1
  %6 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  %7 = call i32 @_Z3addii(i32 4, i32 5)
  store i32 %7, i32* %2, align 4
  %8 = load %struct.Adder*, %struct.Adder** @A, align 8
  %9 = bitcast %struct.Adder* %8 to i32 (%struct.Adder*, i32, i32)***
  %10 = load i32 (%struct.Adder*, i32, i32)**, i32 (%struct.Adder*, i32, i32)*** %9, align 8
  %11 = getelementptr inbounds i32 (%struct.Adder*, i32, i32)*, i32 (%struct.Adder*, i32, i32)** %10, i64 0
  %12 = load i32 (%struct.Adder*, i32, i32)*, i32 (%struct.Adder*, i32, i32)** %11, align 8
  %13 = call i32 %12(%struct.Adder* %8, i32 1, i32 2)
  store i32 %13, i32* %3, align 4
  %14 = load i32 (i32, i32)*, i32 (i32, i32)** @F, align 8
  %15 = call i32 %14(i32 1, i32 2)
  store i32 %15, i32* %4, align 4
  %16 = load { i64, i64 }, { i64, i64 }* @M, align 8
  %17 = extractvalue { i64, i64 } %16, 1
  %18 = bitcast %struct.R* %5 to i8*
  %19 = getelementptr inbounds i8, i8* %18, i64 %17
  %20 = bitcast i8* %19 to %struct.R*
  %21 = extractvalue { i64, i64 } %16, 0
  %22 = and i64 %21, 1
  %23 = icmp ne i64 %22, 0
  br i1 %23, label %24, label %31

; <label>:24:                                     ; preds = %0
  %25 = bitcast %struct.R* %20 to i8**
  %26 = load i8*, i8** %25, align 8
  %27 = sub i64 %21, 1
  %28 = getelementptr i8, i8* %26, i64 %27
  %29 = bitcast i8* %28 to i32 (%struct.R*, i32, i32)**
  %30 = load i32 (%struct.R*, i32, i32)*, i32 (%struct.R*, i32, i32)** %29, align 8
  br label %33

; <label>:31:                                     ; preds = %0
  %32 = inttoptr i64 %21 to i32 (%struct.R*, i32, i32)*
  br label %33

; <label>:33:                                     ; preds = %31, %24
  %34 = phi i32 (%struct.R*, i32, i32)* [ %30, %24 ], [ %32, %31 ]
  %35 = call i32 %34(%struct.R* %20, i32 2, i32 3)
  store i32 %35, i32* %6, align 4
  ret i32 0
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr void @_ZN5AdderC2Ev(%struct.Adder*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.Adder*, align 8
  store %struct.Adder* %0, %struct.Adder** %2, align 8
  %3 = load %struct.Adder*, %struct.Adder** %2, align 8
  %4 = bitcast %struct.Adder* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ({ [3 x i8*] }, { [3 x i8*] }* @_ZTV5Adder, i32 0, inrange i32 0, i32 2) to i32 (...)**), i32 (...)*** %4, align 8
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr i32 @_ZN1S3addEii(%struct.S*, i32, i32) unnamed_addr #2 comdat align 2 {
  %4 = alloca %struct.S*, align 8
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  store %struct.S* %0, %struct.S** %4, align 8
  store i32 %1, i32* %5, align 4
  store i32 %2, i32* %6, align 4
  %7 = load %struct.S*, %struct.S** %4, align 8
  %8 = load i32, i32* %5, align 4
  %9 = load i32, i32* %6, align 4
  %10 = add nsw i32 %8, %9
  ret i32 %10
}

declare void @__cxa_pure_virtual() unnamed_addr

; Function Attrs: noinline uwtable
define internal void @_GLOBAL__sub_I_functions2.cpp() #3 section ".text.startup" {
  call void @__cxx_global_var_init()
  ret void
}

attributes #0 = { noinline optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nobuiltin "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { noinline uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { noinline norecurse optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { builtin }
attributes #6 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 5.0.1 (tags/RELEASE_501/final 332326)"}

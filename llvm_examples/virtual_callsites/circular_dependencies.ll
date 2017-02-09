; ModuleID = 'circular_dependencies.cpp'
source_filename = "circular_dependencies.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.A = type { i32 (...)** }
%struct.B = type { %struct.A }
%struct.C = type { %struct.A }

$_ZN1BC2Ev = comdat any

$_ZN1CC2Ev = comdat any

$_ZN1AC2Ev = comdat any

@field = global %struct.A* null, align 8
@_ZTV1A = unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8* }* @_ZTI1A to i8*), i8* bitcast (void (%struct.A*)* @_ZN1A3fooEv to i8*)], align 8
@_ZTVN10__cxxabiv117__class_type_infoE = external global i8*
@_ZTS1A = constant [3 x i8] c"1A\00"
@_ZTI1A = constant { i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv117__class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @_ZTS1A, i32 0, i32 0) }
@_ZTV1B = unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI1B to i8*), i8* bitcast (void (%struct.B*)* @_ZN1B3fooEv to i8*)], align 8
@_ZTVN10__cxxabiv120__si_class_type_infoE = external global i8*
@_ZTS1B = constant [3 x i8] c"1B\00"
@_ZTI1B = constant { i8*, i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @_ZTS1B, i32 0, i32 0), i8* bitcast ({ i8*, i8* }* @_ZTI1A to i8*) }
@_ZTV1C = unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI1C to i8*), i8* bitcast (void (%struct.C*)* @_ZN1C3fooEv to i8*)], align 8
@_ZTS1C = constant [3 x i8] c"1C\00"
@_ZTI1C = constant { i8*, i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @_ZTS1C, i32 0, i32 0), i8* bitcast ({ i8*, i8* }* @_ZTI1A to i8*) }

; Function Attrs: uwtable
define void @_ZN1A3fooEv(%struct.A*) unnamed_addr #0 align 2 {
  %2 = alloca %struct.A*, align 8
  store %struct.A* %0, %struct.A** %2, align 8
  %3 = load %struct.A*, %struct.A** %2, align 8
  %4 = call i8* @_Znwm(i64 8) #6
  %5 = bitcast i8* %4 to %struct.B*
  call void @_ZN1BC2Ev(%struct.B* %5) #7
  %6 = bitcast %struct.B* %5 to %struct.A*
  store %struct.A* %6, %struct.A** @field, align 8
  ret void
}

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #1

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN1BC2Ev(%struct.B*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.B*, align 8
  store %struct.B* %0, %struct.B** %2, align 8
  %3 = load %struct.B*, %struct.B** %2, align 8
  %4 = bitcast %struct.B* %3 to %struct.A*
  call void @_ZN1AC2Ev(%struct.A* %4) #7
  %5 = bitcast %struct.B* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]* @_ZTV1B, i32 0, i32 2) to i32 (...)**), i32 (...)*** %5, align 8
  ret void
}

; Function Attrs: uwtable
define void @_ZN1B3fooEv(%struct.B*) unnamed_addr #0 align 2 {
  %2 = alloca %struct.B*, align 8
  store %struct.B* %0, %struct.B** %2, align 8
  %3 = load %struct.B*, %struct.B** %2, align 8
  %4 = call i8* @_Znwm(i64 8) #6
  %5 = bitcast i8* %4 to %struct.C*
  call void @_ZN1CC2Ev(%struct.C* %5) #7
  %6 = bitcast %struct.C* %5 to %struct.A*
  store %struct.A* %6, %struct.A** @field, align 8
  ret void
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN1CC2Ev(%struct.C*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.C*, align 8
  store %struct.C* %0, %struct.C** %2, align 8
  %3 = load %struct.C*, %struct.C** %2, align 8
  %4 = bitcast %struct.C* %3 to %struct.A*
  call void @_ZN1AC2Ev(%struct.A* %4) #7
  %5 = bitcast %struct.C* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]* @_ZTV1C, i32 0, i32 2) to i32 (...)**), i32 (...)*** %5, align 8
  ret void
}

; Function Attrs: nounwind uwtable
define void @_ZN1C3fooEv(%struct.C*) unnamed_addr #3 align 2 {
  %2 = alloca %struct.C*, align 8
  store %struct.C* %0, %struct.C** %2, align 8
  %3 = load %struct.C*, %struct.C** %2, align 8
  ret void
}

; Function Attrs: norecurse uwtable
define i32 @main() #4 {
  %1 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  %2 = call i8* @_Znwm(i64 8) #6
  %3 = bitcast i8* %2 to %struct.A*
  %4 = bitcast %struct.A* %3 to i8*
  call void @llvm.memset.p0i8.i64(i8* %4, i8 0, i64 8, i32 8, i1 false)
  call void @_ZN1AC2Ev(%struct.A* %3) #7
  store %struct.A* %3, %struct.A** @field, align 8
  %5 = load %struct.A*, %struct.A** @field, align 8
  %6 = bitcast %struct.A* %5 to void (%struct.A*)***
  %7 = load void (%struct.A*)**, void (%struct.A*)*** %6, align 8
  %8 = getelementptr inbounds void (%struct.A*)*, void (%struct.A*)** %7, i64 0
  %9 = load void (%struct.A*)*, void (%struct.A*)** %8, align 8
  call void %9(%struct.A* %5)
  ret i32 0
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #5

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN1AC2Ev(%struct.A*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.A*, align 8
  store %struct.A* %0, %struct.A** %2, align 8
  %3 = load %struct.A*, %struct.A** %2, align 8
  %4 = bitcast %struct.A* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]* @_ZTV1A, i32 0, i32 2) to i32 (...)**), i32 (...)*** %4, align 8
  ret void
}

attributes #0 = { uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nobuiltin "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { inlinehint nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { norecurse uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { argmemonly nounwind }
attributes #6 = { builtin }
attributes #7 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

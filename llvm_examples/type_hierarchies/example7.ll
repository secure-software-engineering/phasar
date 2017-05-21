; ModuleID = 'example7.ll.pp'
source_filename = "example7.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.Z = type { %struct.C, %struct.Y }
%struct.C = type { %struct.A }
%struct.A = type { i32 (...)** }
%struct.Y = type { %struct.X }
%struct.X = type { i32 (...)** }
%struct.D = type { %struct.B }
%struct.B = type { %struct.A }

$_ZN1ZC2Ev = comdat any

$_ZN1DC2Ev = comdat any

$_ZN1CC2Ev = comdat any

$_ZN1YC2Ev = comdat any

$_ZN1A1fEv = comdat any

$_ZN1X1gEv = comdat any

$_ZN1AC2Ev = comdat any

$_ZN1XC2Ev = comdat any

$_ZN1BC2Ev = comdat any

$_ZTV1Z = comdat any

$_ZTS1Z = comdat any

$_ZTS1C = comdat any

$_ZTS1A = comdat any

$_ZTI1A = comdat any

$_ZTI1C = comdat any

$_ZTS1Y = comdat any

$_ZTS1X = comdat any

$_ZTI1X = comdat any

$_ZTI1Y = comdat any

$_ZTI1Z = comdat any

$_ZTV1C = comdat any

$_ZTV1A = comdat any

$_ZTV1Y = comdat any

$_ZTV1X = comdat any

$_ZTV1D = comdat any

$_ZTS1D = comdat any

$_ZTS1B = comdat any

$_ZTI1B = comdat any

$_ZTI1D = comdat any

$_ZTV1B = comdat any

@_ZTV1Z = linkonce_odr unnamed_addr constant [6 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i32, i32, i8*, i64, i8*, i64 }* @_ZTI1Z to i8*), i8* bitcast (i32 (%struct.A*)* @_ZN1A1fEv to i8*), i8* inttoptr (i64 -8 to i8*), i8* bitcast ({ i8*, i8*, i32, i32, i8*, i64, i8*, i64 }* @_ZTI1Z to i8*), i8* bitcast (i32 (%struct.X*)* @_ZN1X1gEv to i8*)], comdat, align 8
@_ZTVN10__cxxabiv121__vmi_class_type_infoE = external global i8*
@_ZTS1Z = linkonce_odr constant [3 x i8] c"1Z\00", comdat
@_ZTVN10__cxxabiv120__si_class_type_infoE = external global i8*
@_ZTS1C = linkonce_odr constant [3 x i8] c"1C\00", comdat
@_ZTVN10__cxxabiv117__class_type_infoE = external global i8*
@_ZTS1A = linkonce_odr constant [3 x i8] c"1A\00", comdat
@_ZTI1A = linkonce_odr constant { i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv117__class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @_ZTS1A, i32 0, i32 0) }, comdat
@_ZTI1C = linkonce_odr constant { i8*, i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @_ZTS1C, i32 0, i32 0), i8* bitcast ({ i8*, i8* }* @_ZTI1A to i8*) }, comdat
@_ZTS1Y = linkonce_odr constant [3 x i8] c"1Y\00", comdat
@_ZTS1X = linkonce_odr constant [3 x i8] c"1X\00", comdat
@_ZTI1X = linkonce_odr constant { i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv117__class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @_ZTS1X, i32 0, i32 0) }, comdat
@_ZTI1Y = linkonce_odr constant { i8*, i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @_ZTS1Y, i32 0, i32 0), i8* bitcast ({ i8*, i8* }* @_ZTI1X to i8*) }, comdat
@_ZTI1Z = linkonce_odr constant { i8*, i8*, i32, i32, i8*, i64, i8*, i64 } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv121__vmi_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @_ZTS1Z, i32 0, i32 0), i32 0, i32 2, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI1C to i8*), i64 2, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI1Y to i8*), i64 2050 }, comdat
@_ZTV1C = linkonce_odr unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI1C to i8*), i8* bitcast (i32 (%struct.A*)* @_ZN1A1fEv to i8*)], comdat, align 8
@_ZTV1A = linkonce_odr unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8* }* @_ZTI1A to i8*), i8* bitcast (i32 (%struct.A*)* @_ZN1A1fEv to i8*)], comdat, align 8
@_ZTV1Y = linkonce_odr unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI1Y to i8*), i8* bitcast (i32 (%struct.X*)* @_ZN1X1gEv to i8*)], comdat, align 8
@_ZTV1X = linkonce_odr unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8* }* @_ZTI1X to i8*), i8* bitcast (i32 (%struct.X*)* @_ZN1X1gEv to i8*)], comdat, align 8
@_ZTV1D = linkonce_odr unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI1D to i8*), i8* bitcast (i32 (%struct.A*)* @_ZN1A1fEv to i8*)], comdat, align 8
@_ZTS1D = linkonce_odr constant [3 x i8] c"1D\00", comdat
@_ZTS1B = linkonce_odr constant [3 x i8] c"1B\00", comdat
@_ZTI1B = linkonce_odr constant { i8*, i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @_ZTS1B, i32 0, i32 0), i8* bitcast ({ i8*, i8* }* @_ZTI1A to i8*) }, comdat
@_ZTI1D = linkonce_odr constant { i8*, i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @_ZTS1D, i32 0, i32 0), i8* bitcast ({ i8*, i8*, i8* }* @_ZTI1B to i8*) }, comdat
@_ZTV1B = linkonce_odr unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI1B to i8*), i8* bitcast (i32 (%struct.A*)* @_ZN1A1fEv to i8*)], comdat, align 8

; Function Attrs: norecurse nounwind uwtable
define i32 @main() #0 {
  %1 = alloca %struct.Z, align 8
  %2 = alloca %struct.D, align 8
  call void @_ZN1ZC2Ev(%struct.Z* %1) #3
  call void @_ZN1DC2Ev(%struct.D* %2) #3
  ret i32 0
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN1ZC2Ev(%struct.Z*) unnamed_addr #1 comdat align 2 {
  %2 = bitcast %struct.Z* %0 to %struct.C*
  call void @_ZN1CC2Ev(%struct.C* %2) #3
  %3 = bitcast %struct.Z* %0 to i8*
  %4 = getelementptr inbounds i8, i8* %3, i64 8
  %5 = bitcast i8* %4 to %struct.Y*
  call void @_ZN1YC2Ev(%struct.Y* %5) #3
  %6 = bitcast %struct.Z* %0 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([6 x i8*], [6 x i8*]* @_ZTV1Z, i32 0, i32 2) to i32 (...)**), i32 (...)*** %6, align 8
  %7 = bitcast %struct.Z* %0 to i8*
  %8 = getelementptr inbounds i8, i8* %7, i64 8
  %9 = bitcast i8* %8 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([6 x i8*], [6 x i8*]* @_ZTV1Z, i32 0, i32 5) to i32 (...)**), i32 (...)*** %9, align 8
  ret void
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN1DC2Ev(%struct.D*) unnamed_addr #1 comdat align 2 {
  %2 = bitcast %struct.D* %0 to %struct.B*
  call void @_ZN1BC2Ev(%struct.B* %2) #3
  %3 = bitcast %struct.D* %0 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]* @_ZTV1D, i32 0, i32 2) to i32 (...)**), i32 (...)*** %3, align 8
  ret void
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN1CC2Ev(%struct.C*) unnamed_addr #1 comdat align 2 {
  %2 = bitcast %struct.C* %0 to %struct.A*
  call void @_ZN1AC2Ev(%struct.A* %2) #3
  %3 = bitcast %struct.C* %0 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]* @_ZTV1C, i32 0, i32 2) to i32 (...)**), i32 (...)*** %3, align 8
  ret void
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN1YC2Ev(%struct.Y*) unnamed_addr #1 comdat align 2 {
  %2 = bitcast %struct.Y* %0 to %struct.X*
  call void @_ZN1XC2Ev(%struct.X* %2) #3
  %3 = bitcast %struct.Y* %0 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]* @_ZTV1Y, i32 0, i32 2) to i32 (...)**), i32 (...)*** %3, align 8
  ret void
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZN1A1fEv(%struct.A*) unnamed_addr #2 comdat align 2 {
  ret i32 0
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZN1X1gEv(%struct.X*) unnamed_addr #2 comdat align 2 {
  ret i32 1
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN1AC2Ev(%struct.A*) unnamed_addr #1 comdat align 2 {
  %2 = bitcast %struct.A* %0 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]* @_ZTV1A, i32 0, i32 2) to i32 (...)**), i32 (...)*** %2, align 8
  ret void
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN1XC2Ev(%struct.X*) unnamed_addr #1 comdat align 2 {
  %2 = bitcast %struct.X* %0 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]* @_ZTV1X, i32 0, i32 2) to i32 (...)**), i32 (...)*** %2, align 8
  ret void
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN1BC2Ev(%struct.B*) unnamed_addr #1 comdat align 2 {
  %2 = bitcast %struct.B* %0 to %struct.A*
  call void @_ZN1AC2Ev(%struct.A* %2) #3
  %3 = bitcast %struct.B* %0 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]* @_ZTV1B, i32 0, i32 2) to i32 (...)**), i32 (...)*** %3, align 8
  ret void
}

attributes #0 = { norecurse nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { inlinehint nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.1-svn288847-1~exp1 (branches/release_39)"}

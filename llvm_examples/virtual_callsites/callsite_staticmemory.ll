; ModuleID = 'callgraph_staticmemory.cpp'
source_filename = "callgraph_staticmemory.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.Derived = type { %struct.Base }
%struct.Base = type { i32 (...)** }

$_ZN7DerivedC2Ev = comdat any

$_ZN4BaseC2Ev = comdat any

$_ZN7Derived3fooEv = comdat any

$_ZN4Base3fooEv = comdat any

$_ZTV7Derived = comdat any

$_ZTS7Derived = comdat any

$_ZTS4Base = comdat any

$_ZTI4Base = comdat any

$_ZTI7Derived = comdat any

$_ZTV4Base = comdat any

@_ZTV7Derived = linkonce_odr unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI7Derived to i8*), i8* bitcast (void (%struct.Derived*)* @_ZN7Derived3fooEv to i8*)], comdat, align 8
@_ZTVN10__cxxabiv120__si_class_type_infoE = external global i8*
@_ZTS7Derived = linkonce_odr constant [9 x i8] c"7Derived\00", comdat
@_ZTVN10__cxxabiv117__class_type_infoE = external global i8*
@_ZTS4Base = linkonce_odr constant [6 x i8] c"4Base\00", comdat
@_ZTI4Base = linkonce_odr constant { i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv117__class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([6 x i8], [6 x i8]* @_ZTS4Base, i32 0, i32 0) }, comdat
@_ZTI7Derived = linkonce_odr constant { i8*, i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([9 x i8], [9 x i8]* @_ZTS7Derived, i32 0, i32 0), i8* bitcast ({ i8*, i8* }* @_ZTI4Base to i8*) }, comdat
@_ZTV4Base = linkonce_odr unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8* }* @_ZTI4Base to i8*), i8* bitcast (void (%struct.Base*)* @_ZN4Base3fooEv to i8*)], comdat, align 8

; Function Attrs: norecurse uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.Derived, align 8
  %3 = alloca %struct.Base*, align 8
  store i32 0, i32* %1, align 4
  call void @_ZN7DerivedC2Ev(%struct.Derived* %2) #3
  %4 = bitcast %struct.Derived* %2 to %struct.Base*
  store %struct.Base* %4, %struct.Base** %3, align 8
  %5 = load %struct.Base*, %struct.Base** %3, align 8
  %6 = bitcast %struct.Base* %5 to void (%struct.Base*)***
  %7 = load void (%struct.Base*)**, void (%struct.Base*)*** %6, align 8
  %8 = getelementptr inbounds void (%struct.Base*)*, void (%struct.Base*)** %7, i64 0
  %9 = load void (%struct.Base*)*, void (%struct.Base*)** %8, align 8
  call void %9(%struct.Base* %5)
  ret i32 0
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN7DerivedC2Ev(%struct.Derived*) unnamed_addr #1 comdat align 2 {
  %2 = alloca %struct.Derived*, align 8
  store %struct.Derived* %0, %struct.Derived** %2, align 8
  %3 = load %struct.Derived*, %struct.Derived** %2, align 8
  %4 = bitcast %struct.Derived* %3 to %struct.Base*
  call void @_ZN4BaseC2Ev(%struct.Base* %4) #3
  %5 = bitcast %struct.Derived* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]* @_ZTV7Derived, i32 0, i32 2) to i32 (...)**), i32 (...)*** %5, align 8
  ret void
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN4BaseC2Ev(%struct.Base*) unnamed_addr #1 comdat align 2 {
  %2 = alloca %struct.Base*, align 8
  store %struct.Base* %0, %struct.Base** %2, align 8
  %3 = load %struct.Base*, %struct.Base** %2, align 8
  %4 = bitcast %struct.Base* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]* @_ZTV4Base, i32 0, i32 2) to i32 (...)**), i32 (...)*** %4, align 8
  ret void
}

; Function Attrs: nounwind uwtable
define linkonce_odr void @_ZN7Derived3fooEv(%struct.Derived*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.Derived*, align 8
  store %struct.Derived* %0, %struct.Derived** %2, align 8
  %3 = load %struct.Derived*, %struct.Derived** %2, align 8
  ret void
}

; Function Attrs: nounwind uwtable
define linkonce_odr void @_ZN4Base3fooEv(%struct.Base*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.Base*, align 8
  store %struct.Base* %0, %struct.Base** %2, align 8
  %3 = load %struct.Base*, %struct.Base** %2, align 8
  ret void
}

attributes #0 = { norecurse uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { inlinehint nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

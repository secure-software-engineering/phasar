; ModuleID = 'inter_callgraph.cpp'
source_filename = "inter_callgraph.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.Base = type { i32 (...)** }
%struct.Derived = type { %struct.Base }

$_ZN7DerivedC2Ev = comdat any

$_ZN4BaseC2Ev = comdat any

$_ZN7Derived3fooEv = comdat any

$_ZN7Derived3barEv = comdat any

$_ZN4Base3fooEv = comdat any

$_ZN4Base3barEv = comdat any

$_ZTV7Derived = comdat any

$_ZTS7Derived = comdat any

$_ZTS4Base = comdat any

$_ZTI4Base = comdat any

$_ZTI7Derived = comdat any

$_ZTV4Base = comdat any

@_ZTV7Derived = linkonce_odr unnamed_addr constant [4 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI7Derived to i8*), i8* bitcast (void (%struct.Derived*)* @_ZN7Derived3fooEv to i8*), i8* bitcast (i32 (%struct.Derived*)* @_ZN7Derived3barEv to i8*)], comdat, align 8
@_ZTVN10__cxxabiv120__si_class_type_infoE = external global i8*
@_ZTS7Derived = linkonce_odr constant [9 x i8] c"7Derived\00", comdat
@_ZTVN10__cxxabiv117__class_type_infoE = external global i8*
@_ZTS4Base = linkonce_odr constant [6 x i8] c"4Base\00", comdat
@_ZTI4Base = linkonce_odr constant { i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv117__class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([6 x i8], [6 x i8]* @_ZTS4Base, i32 0, i32 0) }, comdat
@_ZTI7Derived = linkonce_odr constant { i8*, i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([9 x i8], [9 x i8]* @_ZTS7Derived, i32 0, i32 0), i8* bitcast ({ i8*, i8* }* @_ZTI4Base to i8*) }, comdat
@_ZTV4Base = linkonce_odr unnamed_addr constant [4 x i8*] [i8* null, i8* bitcast ({ i8*, i8* }* @_ZTI4Base to i8*), i8* bitcast (void (%struct.Base*)* @_ZN4Base3fooEv to i8*), i8* bitcast (i32 (%struct.Base*)* @_ZN4Base3barEv to i8*)], comdat, align 8

; Function Attrs: uwtable
define void @_Z12callFunctionR4Base(%struct.Base* dereferenceable(8)) #0 {
  %2 = alloca %struct.Base*, align 8
  %3 = alloca i32, align 4
  store %struct.Base* %0, %struct.Base** %2, align 8
  %4 = load %struct.Base*, %struct.Base** %2, align 8
  %5 = bitcast %struct.Base* %4 to i32 (%struct.Base*)***
  %6 = load i32 (%struct.Base*)**, i32 (%struct.Base*)*** %5, align 8
  %7 = getelementptr inbounds i32 (%struct.Base*)*, i32 (%struct.Base*)** %6, i64 1
  %8 = load i32 (%struct.Base*)*, i32 (%struct.Base*)** %7, align 8
  %9 = call i32 %8(%struct.Base* %4)
  store i32 %9, i32* %3, align 4
  ret void
}

; Function Attrs: norecurse uwtable
define i32 @main() #1 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.Derived, align 8
  store i32 0, i32* %1, align 4
  call void @_ZN7DerivedC2Ev(%struct.Derived* %2) #4
  %3 = bitcast %struct.Derived* %2 to %struct.Base*
  call void @_Z12callFunctionR4Base(%struct.Base* dereferenceable(8) %3)
  ret i32 0
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN7DerivedC2Ev(%struct.Derived*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.Derived*, align 8
  store %struct.Derived* %0, %struct.Derived** %2, align 8
  %3 = load %struct.Derived*, %struct.Derived** %2, align 8
  %4 = bitcast %struct.Derived* %3 to %struct.Base*
  call void @_ZN4BaseC2Ev(%struct.Base* %4) #4
  %5 = bitcast %struct.Derived* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([4 x i8*], [4 x i8*]* @_ZTV7Derived, i32 0, i32 2) to i32 (...)**), i32 (...)*** %5, align 8
  ret void
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN4BaseC2Ev(%struct.Base*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.Base*, align 8
  store %struct.Base* %0, %struct.Base** %2, align 8
  %3 = load %struct.Base*, %struct.Base** %2, align 8
  %4 = bitcast %struct.Base* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([4 x i8*], [4 x i8*]* @_ZTV4Base, i32 0, i32 2) to i32 (...)**), i32 (...)*** %4, align 8
  ret void
}

; Function Attrs: nounwind uwtable
define linkonce_odr void @_ZN7Derived3fooEv(%struct.Derived*) unnamed_addr #3 comdat align 2 {
  %2 = alloca %struct.Derived*, align 8
  store %struct.Derived* %0, %struct.Derived** %2, align 8
  %3 = load %struct.Derived*, %struct.Derived** %2, align 8
  ret void
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZN7Derived3barEv(%struct.Derived*) unnamed_addr #3 comdat align 2 {
  %2 = alloca %struct.Derived*, align 8
  store %struct.Derived* %0, %struct.Derived** %2, align 8
  %3 = load %struct.Derived*, %struct.Derived** %2, align 8
  ret i32 2
}

; Function Attrs: nounwind uwtable
define linkonce_odr void @_ZN4Base3fooEv(%struct.Base*) unnamed_addr #3 comdat align 2 {
  %2 = alloca %struct.Base*, align 8
  store %struct.Base* %0, %struct.Base** %2, align 8
  %3 = load %struct.Base*, %struct.Base** %2, align 8
  ret void
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZN4Base3barEv(%struct.Base*) unnamed_addr #3 comdat align 2 {
  %2 = alloca %struct.Base*, align 8
  store %struct.Base* %0, %struct.Base** %2, align 8
  %3 = load %struct.Base*, %struct.Base** %2, align 8
  ret i32 1
}

attributes #0 = { uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { norecurse uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { inlinehint nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

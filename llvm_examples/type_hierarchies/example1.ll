; ModuleID = 'example1.cpp'
source_filename = "example1.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.Child = type { %struct.Base }
%struct.Base = type { i32 (...)** }

$_ZN5ChildC2Ev = comdat any

$_ZN4BaseC2Ev = comdat any

$_ZN5Child3fooEv = comdat any

$_ZN4Base3fooEv = comdat any

$_ZTV5Child = comdat any

$_ZTS5Child = comdat any

$_ZTS4Base = comdat any

$_ZTI4Base = comdat any

$_ZTI5Child = comdat any

$_ZTV4Base = comdat any

@_ZTV5Child = linkonce_odr unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI5Child to i8*), i8* bitcast (i32 (%struct.Child*)* @_ZN5Child3fooEv to i8*)], comdat, align 8
@_ZTVN10__cxxabiv120__si_class_type_infoE = external global i8*
@_ZTS5Child = linkonce_odr constant [7 x i8] c"5Child\00", comdat
@_ZTVN10__cxxabiv117__class_type_infoE = external global i8*
@_ZTS4Base = linkonce_odr constant [6 x i8] c"4Base\00", comdat
@_ZTI4Base = linkonce_odr constant { i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv117__class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([6 x i8], [6 x i8]* @_ZTS4Base, i32 0, i32 0) }, comdat
@_ZTI5Child = linkonce_odr constant { i8*, i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([7 x i8], [7 x i8]* @_ZTS5Child, i32 0, i32 0), i8* bitcast ({ i8*, i8* }* @_ZTI4Base to i8*) }, comdat
@_ZTV4Base = linkonce_odr unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8* }* @_ZTI4Base to i8*), i8* bitcast (i32 (%struct.Base*)* @_ZN4Base3fooEv to i8*)], comdat, align 8

; Function Attrs: norecurse nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.Child, align 8
  store i32 0, i32* %1, align 4
  call void @_ZN5ChildC2Ev(%struct.Child* %2) #3
  ret i32 0
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN5ChildC2Ev(%struct.Child*) unnamed_addr #1 comdat align 2 {
  %2 = alloca %struct.Child*, align 8
  store %struct.Child* %0, %struct.Child** %2, align 8
  %3 = load %struct.Child*, %struct.Child** %2, align 8
  %4 = bitcast %struct.Child* %3 to %struct.Base*
  call void @_ZN4BaseC2Ev(%struct.Base* %4) #3
  %5 = bitcast %struct.Child* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]* @_ZTV5Child, i32 0, i32 2) to i32 (...)**), i32 (...)*** %5, align 8
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
define linkonce_odr i32 @_ZN5Child3fooEv(%struct.Child*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.Child*, align 8
  store %struct.Child* %0, %struct.Child** %2, align 8
  %3 = load %struct.Child*, %struct.Child** %2, align 8
  ret i32 2
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZN4Base3fooEv(%struct.Base*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.Base*, align 8
  store %struct.Base* %0, %struct.Base** %2, align 8
  %3 = load %struct.Base*, %struct.Base** %2, align 8
  ret i32 1
}

attributes #0 = { norecurse nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { inlinehint nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

; ModuleID = 'example5.cpp'
source_filename = "example5.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.Child = type { %struct.Base, %struct.OtherBase }
%struct.Base = type { i32 (...)** }
%struct.OtherBase = type { i32 (...)** }

$_ZN5ChildC2Ev = comdat any

$_ZN4BaseC2Ev = comdat any

$_ZN9OtherBaseC2Ev = comdat any

$_ZN5Child3fooEv = comdat any

$_ZN4Base3barEv = comdat any

$_ZN5Child3bazEv = comdat any

$_ZN5Child3tarEv = comdat any

$_ZThn8_N5Child3bazEv = comdat any

$_ZN4Base3fooEv = comdat any

$_ZN9OtherBase3bazEv = comdat any

$_ZTV5Child = comdat any

$_ZTS5Child = comdat any

$_ZTS4Base = comdat any

$_ZTI4Base = comdat any

$_ZTS9OtherBase = comdat any

$_ZTI9OtherBase = comdat any

$_ZTI5Child = comdat any

$_ZTV4Base = comdat any

$_ZTV9OtherBase = comdat any

@_ZTV5Child = linkonce_odr unnamed_addr constant [9 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i32, i32, i8*, i64, i8*, i64 }* @_ZTI5Child to i8*), i8* bitcast (i32 (%struct.Child*)* @_ZN5Child3fooEv to i8*), i8* bitcast (i32 (%struct.Base*)* @_ZN4Base3barEv to i8*), i8* bitcast (i32 (%struct.Child*)* @_ZN5Child3bazEv to i8*), i8* bitcast (i32 (%struct.Child*)* @_ZN5Child3tarEv to i8*), i8* inttoptr (i64 -8 to i8*), i8* bitcast ({ i8*, i8*, i32, i32, i8*, i64, i8*, i64 }* @_ZTI5Child to i8*), i8* bitcast (i32 (%struct.Child*)* @_ZThn8_N5Child3bazEv to i8*)], comdat, align 8
@_ZTVN10__cxxabiv121__vmi_class_type_infoE = external global i8*
@_ZTS5Child = linkonce_odr constant [7 x i8] c"5Child\00", comdat
@_ZTVN10__cxxabiv117__class_type_infoE = external global i8*
@_ZTS4Base = linkonce_odr constant [6 x i8] c"4Base\00", comdat
@_ZTI4Base = linkonce_odr constant { i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv117__class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([6 x i8], [6 x i8]* @_ZTS4Base, i32 0, i32 0) }, comdat
@_ZTS9OtherBase = linkonce_odr constant [11 x i8] c"9OtherBase\00", comdat
@_ZTI9OtherBase = linkonce_odr constant { i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv117__class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([11 x i8], [11 x i8]* @_ZTS9OtherBase, i32 0, i32 0) }, comdat
@_ZTI5Child = linkonce_odr constant { i8*, i8*, i32, i32, i8*, i64, i8*, i64 } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv121__vmi_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([7 x i8], [7 x i8]* @_ZTS5Child, i32 0, i32 0), i32 0, i32 2, i8* bitcast ({ i8*, i8* }* @_ZTI4Base to i8*), i64 2, i8* bitcast ({ i8*, i8* }* @_ZTI9OtherBase to i8*), i64 2050 }, comdat
@_ZTV4Base = linkonce_odr unnamed_addr constant [4 x i8*] [i8* null, i8* bitcast ({ i8*, i8* }* @_ZTI4Base to i8*), i8* bitcast (i32 (%struct.Base*)* @_ZN4Base3fooEv to i8*), i8* bitcast (i32 (%struct.Base*)* @_ZN4Base3barEv to i8*)], comdat, align 8
@_ZTV9OtherBase = linkonce_odr unnamed_addr constant [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8* }* @_ZTI9OtherBase to i8*), i8* bitcast (i32 (%struct.OtherBase*)* @_ZN9OtherBase3bazEv to i8*)], comdat, align 8

; Function Attrs: norecurse nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.Child, align 8
  store i32 0, i32* %1, align 4
  call void @_ZN5ChildC2Ev(%struct.Child* %2) #4
  ret i32 0
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN5ChildC2Ev(%struct.Child*) unnamed_addr #1 comdat align 2 {
  %2 = alloca %struct.Child*, align 8
  store %struct.Child* %0, %struct.Child** %2, align 8
  %3 = load %struct.Child*, %struct.Child** %2, align 8
  %4 = bitcast %struct.Child* %3 to %struct.Base*
  call void @_ZN4BaseC2Ev(%struct.Base* %4) #4
  %5 = bitcast %struct.Child* %3 to i8*
  %6 = getelementptr inbounds i8, i8* %5, i64 8
  %7 = bitcast i8* %6 to %struct.OtherBase*
  call void @_ZN9OtherBaseC2Ev(%struct.OtherBase* %7) #4
  %8 = bitcast %struct.Child* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([9 x i8*], [9 x i8*]* @_ZTV5Child, i32 0, i32 2) to i32 (...)**), i32 (...)*** %8, align 8
  %9 = bitcast %struct.Child* %3 to i8*
  %10 = getelementptr inbounds i8, i8* %9, i64 8
  %11 = bitcast i8* %10 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([9 x i8*], [9 x i8*]* @_ZTV5Child, i32 0, i32 8) to i32 (...)**), i32 (...)*** %11, align 8
  ret void
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN4BaseC2Ev(%struct.Base*) unnamed_addr #1 comdat align 2 {
  %2 = alloca %struct.Base*, align 8
  store %struct.Base* %0, %struct.Base** %2, align 8
  %3 = load %struct.Base*, %struct.Base** %2, align 8
  %4 = bitcast %struct.Base* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([4 x i8*], [4 x i8*]* @_ZTV4Base, i32 0, i32 2) to i32 (...)**), i32 (...)*** %4, align 8
  ret void
}

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN9OtherBaseC2Ev(%struct.OtherBase*) unnamed_addr #1 comdat align 2 {
  %2 = alloca %struct.OtherBase*, align 8
  store %struct.OtherBase* %0, %struct.OtherBase** %2, align 8
  %3 = load %struct.OtherBase*, %struct.OtherBase** %2, align 8
  %4 = bitcast %struct.OtherBase* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]* @_ZTV9OtherBase, i32 0, i32 2) to i32 (...)**), i32 (...)*** %4, align 8
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
define linkonce_odr i32 @_ZN4Base3barEv(%struct.Base*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.Base*, align 8
  store %struct.Base* %0, %struct.Base** %2, align 8
  %3 = load %struct.Base*, %struct.Base** %2, align 8
  ret i32 -1
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZN5Child3bazEv(%struct.Child*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.Child*, align 8
  store %struct.Child* %0, %struct.Child** %2, align 8
  %3 = load %struct.Child*, %struct.Child** %2, align 8
  ret i32 400
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZN5Child3tarEv(%struct.Child*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.Child*, align 8
  store %struct.Child* %0, %struct.Child** %2, align 8
  %3 = load %struct.Child*, %struct.Child** %2, align 8
  ret i32 100
}

; Function Attrs: uwtable
define linkonce_odr i32 @_ZThn8_N5Child3bazEv(%struct.Child*) unnamed_addr #3 comdat align 2 {
  %2 = alloca %struct.Child*, align 8
  store %struct.Child* %0, %struct.Child** %2, align 8
  %3 = load %struct.Child*, %struct.Child** %2, align 8
  %4 = bitcast %struct.Child* %3 to i8*
  %5 = getelementptr inbounds i8, i8* %4, i64 -8
  %6 = bitcast i8* %5 to %struct.Child*
  %7 = tail call i32 @_ZN5Child3bazEv(%struct.Child* %6)
  ret i32 %7
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZN4Base3fooEv(%struct.Base*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.Base*, align 8
  store %struct.Base* %0, %struct.Base** %2, align 8
  %3 = load %struct.Base*, %struct.Base** %2, align 8
  ret i32 1
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZN9OtherBase3bazEv(%struct.OtherBase*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.OtherBase*, align 8
  store %struct.OtherBase* %0, %struct.OtherBase** %2, align 8
  %3 = load %struct.OtherBase*, %struct.OtherBase** %2, align 8
  ret i32 42
}

attributes #0 = { norecurse nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { inlinehint nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

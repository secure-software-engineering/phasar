; ModuleID = 'base.cpp'
source_filename = "base.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.base = type { i32 (...)** }

@_ZTV4base = unnamed_addr constant [4 x i8*] [i8* null, i8* bitcast ({ i8*, i8* }* @_ZTI4base to i8*), i8* bitcast (i32 (%struct.base*)* @_ZN4base3fooEv to i8*), i8* bitcast (i32 (%struct.base*)* @_ZN4base3barEv to i8*)], align 8
@_ZTVN10__cxxabiv117__class_type_infoE = external global i8*
@_ZTS4base = constant [6 x i8] c"4base\00"
@_ZTI4base = constant { i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv117__class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([6 x i8], [6 x i8]* @_ZTS4base, i32 0, i32 0) }

; Function Attrs: nounwind uwtable
define i32 @_ZN4base3fooEv(%struct.base*) unnamed_addr #0 align 2 {
  %2 = alloca %struct.base*, align 8
  store %struct.base* %0, %struct.base** %2, align 8
  %3 = load %struct.base*, %struct.base** %2, align 8
  ret i32 10
}

; Function Attrs: nounwind uwtable
define i32 @_ZN4base3barEv(%struct.base*) unnamed_addr #0 align 2 {
  %2 = alloca %struct.base*, align 8
  store %struct.base* %0, %struct.base** %2, align 8
  %3 = load %struct.base*, %struct.base** %2, align 8
  ret i32 20
}

; Function Attrs: nounwind uwtable
define i32 @_ZN4base3carEv(%struct.base*) #0 align 2 {
  %2 = alloca %struct.base*, align 8
  store %struct.base* %0, %struct.base** %2, align 8
  %3 = load %struct.base*, %struct.base** %2, align 8
  ret i32 30
}

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

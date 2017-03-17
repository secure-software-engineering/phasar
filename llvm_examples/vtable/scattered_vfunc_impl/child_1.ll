; ModuleID = 'child_1.cpp'
source_filename = "child_1.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.child = type { %struct.base }
%struct.base = type { i32 (...)** }

@_ZTV5child = unnamed_addr constant [4 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI5child to i8*), i8* bitcast (i32 (%struct.child*)* @_ZN5child3fooEv to i8*), i8* bitcast (i32 (%struct.child*)* @_ZN5child3barEv to i8*)], align 8
@_ZTVN10__cxxabiv120__si_class_type_infoE = external global i8*
@_ZTS5child = constant [7 x i8] c"5child\00"
@_ZTI4base = external constant i8*
@_ZTI5child = constant { i8*, i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([7 x i8], [7 x i8]* @_ZTS5child, i32 0, i32 0), i8* bitcast (i8** @_ZTI4base to i8*) }

; Function Attrs: nounwind uwtable
define i32 @_ZN5child3fooEv(%struct.child*) unnamed_addr #0 align 2 {
  %2 = alloca %struct.child*, align 8
  store %struct.child* %0, %struct.child** %2, align 8
  %3 = load %struct.child*, %struct.child** %2, align 8
  ret i32 100
}

; Function Attrs: nounwind uwtable
define i32 @_ZN5child5otherEv(%struct.child*) #0 align 2 {
  %2 = alloca %struct.child*, align 8
  store %struct.child* %0, %struct.child** %2, align 8
  %3 = load %struct.child*, %struct.child** %2, align 8
  ret i32 300
}

declare i32 @_ZN5child3barEv(%struct.child*) unnamed_addr #1

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

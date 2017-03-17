; ModuleID = 'main.cpp'
source_filename = "main.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.base = type { i32 (...)** }
%struct.child = type { %struct.base }

$_ZN5childC2Ev = comdat any

$_ZN4baseC2Ev = comdat any

@_ZTV5child = external unnamed_addr constant [4 x i8*]
@_ZTV4base = external unnamed_addr constant [4 x i8*]

; Function Attrs: norecurse uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.base*, align 8
  store i32 0, i32* %1, align 4
  %3 = call i8* @_Znwm(i64 8) #4
  %4 = bitcast i8* %3 to %struct.child*
  call void @_ZN5childC2Ev(%struct.child* %4) #5
  %5 = bitcast %struct.child* %4 to %struct.base*
  store %struct.base* %5, %struct.base** %2, align 8
  %6 = load %struct.base*, %struct.base** %2, align 8
  %7 = bitcast %struct.base* %6 to i32 (%struct.base*)***
  %8 = load i32 (%struct.base*)**, i32 (%struct.base*)*** %7, align 8
  %9 = getelementptr inbounds i32 (%struct.base*)*, i32 (%struct.base*)** %8, i64 0
  %10 = load i32 (%struct.base*)*, i32 (%struct.base*)** %9, align 8
  %11 = call i32 %10(%struct.base* %6)
  %12 = load %struct.base*, %struct.base** %2, align 8
  %13 = icmp eq %struct.base* %12, null
  br i1 %13, label %16, label %14

; <label>:14:                                     ; preds = %0
  %15 = bitcast %struct.base* %12 to i8*
  call void @_ZdlPv(i8* %15) #6
  br label %16

; <label>:16:                                     ; preds = %14, %0
  ret i32 0
}

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #1

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN5childC2Ev(%struct.child*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.child*, align 8
  store %struct.child* %0, %struct.child** %2, align 8
  %3 = load %struct.child*, %struct.child** %2, align 8
  %4 = bitcast %struct.child* %3 to %struct.base*
  call void @_ZN4baseC2Ev(%struct.base* %4) #5
  %5 = bitcast %struct.child* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([4 x i8*], [4 x i8*]* @_ZTV5child, i32 0, i32 2) to i32 (...)**), i32 (...)*** %5, align 8
  ret void
}

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(i8*) #3

; Function Attrs: inlinehint nounwind uwtable
define linkonce_odr void @_ZN4baseC2Ev(%struct.base*) unnamed_addr #2 comdat align 2 {
  %2 = alloca %struct.base*, align 8
  store %struct.base* %0, %struct.base** %2, align 8
  %3 = load %struct.base*, %struct.base** %2, align 8
  %4 = bitcast %struct.base* %3 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ([4 x i8*], [4 x i8*]* @_ZTV4base, i32 0, i32 2) to i32 (...)**), i32 (...)*** %4, align 8
  ret void
}

attributes #0 = { norecurse uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nobuiltin "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { inlinehint nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nobuiltin nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { builtin }
attributes #5 = { nounwind }
attributes #6 = { builtin nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

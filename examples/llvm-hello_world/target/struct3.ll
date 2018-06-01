; ModuleID = 'struct3.cpp'
source_filename = "struct3.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.S = type { i32, i32 }

$_ZN1SC2Eii = comdat any

; Function Attrs: noinline norecurse optnone uwtable
define i32 @main() #0 personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) {
  %1 = alloca i32, align 4
  %2 = alloca %struct.S*, align 8
  %3 = alloca i8*
  %4 = alloca i32
  %5 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  %6 = call i8* @_Znwm(i64 8) #4
  %7 = bitcast i8* %6 to %struct.S*
  invoke void @_ZN1SC2Eii(%struct.S* %7, i32 10, i32 20)
          to label %8 unwind label %22

; <label>:8:                                      ; preds = %0
  store %struct.S* %7, %struct.S** %2, align 8
  %9 = load %struct.S*, %struct.S** %2, align 8
  %10 = getelementptr inbounds %struct.S, %struct.S* %9, i32 0, i32 0
  %11 = load i32, i32* %10, align 4
  %12 = load %struct.S*, %struct.S** %2, align 8
  %13 = getelementptr inbounds %struct.S, %struct.S* %12, i32 0, i32 1
  %14 = load i32, i32* %13, align 4
  %15 = add nsw i32 %11, %14
  store i32 %15, i32* %5, align 4
  %16 = load %struct.S*, %struct.S** %2, align 8
  %17 = icmp eq %struct.S* %16, null
  br i1 %17, label %20, label %18

; <label>:18:                                     ; preds = %8
  %19 = bitcast %struct.S* %16 to i8*
  call void @_ZdlPv(i8* %19) #5
  br label %20

; <label>:20:                                     ; preds = %18, %8
  %21 = load i32, i32* %5, align 4
  ret i32 %21

; <label>:22:                                     ; preds = %0
  %23 = landingpad { i8*, i32 }
          cleanup
  %24 = extractvalue { i8*, i32 } %23, 0
  store i8* %24, i8** %3, align 8
  %25 = extractvalue { i8*, i32 } %23, 1
  store i32 %25, i32* %4, align 4
  call void @_ZdlPv(i8* %6) #5
  br label %26

; <label>:26:                                     ; preds = %22
  %27 = load i8*, i8** %3, align 8
  %28 = load i32, i32* %4, align 4
  %29 = insertvalue { i8*, i32 } undef, i8* %27, 0
  %30 = insertvalue { i8*, i32 } %29, i32 %28, 1
  resume { i8*, i32 } %30
}

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #1

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr void @_ZN1SC2Eii(%struct.S*, i32, i32) unnamed_addr #2 comdat align 2 {
  %4 = alloca %struct.S*, align 8
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  store %struct.S* %0, %struct.S** %4, align 8
  store i32 %1, i32* %5, align 4
  store i32 %2, i32* %6, align 4
  %7 = load %struct.S*, %struct.S** %4, align 8
  %8 = getelementptr inbounds %struct.S, %struct.S* %7, i32 0, i32 0
  %9 = load i32, i32* %5, align 4
  store i32 %9, i32* %8, align 4
  %10 = getelementptr inbounds %struct.S, %struct.S* %7, i32 0, i32 1
  %11 = load i32, i32* %6, align 4
  store i32 %11, i32* %10, align 4
  ret void
}

declare i32 @__gxx_personality_v0(...)

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(i8*) #3

attributes #0 = { noinline norecurse optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nobuiltin "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nobuiltin nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { builtin }
attributes #5 = { builtin nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 5.0.1 (tags/RELEASE_501/final 332326)"}

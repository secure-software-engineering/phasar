; ModuleID = 'exception.cpp'
source_filename = "exception.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%"class.std::runtime_error" = type { %"class.std::exception", %"struct.std::__cow_string" }
%"class.std::exception" = type { i32 (...)** }
%"struct.std::__cow_string" = type { %union.anon }
%union.anon = type { i8* }

@.str = private unnamed_addr constant [6 x i8] c"error\00", align 1
@_ZTISt13runtime_error = external constant i8*

; Function Attrs: noinline optnone uwtable
define void @_Z9may_throwi(i32) #0 personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) {
  %2 = alloca i32, align 4
  %3 = alloca i8*
  %4 = alloca i32
  store i32 %0, i32* %2, align 4
  %5 = load i32, i32* %2, align 4
  %6 = icmp eq i32 %5, 1
  br i1 %6, label %7, label %15

; <label>:7:                                      ; preds = %1
  %8 = call i8* @__cxa_allocate_exception(i64 16) #5
  %9 = bitcast i8* %8 to %"class.std::runtime_error"*
  invoke void @_ZNSt13runtime_errorC1EPKc(%"class.std::runtime_error"* %9, i8* getelementptr inbounds ([6 x i8], [6 x i8]* @.str, i32 0, i32 0))
          to label %10 unwind label %11

; <label>:10:                                     ; preds = %7
  call void @__cxa_throw(i8* %8, i8* bitcast (i8** @_ZTISt13runtime_error to i8*), i8* bitcast (void (%"class.std::runtime_error"*)* @_ZNSt13runtime_errorD1Ev to i8*)) #6
  unreachable

; <label>:11:                                     ; preds = %7
  %12 = landingpad { i8*, i32 }
          cleanup
  %13 = extractvalue { i8*, i32 } %12, 0
  store i8* %13, i8** %3, align 8
  %14 = extractvalue { i8*, i32 } %12, 1
  store i32 %14, i32* %4, align 4
  call void @__cxa_free_exception(i8* %8) #5
  br label %16

; <label>:15:                                     ; preds = %1
  ret void

; <label>:16:                                     ; preds = %11
  %17 = load i8*, i8** %3, align 8
  %18 = load i32, i32* %4, align 4
  %19 = insertvalue { i8*, i32 } undef, i8* %17, 0
  %20 = insertvalue { i8*, i32 } %19, i32 %18, 1
  resume { i8*, i32 } %20
}

declare i8* @__cxa_allocate_exception(i64)

declare void @_ZNSt13runtime_errorC1EPKc(%"class.std::runtime_error"*, i8*) unnamed_addr #1

declare i32 @__gxx_personality_v0(...)

declare void @__cxa_free_exception(i8*)

; Function Attrs: nounwind
declare void @_ZNSt13runtime_errorD1Ev(%"class.std::runtime_error"*) unnamed_addr #2

declare void @__cxa_throw(i8*, i8*, i8*)

; Function Attrs: noinline norecurse optnone uwtable
define i32 @main(i32, i8**) #3 personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i8**, align 8
  %6 = alloca i8, align 1
  %7 = alloca i8*
  %8 = alloca i32
  %9 = alloca %"class.std::runtime_error"*, align 8
  store i32 0, i32* %3, align 4
  store i32 %0, i32* %4, align 4
  store i8** %1, i8*** %5, align 8
  store i8 0, i8* %6, align 1
  %10 = load i32, i32* %4, align 4
  invoke void @_Z9may_throwi(i32 %10)
          to label %11 unwind label %12

; <label>:11:                                     ; preds = %2
  br label %24

; <label>:12:                                     ; preds = %2
  %13 = landingpad { i8*, i32 }
          catch i8* bitcast (i8** @_ZTISt13runtime_error to i8*)
  %14 = extractvalue { i8*, i32 } %13, 0
  store i8* %14, i8** %7, align 8
  %15 = extractvalue { i8*, i32 } %13, 1
  store i32 %15, i32* %8, align 4
  br label %16

; <label>:16:                                     ; preds = %12
  %17 = load i32, i32* %8, align 4
  %18 = call i32 @llvm.eh.typeid.for(i8* bitcast (i8** @_ZTISt13runtime_error to i8*)) #5
  %19 = icmp eq i32 %17, %18
  br i1 %19, label %20, label %28

; <label>:20:                                     ; preds = %16
  %21 = load i8*, i8** %7, align 8
  %22 = call i8* @__cxa_begin_catch(i8* %21) #5
  %23 = bitcast i8* %22 to %"class.std::runtime_error"*
  store %"class.std::runtime_error"* %23, %"class.std::runtime_error"** %9, align 8
  store i8 1, i8* %6, align 1
  call void @__cxa_end_catch()
  br label %24

; <label>:24:                                     ; preds = %20, %11
  %25 = load i8, i8* %6, align 1
  %26 = trunc i8 %25 to i1
  %27 = zext i1 %26 to i32
  ret i32 %27

; <label>:28:                                     ; preds = %16
  %29 = load i8*, i8** %7, align 8
  %30 = load i32, i32* %8, align 4
  %31 = insertvalue { i8*, i32 } undef, i8* %29, 0
  %32 = insertvalue { i8*, i32 } %31, i32 %30, 1
  resume { i8*, i32 } %32
}

; Function Attrs: nounwind readnone
declare i32 @llvm.eh.typeid.for(i8*) #4

declare i8* @__cxa_begin_catch(i8*)

declare void @__cxa_end_catch()

attributes #0 = { noinline optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { noinline norecurse optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind readnone }
attributes #5 = { nounwind }
attributes #6 = { noreturn }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 5.0.1 (tags/RELEASE_501/final 332326)"}

; ModuleID = 'exception.cpp'
source_filename = "exception.cpp"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.13.0"

%"class.std::runtime_error" = type { %"class.std::exception", %"class.std::__1::__libcpp_refstring" }
%"class.std::exception" = type { i32 (...)** }
%"class.std::__1::__libcpp_refstring" = type { i8* }

@.str = private unnamed_addr constant [14 x i8] c"error occured\00", align 1
@_ZTISt13runtime_error = external constant i8*

; Function Attrs: ssp uwtable
define i32 @_Z10myfunctioni(i32) #0 personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i8*
  %5 = alloca i32
  store i32 %0, i32* %3, align 4
  %6 = call i8* @__cxa_allocate_exception(i64 16) #5
  %7 = bitcast i8* %6 to %"class.std::runtime_error"*
  invoke void @_ZNSt13runtime_errorC1EPKc(%"class.std::runtime_error"* %7, i8* getelementptr inbounds ([14 x i8], [14 x i8]* @.str, i32 0, i32 0))
          to label %8 unwind label %9

; <label>:8:                                      ; preds = %1
  call void @__cxa_throw(i8* %6, i8* bitcast (i8** @_ZTISt13runtime_error to i8*), i8* bitcast (void (%"class.std::runtime_error"*)* @_ZNSt13runtime_errorD1Ev to i8*)) #6
  unreachable

; <label>:9:                                      ; preds = %1
  %10 = landingpad { i8*, i32 }
          cleanup
  %11 = extractvalue { i8*, i32 } %10, 0
  store i8* %11, i8** %4, align 8
  %12 = extractvalue { i8*, i32 } %10, 1
  store i32 %12, i32* %5, align 4
  call void @__cxa_free_exception(i8* %6) #5
  br label %15
                                                  ; No predecessors!
  %14 = load i32, i32* %2, align 4
  ret i32 %14

; <label>:15:                                     ; preds = %9
  %16 = load i8*, i8** %4, align 8
  %17 = load i32, i32* %5, align 4
  %18 = insertvalue { i8*, i32 } undef, i8* %16, 0
  %19 = insertvalue { i8*, i32 } %18, i32 %17, 1
  resume { i8*, i32 } %19
}

declare i8* @__cxa_allocate_exception(i64)

declare void @_ZNSt13runtime_errorC1EPKc(%"class.std::runtime_error"*, i8*) unnamed_addr #1

declare i32 @__gxx_personality_v0(...)

declare void @__cxa_free_exception(i8*)

; Function Attrs: nounwind
declare void @_ZNSt13runtime_errorD1Ev(%"class.std::runtime_error"*) unnamed_addr #2

declare void @__cxa_throw(i8*, i8*, i8*)

; Function Attrs: norecurse ssp uwtable
define i32 @main() #3 personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i8*
  %5 = alloca i32
  %6 = alloca %"class.std::runtime_error"*, align 8
  store i32 0, i32* %1, align 4
  store i32 13, i32* %2, align 4
  %7 = load i32, i32* %2, align 4
  %8 = invoke i32 @_Z10myfunctioni(i32 %7)
          to label %9 unwind label %10

; <label>:9:                                      ; preds = %0
  store i32 %8, i32* %3, align 4
  br label %28

; <label>:10:                                     ; preds = %0
  %11 = landingpad { i8*, i32 }
          catch i8* bitcast (i8** @_ZTISt13runtime_error to i8*)
  %12 = extractvalue { i8*, i32 } %11, 0
  store i8* %12, i8** %4, align 8
  %13 = extractvalue { i8*, i32 } %11, 1
  store i32 %13, i32* %5, align 4
  br label %14

; <label>:14:                                     ; preds = %10
  %15 = load i32, i32* %5, align 4
  %16 = call i32 @llvm.eh.typeid.for(i8* bitcast (i8** @_ZTISt13runtime_error to i8*)) #5
  %17 = icmp eq i32 %15, %16
  br i1 %17, label %18, label %29

; <label>:18:                                     ; preds = %14
  %19 = load i8*, i8** %4, align 8
  %20 = call i8* @__cxa_begin_catch(i8* %19) #5
  %21 = bitcast i8* %20 to %"class.std::runtime_error"*
  store %"class.std::runtime_error"* %21, %"class.std::runtime_error"** %6, align 8
  %22 = load %"class.std::runtime_error"*, %"class.std::runtime_error"** %6, align 8
  %23 = bitcast %"class.std::runtime_error"* %22 to i8* (%"class.std::runtime_error"*)***
  %24 = load i8* (%"class.std::runtime_error"*)**, i8* (%"class.std::runtime_error"*)*** %23, align 8
  %25 = getelementptr inbounds i8* (%"class.std::runtime_error"*)*, i8* (%"class.std::runtime_error"*)** %24, i64 2
  %26 = load i8* (%"class.std::runtime_error"*)*, i8* (%"class.std::runtime_error"*)** %25, align 8
  %27 = call i8* %26(%"class.std::runtime_error"* %22) #5
  call void @__cxa_end_catch()
  br label %28

; <label>:28:                                     ; preds = %18, %9
  ret i32 0

; <label>:29:                                     ; preds = %14
  %30 = load i8*, i8** %4, align 8
  %31 = load i32, i32* %5, align 4
  %32 = insertvalue { i8*, i32 } undef, i8* %30, 0
  %33 = insertvalue { i8*, i32 } %32, i32 %31, 1
  resume { i8*, i32 } %33
}

; Function Attrs: nounwind readnone
declare i32 @llvm.eh.typeid.for(i8*) #4

declare i8* @__cxa_begin_catch(i8*)

declare void @__cxa_end_catch()

attributes #0 = { ssp uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { norecurse ssp uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind readnone }
attributes #5 = { nounwind }
attributes #6 = { noreturn }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"PIC Level", i32 2}
!1 = !{!"clang version 3.9.1 (tags/RELEASE_391/final)"}

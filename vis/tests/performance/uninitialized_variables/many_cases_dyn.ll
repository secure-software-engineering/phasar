; ModuleID = 'many_cases_dyn.cpp'
source_filename = "many_cases_dyn.cpp"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.13.0"

%"class.std::runtime_error" = type { %"class.std::exception", %"class.std::__1::__libcpp_refstring" }
%"class.std::exception" = type { i32 (...)** }
%"class.std::__1::__libcpp_refstring" = type { i8* }
%struct.Y = type { %struct.X, i32 }
%struct.X = type { i32, i32 }

@global = global i32 1, align 4
@.str = private unnamed_addr constant [14 x i8] c"error occured\00", align 1
@_ZTISt13runtime_error = external constant i8*

; Function Attrs: ssp uwtable
define i32 @_Z10myfunctioni(i32) #0 personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i8*
  %5 = alloca i32
  store i32 %0, i32* %3, align 4
  %6 = call i8* @__cxa_allocate_exception(i64 16) #8
  %7 = bitcast i8* %6 to %"class.std::runtime_error"*
  invoke void @_ZNSt13runtime_errorC1EPKc(%"class.std::runtime_error"* %7, i8* getelementptr inbounds ([14 x i8], [14 x i8]* @.str, i32 0, i32 0))
          to label %8 unwind label %9

; <label>:8:                                      ; preds = %1
  call void @__cxa_throw(i8* %6, i8* bitcast (i8** @_ZTISt13runtime_error to i8*), i8* bitcast (void (%"class.std::runtime_error"*)* @_ZNSt13runtime_errorD1Ev to i8*)) #9
  unreachable

; <label>:9:                                      ; preds = %1
  %10 = landingpad { i8*, i32 }
          cleanup
  %11 = extractvalue { i8*, i32 } %10, 0
  store i8* %11, i8** %4, align 8
  %12 = extractvalue { i8*, i32 } %10, 1
  store i32 %12, i32* %5, align 4
  call void @__cxa_free_exception(i8* %6) #8
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

; Function Attrs: ssp uwtable
define i32 @_Z3dynv() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32*, align 8
  %5 = alloca i32*, align 8
  %6 = alloca i32*, align 8
  %7 = alloca %struct.Y*, align 8
  %8 = load i32, i32* %1, align 4
  %9 = load i32, i32* %2, align 4
  %10 = add nsw i32 %8, %9
  store i32 %10, i32* %3, align 4
  store i32* %1, i32** %4, align 8
  store i32* %2, i32** %5, align 8
  %11 = load i32*, i32** %5, align 8
  store i32 42, i32* %11, align 4
  %12 = call i8* @_Znwm(i64 4) #10
  %13 = bitcast i8* %12 to i32*
  store i32 13, i32* %13, align 4
  store i32* %13, i32** %6, align 8
  %14 = load i32, i32* %1, align 4
  %15 = load i32, i32* %2, align 4
  %16 = add nsw i32 %14, %15
  %17 = load i32*, i32** %6, align 8
  %18 = load i32, i32* %17, align 4
  %19 = add nsw i32 %18, %16
  store i32 %19, i32* %17, align 4
  %20 = load i32, i32* %3, align 4
  %21 = load i32*, i32** %6, align 8
  %22 = load i32, i32* %21, align 4
  %23 = add nsw i32 %22, %20
  store i32 %23, i32* %21, align 4
  %24 = load i32*, i32** %6, align 8
  %25 = icmp eq i32* %24, null
  br i1 %25, label %28, label %26

; <label>:26:                                     ; preds = %0
  %27 = bitcast i32* %24 to i8*
  call void @_ZdlPv(i8* %27) #11
  br label %28

; <label>:28:                                     ; preds = %26, %0
  %29 = call i8* @_Znwm(i64 12) #10
  %30 = bitcast i8* %29 to %struct.Y*
  store %struct.Y* %30, %struct.Y** %7, align 8
  %31 = load %struct.Y*, %struct.Y** %7, align 8
  %32 = icmp eq %struct.Y* %31, null
  br i1 %32, label %35, label %33

; <label>:33:                                     ; preds = %28
  %34 = bitcast %struct.Y* %31 to i8*
  call void @_ZdlPv(i8* %34) #11
  br label %35

; <label>:35:                                     ; preds = %33, %28
  %36 = load i32, i32* %2, align 4
  ret i32 %36
}

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #3

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(i8*) #4

; Function Attrs: ssp uwtable
define i32 @_Z9recursionj(i32) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  %4 = load i32, i32* %3, align 4
  %5 = icmp ugt i32 %4, 0
  br i1 %5, label %6, label %10

; <label>:6:                                      ; preds = %1
  %7 = load i32, i32* %3, align 4
  %8 = sub i32 %7, 1
  %9 = call i32 @_Z9recursionj(i32 %8)
  store i32 %9, i32* %2, align 4
  br label %12

; <label>:10:                                     ; preds = %1
  %11 = load i32, i32* %3, align 4
  store i32 %11, i32* %2, align 4
  br label %12

; <label>:12:                                     ; preds = %10, %6
  %13 = load i32, i32* %2, align 4
  ret i32 %13
}

; Function Attrs: ssp uwtable
define i32 @_Z8functionii(i32, i32) #0 personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i8*
  %10 = alloca i32
  %11 = alloca %"class.std::runtime_error"*, align 8
  %12 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  store i32 %1, i32* %4, align 4
  %13 = load i32, i32* %3, align 4
  store i32 %13, i32* %6, align 4
  store i32 13, i32* %7, align 4
  %14 = load i32, i32* %7, align 4
  %15 = invoke i32 @_Z10myfunctioni(i32 %14)
          to label %16 unwind label %17

; <label>:16:                                     ; preds = %2
  store i32 %15, i32* %8, align 4
  br label %35

; <label>:17:                                     ; preds = %2
  %18 = landingpad { i8*, i32 }
          catch i8* bitcast (i8** @_ZTISt13runtime_error to i8*)
  %19 = extractvalue { i8*, i32 } %18, 0
  store i8* %19, i8** %9, align 8
  %20 = extractvalue { i8*, i32 } %18, 1
  store i32 %20, i32* %10, align 4
  br label %21

; <label>:21:                                     ; preds = %17
  %22 = load i32, i32* %10, align 4
  %23 = call i32 @llvm.eh.typeid.for(i8* bitcast (i8** @_ZTISt13runtime_error to i8*)) #8
  %24 = icmp eq i32 %22, %23
  br i1 %24, label %25, label %41

; <label>:25:                                     ; preds = %21
  %26 = load i8*, i8** %9, align 8
  %27 = call i8* @__cxa_begin_catch(i8* %26) #8
  %28 = bitcast i8* %27 to %"class.std::runtime_error"*
  store %"class.std::runtime_error"* %28, %"class.std::runtime_error"** %11, align 8
  %29 = load %"class.std::runtime_error"*, %"class.std::runtime_error"** %11, align 8
  %30 = bitcast %"class.std::runtime_error"* %29 to i8* (%"class.std::runtime_error"*)***
  %31 = load i8* (%"class.std::runtime_error"*)**, i8* (%"class.std::runtime_error"*)*** %30, align 8
  %32 = getelementptr inbounds i8* (%"class.std::runtime_error"*)*, i8* (%"class.std::runtime_error"*)** %31, i64 2
  %33 = load i8* (%"class.std::runtime_error"*)*, i8* (%"class.std::runtime_error"*)** %32, align 8
  %34 = call i8* %33(%"class.std::runtime_error"* %29) #8
  call void @__cxa_end_catch()
  br label %35

; <label>:35:                                     ; preds = %25, %16
  %36 = load i32, i32* %4, align 4
  store i32 %36, i32* %12, align 4
  %37 = call i32 @_Z9recursionj(i32 5)
  %38 = load i32, i32* %5, align 4
  %39 = load i32, i32* %12, align 4
  %40 = add nsw i32 %38, %39
  ret i32 %40

; <label>:41:                                     ; preds = %21
  %42 = load i8*, i8** %9, align 8
  %43 = load i32, i32* %10, align 4
  %44 = insertvalue { i8*, i32 } undef, i8* %42, 0
  %45 = insertvalue { i8*, i32 } %44, i32 %43, 1
  resume { i8*, i32 } %45
}

; Function Attrs: nounwind readnone
declare i32 @llvm.eh.typeid.for(i8*) #5

declare i8* @__cxa_begin_catch(i8*)

declare void @__cxa_end_catch()

; Function Attrs: nounwind ssp uwtable
define i32 @_Z6ifTesti(i32) #6 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  %6 = load i32, i32* %2, align 4
  %7 = icmp slt i32 %6, 5
  br i1 %7, label %8, label %9

; <label>:8:                                      ; preds = %1
  store i32 2, i32* %3, align 4
  br label %10

; <label>:9:                                      ; preds = %1
  store i32 -5, i32* %3, align 4
  br label %10

; <label>:10:                                     ; preds = %9, %8
  store i32 0, i32* %4, align 4
  store i32 0, i32* %5, align 4
  br label %11

; <label>:11:                                     ; preds = %14, %10
  %12 = load i32, i32* %3, align 4
  %13 = icmp sgt i32 %12, 0
  br i1 %13, label %14, label %22

; <label>:14:                                     ; preds = %11
  %15 = load i32, i32* %4, align 4
  %16 = load i32, i32* %3, align 4
  %17 = mul nsw i32 %15, %16
  %18 = load i32, i32* %5, align 4
  %19 = add nsw i32 %18, %17
  store i32 %19, i32* %5, align 4
  %20 = load i32, i32* %4, align 4
  %21 = add nsw i32 %20, 1
  store i32 %21, i32* %4, align 4
  br label %11

; <label>:22:                                     ; preds = %11
  %23 = load i32, i32* %5, align 4
  ret i32 %23
}

; Function Attrs: norecurse ssp uwtable
define i32 @main(i32, i8**) #7 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i8**, align 8
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  store i32 0, i32* %3, align 4
  store i32 %0, i32* %4, align 4
  store i8** %1, i8*** %5, align 8
  %10 = load i32, i32* %7, align 4
  %11 = call i32 @_Z8functionii(i32 %10, i32 12)
  store i32 %11, i32* %8, align 4
  store i32 2, i32* %9, align 4
  %12 = call i32 @_Z6ifTesti(i32 2)
  store i32 %12, i32* %9, align 4
  store i32 2, i32* %7, align 4
  ret i32 0
}

attributes #0 = { ssp uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nobuiltin "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nobuiltin nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { nounwind readnone }
attributes #6 = { nounwind ssp uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #7 = { norecurse ssp uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="core2" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #8 = { nounwind }
attributes #9 = { noreturn }
attributes #10 = { builtin }
attributes #11 = { builtin nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"PIC Level", i32 2}
!1 = !{!"clang version 3.9.1 (tags/RELEASE_391/final)"}

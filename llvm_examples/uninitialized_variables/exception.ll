; ModuleID = 'exception.cpp'
source_filename = "exception.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%"class.std::basic_string" = type { %"struct.std::basic_string<char, std::char_traits<char>, std::allocator<char> >::_Alloc_hider" }
%"struct.std::basic_string<char, std::char_traits<char>, std::allocator<char> >::_Alloc_hider" = type { i8* }
%"class.std::allocator" = type { i8 }
%"class.std::runtime_error" = type { %"class.std::exception", %"class.std::basic_string" }
%"class.std::exception" = type { i32 (...)** }

@.str = private unnamed_addr constant [14 x i8] c"error occured\00", align 1
@_ZTISt13runtime_error = external constant i8*

; Function Attrs: uwtable
define i32 @_Z10myfunctioni(i32) #0 personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca %"class.std::basic_string", align 8
  %5 = alloca %"class.std::allocator", align 1
  %6 = alloca i8*
  %7 = alloca i32
  %8 = alloca i1, align 1
  store i32 %0, i32* %3, align 4
  %9 = call i8* @__cxa_allocate_exception(i64 16) #5
  store i1 true, i1* %8, align 1
  %10 = bitcast i8* %9 to %"class.std::runtime_error"*
  call void @_ZNSaIcEC1Ev(%"class.std::allocator"* %5) #5
  invoke void @_ZNSsC1EPKcRKSaIcE(%"class.std::basic_string"* %4, i8* getelementptr inbounds ([14 x i8], [14 x i8]* @.str, i32 0, i32 0), %"class.std::allocator"* dereferenceable(1) %5)
          to label %11 unwind label %13

; <label>:11:                                     ; preds = %1
  invoke void @_ZNSt13runtime_errorC1ERKSs(%"class.std::runtime_error"* %10, %"class.std::basic_string"* dereferenceable(8) %4)
          to label %12 unwind label %17

; <label>:12:                                     ; preds = %11
  store i1 false, i1* %8, align 1
  invoke void @__cxa_throw(i8* %9, i8* bitcast (i8** @_ZTISt13runtime_error to i8*), i8* bitcast (void (%"class.std::runtime_error"*)* @_ZNSt13runtime_errorD1Ev to i8*)) #6
          to label %32 unwind label %17

; <label>:13:                                     ; preds = %1
  %14 = landingpad { i8*, i32 }
          cleanup
  %15 = extractvalue { i8*, i32 } %14, 0
  store i8* %15, i8** %6, align 8
  %16 = extractvalue { i8*, i32 } %14, 1
  store i32 %16, i32* %7, align 4
  br label %21

; <label>:17:                                     ; preds = %12, %11
  %18 = landingpad { i8*, i32 }
          cleanup
  %19 = extractvalue { i8*, i32 } %18, 0
  store i8* %19, i8** %6, align 8
  %20 = extractvalue { i8*, i32 } %18, 1
  store i32 %20, i32* %7, align 4
  call void @_ZNSsD1Ev(%"class.std::basic_string"* %4) #5
  br label %21

; <label>:21:                                     ; preds = %17, %13
  call void @_ZNSaIcED1Ev(%"class.std::allocator"* %5) #5
  %22 = load i1, i1* %8, align 1
  br i1 %22, label %23, label %24

; <label>:23:                                     ; preds = %21
  call void @__cxa_free_exception(i8* %9) #5
  br label %24

; <label>:24:                                     ; preds = %23, %21
  br label %27
                                                  ; No predecessors!
  %26 = load i32, i32* %2, align 4
  ret i32 %26

; <label>:27:                                     ; preds = %24
  %28 = load i8*, i8** %6, align 8
  %29 = load i32, i32* %7, align 4
  %30 = insertvalue { i8*, i32 } undef, i8* %28, 0
  %31 = insertvalue { i8*, i32 } %30, i32 %29, 1
  resume { i8*, i32 } %31

; <label>:32:                                     ; preds = %12
  unreachable
}

declare i8* @__cxa_allocate_exception(i64)

; Function Attrs: nounwind
declare void @_ZNSaIcEC1Ev(%"class.std::allocator"*) unnamed_addr #1

declare void @_ZNSsC1EPKcRKSaIcE(%"class.std::basic_string"*, i8*, %"class.std::allocator"* dereferenceable(1)) unnamed_addr #2

declare i32 @__gxx_personality_v0(...)

declare void @_ZNSt13runtime_errorC1ERKSs(%"class.std::runtime_error"*, %"class.std::basic_string"* dereferenceable(8)) unnamed_addr #2

; Function Attrs: nounwind
declare void @_ZNSt13runtime_errorD1Ev(%"class.std::runtime_error"*) unnamed_addr #1

declare void @__cxa_throw(i8*, i8*, i8*)

; Function Attrs: nounwind
declare void @_ZNSsD1Ev(%"class.std::basic_string"*) unnamed_addr #1

; Function Attrs: nounwind
declare void @_ZNSaIcED1Ev(%"class.std::allocator"*) unnamed_addr #1

declare void @__cxa_free_exception(i8*)

; Function Attrs: norecurse uwtable
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

attributes #0 = { uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { norecurse uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind readnone }
attributes #5 = { nounwind }
attributes #6 = { noreturn }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0-svn274465-1~exp1 (trunk)"}

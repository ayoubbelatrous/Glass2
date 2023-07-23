; ModuleID = 'generated.c'
source_filename = "generated.c"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-w64-windows-gnu"

%struct.Entity = type { ptr }

@.str = private unnamed_addr constant [6 x i8] c"Hello\00", align 1
@__data1 = dso_local global ptr @.str, align 8
@.str.1 = private unnamed_addr constant [10 x i8] c"Hello: %s\00", align 1
@__data2 = dso_local global ptr @.str.1, align 8

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca %struct.Entity, align 8
  %2 = alloca i64, align 8
  %3 = ptrtoint ptr %1 to i64
  store i64 %3, ptr %2, align 8
  %4 = load i64, ptr %2, align 8
  %5 = inttoptr i64 %4 to ptr
  %6 = getelementptr inbounds %struct.Entity, ptr %5, i32 0, i32 0
  %7 = load ptr, ptr %6, align 8
  ret i32 0
}

attributes #0 = { noinline nounwind optnone uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"uwtable", i32 2}
!3 = !{!"clang version 16.0.2"}

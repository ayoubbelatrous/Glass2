; ModuleID = 'generated.c'
source_filename = "generated.c"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-w64-windows-gnu"

%struct.Player = type { %struct.Position }
%struct.Position = type { i32, i32 }

@.str = private unnamed_addr constant [23 x i8] c"Player: {x: %i, y: %i}\00", align 1
@__data1 = dso_local global ptr @.str, align 8

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca %struct.Player, align 4
  %2 = alloca i64, align 8
  %3 = alloca i64, align 8
  %4 = alloca i64, align 8
  %5 = alloca i32, align 4
  %6 = alloca i64, align 8
  %7 = alloca i64, align 8
  %8 = alloca i32, align 4
  %9 = alloca ptr, align 8
  %10 = alloca ptr, align 8
  %11 = alloca i64, align 8
  %12 = alloca i64, align 8
  %13 = alloca i64, align 8
  %14 = alloca i64, align 8
  %15 = alloca i64, align 8
  %16 = alloca i64, align 8
  %17 = alloca i32, align 4
  %18 = ptrtoint ptr %1 to i64
  store i64 %18, ptr %2, align 8
  %19 = load i64, ptr %2, align 8
  %20 = inttoptr i64 %19 to ptr
  %21 = getelementptr inbounds %struct.Player, ptr %20, i32 0, i32 0
  %22 = ptrtoint ptr %21 to i64
  store i64 %22, ptr %3, align 8
  %23 = load i64, ptr %3, align 8
  %24 = inttoptr i64 %23 to ptr
  %25 = getelementptr inbounds %struct.Position, ptr %24, i32 0, i32 0
  %26 = ptrtoint ptr %25 to i64
  store i64 %26, ptr %4, align 8
  store i32 30, ptr %5, align 4
  %27 = load i64, ptr %4, align 8
  %28 = inttoptr i64 %27 to ptr
  store i32 30, ptr %28, align 4
  %29 = load i64, ptr %2, align 8
  %30 = inttoptr i64 %29 to ptr
  %31 = getelementptr inbounds %struct.Player, ptr %30, i32 0, i32 0
  %32 = ptrtoint ptr %31 to i64
  store i64 %32, ptr %6, align 8
  %33 = load i64, ptr %6, align 8
  %34 = inttoptr i64 %33 to ptr
  %35 = getelementptr inbounds %struct.Position, ptr %34, i32 0, i32 1
  %36 = ptrtoint ptr %35 to i64
  store i64 %36, ptr %7, align 8
  store i32 10, ptr %8, align 4
  %37 = load i64, ptr %7, align 8
  %38 = inttoptr i64 %37 to ptr
  store i32 10, ptr %38, align 4
  %39 = load ptr, ptr @__data1, align 8
  store ptr %39, ptr %9, align 8
  %40 = load ptr, ptr %9, align 8
  store ptr %40, ptr %10, align 8
  %41 = load i64, ptr %2, align 8
  %42 = inttoptr i64 %41 to ptr
  %43 = getelementptr inbounds %struct.Player, ptr %42, i32 0, i32 0
  %44 = ptrtoint ptr %43 to i64
  store i64 %44, ptr %11, align 8
  %45 = load i64, ptr %11, align 8
  %46 = inttoptr i64 %45 to ptr
  %47 = getelementptr inbounds %struct.Position, ptr %46, i32 0, i32 0
  %48 = ptrtoint ptr %47 to i64
  store i64 %48, ptr %12, align 8
  %49 = load i64, ptr %12, align 8
  %50 = inttoptr i64 %49 to ptr
  %51 = load i64, ptr %50, align 8
  store i64 %51, ptr %13, align 8
  %52 = load i64, ptr %2, align 8
  %53 = inttoptr i64 %52 to ptr
  %54 = getelementptr inbounds %struct.Player, ptr %53, i32 0, i32 0
  %55 = ptrtoint ptr %54 to i64
  store i64 %55, ptr %14, align 8
  %56 = load i64, ptr %14, align 8
  %57 = inttoptr i64 %56 to ptr
  %58 = getelementptr inbounds %struct.Position, ptr %57, i32 0, i32 1
  %59 = ptrtoint ptr %58 to i64
  store i64 %59, ptr %15, align 8
  %60 = load i64, ptr %15, align 8
  %61 = inttoptr i64 %60 to ptr
  %62 = load i64, ptr %61, align 8
  store i64 %62, ptr %16, align 8
  %63 = load ptr, ptr %10, align 8
  %64 = load i64, ptr %13, align 8
  %65 = load i64, ptr %16, align 8
  %66 = call i32 (ptr, ...) @printf(ptr noundef %63, i64 noundef %64, i64 noundef %65)
  store i32 %66, ptr %17, align 4
  ret i32 0
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @printf(ptr noundef nonnull %0, ...) #0 {
  %2 = alloca ptr, align 8
  %3 = alloca i32, align 4
  %4 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  call void @llvm.va_start(ptr %4)
  %5 = call ptr @__acrt_iob_func(i32 noundef 1)
  %6 = load ptr, ptr %2, align 8
  %7 = load ptr, ptr %4, align 8
  %8 = call i32 @__mingw_vfprintf(ptr noundef %5, ptr noundef %6, ptr noundef %7) #4
  store i32 %8, ptr %3, align 4
  call void @llvm.va_end(ptr %4)
  %9 = load i32, ptr %3, align 4
  ret i32 %9
}

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_start(ptr) #1

; Function Attrs: nounwind
declare dso_local i32 @__mingw_vfprintf(ptr noundef, ptr noundef, ptr noundef) #2

declare dllimport ptr @__acrt_iob_func(i32 noundef) #3

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_end(ptr) #1

attributes #0 = { noinline nounwind optnone uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind willreturn }
attributes #2 = { nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nounwind }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"uwtable", i32 2}
!3 = !{!"clang version 16.0.2"}

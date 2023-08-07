; ModuleID = 'generated.c'
source_filename = "generated.c"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-w64-windows-gnu"

%struct.vec3 = type { float, float, float }

@.str = private unnamed_addr constant [28 x i8] c"%s = {x: %f, y: %f, z: %f}\0A\00", align 1
@__data1 = dso_local local_unnamed_addr global ptr @.str, align 8
@__data2 = dso_local local_unnamed_addr global ptr @.str, align 8
@.str.1 = private unnamed_addr constant [2 x i8] c"a\00", align 1
@__data3 = dso_local local_unnamed_addr global ptr @.str.1, align 8
@__data4 = dso_local local_unnamed_addr global ptr @.str.1, align 8
@.str.2 = private unnamed_addr constant [2 x i8] c"b\00", align 1
@__data5 = dso_local local_unnamed_addr global ptr @.str.2, align 8
@__data6 = dso_local local_unnamed_addr global ptr @.str.2, align 8
@.str.3 = private unnamed_addr constant [2 x i8] c"x\00", align 1
@__data7 = dso_local local_unnamed_addr global ptr @.str.3, align 8
@__data8 = dso_local local_unnamed_addr global ptr @.str.3, align 8

; Function Attrs: mustprogress nofree nosync nounwind willreturn memory(read, argmem: readwrite, inaccessiblemem: none) uwtable
define dso_local void @add(ptr noalias nocapture writeonly sret(%struct.vec3) align 4 %0, ptr noundef %1, ptr noundef %2) local_unnamed_addr #0 {
  %4 = getelementptr inbounds %struct.vec3, ptr %1, i64 0, i32 2
  %5 = load float, ptr %4, align 4, !tbaa !4
  %6 = getelementptr inbounds %struct.vec3, ptr %2, i64 0, i32 2
  %7 = load float, ptr %6, align 4, !tbaa !4
  %8 = fadd float %5, %7
  %9 = fptosi float %8 to i32
  %10 = sitofp i32 %9 to float
  %11 = load <2 x float>, ptr %1, align 4, !tbaa !4
  %12 = load <2 x float>, ptr %2, align 4, !tbaa !4
  %13 = fadd <2 x float> %11, %12
  %14 = fptosi <2 x float> %13 to <2 x i32>
  %15 = sitofp <2 x i32> %14 to <2 x float>
  store <2 x float> %15, ptr %0, align 4
  %16 = getelementptr inbounds i8, ptr %0, i64 8
  store float %10, ptr %16, align 4, !tbaa.struct !8
  ret void
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: nounwind uwtable
define dso_local void @print_vec3(ptr noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #2 {
  %3 = load ptr, ptr @__data2, align 8, !tbaa !9
  %4 = load float, ptr %1, align 4, !tbaa !4
  %5 = getelementptr inbounds %struct.vec3, ptr %1, i64 0, i32 1
  %6 = load float, ptr %5, align 4, !tbaa !4
  %7 = getelementptr inbounds %struct.vec3, ptr %1, i64 0, i32 2
  %8 = load float, ptr %7, align 4, !tbaa !4
  %9 = fpext float %4 to double
  %10 = fpext float %6 to double
  %11 = fpext float %8 to double
  tail call void (ptr, ...) @printf(ptr noundef dereferenceable(1) %3, ptr noundef %0, double noundef %9, double noundef %10, double noundef %11)
  ret void
}

; Function Attrs: inlinehint nounwind uwtable
define internal void @printf(ptr noundef nonnull %0, ...) unnamed_addr #3 {
  %2 = alloca ptr, align 8
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %2) #7
  call void @llvm.va_start(ptr nonnull %2)
  %3 = call ptr @__acrt_iob_func(i32 noundef 1) #7
  %4 = load ptr, ptr %2, align 8, !tbaa !9
  %5 = call i32 @__mingw_vfprintf(ptr noundef %3, ptr noundef nonnull %0, ptr noundef %4) #7
  call void @llvm.va_end(ptr nonnull %2)
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %2) #7
  ret void
}

; Function Attrs: nounwind uwtable
define dso_local i32 @main() local_unnamed_addr #2 {
  %1 = load ptr, ptr @__data4, align 8, !tbaa !9
  %2 = load ptr, ptr @__data2, align 8, !tbaa !9
  tail call void (ptr, ...) @printf(ptr noundef dereferenceable(1) %2, ptr noundef %1, double noundef 1.000000e+01, double noundef 5.000000e+00, double noundef 1.000000e+01)
  %3 = load ptr, ptr @__data6, align 8, !tbaa !9
  %4 = load ptr, ptr @__data2, align 8, !tbaa !9
  tail call void (ptr, ...) @printf(ptr noundef dereferenceable(1) %4, ptr noundef %3, double noundef 1.000000e+01, double noundef 1.000000e+01, double noundef 1.000000e+01)
  %5 = load ptr, ptr @__data8, align 8, !tbaa !9
  %6 = load ptr, ptr @__data2, align 8, !tbaa !9
  tail call void (ptr, ...) @printf(ptr noundef dereferenceable(1) %6, ptr noundef %5, double noundef 2.000000e+01, double noundef 1.500000e+01, double noundef 2.000000e+01)
  ret i32 0
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn
declare void @llvm.va_start(ptr) #4

; Function Attrs: nounwind
declare dso_local i32 @__mingw_vfprintf(ptr noundef, ptr noundef, ptr noundef) local_unnamed_addr #5

declare dllimport ptr @__acrt_iob_func(i32 noundef) local_unnamed_addr #6

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn
declare void @llvm.va_end(ptr) #4

attributes #0 = { mustprogress nofree nosync nounwind willreturn memory(read, argmem: readwrite, inaccessiblemem: none) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { inlinehint nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { mustprogress nocallback nofree nosync nounwind willreturn }
attributes #5 = { nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #6 = { "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #7 = { nounwind }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"uwtable", i32 2}
!3 = !{!"clang version 16.0.2"}
!4 = !{!5, !5, i64 0}
!5 = !{!"float", !6, i64 0}
!6 = !{!"omnipotent char", !7, i64 0}
!7 = !{!"Simple C/C++ TBAA"}
!8 = !{i64 0, i64 4, !4}
!9 = !{!10, !10, i64 0}
!10 = !{!"any pointer", !6, i64 0}

; ModuleID = 'Glass'
source_filename = "Glass"

%TypeInfoTableElem = type { i64, i64, i64, i64 }
%ptr = type opaque
%TypeInfo = type { i8*, i64 }

@0 = private unnamed_addr constant [4 x i8] c"i32\00", align 1
@1 = private unnamed_addr constant [4 x i8] c"f32\00", align 1
@2 = private unnamed_addr constant [4 x i8] c"Any\00", align 1
@TypeInfoArray = constant [3 x %TypeInfoTableElem] [%TypeInfoTableElem { i8* getelementptr inbounds ([4 x i8], [4 x i8]* @0, i32 0, i32 0), i64 3, i64 0, i64 0 }, %TypeInfoTableElem { i8* getelementptr inbounds ([4 x i8], [4 x i8]* @1, i32 0, i32 0), i64 11, i64 0, i64 0 }, %TypeInfoTableElem { i8* getelementptr inbounds ([4 x i8], [4 x i8]* @2, i32 0, i32 0), i64 0, i64 0, i64 0 }]
@3 = private unnamed_addr constant [19 x i8] c"Got i32 flags: %i\0A\00", align 1
@4 = private unnamed_addr constant [19 x i8] c"Got f32 flags: %i\0A\00", align 1
@5 = private unnamed_addr constant [19 x i8] c"Got Any flags: %i\0A\00", align 1
@6 = private unnamed_addr constant [18 x i8] c"Got i32 name: %s\0A\00", align 1
@7 = private unnamed_addr constant [18 x i8] c"Got f32 name: %s\0A\00", align 1
@8 = private unnamed_addr constant [18 x i8] c"Got Any name: %s\0A\00", align 1

declare i32 @printf(i8*, ...)

declare %ptr* @malloc(i64)

define i32 @main() !dbg !3 {
entry:
  %0 = alloca %TypeInfo, align 8
  %1 = alloca %TypeInfo, align 8
  %2 = alloca %TypeInfo, align 8
  %3 = alloca %TypeInfo*, align 8
  %4 = alloca %TypeInfo*, align 8
  call void @llvm.dbg.declare(metadata %TypeInfo** %4, metadata !9, metadata !DIExpression()), !dbg !22
  store %TypeInfo* bitcast ([3 x %TypeInfoTableElem]* @TypeInfoArray to %TypeInfo*), %TypeInfo** %4, align 8, !dbg !23
  call void @llvm.dbg.declare(metadata %TypeInfo** %3, metadata !18, metadata !DIExpression()), !dbg !24
  store %TypeInfo* bitcast (%TypeInfoTableElem* getelementptr inbounds (%TypeInfoTableElem, [3 x %TypeInfoTableElem]* @TypeInfoArray, i64 1) to %TypeInfo*), %TypeInfo** %3, align 8, !dbg !25
  %5 = load %TypeInfo, %TypeInfo* bitcast ([3 x %TypeInfoTableElem]* @TypeInfoArray to %TypeInfo*), align 8, !dbg !26
  call void @llvm.dbg.declare(metadata %TypeInfo* %2, metadata !19, metadata !DIExpression()), !dbg !27
  store %TypeInfo %5, %TypeInfo* %2, align 8, !dbg !26
  %6 = load %TypeInfo, %TypeInfo* bitcast (%TypeInfoTableElem* getelementptr inbounds (%TypeInfoTableElem, [3 x %TypeInfoTableElem]* @TypeInfoArray, i64 1) to %TypeInfo*), align 8, !dbg !28
  call void @llvm.dbg.declare(metadata %TypeInfo* %1, metadata !20, metadata !DIExpression()), !dbg !29
  store %TypeInfo %6, %TypeInfo* %1, align 8, !dbg !28
  %7 = load %TypeInfo, %TypeInfo* bitcast (%TypeInfoTableElem* getelementptr (%TypeInfoTableElem, [3 x %TypeInfoTableElem]* @TypeInfoArray, i64 2) to %TypeInfo*), align 8, !dbg !30
  call void @llvm.dbg.declare(metadata %TypeInfo* %0, metadata !21, metadata !DIExpression()), !dbg !31
  store %TypeInfo %7, %TypeInfo* %0, align 8, !dbg !30
  %8 = getelementptr inbounds %TypeInfo, %TypeInfo* %2, i32 0, i32 1, !dbg !32
  %9 = load i64, i64* %8, align 4, !dbg !32
  %10 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([19 x i8], [19 x i8]* @3, i32 0, i32 0), i64 %9), !dbg !32
  %11 = getelementptr inbounds %TypeInfo, %TypeInfo* %1, i32 0, i32 1, !dbg !33
  %12 = load i64, i64* %11, align 4, !dbg !33
  %13 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([19 x i8], [19 x i8]* @4, i32 0, i32 0), i64 %12), !dbg !33
  %14 = getelementptr inbounds %TypeInfo, %TypeInfo* %0, i32 0, i32 1, !dbg !34
  %15 = load i64, i64* %14, align 4, !dbg !34
  %16 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([19 x i8], [19 x i8]* @5, i32 0, i32 0), i64 %15), !dbg !34
  %17 = getelementptr inbounds %TypeInfo, %TypeInfo* %2, i32 0, i32 0, !dbg !35
  %18 = load i8*, i8** %17, align 8, !dbg !35
  %19 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([18 x i8], [18 x i8]* @6, i32 0, i32 0), i8* %18), !dbg !35
  %20 = getelementptr inbounds %TypeInfo, %TypeInfo* %1, i32 0, i32 0, !dbg !36
  %21 = load i8*, i8** %20, align 8, !dbg !36
  %22 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([18 x i8], [18 x i8]* @7, i32 0, i32 0), i8* %21), !dbg !36
  %23 = getelementptr inbounds %TypeInfo, %TypeInfo* %0, i32 0, i32 0, !dbg !37
  %24 = load i8*, i8** %23, align 8, !dbg !37
  %25 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([18 x i8], [18 x i8]* @8, i32 0, i32 0), i8* %24), !dbg !37
  ret i32 0, !dbg !38
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "Glass Compiler", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "Main.glass", directory: ".")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = distinct !DISubprogram(name: "main", scope: !4, file: !4, line: 40, type: !5, scopeLine: 40, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !8)
!4 = !DIFile(filename: "HelloWorld.glass", directory: ".\\Examples")
!5 = !DISubroutineType(types: !6)
!6 = !{!7}
!7 = !DIBasicType(name: "i32", size: 32, encoding: DW_ATE_signed)
!8 = !{!9, !18, !19, !20, !21}
!9 = !DILocalVariable(name: "ti_ptr", scope: !3, file: !4, line: 42, type: !10)
!10 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !11, size: 64)
!11 = !DICompositeType(tag: DW_TAG_structure_type, name: "TypeInfo", line: 21, size: 128, align: 32, elements: !12)
!12 = !{!13, !16}
!13 = !DIDerivedType(tag: DW_TAG_member, name: "name", line: 22, baseType: !14, size: 64, align: 64)
!14 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !15, size: 64)
!15 = !DIBasicType(name: "u8", size: 8, encoding: DW_ATE_unsigned)
!16 = !DIDerivedType(tag: DW_TAG_member, name: "flags", line: 23, baseType: !17, size: 64, align: 32, offset: 64)
!17 = !DIBasicType(name: "u64", size: 64, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "fti_ptr", scope: !3, file: !4, line: 43, type: !10)
!19 = !DILocalVariable(name: "ti", scope: !3, file: !4, line: 45, type: !11)
!20 = !DILocalVariable(name: "fti", scope: !3, file: !4, line: 46, type: !11)
!21 = !DILocalVariable(name: "any_ti", scope: !3, file: !4, line: 48, type: !11)
!22 = !DILocation(line: 42, scope: !3)
!23 = !DILocation(line: 43, column: 13, scope: !3)
!24 = !DILocation(line: 43, scope: !3)
!25 = !DILocation(line: 44, column: 13, scope: !3)
!26 = !DILocation(line: 46, column: 12, scope: !3)
!27 = !DILocation(line: 45, scope: !3)
!28 = !DILocation(line: 47, column: 12, scope: !3)
!29 = !DILocation(line: 46, scope: !3)
!30 = !DILocation(line: 49, column: 12, scope: !3)
!31 = !DILocation(line: 48, scope: !3)
!32 = !DILocation(line: 51, column: 3, scope: !3)
!33 = !DILocation(line: 52, column: 3, scope: !3)
!34 = !DILocation(line: 53, column: 3, scope: !3)
!35 = !DILocation(line: 55, column: 3, scope: !3)
!36 = !DILocation(line: 56, column: 3, scope: !3)
!37 = !DILocation(line: 57, column: 3, scope: !3)
!38 = !DILocation(line: 59, column: 3, scope: !3)

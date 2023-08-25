; ModuleID = 'Glass'
source_filename = "Glass"

%TypeInfoTableElem = type { i64, i64, i64, i64, i64, i64, i64, i64 }
%TypeInfo = type { i64, i8*, i64, i64 }

@TypeInfoArray = constant [1 x %TypeInfoTableElem] zeroinitializer
@0 = private unnamed_addr constant [16 x i8] c"i32_ti.ID = %p\0A\00", align 1
@1 = private unnamed_addr constant [18 x i8] c"i32_ti.Name = %p\0A\00", align 1

declare i32 @printf(i8*, ...)

define i32 @main() !dbg !3 {
entry:
  %0 = alloca %TypeInfo, align 8
  %1 = alloca %TypeInfo*, align 8
  call void @llvm.dbg.declare(metadata %TypeInfo** %1, metadata !9, metadata !DIExpression()), !dbg !21
  store %TypeInfo* bitcast ([1 x %TypeInfoTableElem]* @TypeInfoArray to %TypeInfo*), %TypeInfo** %1, align 8, !dbg !22
  %2 = load %TypeInfo, %TypeInfo** %1, align 8, !dbg !22
  call void @llvm.dbg.declare(metadata %TypeInfo* %0, metadata !20, metadata !DIExpression()), !dbg !23
  store %TypeInfo %2, %TypeInfo* %0, align 8, !dbg !24
  %3 = getelementptr inbounds %TypeInfo, %TypeInfo* %0, i32 0, i32 0, !dbg !25
  %4 = load i64, i64* %3, align 4, !dbg !25
  %5 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([16 x i8], [16 x i8]* @0, i32 0, i32 0), i64 %4), !dbg !25
  %6 = getelementptr inbounds %TypeInfo, %TypeInfo* %0, i32 0, i32 1, !dbg !26
  %7 = load i8*, i8** %6, align 8, !dbg !26
  %8 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([18 x i8], [18 x i8]* @1, i32 0, i32 0), i8* %7), !dbg !26
  ret i32 0, !dbg !27
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "Glass Compiler", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "Main.glass", directory: ".")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = distinct !DISubprogram(name: "main", scope: !4, file: !4, line: 35, type: !5, scopeLine: 35, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !8)
!4 = !DIFile(filename: "HelloWorld.glass", directory: ".\\Examples")
!5 = !DISubroutineType(types: !6)
!6 = !{!7}
!7 = !DIBasicType(name: "i32", size: 32, encoding: DW_ATE_signed)
!8 = !{!9, !20}
!9 = !DILocalVariable(name: "i32_ti", scope: !3, file: !4, line: 37, type: !10)
!10 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !11, size: 64)
!11 = !DICompositeType(tag: DW_TAG_structure_type, name: "TypeInfo", line: 3435973836, size: 256, align: 32, elements: !12)
!12 = !{!13, !15, !18, !19}
!13 = !DIDerivedType(tag: DW_TAG_member, name: "id", line: 3435973836, baseType: !14, size: 64, offset: 4)
!14 = !DIBasicType(name: "u64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DIDerivedType(tag: DW_TAG_member, name: "name", line: 3435973836, baseType: !16, size: 64, align: 64, offset: 8)
!16 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !17, size: 64)
!17 = !DIBasicType(name: "u8", size: 8, encoding: DW_ATE_unsigned)
!18 = !DIDerivedType(tag: DW_TAG_member, name: "size", line: 3435973836, baseType: !14, size: 64, align: 128, offset: 4)
!19 = !DIDerivedType(tag: DW_TAG_member, name: "flags", line: 3435973836, baseType: null, size: 64, align: 192, offset: 4)
!20 = !DILocalVariable(name: "info", scope: !3, file: !4, line: 39, type: !11)
!21 = !DILocation(line: 37, scope: !3)
!22 = !DILocation(line: 38, column: 13, scope: !3)
!23 = !DILocation(line: 39, scope: !3)
!24 = !DILocation(line: 40, column: 12, scope: !3)
!25 = !DILocation(line: 42, column: 3, scope: !3)
!26 = !DILocation(line: 43, column: 3, scope: !3)
!27 = !DILocation(line: 47, column: 3, scope: !3)

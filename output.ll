; ModuleID = 'Glass'
source_filename = "Glass"

%Array = type { i64, %ptr* }
%ptr = type opaque
%Any = type { i64, %ptr* }

@0 = private unnamed_addr constant [8 x i8] c"Hi: %s\0A\00", align 1
@1 = private unnamed_addr constant [10 x i8] c"Arg: %s!\0A\00", align 1
@2 = private unnamed_addr constant [6 x i8] c"Hello\00", align 1
@3 = private unnamed_addr constant [12 x i8] c"Hello World\00", align 1

declare i32 @printf(i8*, ...)

define i32 @print_n(i8* %0) !dbg !3 {
entry:
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i8*, align 8
  call void @llvm.dbg.declare(metadata i8** %3, metadata !11, metadata !DIExpression()), !dbg !15
  store i8* %0, i8** %3, align 8
  call void @llvm.dbg.declare(metadata i32* %2, metadata !12, metadata !DIExpression()), !dbg !16
  store i32 10, i32* %2, align 4, !dbg !17
  call void @llvm.dbg.declare(metadata i32* %1, metadata !14, metadata !DIExpression()), !dbg !18
  store i32 0, i32* %1, align 4, !dbg !19
  br label %loop.cond, !dbg !19

loop.cond:                                        ; preds = %loop.body, %entry
  %4 = load i32, i32* %1, align 4, !dbg !19
  %5 = load i32, i32* %2, align 4, !dbg !20
  %6 = icmp ult i32 %4, %5, !dbg !20
  br i1 %6, label %loop.body, label %after.loop, !dbg !20

loop.body:                                        ; preds = %loop.cond
  %7 = load i8*, i8** %3, align 8, !dbg !21
  %8 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @0, i32 0, i32 0), i8* %7), !dbg !21
  %9 = load i32, i32* %1, align 4, !dbg !21
  %addition = add i32 %9, 1, !dbg !22
  store i32 %addition, i32* %1, align 4, !dbg !22
  br label %loop.cond, !dbg !22

after.loop:                                       ; preds = %loop.cond
  ret i32 0, !dbg !23
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

define i32 @print_vars(%Array %0) !dbg !24 {
entry:
  %1 = alloca %Any, align 8
  %2 = alloca i64, align 8
  %3 = alloca i64, align 8
  %4 = alloca i64, align 8
  %5 = alloca %Any*, align 8
  %6 = alloca %Array, align 8
  call void @llvm.dbg.declare(metadata %Array* %6, metadata !33, metadata !DIExpression()), !dbg !40
  store %Array %0, %Array* %6, align 8
  %7 = getelementptr inbounds %Array, %Array* %6, i32 0, i32 1
  %8 = load %ptr*, %ptr** %7, align 8, !dbg !41
  %9 = bitcast %ptr* %8 to %Any*, !dbg !41
  call void @llvm.dbg.declare(metadata %Any** %5, metadata !34, metadata !DIExpression()), !dbg !42
  store %Any* %9, %Any** %5, align 8, !dbg !41
  %10 = getelementptr inbounds %Array, %Array* %6, i32 0, i32 0, !dbg !41
  %11 = load i64, i64* %10, align 4, !dbg !43
  call void @llvm.dbg.declare(metadata i64* %4, metadata !36, metadata !DIExpression()), !dbg !44
  store i64 %11, i64* %4, align 4, !dbg !43
  call void @llvm.dbg.declare(metadata i64* %3, metadata !37, metadata !DIExpression()), !dbg !45
  store i64 0, i64* %3, align 4, !dbg !46
  call void @llvm.dbg.declare(metadata i64* %2, metadata !38, metadata !DIExpression()), !dbg !47
  store i64 1, i64* %2, align 4, !dbg !48
  br label %loop.cond, !dbg !48

loop.cond:                                        ; preds = %loop.body, %entry
  %12 = load i64, i64* %3, align 4, !dbg !48
  %13 = load i64, i64* %4, align 4, !dbg !49
  %14 = icmp ult i64 %12, %13, !dbg !49
  br i1 %14, label %loop.body, label %after.loop, !dbg !49

loop.body:                                        ; preds = %loop.cond
  %15 = load %Any*, %Any** %5, align 8, !dbg !49
  %16 = load i64, i64* %3, align 4, !dbg !50
  %array_access = getelementptr %Any, %Any* %15, i64 %16, !dbg !50
  %17 = load %Any, %Any* %array_access, align 8, !dbg !50
  call void @llvm.dbg.declare(metadata %Any* %1, metadata !39, metadata !DIExpression()), !dbg !51
  store %Any %17, %Any* %1, align 8, !dbg !50
  %18 = getelementptr inbounds %Any, %Any* %1, i32 0, i32 1, !dbg !52
  %19 = load %ptr*, %ptr** %18, align 8, !dbg !52
  %20 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([10 x i8], [10 x i8]* @1, i32 0, i32 0), %ptr* %19), !dbg !52
  %21 = load i64, i64* %3, align 4, !dbg !52
  %22 = load i64, i64* %2, align 4, !dbg !53
  %addition = add i64 %21, %22, !dbg !53
  store i64 %addition, i64* %3, align 4, !dbg !53
  br label %loop.cond, !dbg !53

after.loop:                                       ; preds = %loop.cond
  ret i32 0, !dbg !54
}

define i32 @main() !dbg !55 {
entry:
  %0 = alloca %Array, align 8
  %1 = alloca %Any, align 8
  %2 = alloca i32, align 4
  call void @llvm.dbg.declare(metadata i32* %2, metadata !59, metadata !DIExpression()), !dbg !60
  store i32 30, i32* %2, align 4, !dbg !61
  %any_varargs = alloca %Any, i64 1, align 8, !dbg !62
  %varargs_getelem = getelementptr %Any, %Any* %any_varargs, i64 0, !dbg !62
  %3 = getelementptr inbounds %Any, %Any* %1, i32 0, i32 0, !dbg !62
  %4 = getelementptr inbounds %Any, %Any* %1, i32 0, i32 1, !dbg !62
  store %ptr* bitcast ([6 x i8]* @2 to %ptr*), %ptr** %4, align 8, !dbg !62
  store i64 7, i64* %3, align 4, !dbg !62
  %5 = load %Any, %Any* %1, align 8, !dbg !62
  store %Any %5, %Any* %varargs_getelem, align 8, !dbg !62
  %6 = getelementptr inbounds %Array, %Array* %0, i32 0, i32 0, !dbg !62
  %7 = getelementptr inbounds %Array, %Array* %0, i32 0, i32 1, !dbg !62
  %"varargs first" = getelementptr %Any, %Any* %any_varargs, i64 0, !dbg !62
  %8 = bitcast %Any* %"varargs first" to %ptr*, !dbg !62
  store %ptr* %8, %ptr** %7, align 8, !dbg !62
  store i64 1, i64* %6, align 4, !dbg !62
  %9 = load %Array, %Array* %0, align 8, !dbg !62
  %10 = call i32 @print_vars(%Array %9), !dbg !62
  %11 = call i32 @print_n(i8* getelementptr inbounds ([12 x i8], [12 x i8]* @3, i32 0, i32 0)), !dbg !63
  ret i32 0, !dbg !64
}

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "Glass Compiler", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "Main.glass", directory: ".")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = distinct !DISubprogram(name: "print_n", scope: !4, file: !4, line: 2, type: !5, scopeLine: 2, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !10)
!4 = !DIFile(filename: "HelloWorld.glass", directory: ".\\Examples")
!5 = !DISubroutineType(types: !6)
!6 = !{!7, !8}
!7 = !DIBasicType(name: "i32", size: 32, encoding: DW_ATE_signed)
!8 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !9, size: 64)
!9 = !DIBasicType(name: "u8", size: 8, encoding: DW_ATE_unsigned)
!10 = !{!11, !12, !14}
!11 = !DILocalVariable(name: "string", scope: !3, file: !4, line: 2, type: !8)
!12 = !DILocalVariable(name: "count", scope: !3, file: !4, line: 4, type: !13)
!13 = !DIBasicType(name: "u32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "i", scope: !3, file: !4, line: 5, type: !13)
!15 = !DILocation(line: 2, scope: !3)
!16 = !DILocation(line: 4, scope: !3)
!17 = !DILocation(line: 5, column: 7, scope: !3)
!18 = !DILocation(line: 5, scope: !3)
!19 = !DILocation(line: 6, column: 7, scope: !3)
!20 = !DILocation(line: 8, column: 9, scope: !3)
!21 = !DILocation(line: 9, column: 4, scope: !3)
!22 = !DILocation(line: 10, column: 4, scope: !3)
!23 = !DILocation(line: 13, column: 3, scope: !3)
!24 = distinct !DISubprogram(name: "print_vars", scope: !4, file: !4, line: 15, type: !25, scopeLine: 15, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !32)
!25 = !DISubroutineType(types: !26)
!26 = !{!7, !27}
!27 = !DICompositeType(tag: DW_TAG_structure_type, name: "Any", line: 3435973836, size: 16, align: 32, elements: !28)
!28 = !{!29, !30}
!29 = !DIBasicType(name: "u64", size: 64, encoding: DW_ATE_unsigned)
!30 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !31, size: 64)
!31 = !DIBasicType(tag: DW_TAG_unspecified_type, name: "void")
!32 = !{!33, !34, !36, !37, !38, !39}
!33 = !DILocalVariable(name: "args", scope: !24, file: !4, line: 15, type: !27)
!34 = !DILocalVariable(name: "anies", scope: !24, file: !4, line: 17, type: !35)
!35 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !27, size: 64)
!36 = !DILocalVariable(name: "count", scope: !24, file: !4, line: 18, type: !29)
!37 = !DILocalVariable(name: "i", scope: !24, file: !4, line: 20, type: !29)
!38 = !DILocalVariable(name: "one", scope: !24, file: !4, line: 21, type: !29)
!39 = !DILocalVariable(name: "any", scope: !24, file: !4, line: 25, type: !27)
!40 = !DILocation(line: 15, scope: !24)
!41 = !DILocation(line: 18, column: 8, scope: !24)
!42 = !DILocation(line: 17, scope: !24)
!43 = !DILocation(line: 19, column: 7, scope: !24)
!44 = !DILocation(line: 18, scope: !24)
!45 = !DILocation(line: 20, scope: !24)
!46 = !DILocation(line: 21, column: 7, scope: !24)
!47 = !DILocation(line: 21, scope: !24)
!48 = !DILocation(line: 22, column: 7, scope: !24)
!49 = !DILocation(line: 24, column: 9, scope: !24)
!50 = !DILocation(line: 26, column: 8, scope: !24)
!51 = !DILocation(line: 25, scope: !24)
!52 = !DILocation(line: 28, column: 4, scope: !24)
!53 = !DILocation(line: 30, column: 4, scope: !24)
!54 = !DILocation(line: 33, column: 3, scope: !24)
!55 = distinct !DISubprogram(name: "main", scope: !4, file: !4, line: 39, type: !56, scopeLine: 39, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !58)
!56 = !DISubroutineType(types: !57)
!57 = !{!7}
!58 = !{!59}
!59 = !DILocalVariable(name: "data", scope: !55, file: !4, line: 41, type: !7)
!60 = !DILocation(line: 41, scope: !55)
!61 = !DILocation(line: 42, column: 7, scope: !55)
!62 = !DILocation(line: 44, column: 3, scope: !55)
!63 = !DILocation(line: 45, column: 3, scope: !55)
!64 = !DILocation(line: 47, column: 3, scope: !55)

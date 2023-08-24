; ModuleID = 'Glass'
source_filename = "Glass"

%ptr = type opaque
%Color = type { i8, i8, i8, i8 }
%Font = type { i32, i32, i32, %Texture, %ptr*, %ptr* }
%Texture = type { i32, i32, i32, i32, i32 }
%vec2 = type { float, float }
%GlyphInfo = type { i32, i32, i32, i32, %Image }
%Image = type { %ptr*, i32, i32, i32, i32 }

@0 = private unnamed_addr constant [13 x i8] c"Hello Window\00", align 1

declare i32 @printf(i8*, ...)

declare i32 @sprintf(i8*, i8*, ...)

declare i64* @fopen(i8*, i8*)

declare %ptr @fclose(i64*)

declare i64* @malloc(i64)

declare %ptr @free(i64)

declare i64 @strlen(i8*)

declare i64 @memcpy(i8*, i8*, i32)

declare i64 @memset(i64*, i32, i32)

declare %ptr @putchar(i8)

declare %ptr @abort()

declare i32 @strcmp(i8*, i8*)

declare i32 @strncmp(i8*, i8*, i32)

declare i32 @isalpha(i32)

declare i32 @isdigit(i32)

declare i32 @isspace(i32)

declare i32 @fseek(i64*, i64, i32)

declare i64 @ftell(i64*)

declare i64 @fread(i8*, i64, i64, i64*)

declare %ptr @exit(i32)

declare i32 @rand()

declare %ptr @SetConfigFlags(i32)

declare %ptr @InitWindow(i32, i32, i8*)

declare i32 @WindowShouldClose()

declare %ptr @BeginDrawing()

declare %ptr @EndDrawing()

declare %ptr @ClearBackground(%Color)

declare %ptr @DrawRectangle(i32, i32, i32, i32, %Color)

declare %ptr @DrawText(i8*, i32, i32, i32, %Color)

declare %ptr @DrawTextEx(%Font, i8*, %vec2, float, float, %Color)

declare %ptr @DrawTextCodepoints(%Font, i32*, i32, %vec2, float, float, %Color)

declare %ptr @DrawTextCodepoint(%Font, i32, %vec2, float, %Color)

declare i32 @GetGlyphIndex(%Font, i32)

declare %GlyphInfo @GetGlyphInfo(%Font, i32)

declare %ptr @SetTargetFPS(i32)

declare %ptr @DrawFPS(i32, i32)

declare %Font @LoadFont(i8*)

declare %Font @LoadFontEx(i8*, i32, i32*, i32)

declare float @GetMouseWheelMove()

define i32 @Update() !dbg !3 {
entry:
  %0 = alloca %Color, align 8
  call void @llvm.dbg.declare(metadata %Color* %0, metadata !9, metadata !DIExpression()), !dbg !17
  %1 = getelementptr inbounds %Color, %Color* %0, i32 0, i32 0, !dbg !18
  store i32 -128, i8* %1, align 4, !dbg !19
  %2 = getelementptr inbounds %Color, %Color* %0, i32 0, i32 1, !dbg !19
  store i32 -128, i8* %2, align 4, !dbg !20
  %3 = getelementptr inbounds %Color, %Color* %0, i32 0, i32 2, !dbg !20
  store i32 -128, i8* %3, align 4, !dbg !21
  %4 = getelementptr inbounds %Color, %Color* %0, i32 0, i32 3, !dbg !21
  store i32 -128, i8* %4, align 4, !dbg !22
  %5 = call %ptr @BeginDrawing(), !dbg !22
  %6 = load %Color, %Color* %0, align 1, !dbg !22
  %7 = call %ptr @ClearBackground(%Color %6), !dbg !23
  %8 = call %ptr @DrawFPS(i32 60, i32 60), !dbg !24
  %9 = call %ptr @EndDrawing(), !dbg !24
  ret i32 0, !dbg !25
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

define i32 @main() !dbg !26 {
entry:
  %0 = alloca i32, align 4
  call void @llvm.dbg.declare(metadata i32* %0, metadata !28, metadata !DIExpression()), !dbg !29
  store i32 4, i32* %0, align 4, !dbg !30
  %1 = load i32, i32* %0, align 4, !dbg !30
  %2 = call %ptr @SetConfigFlags(i32 %1), !dbg !31
  %3 = call %ptr @InitWindow(i32 500, i32 700, i8* getelementptr inbounds ([13 x i8], [13 x i8]* @0, i32 0, i32 0)), !dbg !32
  br label %loop.cond, !dbg !32

loop.cond:                                        ; preds = %loop.body, %entry
  %4 = call i32 @WindowShouldClose(), !dbg !32
  %comp = icmp ne i32 %4, 1, !dbg !33
  br i1 %comp, label %loop.body, label %after.loop, !dbg !33

loop.body:                                        ; preds = %loop.cond
  %5 = call i32 @Update(), !dbg !33
  br label %loop.cond, !dbg !34

after.loop:                                       ; preds = %loop.cond
  ret i32 0, !dbg !35
}

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "Glass Compiler", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "Main.glass", directory: ".")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = distinct !DISubprogram(name: "Update", scope: !4, file: !4, type: !5, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !8)
!4 = !DIFile(filename: "Main.glass", directory: ".\\Examples")
!5 = !DISubroutineType(types: !6)
!6 = !{!7}
!7 = !DIBasicType(name: "i32", size: 32, encoding: DW_ATE_signed)
!8 = !{!9}
!9 = !DILocalVariable(name: "clear_color", scope: !3, file: !4, line: 2, type: !10)
!10 = !DICompositeType(tag: DW_TAG_structure_type, name: "Color", line: 7, size: 32, align: 32, elements: !11)
!11 = !{!12, !14, !15, !16}
!12 = !DIDerivedType(tag: DW_TAG_member, name: "r", line: 8, baseType: !13, size: 8, offset: 1)
!13 = !DIBasicType(name: "u8", size: 8, encoding: DW_ATE_unsigned)
!14 = !DIDerivedType(tag: DW_TAG_member, name: "g", line: 9, baseType: !13, size: 8, align: 8, offset: 1)
!15 = !DIDerivedType(tag: DW_TAG_member, name: "b", line: 10, baseType: !13, size: 8, align: 16, offset: 1)
!16 = !DIDerivedType(tag: DW_TAG_member, name: "a", line: 11, baseType: !13, size: 8, align: 24, offset: 1)
!17 = !DILocation(line: 2, scope: !3)
!18 = !DILocation(line: 3, column: 9, scope: !3)
!19 = !DILocation(line: 5, column: 15, scope: !3)
!20 = !DILocation(line: 6, column: 15, scope: !3)
!21 = !DILocation(line: 7, column: 15, scope: !3)
!22 = !DILocation(line: 8, column: 15, scope: !3)
!23 = !DILocation(line: 11, column: 3, scope: !3)
!24 = !DILocation(line: 13, column: 3, scope: !3)
!25 = !DILocation(line: 17, column: 3, scope: !3)
!26 = distinct !DISubprogram(name: "main", scope: !4, file: !4, line: 19, type: !5, scopeLine: 19, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !27)
!27 = !{!28}
!28 = !DILocalVariable(name: "flags", scope: !26, file: !4, line: 21, type: !7)
!29 = !DILocation(line: 21, scope: !26)
!30 = !DILocation(line: 22, column: 7, scope: !26)
!31 = !DILocation(line: 24, column: 3, scope: !26)
!32 = !DILocation(line: 26, column: 3, scope: !26)
!33 = !DILocation(line: 28, column: 10, scope: !26)
!34 = !DILocation(line: 29, column: 4, scope: !26)
!35 = !DILocation(line: 32, column: 3, scope: !26)

@0 = private unnamed_addr constant [18 x i8] c"Hello World, %i\\n\00", align 1

declare i32 @printf(i8* %0, ...)

declare i32* @malloc(i32 %0)

define i32 @main() {
entry:
  %0 = call i32* @malloc(i32 10)
  %1 = alloca i32*, align 8
  store i32* %0, i32** %1, align 8
  %2 = load i32*, i32** %1, align 8
  %3 = getelementptr i32, i32* %2, i32 0
  store i32 0, i32* %3, align 4
  %4 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([18 x i8], [18 x i8]* @0, i32 0, i32 0), i32 4)
  %5 = load i32*, i32** %1, align 8
  %6 = getelementptr i32, i32* %5, i32 0
  %7 = load i32, i32* %6, align 4
  ret i32 %7
}
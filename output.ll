@0 = private unnamed_addr constant [10 x i8] c"Helllo Me\00", align 1

declare extern_weak i32 @printf(i8* %0, ...)

define i32 @print(i8* %0) {
entry:
  %1 = alloca i8*, align 8
  store i8* %0, i8** %1, align 8
  ret i32 0
}

define i32 @main() {
entry:
  %0 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([10 x i8], [10 x i8]* @0, i32 0, i32 0))
  ret i32 0
}
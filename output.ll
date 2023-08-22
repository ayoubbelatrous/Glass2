
@0 = private unnamed_addr constant [14 x i8] c"Hello World\0A\00", align 1
@1 = private unnamed_addr constant [14 x i8] c"Hello World 2\00", align 1

declare i32 @printf(i8* %0, ...)

declare void* @malloc(i32 %0)

declare void @free(void* %0)

define i32 @main() {
entry:
  %0 = alloca i8, align 1
  store i8 1, i8* %0, align 1
  %1 = alloca i8, align 1
  store i8 -1, i8* %1, align 1
  %2 = load i8, i8* %0, align 1
  %3 = icmp ugt i8 %2, 0
  br i1 %3, label %then, label %cont

then:                                             ; preds = %entry
  %4 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @0, i32 0, i32 0))
  br label %cont

cont:                                             ; preds = %then, %entry
  %5 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @1, i32 0, i32 0))
  ret i32 0
}
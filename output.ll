@0 = private unnamed_addr constant [17 x i8] c"Hello World, %i\0A\00", align 1
@1 = private unnamed_addr constant [16 x i8] c"Other Condition\00", align 1

declare i32 @printf(i8* %0, ...)

declare i8* @malloc(i32 %0)

declare void @free(i8* %0)

define i32 @main() {
entry:
  %0 = alloca i8, align 1
  store i8 10, i8* %0, align 1
  %1 = alloca i8, align 1
  store i8 -1, i8* %1, align 1
  br label %loop.cond

loop.cond:                                        ; preds = %loop.body, %entry
  %2 = load i8, i8* %0, align 1
  %3 = icmp ugt i8 %2, 0
  br i1 %3, label %loop.body, label %after.loop

loop.body:                                        ; preds = %loop.cond
  %4 = load i8, i8* %0, align 1
  %5 = load i8, i8* %1, align 1
  %addition = add i8 %4, %5
  store i8 %addition, i8* %0, align 1
  %6 = load i8, i8* %0, align 1
  %7 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @0, i32 0, i32 0), i8 %6)
  br label %loop.cond

after.loop:                                       ; preds = %loop.cond
  %8 = alloca i8, align 1
  store i8 10, i8* %8, align 1
  %9 = load i8, i8* %8, align 1
  %10 = icmp ugt i8 %9, 0
  br i1 %10, label %then, label %cont

then:                                             ; preds = %after.loop
  %11 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([16 x i8], [16 x i8]* @1,
i32 0, i32 0))
  br label %cont

cont:                                             ; preds = %then, %after.loop
  ret i32 0
}
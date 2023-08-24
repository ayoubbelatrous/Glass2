; ModuleID = 'Glass'
source_filename = "Glass"

%ptr = type opaque
%Any = type { i64, %ptr* }

declare i32 @printf(i8*, ...)

declare %ptr* @malloc(i32)

declare %ptr @free(%ptr*)

declare i64 @strlen(i8*)

declare i32 @rand()

define i32 @take_array(%Any* %0) {
entry:
  %1 = alloca %Any*, align 8
  store %Any* %0, %Any** %1, align 8
  ret i32 0
}

define i32 @main() {
entry:
  ret i32 0
}

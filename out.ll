declare void @print_int(i32)
declare void @print_char(i8)
declare void @print_string(i8*)
declare void @print_float(float)


define i8 @nextChar(i8 %p0) {
entry:
  %c = alloca i8, align 1
  store i8 %p0, i8* %c, align 1
  %c0 = load i8, i8* %c, align 1
  %v0 = zext i8 %c0 to i32
  %v1 = add i32 0, 1
  %v2 = add i32 %v0, %v1
  %sc0 = trunc i32 %v2 to i8
  store i8 %sc0, i8* %c, align 1
  %c3 = load i8, i8* %c, align 1
  %v3 = zext i8 %c3 to i32
  %retc1 = trunc i32 %v3 to i8
  ret i8 %retc1
}

define i32 @main() {
entry:
  %ch = alloca i8, align 1
  %v4 = add i32 0, 65
  %ac0 = trunc i32 %v4 to i8
  %callr0 = call i8 @nextChar(i8 %ac0)
  %v5 = zext i8 %callr0 to i32
  %sc2 = trunc i32 %v5 to i8
  store i8 %sc2, i8* %ch, align 1
  %c6 = load i8, i8* %ch, align 1
  %v6 = zext i8 %c6 to i32
  %pc3 = trunc i32 %v6 to i8
  call void @print_char(i8 %pc3)
  %v7 = add i32 0, 0
  ret i32 %v7
}


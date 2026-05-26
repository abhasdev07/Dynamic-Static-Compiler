declare void @print_int(i32)
declare void @print_char(i8)
declare void @print_string(i8*)
declare void @print_float(float)


define i32 @main() {
entry:
  %x = alloca i32, align 4
  %v0 = add i32 0, 90
  store i32 %v0, i32* %x, align 4
  %v1 = load i32, i32* %x, align 4
  %v2 = add i32 0, 1
  %v3 = add i32 %v1, %v2
  store i32 %v3, i32* %x, align 4
  %v4 = load i32, i32* %x, align 4
  %v5 = add i32 0, 10
  %v6 = add i32 %v4, %v5
  store i32 %v6, i32* %x, align 4
  %v7 = load i32, i32* %x, align 4
  call void @print_int(i32 %v7)
  %v8 = add i32 0, 0
  ret i32 %v8
}


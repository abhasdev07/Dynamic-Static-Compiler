declare void @print_int(i32)
declare void @print_char(i8)
declare void @print_string(i8*)
declare void @print_float(float)

@.str.0 = private unnamed_addr constant [7 x i8] c"sum = \00", align 1
@.str.1 = private unnamed_addr constant [13 x i8] c"next char = \00", align 1

define i32 @nextChar() {
entry:
  %c = alloca i8, align 1
  %psc0 = trunc i32 %p0 to i8
  store i8 %psc0, i8* %c, align 1
  %c0 = load i8, i8* %c, align 1
  %v0 = zext i8 %c0 to i32
  %v1 = add i32 0, 1
  %v2 = add i32 %v0, %v1
  %sc1 = trunc i32 %v2 to i8
  store i8 %sc1, i8* %c, align 1
  %c3 = load i8, i8* %c, align 1
  %v3 = zext i8 %c3 to i32
  ret i32 %v3
}

define i32 @add(i32 %p0, i32 %p1) {
entry:
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  store i32 %p0, i32* %a, align 4
  store i32 %p1, i32* %b, align 4
  %v4 = load i32, i32* %a, align 4
  %v5 = load i32, i32* %b, align 4
  %v6 = add i32 %v4, %v5
  ret i32 %v6
}

define i32 @main() {
entry:
  %x = alloca i32, align 4
  %ch = alloca i8, align 1
  %v7 = add i32 0, 10
  %v8 = add i32 0, 20
  store i32 %v9, i32* %x, align 4
  %pslit0 = getelementptr inbounds [7 x i8], [7 x i8]* @.str.0, i64 0, i64 0
  call void @print_string(i8* %pslit0)
  %v10 = load i32, i32* %x, align 4
  call void @print_int(i32 %v10)
  %v11 = add i32 0, 65
  %sc1 = trunc i32 %v12 to i8
  store i8 %sc1, i8* %ch, align 1
  %pslit2 = getelementptr inbounds [13 x i8], [13 x i8]* @.str.1, i64 0, i64 0
  call void @print_string(i8* %pslit2)
  %c13 = load i8, i8* %ch, align 1
  %v13 = zext i8 %c13 to i32
  %pc3 = trunc i32 %v13 to i8
  call void @print_char(i8 %pc3)
  %v14 = add i32 0, 0
  ret i32 %v14
}


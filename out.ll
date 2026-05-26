declare void @print_int(i32)
declare void @print_char(i8)
declare void @print_string(i8*)
declare void @print_float(float)

@.str.0 = private unnamed_addr constant [6 x i8] c"Hello\00", align 1
@.str.1 = private unnamed_addr constant [9 x i8] c"Compiler\00", align 1

define i32 @add(i32 %p0, i32 %p1) {
entry:
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  store i32 %p0, i32* %a, align 4
  store i32 %p1, i32* %b, align 4
  %v0 = load i32, i32* %a, align 4
  %v1 = load i32, i32* %b, align 4
  %v2 = add i32 %v0, %v1
  ret i32 %v2
}

define i32 @main() {
entry:
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %z = alloca i32, align 4
  %ch = alloca i8, align 1
  %msg = alloca i8*, align 8
  %compiler = alloca i8*, align 8
  %arr = alloca [5 x i32], align 4
  %i = alloca i32, align 4
  %j = alloca i32, align 4
  %fact = alloca i32, align 4
  %n = alloca i32, align 4
  %v3 = add i32 0, 10
  store i32 %v3, i32* %x, align 4
  %v4 = add i32 0, 20
  store i32 %v4, i32* %y, align 4
  %v5 = load i32, i32* %x, align 4
  %v6 = load i32, i32* %y, align 4
  store i32 %v7, i32* %z, align 4
  %v8 = load i32, i32* %z, align 4
  call void @print_int(i32 %v8)
  %v9 = load i32, i32* %z, align 4
  %v10 = add i32 0, 20
  %cmp0 = icmp sgt i32 %v9, %v10
  %v11 = zext i1 %cmp0 to i32
  %jf1 = icmp eq i32 %v11, 0
  br i1 %jf1, label %else_0, label %jf_cont_1
jf_cont_1:
  %v12 = add i32 0, 111
  call void @print_int(i32 %v12)
  br label %endif_1
else_0:
  %v13 = add i32 0, 222
  call void @print_int(i32 %v13)
endif_1:
  %v14 = add i32 0, 65
  %sc2 = trunc i32 %v14 to i8
  store i8 %sc2, i8* %ch, align 1
  %c15 = load i8, i8* %ch, align 1
  %v15 = zext i8 %c15 to i32
  %pc3 = trunc i32 %v15 to i8
  call void @print_char(i8 %pc3)
  %c16 = load i8, i8* %ch, align 1
  %v16 = zext i8 %c16 to i32
  %v17 = add i32 0, 1
  %v18 = add i32 %v16, %v17
  %sc4 = trunc i32 %v18 to i8
  store i8 %sc4, i8* %ch, align 1
  %c19 = load i8, i8* %ch, align 1
  %v19 = zext i8 %c19 to i32
  %pc5 = trunc i32 %v19 to i8
  call void @print_char(i8 %pc5)
  %c20 = load i8, i8* %ch, align 1
  %v20 = zext i8 %c20 to i32
  %v21 = add i32 0, 2
  %v22 = add i32 %v20, %v21
  %sc6 = trunc i32 %v22 to i8
  store i8 %sc6, i8* %ch, align 1
  %c23 = load i8, i8* %ch, align 1
  %v23 = zext i8 %c23 to i32
  %pc7 = trunc i32 %v23 to i8
  call void @print_char(i8 %pc7)
  %c24 = load i8, i8* %ch, align 1
  %v24 = zext i8 %c24 to i32
  %v25 = add i32 0, 66
  %cmp8 = icmp sgt i32 %v24, %v25
  %v26 = zext i1 %cmp8 to i32
  %jf9 = icmp eq i32 %v26, 0
  br i1 %jf9, label %else_2, label %jf_cont_9
jf_cont_9:
  %v27 = add i32 0, 333
  call void @print_int(i32 %v27)
  br label %endif_3
else_2:
  %v28 = add i32 0, 444
  call void @print_int(i32 %v28)
endif_3:
  %strinit10 = getelementptr inbounds [6 x i8], [6 x i8]* @.str.0, i64 0, i64 0
  store i8* %strinit10, i8** %msg, align 8
  %pstr11 = load i8*, i8** %msg, align 8
  call void @print_string(i8* %pstr11)
  %v29 = add i32 0, 0
  %sp12 = load i8*, i8** %msg, align 8
  %cp12 = getelementptr i8, i8* %sp12, i32 %v29
  %cb12 = load i8, i8* %cp12, align 1
  %v30 = zext i8 %cb12 to i32
  %pc13 = trunc i32 %v30 to i8
  call void @print_char(i8 %pc13)
  %v31 = add i32 0, 1
  %sp14 = load i8*, i8** %msg, align 8
  %cp14 = getelementptr i8, i8* %sp14, i32 %v31
  %cb14 = load i8, i8* %cp14, align 1
  %v32 = zext i8 %cb14 to i32
  %pc15 = trunc i32 %v32 to i8
  call void @print_char(i8 %pc15)
  %v33 = add i32 0, 4
  %sp16 = load i8*, i8** %msg, align 8
  %cp16 = getelementptr i8, i8* %sp16, i32 %v33
  %cb16 = load i8, i8* %cp16, align 1
  %v34 = zext i8 %cb16 to i32
  %pc17 = trunc i32 %v34 to i8
  call void @print_char(i8 %pc17)
  %strinit18 = getelementptr inbounds [9 x i8], [9 x i8]* @.str.1, i64 0, i64 0
  store i8* %strinit18, i8** %compiler, align 8
  %pstr19 = load i8*, i8** %compiler, align 8
  call void @print_string(i8* %pstr19)
  %v35 = add i32 0, 0
  %sp20 = load i8*, i8** %compiler, align 8
  %cp20 = getelementptr i8, i8* %sp20, i32 %v35
  %cb20 = load i8, i8* %cp20, align 1
  %v36 = zext i8 %cb20 to i32
  %pc21 = trunc i32 %v36 to i8
  call void @print_char(i8 %pc21)
  %v37 = add i32 0, 3
  %sp22 = load i8*, i8** %compiler, align 8
  %cp22 = getelementptr i8, i8* %sp22, i32 %v37
  %cb22 = load i8, i8* %cp22, align 1
  %v38 = zext i8 %cb22 to i32
  %pc23 = trunc i32 %v38 to i8
  call void @print_char(i8 %pc23)
  %v39 = add i32 0, 5
  %v40 = add i32 0, 0
  %v41 = add i32 0, 5
  %ap24 = getelementptr inbounds [5 x i32], [5 x i32]* %arr, i64 0, i64 %v40
  store i32 %v41, i32* %ap24, align 4
  %v42 = add i32 0, 1
  %v43 = add i32 0, 10
  %ap25 = getelementptr inbounds [5 x i32], [5 x i32]* %arr, i64 0, i64 %v42
  store i32 %v43, i32* %ap25, align 4
  %v44 = add i32 0, 2
  %v45 = add i32 0, 15
  %ap26 = getelementptr inbounds [5 x i32], [5 x i32]* %arr, i64 0, i64 %v44
  store i32 %v45, i32* %ap26, align 4
  %v46 = add i32 0, 3
  %v47 = add i32 0, 0
  %ap27 = getelementptr inbounds [5 x i32], [5 x i32]* %arr, i64 0, i64 %v47
  %v48 = load i32, i32* %ap27, align 4
  %v49 = add i32 0, 1
  %ap28 = getelementptr inbounds [5 x i32], [5 x i32]* %arr, i64 0, i64 %v49
  %v50 = load i32, i32* %ap28, align 4
  %v51 = add i32 %v48, %v50
  %ap29 = getelementptr inbounds [5 x i32], [5 x i32]* %arr, i64 0, i64 %v46
  store i32 %v51, i32* %ap29, align 4
  %v52 = add i32 0, 4
  %v53 = add i32 0, 2
  %ap30 = getelementptr inbounds [5 x i32], [5 x i32]* %arr, i64 0, i64 %v53
  %v54 = load i32, i32* %ap30, align 4
  %v55 = add i32 0, 2
  %v56 = mul i32 %v54, %v55
  %ap31 = getelementptr inbounds [5 x i32], [5 x i32]* %arr, i64 0, i64 %v52
  store i32 %v56, i32* %ap31, align 4
  %v57 = add i32 0, 3
  %ap32 = getelementptr inbounds [5 x i32], [5 x i32]* %arr, i64 0, i64 %v57
  %v58 = load i32, i32* %ap32, align 4
  call void @print_int(i32 %v58)
  %v59 = add i32 0, 4
  %ap33 = getelementptr inbounds [5 x i32], [5 x i32]* %arr, i64 0, i64 %v59
  %v60 = load i32, i32* %ap33, align 4
  call void @print_int(i32 %v60)
  %v61 = add i32 0, 0
  store i32 %v61, i32* %i, align 4
while_start_4:
  %v62 = load i32, i32* %i, align 4
  %v63 = add i32 0, 5
  %cmp34 = icmp slt i32 %v62, %v63
  %v64 = zext i1 %cmp34 to i32
  %jf35 = icmp eq i32 %v64, 0
  br i1 %jf35, label %while_end_5, label %jf_cont_35
jf_cont_35:
  %v65 = load i32, i32* %i, align 4
  %ap36 = getelementptr inbounds [5 x i32], [5 x i32]* %arr, i64 0, i64 %v65
  %v66 = load i32, i32* %ap36, align 4
  call void @print_int(i32 %v66)
  %v67 = load i32, i32* %i, align 4
  %v68 = add i32 0, 1
  %v69 = add i32 %v67, %v68
  store i32 %v69, i32* %i, align 4
  br label %while_start_4
while_end_5:
  %v70 = add i32 0, 0
  store i32 %v70, i32* %j, align 4
for_start_6:
  %v71 = load i32, i32* %j, align 4
  %v72 = add i32 0, 3
  %cmp37 = icmp slt i32 %v71, %v72
  %v73 = zext i1 %cmp37 to i32
  %jf38 = icmp eq i32 %v73, 0
  br i1 %jf38, label %for_end_7, label %jf_cont_38
jf_cont_38:
  %v74 = load i32, i32* %j, align 4
  %v75 = add i32 0, 1
  %cmp39 = icmp eq i32 %v74, %v75
  %v76 = zext i1 %cmp39 to i32
  %jf40 = icmp eq i32 %v76, 0
  br i1 %jf40, label %else_8, label %jf_cont_40
jf_cont_40:
  %v77 = add i32 0, 999
  call void @print_int(i32 %v77)
  br label %endif_9
else_8:
  %v78 = load i32, i32* %j, align 4
  call void @print_int(i32 %v78)
endif_9:
  %v79 = load i32, i32* %j, align 4
  %v80 = add i32 0, 1
  %v81 = add i32 %v79, %v80
  store i32 %v81, i32* %j, align 4
  br label %for_start_6
for_end_7:
  %v82 = add i32 0, 1
  store i32 %v82, i32* %fact, align 4
  %v83 = add i32 0, 5
  store i32 %v83, i32* %n, align 4
while_start_10:
  %v84 = load i32, i32* %n, align 4
  %v85 = add i32 0, 1
  %cmp41 = icmp sgt i32 %v84, %v85
  %v86 = zext i1 %cmp41 to i32
  %jf42 = icmp eq i32 %v86, 0
  br i1 %jf42, label %while_end_11, label %jf_cont_42
jf_cont_42:
  %v87 = load i32, i32* %fact, align 4
  %v88 = load i32, i32* %n, align 4
  %v89 = mul i32 %v87, %v88
  store i32 %v89, i32* %fact, align 4
  %v90 = load i32, i32* %n, align 4
  %v91 = add i32 0, 1
  %v92 = sub i32 %v90, %v91
  store i32 %v92, i32* %n, align 4
  br label %while_start_10
while_end_11:
  %v93 = load i32, i32* %fact, align 4
  call void @print_int(i32 %v93)
  %v94 = add i32 0, 0
  ret i32 %v94
}


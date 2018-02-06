define i32 @r520(i32* noalias nocapture, i32* noalias nocapture) {
entry:
  br label %2

; <label>:2:                                      ; preds = %entry
  %3 = getelementptr i32, i32* %0, i32 18
  %4 = load i32, i32* %3
  br label %5

; <label>:5:                                      ; preds = %138, %2
  %6 = shl i32 %4, 2
  %7 = getelementptr i32, i32* %0, i32 1
  store i32 %6, i32* %7
  %8 = getelementptr i32, i32* %0, i32 1
  %9 = load i32, i32* %8
  %10 = getelementptr i32, i32* %0, i32 20
  %11 = load i32, i32* %10
  %12 = add i32 %11, %9
  %13 = getelementptr i32, i32* %0, i32 3
  store i32 %12, i32* %13
  %14 = getelementptr i32, i32* %0, i32 18
  %15 = load i32, i32* %14
  %16 = add i32 %15, 0
  %17 = getelementptr i32, i32* %0, i32 4
  store i32 %16, i32* %17
  %18 = getelementptr i32, i32* %0, i32 4
  %19 = load i32, i32* %18
  br label %20

; <label>:20:                                     ; preds = %20, %5
  %21 = add i32 %19, 0
  %22 = getelementptr i32, i32* %0, i32 18
  store i32 %21, i32* %22
  %23 = getelementptr i32, i32* %0, i32 3
  %24 = load i32, i32* %23
  %25 = add i32 %24, 4
  %26 = sub i32 %25, 42124
  %27 = sdiv exact i32 %26, 4
  %28 = getelementptr i32, i32* %1, i32 %27
  %29 = load i32, i32* %28
  %30 = getelementptr i32, i32* %0, i32 7
  store i32 %29, i32* %30
  %31 = getelementptr i32, i32* %0, i32 2
  %32 = load i32, i32* %31
  %33 = getelementptr i32, i32* %0, i32 7
  %34 = load i32, i32* %33
  %35 = icmp slt i32 %34, %32
  %36 = sext i1 %35 to i32
  %37 = getelementptr i32, i32* %0, i32 1
  store i32 %36, i32* %37
  %38 = getelementptr i32, i32* %0, i32 3
  %39 = load i32, i32* %38
  %40 = add i32 %39, 4
  %41 = getelementptr i32, i32* %0, i32 3
  store i32 %40, i32* %41
  %42 = getelementptr i32, i32* %0, i32 18
  %43 = load i32, i32* %42
  %44 = add i32 %43, 1
  %45 = getelementptr i32, i32* %0, i32 4
  store i32 %44, i32* %45
  %46 = getelementptr i32, i32* %0, i32 1
  %47 = load i32, i32* %46
  %48 = icmp ne i32 %47, 0
  br i1 %48, label %20, label %52
                                                  ; No predecessors!
  %50 = getelementptr i32, i32* %0, i32 6
  %51 = load i32, i32* %50
  br label %52

; <label>:52:                                     ; preds = %20, %49
  %53 = shl i32 %51, 2
  %54 = getelementptr i32, i32* %0, i32 1
  store i32 %53, i32* %54
  %55 = getelementptr i32, i32* %0, i32 1
  %56 = load i32, i32* %55
  %57 = getelementptr i32, i32* %0, i32 19
  %58 = load i32, i32* %57
  %59 = add i32 %58, %56
  %60 = getelementptr i32, i32* %0, i32 8
  store i32 %59, i32* %60
  %61 = getelementptr i32, i32* %0, i32 6
  %62 = load i32, i32* %61
  %63 = add i32 %62, 0
  %64 = getelementptr i32, i32* %0, i32 9
  store i32 %63, i32* %64
  %65 = getelementptr i32, i32* %0, i32 9
  %66 = load i32, i32* %65
  br label %67

; <label>:67:                                     ; preds = %67, %52
  %68 = add i32 %66, 0
  %69 = getelementptr i32, i32* %0, i32 6
  store i32 %68, i32* %69
  %70 = getelementptr i32, i32* %0, i32 8
  %71 = load i32, i32* %70
  %72 = add i32 %71, -4
  %73 = sub i32 %72, 42124
  %74 = sdiv exact i32 %73, 4
  %75 = getelementptr i32, i32* %1, i32 %74
  %76 = load i32, i32* %75
  %77 = getelementptr i32, i32* %0, i32 10
  store i32 %76, i32* %77
  %78 = getelementptr i32, i32* %0, i32 10
  %79 = load i32, i32* %78
  %80 = getelementptr i32, i32* %0, i32 2
  %81 = load i32, i32* %80
  %82 = icmp slt i32 %81, %79
  %83 = sext i1 %82 to i32
  %84 = getelementptr i32, i32* %0, i32 1
  store i32 %83, i32* %84
  %85 = getelementptr i32, i32* %0, i32 8
  %86 = load i32, i32* %85
  %87 = add i32 %86, -4
  %88 = getelementptr i32, i32* %0, i32 8
  store i32 %87, i32* %88
  %89 = getelementptr i32, i32* %0, i32 6
  %90 = load i32, i32* %89
  %91 = add i32 %90, -1
  %92 = getelementptr i32, i32* %0, i32 9
  store i32 %91, i32* %92
  %93 = getelementptr i32, i32* %0, i32 1
  %94 = load i32, i32* %93
  %95 = icmp ne i32 %94, 0
  br i1 %95, label %67, label %101
                                                  ; No predecessors!
  %97 = getelementptr i32, i32* %0, i32 18
  %98 = load i32, i32* %97
  %99 = getelementptr i32, i32* %0, i32 6
  %100 = load i32, i32* %99
  br label %101

; <label>:101:                                    ; preds = %67, %96
  %102 = icmp slt i32 %100, %98
  %103 = sext i1 %102 to i32
  %104 = getelementptr i32, i32* %0, i32 1
  store i32 %103, i32* %104
  %105 = getelementptr i32, i32* %0, i32 1
  %106 = load i32, i32* %105
  %107 = icmp ne i32 %106, 0
  br i1 %107, label %138, label %111
                                                  ; No predecessors!
  %109 = getelementptr i32, i32* %0, i32 3
  %110 = load i32, i32* %109
  br label %111

; <label>:111:                                    ; preds = %101, %108
  %112 = add i32 %110, 0
  %113 = sub i32 %112, 42124
  %114 = sdiv exact i32 %113, 4
  %115 = getelementptr i32, i32* %1, i32 %114
  %116 = getelementptr i32, i32* %0, i32 10
  %117 = load i32, i32* %116
  store i32 %117, i32* %115
  %118 = getelementptr i32, i32* %0, i32 8
  %119 = load i32, i32* %118
  %120 = add i32 %119, 0
  %121 = sub i32 %120, 42124
  %122 = sdiv exact i32 %121, 4
  %123 = getelementptr i32, i32* %1, i32 %122
  %124 = getelementptr i32, i32* %0, i32 7
  %125 = load i32, i32* %124
  store i32 %125, i32* %123
  %126 = getelementptr i32, i32* %0, i32 9
  %127 = load i32, i32* %126
  %128 = add i32 %127, 0
  %129 = getelementptr i32, i32* %0, i32 6
  store i32 %128, i32* %129
  %130 = getelementptr i32, i32* %0, i32 4
  %131 = load i32, i32* %130
  %132 = add i32 %131, 0
  %133 = getelementptr i32, i32* %0, i32 18
  store i32 %132, i32* %133
  %134 = getelementptr i32, i32* %0, i32 18
  %135 = load i32, i32* %134
  %136 = getelementptr i32, i32* %0, i32 6
  %137 = load i32, i32* %136
  br label %138

; <label>:138:                                    ; preds = %101, %111
  %139 = icmp slt i32 %137, %135
  %140 = sext i1 %139 to i32
  %141 = getelementptr i32, i32* %0, i32 1
  store i32 %140, i32* %141
  %142 = getelementptr i32, i32* %0, i32 1
  %143 = load i32, i32* %142
  %144 = icmp eq i32 %143, 0
  br i1 %144, label %5, label %146
                                                  ; No predecessors!
  ret i32 624

; <label>:146:                                    ; preds = %138
  ret i32 624
}

// RUN: %cheri_purecap_cc1 -std=c11 -O2 -emit-llvm -o - %s | %cheri_FileCheck %s -enable-var-scope
// RUN: %cheri_purecap_cc1 -mllvm -cheri-cap-table-abi=pcrel -std=c11 -O2 -S -o - %s | %cheri_FileCheck -D\$CAP_SIZE=32 -check-prefixes=ASM,%cheri_type-ASM %s
int global;

unsigned long sizeof_cap(void) {
  return sizeof(void* __capability);
  // CHECK-LABEL: define i64 @sizeof_cap() local_unnamed_addr
  // CHECK: ret i64 [[$CAP_SIZE]]
  // ASM-LABEL: sizeof_cap
  // ASM: cjr     $c17
  // ASM: daddiu   $2, $zero, [[$CAP_SIZE]]
}

typedef struct {
  __uintcap_t intptr;
} IntptrStruct;

// TODO: we should just return the struct in $c3 instead of using an output parameter
IntptrStruct set_int() {
  IntptrStruct p;
  p.intptr = 0;
  return p;
  // CHECK-LABEL: define inreg { i8 addrspace(200)* } @set_int() local_unnamed_addr
  // CHECK: [[VAR0:%.+]] = tail call i8 addrspace(200)* @llvm.cheri.cap.offset.increment(i8 addrspace(200)* null, i64 0)
  // CHECK: %.fca.0.insert = insertvalue { i8 addrspace(200)* } undef, i8 addrspace(200)* [[VAR0]], 0
  // CHECK: ret { i8 addrspace(200)* } %.fca.0.insert
  // ASM-LABEL: set_int
  // ASM:  cjr     $c17
  // ASM-NEXT:  cincoffset      $c3, $cnull, 0
}

IntptrStruct set_int2(IntptrStruct p) {
  return p;
  // CHECK-LABEL: define inreg { i8 addrspace(200)* } @set_int2(i8 addrspace(200)* inreg %p.coerce) local_unnamed_addr
  // CHECK: %.fca.0.insert = insertvalue { i8 addrspace(200)* } undef, i8 addrspace(200)* %p.coerce, 0
  // CHECK: ret { i8 addrspace(200)* } %.fca.0.insert
  // ASM-LABEL: set_int2
  // ASM:       cjr     $c17
  // ASM-NEXT:  nop
}

__uintcap_t set_int3(IntptrStruct p) {
  return p.intptr;
  // CHECK-LABEL: define i8 addrspace(200)* @set_int3(i8 addrspace(200)* inreg readnone returned %p.coerce) local_unnamed_addr
  // CHECK: ret i8 addrspace(200)* %p.coerce
  // ASM-LABEL: set_int3
  // ASM:       cjr     $c17
  // ASM-NEXT:  nop
}

typedef struct {
  __uintcap_t intptr;
  void* __capability ptr;
} TwoCapsStruct;

TwoCapsStruct two_caps_struct(TwoCapsStruct in) {
  TwoCapsStruct t;
  t.intptr = in.intptr + 1;
  t.ptr = in.ptr;
  return t;
  // argument is split up into two cap regs, but return value is indirect
  // CHECK-LABEL: define void @two_caps_struct(%struct.TwoCapsStruct addrspace(200)* noalias nocapture sret %agg.result, {{.*}}, i8 addrspace(200)* inreg %in.coerce0, i8 addrspace(200)* inreg %in.coerce1) local_unnamed_addr
  // CHECK: [[VAR1:%.+]] = tail call i8 addrspace(200)* @llvm.cheri.cap.offset.increment(i8 addrspace(200)* %in.coerce0, i64 1)
  // CHECK: [[INTPTR_MEMBER:%.+]] = getelementptr inbounds %struct.TwoCapsStruct, %struct.TwoCapsStruct addrspace(200)* %agg.result, i64 0, i32 0
  // CHECK: store i8 addrspace(200)* [[VAR1]], i8 addrspace(200)* addrspace(200)* [[INTPTR_MEMBER]], align [[$CAP_SIZE]]
  // CHECK: [[CAP_MEMBER:%.+]] = getelementptr inbounds %struct.TwoCapsStruct, %struct.TwoCapsStruct addrspace(200)* %agg.result, i64 0, i32 1
  // CHECK: store i8 addrspace(200)* %in.coerce1, i8 addrspace(200)* addrspace(200)* [[CAP_MEMBER]], align [[$CAP_SIZE]]
  // CHECK: ret void
  // ASM-LABEL: two_caps_struct
  // ASM:       cincoffset      $c1, $c4, 1
  // ASM-NEXT:  csc     $c1, $zero, 0($c3)
  // ASM-NEXT:  cjr     $c17
  // ASM-NEXT:  csc     $c5, $zero, [[$CAP_SIZE]]($c3)

}

typedef union {
  __uintcap_t intptr;
  void * __capability ptr;
  long longvalue;
} IntCapSizeUnion;
_Static_assert(sizeof(IntCapSizeUnion) == sizeof(void*), "");

IntCapSizeUnion intcap_size_union() {
  IntCapSizeUnion i;
  i.ptr = &global;
  return i;
  // CHECK-LABEL: define inreg i8 addrspace(200)* @intcap_size_union() local_unnamed_addr
  // CHECK: ret i8 addrspace(200)* bitcast (i32 addrspace(200)* @global to i8 addrspace(200)*)
  // ASM-LABEL: intcap_size_union
  // ASM: clcbi $c3, %captab20(global)($c{{.+}})
}

// Check that a union with size > intcap_t is not returned as a value
typedef union {
  __uintcap_t intptr;
  void* __capability ptr;
  long longvalue;
  char buffer[sizeof(__uintcap_t) + 1];
} GreaterThanIntCapSizeUnion;
_Static_assert(sizeof(GreaterThanIntCapSizeUnion) > sizeof(void*), "");

GreaterThanIntCapSizeUnion greater_than_intcap_size_union() {
  GreaterThanIntCapSizeUnion g;
  g.ptr = &global;
  return g;
  // CHECK-LABEL: define void @greater_than_intcap_size_union(%union.GreaterThanIntCapSizeUnion addrspace(200)* noalias nocapture sret %agg.result) local_unnamed_addr
  // CHECK: [[CAP_MEMBER:%.+]] = getelementptr inbounds %union.GreaterThanIntCapSizeUnion, %union.GreaterThanIntCapSizeUnion addrspace(200)* %agg.result, i64 0, i32 0
  // CHECK: store i8 addrspace(200)* bitcast (i32 addrspace(200)* @global to i8 addrspace(200)*), i8 addrspace(200)* addrspace(200)* [[CAP_MEMBER]], align [[$CAP_SIZE]]
  // CHECK: ret void
  // ASM-LABEL: greater_than_intcap_size_union
  // ASM:       clcbi $c1, %captab20(global)($c{{.+}})
  // ASM-NEXT:  cjr     $c17
  // ASM-NEXT:  csc     $c1, $zero, 0($c3)
}

// Check that we didn't break the normal case of returning small structs in integer registers
typedef struct {
  long l1;
} OneLong;

OneLong one_long() {
  OneLong o = { 1 };
  return o;
  // CHECK-LABEL: define inreg { i64 } @one_long() local_unnamed_addr
  // CHECK: ret { i64 } { i64 1 }
  // ASM-LABEL: one_long
  // ASM:       cjr     $c17
  // ASM-NEXT:  daddiu  $2, $zero, 1
}
typedef struct {
  long l1;
  long l2;
} TwoLongs;

TwoLongs two_longs() {
  TwoLongs t = { 1, 2 };
  return t;
  // CHECK-LABEL: define inreg { i64, i64 } @two_longs() local_unnamed_addr
  // CHECK:   ret { i64, i64 } { i64 1, i64 2 }
  // ASM-LABEL: two_longs
  // ASM:       daddiu  $2, $zero, 1
  // ASM-NEXT:  cjr     $c17
  // ASM-NEXT:  daddiu  $3, $zero, 2
}

typedef struct {
  long l1;
  long l2;
  long l3;
} ThreeLongs;

ThreeLongs three_longs() {
  ThreeLongs t = { 1, 2, 3 };
  return t;
  // CHECK-LABEL: define void @three_longs(%struct.ThreeLongs addrspace(200)* noalias nocapture sret %agg.result) local_unnamed_addr
  // ASM-LABEL: three_longs
  // Clang now uses a memcpy from a global for cheri128
  // CHERI128-ASM: clcbi $c4, %captab20(.L__const.three_longs.t)($c{{.+}})
  // CHERI128-ASM: clcbi   $c12, %capcall20(memcpy)($c{{.+}})
  // For cheri256 clang will inline the memcpy from a global (since it is smaller than 1 cap)
  // CHERI256-ASM:      clcbi $c1, %captab20(.L__const.three_longs.t)($c{{.+}})
  // CHERI256-ASM-NEXT: cld	$1, $zero, 16($c1)
  // CHERI256-ASM-NEXT: cld	$2, $zero, 8($c1)
  // CHERI256-ASM-NEXT: cld	$3, $zero, 0($c1)
  // CHERI256-ASM-NEXT: csd	$1, $zero, 16($c3)
  // CHERI256-ASM-NEXT: csd	$2, $zero, 8($c3)
  // CHERI256-ASM-NEXT: cjr	$c17
  // CHERI256-ASM-NEXT: csd	$3, $zero, 0($c3)
}

typedef struct {
  int l1;
  long l2;
} IntAndLong;

IntAndLong int_and_long() {
  // FIXME: this seems very wrong, we are returning registers so it should not assume this is an in-memory big-endian representation
  // CHECK-LABEL: define inreg { i64, i64 } @int_and_long() local_unnamed_addr
  // TODO-CHECK-NOT: ret { i64, i64 } { i64 8589934592, i64 3 }
  // TODO-CHECK:     ret { i32, i64 } { i32 2, i64 3 }
  // ASM-LABEL: int_and_long
  // TODO-ASM-NOT: daddiu  $1, $zero, 1
  // TODO-ASM-NOT: dsll    $2, $1, 33
  // TODO-ASM: daddiu  $2, $zero, 3
  // ASM:            cjr     $c17
  // ASM-NEXT:       daddiu  $3, $zero, 3
  IntAndLong t = { 2, 3 };
  return t;
}

IntAndLong int_and_long2(IntAndLong arg) {
  // CHECK-LABEL: define inreg { i64, i64 } @int_and_long2(i64 inreg %arg.coerce0, i64 inreg %arg.coerce1) local_unnamed_addr
  // TODO-ASM: daddiu  $2, $zero, 3
  // ASM-LABEL: int_and_long2
  // ASM:     move     $2, $4
  // ASM-NEXT: cjr     $c17
  // ASM-NEXT: move     $3, $5
  return arg;
}

#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include <windows.h>

extern int nWinVersion;
extern WORD nExplorerQFE;

#define WIN_VERSION_UNSUPPORTED    (-1)
#define WIN_VERSION_7              0
#define WIN_VERSION_8              1
#define WIN_VERSION_81             2
#define WIN_VERSION_811            3
#define WIN_VERSION_10_T1          4  // 1507 (Initial Release)
#define WIN_VERSION_10_T2          5  // 1511 (November Update)
#define WIN_VERSION_10_R1          6  // 1607 (Anniversary Update)
#define WIN_VERSION_10_R2          7  // 1703 (Creators Update)
#define WIN_VERSION_10_R3          8  // 1709 (Fall Creators Update)
#define WIN_VERSION_10_R4          9  // 1803 (April 2018 Update)
#define WIN_VERSION_10_R5          10 // 1809 (October 2018 Update)
#define WIN_VERSION_10_19H1        11 // 1903 (May 2019 Update) and 1909 (November 2019 Update)

// helper macros
#define FIRST_NONEMPTY_ARG_2(a, b) \
                                   ( (sizeof(#a) > sizeof("")) ? (a+0) : (b) )
#define FIRST_NONEMPTY_ARG_3(a, b, c) \
                                   ( (sizeof(#a) > sizeof("")) ? (a+0) : \
                                   ( (sizeof(#b) > sizeof("")) ? (b+0) : (c) ))
#define FIRST_NONEMPTY_ARG_4(a, b, c, d) \
                                   ( (sizeof(#a) > sizeof("")) ? (a+0) : \
                                   ( (sizeof(#b) > sizeof("")) ? (b+0) : \
                                   ( (sizeof(#c) > sizeof("")) ? (c+0) : (d) )))
#define FIRST_NONEMPTY_ARG_5(a, b, c, d, e) \
                                   ( (sizeof(#a) > sizeof("")) ? (a+0) : \
                                   ( (sizeof(#b) > sizeof("")) ? (b+0) : \
                                   ( (sizeof(#c) > sizeof("")) ? (c+0) : \
                                   ( (sizeof(#d) > sizeof("")) ? (d+0) : (e) ))))
#define FIRST_NONEMPTY_ARG_6(a, b, c, d, e, f) \
                                   ( (sizeof(#a) > sizeof("")) ? (a+0) : \
                                   ( (sizeof(#b) > sizeof("")) ? (b+0) : \
                                   ( (sizeof(#c) > sizeof("")) ? (c+0) : \
                                   ( (sizeof(#d) > sizeof("")) ? (d+0) : \
                                   ( (sizeof(#e) > sizeof("")) ? (e+0) : (f) )))))
#define FIRST_NONEMPTY_ARG_7(a, b, c, d, e, f, g) \
                                   ( (sizeof(#a) > sizeof("")) ? (a+0) : \
                                   ( (sizeof(#b) > sizeof("")) ? (b+0) : \
                                   ( (sizeof(#c) > sizeof("")) ? (c+0) : \
                                   ( (sizeof(#d) > sizeof("")) ? (d+0) : \
                                   ( (sizeof(#e) > sizeof("")) ? (e+0) : \
                                   ( (sizeof(#f) > sizeof("")) ? (f+0) : (g) ))))))
#define FIRST_NONEMPTY_ARG_8(a, b, c, d, e, f, g, h) \
                                   ( (sizeof(#a) > sizeof("")) ? (a+0) : \
                                   ( (sizeof(#b) > sizeof("")) ? (b+0) : \
                                   ( (sizeof(#c) > sizeof("")) ? (c+0) : \
                                   ( (sizeof(#d) > sizeof("")) ? (d+0) : \
                                   ( (sizeof(#e) > sizeof("")) ? (e+0) : \
                                   ( (sizeof(#f) > sizeof("")) ? (f+0) : \
                                   ( (sizeof(#g) > sizeof("")) ? (g+0) : (h) )))))))
#define FIRST_NONEMPTY_ARG_9(a, b, c, d, e, f, g, h, i) \
                                   ( (sizeof(#a) > sizeof("")) ? (a+0) : \
                                   ( (sizeof(#b) > sizeof("")) ? (b+0) : \
                                   ( (sizeof(#c) > sizeof("")) ? (c+0) : \
                                   ( (sizeof(#d) > sizeof("")) ? (d+0) : \
                                   ( (sizeof(#e) > sizeof("")) ? (e+0) : \
                                   ( (sizeof(#f) > sizeof("")) ? (f+0) : \
                                   ( (sizeof(#g) > sizeof("")) ? (g+0) : \
                                   ( (sizeof(#h) > sizeof("")) ? (h+0) : (i) ))))))))
#define FIRST_NONEMPTY_ARG_10(a, b, c, d, e, f, g, h, i, j) \
                                   ( (sizeof(#a) > sizeof("")) ? (a+0) : \
                                   ( (sizeof(#b) > sizeof("")) ? (b+0) : \
                                   ( (sizeof(#c) > sizeof("")) ? (c+0) : \
                                   ( (sizeof(#d) > sizeof("")) ? (d+0) : \
                                   ( (sizeof(#e) > sizeof("")) ? (e+0) : \
                                   ( (sizeof(#f) > sizeof("")) ? (f+0) : \
                                   ( (sizeof(#g) > sizeof("")) ? (g+0) : \
                                   ( (sizeof(#h) > sizeof("")) ? (h+0) : \
                                   ( (sizeof(#i) > sizeof("")) ? (i+0) : (j) )))))))))
#define FIRST_NONEMPTY_ARG_11(a, b, c, d, e, f, g, h, i, j, k) \
                                   ( (sizeof(#a) > sizeof("")) ? (a+0) : \
                                   ( (sizeof(#b) > sizeof("")) ? (b+0) : \
                                   ( (sizeof(#c) > sizeof("")) ? (c+0) : \
                                   ( (sizeof(#d) > sizeof("")) ? (d+0) : \
                                   ( (sizeof(#e) > sizeof("")) ? (e+0) : \
                                   ( (sizeof(#f) > sizeof("")) ? (f+0) : \
                                   ( (sizeof(#g) > sizeof("")) ? (g+0) : \
                                   ( (sizeof(#h) > sizeof("")) ? (h+0) : \
                                   ( (sizeof(#i) > sizeof("")) ? (i+0) : \
                                   ( (sizeof(#j) > sizeof("")) ? (j+0) : (k) ))))))))))

#define DO2(d7, dx)                ( (nWinVersion == WIN_VERSION_7) ? (d7) : (dx) )
#define DO3(d7, d8, dx)            ( (nWinVersion == WIN_VERSION_7) ? (d7) : \
                                   ( (nWinVersion == WIN_VERSION_8) ? FIRST_NONEMPTY_ARG_2(d8, d7) : (dx) ))
#define DO4(d7, d8, d81, dx)       ( (nWinVersion == WIN_VERSION_7) ? (d7) : \
                                   ( (nWinVersion == WIN_VERSION_8) ? FIRST_NONEMPTY_ARG_2(d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_81) ? FIRST_NONEMPTY_ARG_3(d81, d8, d7) : (dx) )))
#define DO5(d7, d8, d81, d811, dx) ( (nWinVersion == WIN_VERSION_7) ? (d7) : \
                                   ( (nWinVersion == WIN_VERSION_8) ? FIRST_NONEMPTY_ARG_2(d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_81) ? FIRST_NONEMPTY_ARG_3(d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_811) ? FIRST_NONEMPTY_ARG_4(d811, d81, d8, d7) : (dx) ))))
#define DO6(d7, d8, d81, d811, d10_t1, dx) \
                                   ( (nWinVersion == WIN_VERSION_7) ? (d7) : \
                                   ( (nWinVersion == WIN_VERSION_8) ? FIRST_NONEMPTY_ARG_2(d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_81) ? FIRST_NONEMPTY_ARG_3(d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_811) ? FIRST_NONEMPTY_ARG_4(d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_T1) ? FIRST_NONEMPTY_ARG_5(d10_t1, d811, d81, d8, d7) : (dx) )))))
#define DO7(d7, d8, d81, d811, d10_t1, d10_t2, dx) \
                                   ( (nWinVersion == WIN_VERSION_7) ? (d7) : \
                                   ( (nWinVersion == WIN_VERSION_8) ? FIRST_NONEMPTY_ARG_2(d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_81) ? FIRST_NONEMPTY_ARG_3(d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_811) ? FIRST_NONEMPTY_ARG_4(d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_T1) ? FIRST_NONEMPTY_ARG_5(d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_T2) ? FIRST_NONEMPTY_ARG_6(d10_t2, d10_t1, d811, d81, d8, d7) : (dx) ))))))
#define DO8(d7, d8, d81, d811, d10_t1, d10_t2, d10_r1, dx) \
                                   ( (nWinVersion == WIN_VERSION_7) ? (d7) : \
                                   ( (nWinVersion == WIN_VERSION_8) ? FIRST_NONEMPTY_ARG_2(d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_81) ? FIRST_NONEMPTY_ARG_3(d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_811) ? FIRST_NONEMPTY_ARG_4(d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_T1) ? FIRST_NONEMPTY_ARG_5(d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_T2) ? FIRST_NONEMPTY_ARG_6(d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R1) ? FIRST_NONEMPTY_ARG_7(d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : (dx) )))))))
#define DO9(d7, d8, d81, d811, d10_t1, d10_t2, d10_r1, d10_r2, dx) \
                                   ( (nWinVersion == WIN_VERSION_7) ? (d7) : \
                                   ( (nWinVersion == WIN_VERSION_8) ? FIRST_NONEMPTY_ARG_2(d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_81) ? FIRST_NONEMPTY_ARG_3(d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_811) ? FIRST_NONEMPTY_ARG_4(d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_T1) ? FIRST_NONEMPTY_ARG_5(d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_T2) ? FIRST_NONEMPTY_ARG_6(d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R1) ? FIRST_NONEMPTY_ARG_7(d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R2) ? FIRST_NONEMPTY_ARG_8(d10_r2, d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : (dx) ))))))))
#define DO10(d7, d8, d81, d811, d10_t1, d10_t2, d10_r1, d10_r2, d10_r3, dx) \
                                   ( (nWinVersion == WIN_VERSION_7) ? (d7) : \
                                   ( (nWinVersion == WIN_VERSION_8) ? FIRST_NONEMPTY_ARG_2(d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_81) ? FIRST_NONEMPTY_ARG_3(d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_811) ? FIRST_NONEMPTY_ARG_4(d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_T1) ? FIRST_NONEMPTY_ARG_5(d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_T2) ? FIRST_NONEMPTY_ARG_6(d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R1) ? FIRST_NONEMPTY_ARG_7(d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R2) ? FIRST_NONEMPTY_ARG_8(d10_r2, d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R3) ? FIRST_NONEMPTY_ARG_9(d10_r3, d10_r2, d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : (dx) )))))))))
#define DO11(d7, d8, d81, d811, d10_t1, d10_t2, d10_r1, d10_r2, d10_r3, d10_r4, dx) \
                                   ( (nWinVersion == WIN_VERSION_7) ? (d7) : \
                                   ( (nWinVersion == WIN_VERSION_8) ? FIRST_NONEMPTY_ARG_2(d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_81) ? FIRST_NONEMPTY_ARG_3(d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_811) ? FIRST_NONEMPTY_ARG_4(d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_T1) ? FIRST_NONEMPTY_ARG_5(d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_T2) ? FIRST_NONEMPTY_ARG_6(d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R1) ? FIRST_NONEMPTY_ARG_7(d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R2) ? FIRST_NONEMPTY_ARG_8(d10_r2, d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R3) ? FIRST_NONEMPTY_ARG_9(d10_r3, d10_r2, d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R4) ? FIRST_NONEMPTY_ARG_10(d10_r4, d10_r3, d10_r2, d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : (dx) ))))))))))
#define DO12(d7, d8, d81, d811, d10_t1, d10_t2, d10_r1, d10_r2, d10_r3, d10_r4, d10_r5, dx) \
                                   ( (nWinVersion == WIN_VERSION_7) ? (d7) : \
                                   ( (nWinVersion == WIN_VERSION_8) ? FIRST_NONEMPTY_ARG_2(d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_81) ? FIRST_NONEMPTY_ARG_3(d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_811) ? FIRST_NONEMPTY_ARG_4(d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_T1) ? FIRST_NONEMPTY_ARG_5(d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_T2) ? FIRST_NONEMPTY_ARG_6(d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R1) ? FIRST_NONEMPTY_ARG_7(d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R2) ? FIRST_NONEMPTY_ARG_8(d10_r2, d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R3) ? FIRST_NONEMPTY_ARG_9(d10_r3, d10_r2, d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R4) ? FIRST_NONEMPTY_ARG_10(d10_r4, d10_r3, d10_r2, d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : \
                                   ( (nWinVersion == WIN_VERSION_10_R5) ? FIRST_NONEMPTY_ARG_11(d10_r5, d10_r4, d10_r3, d10_r2, d10_r1, d10_t2, d10_t1, d811, d81, d8, d7) : (dx) )))))))))))

#ifdef _WIN64
#define DEF3264(d32, d64)          (d64)
#else
#define DEF3264(d32, d64)          (d32)
#endif

#define DO2_3264(d7_32, d7_64, dx_32, dx_64) \
                                   DEF3264(DO2(d7_32, dx_32), DO2(d7_64, dx_64))

#define DO3_3264(d7_32, d7_64, d8_32, d8_64, dx_32, dx_64) \
                                   DEF3264(DO3(d7_32, d8_32, dx_32), DO3(d7_64, d8_64, dx_64))

#define DO4_3264(d7_32, d7_64, d8_32, d8_64, d81_32, d81_64, dx_32, dx_64) \
                                   DEF3264(DO4(d7_32, d8_32, d81_32, dx_32), DO4(d7_64, d8_64, d81_64, dx_64))

#define DO5_3264(d7_32, d7_64, d8_32, d8_64, d81_32, d81_64, d811_32, d811_64, dx_32, dx_64) \
                                   DEF3264(DO5(d7_32, d8_32, d81_32, d811_32, dx_32), DO5(d7_64, d8_64, d81_64, d811_64, dx_64))

#define DO6_3264(d7_32, d7_64, d8_32, d8_64, d81_32, d81_64, d811_32, d811_64, d10_t1_32, d10_t1_64, dx_32, dx_64) \
                                   DEF3264(DO6(d7_32, d8_32, d81_32, d811_32, d10_t1_32, dx_32), DO6(d7_64, d8_64, d81_64, d811_64, d10_t1_64, dx_64))

#define DO7_3264(d7_32, d7_64, d8_32, d8_64, d81_32, d81_64, d811_32, d811_64, d10_t1_32, d10_t1_64, d10_t2_32, d10_t2_64, dx_32, dx_64) \
                                   DEF3264(DO7(d7_32, d8_32, d81_32, d811_32, d10_t1_32, d10_t2_32, dx_32), DO7(d7_64, d8_64, d81_64, d811_64, d10_t1_64, d10_t2_64, dx_64))

#define DO8_3264(d7_32, d7_64, d8_32, d8_64, d81_32, d81_64, d811_32, d811_64, d10_t1_32, d10_t1_64, d10_t2_32, d10_t2_64, d10_r1_32, d10_r1_64, dx_32, dx_64) \
                                   DEF3264(DO8(d7_32, d8_32, d81_32, d811_32, d10_t1_32, d10_t2_32, d10_r1_32, dx_32), DO8(d7_64, d8_64, d81_64, d811_64, d10_t1_64, d10_t2_64, d10_r1_64, dx_64))

#define DO9_3264(d7_32, d7_64, d8_32, d8_64, d81_32, d81_64, d811_32, d811_64, d10_t1_32, d10_t1_64, d10_t2_32, d10_t2_64, d10_r1_32, d10_r1_64, d10_r2_32, d10_r2_64, dx_32, dx_64) \
                                   DEF3264(DO9(d7_32, d8_32, d81_32, d811_32, d10_t1_32, d10_t2_32, d10_r1_32, d10_r2_32, dx_32), DO9(d7_64, d8_64, d81_64, d811_64, d10_t1_64, d10_t2_64, d10_r1_64, d10_r2_64, dx_64))

#define DO10_3264(d7_32, d7_64, d8_32, d8_64, d81_32, d81_64, d811_32, d811_64, d10_t1_32, d10_t1_64, d10_t2_32, d10_t2_64, d10_r1_32, d10_r1_64, d10_r2_32, d10_r2_64, d10_r3_32, d10_r3_64, dx_32, dx_64) \
                                   DEF3264(DO10(d7_32, d8_32, d81_32, d811_32, d10_t1_32, d10_t2_32, d10_r1_32, d10_r2_32, d10_r3_32, dx_32), DO10(d7_64, d8_64, d81_64, d811_64, d10_t1_64, d10_t2_64, d10_r1_64, d10_r2_64, d10_r3_64, dx_64))

#define DO11_3264(d7_32, d7_64, d8_32, d8_64, d81_32, d81_64, d811_32, d811_64, d10_t1_32, d10_t1_64, d10_t2_32, d10_t2_64, d10_r1_32, d10_r1_64, d10_r2_32, d10_r2_64, d10_r3_32, d10_r3_64, d10_r4_32, d10_r4_64, dx_32, dx_64) \
                                   DEF3264(DO11(d7_32, d8_32, d81_32, d811_32, d10_t1_32, d10_t2_32, d10_r1_32, d10_r2_32, d10_r3_32, d10_r4_32, dx_32), DO11(d7_64, d8_64, d81_64, d811_64, d10_t1_64, d10_t2_64, d10_r1_64, d10_r2_64, d10_r3_64, d10_r4_64, dx_64))

#define DO12_3264(d7_32, d7_64, d8_32, d8_64, d81_32, d81_64, d811_32, d811_64, d10_t1_32, d10_t1_64, d10_t2_32, d10_t2_64, d10_r1_32, d10_r1_64, d10_r2_32, d10_r2_64, d10_r3_32, d10_r3_64, d10_r4_32, d10_r4_64, d10_r5_32, d10_r5_64, dx_32, dx_64) \
                                   DEF3264(DO12(d7_32, d8_32, d81_32, d811_32, d10_t1_32, d10_t2_32, d10_r1_32, d10_r2_32, d10_r3_32, d10_r4_32, d10_r5_32, dx_32), DO12(d7_64, d8_64, d81_64, d811_64, d10_t1_64, d10_t2_64, d10_r1_64, d10_r2_64, d10_r3_64, d10_r4_64, d10_r5_64, dx_64))

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam)       ((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam)       ((int)(short)HIWORD(lParam))
#endif

// This macros allow you to declare a __thiscall function in C
// The hack is done in x86 and is not required in x64
#ifdef _WIN64
#define THISCALL_C
#else
#define THISCALL_C				        __fastcall
#endif

#ifdef _WIN64
#define THISCALL_C_THIS_ARG(this_arg)   this_arg
#else
#define THISCALL_C_THIS_ARG(this_arg)   this_arg, LONG_PTR _edx_var
#endif

#ifdef _WIN64
#define THISCALL_C_THIS_TYPE(this_type) this_type
#else
#define THISCALL_C_THIS_TYPE(this_type) this_type, LONG_PTR
#endif

#ifdef _WIN64
#define THISCALL_C_THIS_VAL(this_val)   this_val
#else
#define THISCALL_C_THIS_VAL(this_val)   this_val, _edx_var
#endif

// Structs
typedef struct _secondary_task_list_get {
	int count;
	LONG_PTR *dpa_ptr;
} SECONDARY_TASK_LIST_GET;

// General functions
VS_FIXEDFILEINFO *GetModuleVersionInfo(HMODULE hModule, UINT *puPtrLen);
void **FindImportPtr(HMODULE hFindInModule, char *pModuleName, char *pImportName);
void PatchPtr(void **ppAddress, void *pPtr);
void PatchMemory(void *pDest, void *pSrc, size_t nSize);
BOOL StringBeginsWith(WCHAR *pString, WCHAR *pBeginStr);

// Taskbar functions
DWORD TaskbarGetPreference(LONG_PTR lpMMTaskListLongPtr);
LONG_PTR SecondaryTaskListGetFirstLongPtr(SECONDARY_TASK_LIST_GET *p_secondary_task_list_get);
LONG_PTR SecondaryTaskListGetNextLongPtr(SECONDARY_TASK_LIST_GET *p_secondary_task_list_get);

#endif // _FUNCTIONS_H_

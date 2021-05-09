#pragma once

#ifdef __cplusplus
#define TYPEDEF
#define OC(...)
#define unstruct(X)
#else
#define TYPEDEF typedef
#define OC(...) __VA_ARGS__
#define unstruct(X) typedef struct X X
#define unenum(X) typedef enum X X
#endif

#define BAD UINT32_MAX
#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)
#define arraysize(X) (sizeof(X)/sizeof(*X))


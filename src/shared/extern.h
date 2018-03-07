#ifdef DLL_EXPORT
#   define FB_EXTERN __declspec(dllexport)
#else
#   define FB_EXTERN __declspec(dllimport)
#endif

#ifdef DLL_EXPORT
#   define FB_EXPORT __declspec(dllexport)
#else
#   define FB_EXPORT 
#endif

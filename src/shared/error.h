
enum FbErrorCode {
    FB_ERR_NONE = 0,
    FB_ERR_FILE_NOT_FOUND,
    FB_ERR_GL_UNSUPPORTED,
    FB_ERR_REMOTERY_INIT,
};

extern char ERROR_buffer[256];

enum FbErrorCode ERROR_fmt_(enum FbErrorCode err, const char *fmt, ...);

#define ERROR_fmt(err, fmt, ...) ERROR_fmt_(err, "0x%04x %s(%d): " fmt, err, __FILE__, __LINE__, __VA_ARGS__)

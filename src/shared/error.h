
enum FbErrorCode {
    FB_ERR_NONE = 0,
    FB_ERR_FILE_NOT_FOUND,
    FB_ERR_FILE_CREATE,
    FB_ERR_GL_UNSUPPORTED,
    FB_ERR_REMOTERY_INIT,
    FB_ERR_PARSE_TOML,
};

extern char ERR_buffer[256];

enum FbErrorCode ERR_fmt_(enum FbErrorCode err, const char *fmt, ...);

#define ERR_fmt(err, fmt, ...) ERR_fmt_(err, "0x%04x %s(%d): " fmt, err, __FILE__, __LINE__, __VA_ARGS__)

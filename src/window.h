struct FbWindowConfig {
    char *title;
    unsigned width, height;

};

struct FbWindow;
struct GLFWwindow;
typedef void (* GLFWkeyfun)(struct GLFWwindow*,int,int,int,int);

extern unsigned window_new(struct FbWindowConfig cfg, struct FbWindow **wnd);
extern unsigned window_destroy(struct FbWindow *wnd);
extern void     window_set_key_callback(struct FbWindow *wnd, GLFWkeyfun cb);
extern void     window_cxt(struct FbWindow *wnd);
extern void     window_swap(struct FbWindow *wnd);
extern int      window_open(struct FbWindow *wnd);
extern void     window_poll(struct FbWindow *wnd);

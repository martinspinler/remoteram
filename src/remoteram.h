#ifdef __cplusplus
extern "C" {
#endif

struct __remoteram;
typedef struct __remoteram *remoteram_t;

remoteram_t remoteram_create_server(const char *addr, int verbosity);
void remoteram_destroy_server(remoteram_t rr);

#ifdef __cplusplus
} // extern "C"
#endif


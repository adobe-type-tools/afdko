extern "C" int c_main(int argc, char *argv[]);

#ifdef PTHREAD_STATIC_WORKAROUND
#include <pthread.h>
#endif

int main(int argc, char *argv[]) {
#ifdef PTHREAD_STATIC_WORKAROUND
    static_cast<void>(pthread_create);
    static_cast<void>(pthread_cancel);
#endif
    c_main(argc, argv);
}

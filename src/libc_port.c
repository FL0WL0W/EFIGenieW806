#include "libc_port.h"

int fileno(FILE *stream) {
    if (stream == NULL) {
        return -1;
    }
    
    // Check if the stream is stdout, stdin, or stderr and return appropriate file descriptor
    if (stream == stdout) {
        return STDOUT_FILENO;
    } else if (stream == stdin) {
        return STDIN_FILENO;
    } else if (stream == stderr) {
        return STDERR_FILENO;
    }
}

int fputc(int c, FILE *stream) {
    return write(fileno(stream), &c, 1) == 1 ? c : EOF;
}
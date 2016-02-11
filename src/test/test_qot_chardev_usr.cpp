#include <iostream>
#include <gtest/gtest.h>

extern "C" {
    #include <unistd.h>
    #include <fcntl.h>
    #include <stdio.h>
    #include <string.h>
    #include <stdint.h>
    #include <time.h>
    #include <sys/ioctl.h>
    #include <sys/poll.h>
    #include "../qot_types.h"
}

TEST(QotChardevUsr, CanOpen) {
    int fd = open("/dev/qotusr", O_RDWR);
    EXPECT_GT(fd,0);
    close(fd);
}

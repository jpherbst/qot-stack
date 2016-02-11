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

TEST(QotChardevAdm, CanOpen) {
    int fd = open("/dev/qotadm", O_RDWR);
    EXPECT_GT(fd,0);
    close(fd);
}

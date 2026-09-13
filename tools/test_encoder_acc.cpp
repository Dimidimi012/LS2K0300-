// test_encoder_acc.cpp —— 编码器累计计数标定工具
// 用途：推车走固定距离，读累计脉冲，反推 PULSE_PER_REV
// 编译（虚拟机）:
//   cd ~/LS2K0300_SmartCar
//   /opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin/loongarch64-linux-gnu-g++ -static tools/test_encoder_acc.cpp -o out/test_encoder_acc
// 上传（虚拟机）:
//   scp -O out/test_encoder_acc root@192.168.3.94:/home/root/
// 运行（板子）:
//   /home/root/test_encoder_acc
//   看到 "ready" 后，推车沿直线走 1 米，走完按 Ctrl+C，程序打印累计计数
#include <cstdio>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <csignal>

static volatile bool stop = false;
static void on_sig(int) { stop = true; }

int main(void)
{
    int q1 = open("/dev/zf_encoder_quad_1", O_RDWR);
    int q2 = open("/dev/zf_encoder_quad_2", O_RDWR);
    printf("open quad1=%d quad2=%d errno=%s\n", q1, q2, strerror(errno));
    if (q1 < 0 || q2 < 0) return 1;

    int16_t zero = 0;
    write(q1, &zero, 2);
    write(q2, &zero, 2);

    signal(SIGINT, on_sig);

    int16_t prev1 = 0, prev2 = 0;
    int32_t acc1 = 0, acc2 = 0;
    printf("ready: 现在推车沿直线走 1 米，走完按 Ctrl+C\n");

    while (!stop)
    {
        int16_t c1 = 0, c2 = 0;
        ssize_t r1 = read(q1, &c1, 2);
        ssize_t r2 = read(q2, &c2, 2);
        // int16 差值回绕安全：只要每 100ms 增量 < 32767 就准确
        if (r1 == 2) { acc1 += (int32_t)(int16_t)(c1 - prev1); prev1 = c1; }
        if (r2 == 2) { acc2 += (int32_t)(int16_t)(c2 - prev2); prev2 = c2; }
        printf("acc_quad1=%8d acc_quad2=%8d\r", (int)acc1, (int)acc2);
        fflush(stdout);
        usleep(100000);
    }

    printf("\nFINAL acc_quad1=%d acc_quad2=%d\n", (int)acc1, (int)acc2);
    printf("每转计数 = acc / (距离/0.1957/2.267)\n");
    return 0;
}

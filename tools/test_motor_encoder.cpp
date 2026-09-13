// test_motor_encoder.cpp —— 电机口/编码器口 联动检测
// 目的：一次性测清当前插线状态：
//   - MOTOR1 / MOTOR2 口各驱动哪个物理轮子
//   - DIR=1/0 时轮子转向（前/后）
//   - 每个轮子对应的编码器（quad1/quad2）及读数符号
// 用法：车架空（四轮离地），运行后依次自动执行 4 个阶段，每阶段 2 秒，
//       肉眼观察并记录：哪个轮子转？往车头方向还是反方向转？
// 编译（虚拟机）:
//   cd ~/LS2K0300_SmartCar
//   /opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin/loongarch64-linux-gnu-g++ -static tools/test_motor_encoder.cpp -o out/test_motor_encoder
// 上传（虚拟机）:
//   scp -O out/test_motor_encoder root@192.168.3.94:/home/root/
// 运行（板子）:  /home/root/test_motor_encoder
#include <cstdio>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <csignal>

static volatile bool stop = false;
static void on_sig(int) { stop = true; }

static int fd_open(const char *p)
{
    int fd = open(p, O_RDWR);
    printf("  open %-28s = %d %s\n", p, fd, (fd >= 0) ? "ok" : "FAIL");
    return fd;
}

int main(void)
{
    printf("=== 电机/编码器联动检测 ===\n");
    int q1 = fd_open("/dev/zf_encoder_quad_1");
    int q2 = fd_open("/dev/zf_encoder_quad_2");
    int p1 = fd_open("/dev/zf_pwm_motor_1");
    int p2 = fd_open("/dev/zf_pwm_motor_2");
    int g1 = fd_open("/dev/zf_gpio_motor_1");
    int g2 = fd_open("/dev/zf_gpio_motor_2");
    signal(SIGINT, on_sig);

    int16_t z = 0;
    if (q1 >= 0) write(q1, &z, 2);
    if (q2 >= 0) write(q2, &z, 2);

    // 单阶段测试：设置某口 DIR + 占空比，跑 ms 毫秒，累计两个编码器
    for (int phase = 1; phase <= 4 && !stop; phase++)
    {
        int pwm = (phase <= 2) ? p1 : p2;
        int gpio = (phase <= 2) ? g1 : g2;
        int dir = (phase % 2 == 1) ? 1 : 0;      // 奇：DIR=1；偶：DIR=0
        uint16_t duty = 2500;                      // 25%（duty_max=10000）
        const char *who = (phase <= 2) ? "MOTOR1" : "MOTOR2";

        if (pwm >= 0) { uint16_t d = 0; write(pwm, &d, 2); }
        usleep(300000);
        if (q1 >= 0) write(q1, &z, 2);
        if (q2 >= 0) write(q2, &z, 2);

        printf("\n== [%d/4] %s DIR=%d duty=25%% (2s) -> 观察哪个轮子转、往哪转 ==\n",
               phase, who, dir);
        char lv = (char)(dir ? '1' : '0');
        if (gpio >= 0) write(gpio, &lv, 1);
        if (pwm >= 0) write(pwm, &duty, 2);

        int32_t a1 = 0, a2 = 0;
        int16_t pv1 = 0, pv2 = 0;
        for (int i = 0; i < 20; i++)
        {
            int16_t c1 = 0, c2 = 0;
            if (q1 >= 0 && read(q1, &c1, 2) == 2) { a1 += (int32_t)(int16_t)(c1 - pv1); pv1 = c1; }
            if (q2 >= 0 && read(q2, &c2, 2) == 2) { a2 += (int32_t)(int16_t)(c2 - pv2); pv2 = c2; }
            printf("  t=%3d quad1_acc=%8d quad2_acc=%8d\r", i, (int)a1, (int)a2);
            fflush(stdout);
            usleep(100000);
            if (stop) break;
        }
        if (pwm >= 0) { uint16_t d = 0; write(pwm, &d, 2); }
        printf("\n  RESULT %s DIR=%d : quad1_acc=%d quad2_acc=%d\n", who, dir, (int)a1, (int)a2);
    }

    if (p1 >= 0) { uint16_t d = 0; write(p1, &d, 2); }
    if (p2 >= 0) { uint16_t d = 0; write(p2, &d, 2); }
    printf("\ndone\n");
    return 0;
}

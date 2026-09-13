// test_encoder.cpp —— 编码器最小自检程序 v2
// 同时读取 quad(正交) 与 dir(方向) 两组编码器设备，定位实际有信号的通道
// 编译（虚拟机）:
//   cd ~/LS2K0300_SmartCar
//   /opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin/loongarch64-linux-gnu-g++ -static tools/test_encoder.cpp -o out/test_encoder
// 上传（虚拟机）:
//   scp -O out/test_encoder root@192.168.3.94:/home/root/
// 运行（板子）:  /home/root/test_encoder   （运行中用手转动后轮，看哪组数值变化）
#include <cstdio>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

int main(void)
{
    int q1 = open("/dev/zf_encoder_quad_1", O_RDWR);
    int q2 = open("/dev/zf_encoder_quad_2", O_RDWR);
    int d1 = open("/dev/zf_encoder_dir_1",  O_RDWR);
    int d2 = open("/dev/zf_encoder_dir_2",  O_RDWR);
    printf("open quad1=%d quad2=%d dir1=%d dir2=%d errno=%s\n", q1, q2, d1, d2, strerror(errno));

    int16_t zero = 0;
    if (q1 >= 0) write(q1, &zero, 2);
    if (q2 >= 0) write(q2, &zero, 2);
    if (d1 >= 0) write(d1, &zero, 2);
    if (d2 >= 0) write(d2, &zero, 2);

    for (int i = 0; i < 100; i++)
    {
        int16_t cq1 = 0, cq2 = 0, cd1 = 0, cd2 = 0;
        ssize_t rq1 = (q1 >= 0) ? read(q1, &cq1, 2) : -1;
        ssize_t rq2 = (q2 >= 0) ? read(q2, &cq2, 2) : -1;
        ssize_t rd1 = (d1 >= 0) ? read(d1, &cd1, 2) : -1;
        ssize_t rd2 = (d2 >= 0) ? read(d2, &cd2, 2) : -1;
        printf("t=%3d quad1=%6d(%zd) quad2=%6d(%zd) dir1=%6d(%zd) dir2=%6d(%zd)\n",
               i, (int)cq1, rq1, (int)cq2, rq2, (int)cd1, rd1, (int)cd2, rd2);

        if (q1 >= 0) write(q1, &zero, 2);
        if (q2 >= 0) write(q2, &zero, 2);
        if (d1 >= 0) write(d1, &zero, 2);
        if (d2 >= 0) write(d2, &zero, 2);
        usleep(100000);
    }

    if (q1 >= 0) close(q1);
    if (q2 >= 0) close(q2);
    if (d1 >= 0) close(d1);
    if (d2 >= 0) close(d2);
    printf("done\n");
    return 0;
}

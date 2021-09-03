#include "render_task.h"
#include <iostream>

render_task::render_task(uint64_t version,
                         QAtomicInteger<uint64_t> *version_ptr,
                         QMutex *mutex,
                         uchar *data,
                         int stride,
                         int x1,
                         int y1,
                         int x2,
                         int y2,
                         int w,
                         int h,
                         int x_b,
                         int y_b,
                         double scale,
                         std::complex<double> focus)
    : version(version), version_ptr(version_ptr), mutex(mutex), data(data), stride(stride), x1(x1),
      y1(y1), x2(x2), y2(y2), w(w), h(h), x_block_size(x_b), y_block_size(y_b), scale(scale),
      focus(focus)
{}

void render_task::run()
{
    QMutexLocker locker(mutex);

    for (int y = y1; y < y2; y += y_block_size) {
        for (int x = x1; x < x2; x += x_block_size) {
            if (version != version_ptr->loadAcquire()) {
                return; // cancelled
            }
            uchar *ptr = data + stride * y + 3 * x;
            uchar res = value(x + x_block_size / 2, y + y_block_size / 2) * 255;
            fill_block(ptr, res, x, y);
        }
    }
    emit finished();
}

void render_task::fill_block(uchar *start, double value, int x_cur, int y_cur)
{
    for (int y = 0; y < std::min(y2 - y_cur, y_block_size); ++y) {
        uchar *ptr = start + y * stride;
        for (int x = 0; x < std::min(x2 - x_cur, x_block_size); ++x) {
            *ptr++ = value * (x + x_cur) / w;
            *ptr++ = 0; // todo leave zeros, they are default
            *ptr++ = value * (y + y_cur) / h;
        }
    }
}

double render_task::value(int x, int y)
{
    std::complex<double> c(x - w / 2., y - h / 2.);
    c *= scale;
    c += focus;

    std::complex<double> z = 0;
    int const MAX_STEPS = 100;
    int step = 0;

    for (;;) {
        if (std::abs(z) >= 2.) {
            return (step % 51) / 50.;
        }
        if (step == MAX_STEPS) {
            return 0;
        }
        z = z * z + c;
        ++step;
    }
}

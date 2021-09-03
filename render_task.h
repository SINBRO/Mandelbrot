#ifndef RENDER_TASK_H
#define RENDER_TASK_H

#include "mandelbrot_widget.h"

#include <complex>
#include <QAtomicInt>
#include <QImage>
#include <QMutex>
#include <QRunnable>
#include <QThreadPool>
#include <complex>

class render_task : public QObject, public QRunnable
{
    Q_OBJECT
public:
    render_task(uint64_t version,
                QAtomicInteger<uint64_t> *version_ptr,
                QMutex *mutex,
                uchar* data,
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
                std::complex<double> focus);

    void run() override;

private:
    double value(int x, int y);
    void fill_block(uchar* start, double value, int x_cur, int y_cur);

    uint64_t version;
    QAtomicInteger<uint64_t> *version_ptr;
    QMutex *mutex;

    uchar *data;
    int stride;
    int x1, y1, x2, y2, w, h;
    int x_block_size, y_block_size;
    double scale;
    std::complex<double> focus;

signals:
    void finished();
};


#endif // RENDER_TASK_H

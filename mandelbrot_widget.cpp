#include "mandelbrot_widget.h"
#include "ui_mandelbrot_widget.h"

#include <iostream> // TODO
#include <QPainter>

mandelbrot_widget::mandelbrot_widget(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::mandelbrot_widget), mutexes(thread_cnt), thread_pool()
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/Mandelbrot.png"));
    image = QImage(width(), height(), QImage::Format_RGB888);
    split_canvas();
    refresh_tasks();
}

mandelbrot_widget::~mandelbrot_widget()
{
    delete ui;
    ++version;
    thread_pool.waitForDone();
}

void mandelbrot_widget::paintEvent(QPaintEvent *ev)
{
    QMainWindow::paintEvent(ev);
    QPainter painter(this);

    painter.drawImage(0, 0, image);
}

void mandelbrot_widget::resizeEvent(QResizeEvent *)
{
    ++version;
    thread_pool.waitForDone(); // cancels
    image = QImage(size(), QImage::Format_RGB888);
    refresh_tasks();
}

void mandelbrot_widget::wheelEvent(QWheelEvent *ev)
{
    auto pos = ev->position();
    auto scale_prev = scale;
    scale /= (pow(1.1, ev->angleDelta().y() / 100.));
    if (ev->angleDelta().y() > 0) {
        auto delta = std::complex<double>(width() / 2. - pos.x(), height() / 2. - pos.y()) * (scale - scale_prev);
        focus += delta;
    }
    refresh_tasks();
}

void mandelbrot_widget::mousePressEvent(QMouseEvent *ev)
{
    if (ev->buttons() & Qt::LeftButton) {
        drag_pos = ev->pos();
    }
}

void mandelbrot_widget::mouseMoveEvent(QMouseEvent *ev)
{
    if (ev->buttons() & Qt::LeftButton) {
        auto pos = ev->pos();
        auto delta = std::complex<double>(drag_pos.x() - pos.x(), drag_pos.y() - pos.y());
        drag_pos = pos;
        focus += delta * scale;
        refresh_tasks();
    }
}

void mandelbrot_widget::redraw()
{
    update();
}

// An attempt to get a division with most square-ish parts
void mandelbrot_widget::split_canvas()
{
    int w = width();
    int h = height();
    x_n = thread_cnt;
    y_n = 1;
    double ratio = ((double) w) * y_n / x_n / h;
    for (int i = 1; i * i <= thread_cnt;
         ++i) { // yeah it's kinda dumb but not like there will be 10^9 threads
        if (thread_cnt % i == 0) {
            int new_x_parts = thread_cnt / i;
            double new_ratio = ((double) w) * i / new_x_parts / h;
            if (std::abs(1 - new_ratio) < std::abs(1 - ratio)) {
                ratio = new_ratio;
                y_n = i;
                x_n = new_x_parts;
            }
        }
    }
    assert(x_n * y_n == thread_cnt);
}

void mandelbrot_widget::refresh_tasks()
{
    ++version;
    for (int x = 0; x < x_n; ++x) {
        for (int y = 0; y < y_n; ++y) {
            schedule_task(x, y, BASIC_BLOCK_SIZE, BASIC_BLOCK_SIZE);
        }
    }
}

// x & y are segment coords, not pixel
void mandelbrot_widget::schedule_task(int x, int y, int x_block_size, int y_block_size)
{
    int w = width();
    int h = height();
    int x_step = (w + x_n - 1) / x_n;
    int y_step = (h + y_n - 1) / y_n;
    render_task *task = new render_task(version.loadAcquire(),
                                        &version,
                                        &mutexes[x * y_n + y],
                                        image.bits(),
                                        image.bytesPerLine(),
                                        x * x_step,
                                        y * y_step,
                                        std::min((x + 1) * x_step, w),
                                        std::min((y + 1) * y_step, h),
                                        w,
                                        h,
                                        x_block_size,
                                        y_block_size,
                                        scale,
                                        focus);
    connect(task, &render_task::finished, this, &mandelbrot_widget::redraw);
    if (x_block_size > 1 || y_block_size > 1) {
        connect(task, &render_task::finished, this, [=]() {
            this->schedule_task(x, y, (x_block_size + 1) / 2, (y_block_size + 1) / 2);
        });
    }
    thread_pool.start(task);
}

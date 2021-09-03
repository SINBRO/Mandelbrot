#ifndef MANDELBROT_WIDGET_H
#define MANDELBROT_WIDGET_H

#include <QMainWindow>
#include <QMutex>
#include <QThreadPool>
#include <QWheelEvent>
#include <QMouseEvent>
#include <complex>

#include "render_task.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class mandelbrot_widget;
}
QT_END_NAMESPACE

class mandelbrot_widget : public QMainWindow
{
    Q_OBJECT

public:
    mandelbrot_widget(QWidget *parent = nullptr);
    ~mandelbrot_widget();
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    void schedule_task(int x, int y, int x_block_size, int y_bloc_size);
private:
    void split_canvas();
    void refresh_tasks();

    Ui::mandelbrot_widget *ui;

    QImage image;

    int thread_cnt = QThread::idealThreadCount();
    std::vector<QMutex> mutexes;
    QThreadPool thread_pool;
    int x_n, y_n; // the image grid division between threads
    QAtomicInteger<uint64_t> version = 0;
    double scale = 0.005;
    QPoint drag_pos;
    std::complex<double> focus = 0.;

    static const int BASIC_BLOCK_SIZE = 32;
private slots:
    void redraw();

};

#endif // MANDELBROT_WIDGET_H

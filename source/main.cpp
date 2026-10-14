#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("OS_MEM_11 - Memory Allocation Simulator");
    window.resize(900, 600);

    auto *centralWidget = new QWidget(&window);
    auto *layout = new QVBoxLayout(centralWidget);

    auto *title = new QLabel("MÔ PHỎNG CẤP PHÁT BỘ NHỚ", centralWidget);
    title->setStyleSheet("font-size: 24px; font-weight: bold;");
    title->setAlignment(Qt::AlignCenter);

    auto *algorithms = new QLabel(
        "First-Fit  |  Best-Fit  |  Worst-Fit",
        centralWidget
    );
    algorithms->setStyleSheet("font-size: 16px;");
    algorithms->setAlignment(Qt::AlignCenter);

    auto *status = new QLabel(
        "Bộ khung giao diện đã chạy. Thuật toán sẽ được thêm sau.",
        centralWidget
    );
    status->setAlignment(Qt::AlignCenter);

    auto *closeButton = new QPushButton("Đóng", centralWidget);
    closeButton->setFixedWidth(120);

    layout->addStretch();
    layout->addWidget(title);
    layout->addSpacing(12);
    layout->addWidget(algorithms);
    layout->addSpacing(20);
    layout->addWidget(status);
    layout->addSpacing(24);
    layout->addWidget(closeButton, 0, Qt::AlignCenter);
    layout->addStretch();

    QObject::connect(closeButton, &QPushButton::clicked, &window, &QMainWindow::close);

    window.setCentralWidget(centralWidget);
    window.show();

    return app.exec();
}

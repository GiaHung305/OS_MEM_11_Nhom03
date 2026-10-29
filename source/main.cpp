#include "main_window.h"
#include <QApplication>
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("OS_MEM_11");
    MainWindow window;
    window.show();
    return app.exec();
}

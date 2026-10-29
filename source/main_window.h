#pragma once
#include <QMainWindow>
#include <vector>
#include <QString>
class QLineEdit;
class QPushButton;
class QComboBox;
class QLabel;
class QSpinBox;
class QTableWidget;
class QHBoxLayout;
class MemoryMapWidget;
class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
private:
    struct Panel {
        MemoryMapWidget* map;
        QTableWidget* blocks;
        QLabel* metrics;
        QTableWidget* trace;
        QLabel* message;
    };
    QLineEdit *configPath_, *eventsPath_;
    QPushButton *load_, *configBrowse_, *eventsBrowse_, *next_, *run_, *pause_, *reset_, *export_;
    QComboBox* strategy_;
    QSpinBox *speed_, *history_;
    QLabel *info_, *state_;
    QWidget* panelsWidget_;
    QHBoxLayout* panelsLayout_;
    std::vector<Panel> panels_;
    void rebuildPanels();
};

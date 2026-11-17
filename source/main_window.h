#pragma once
#include <QMainWindow>
#include "models.h"
#include <vector>
#include <QString>
// Khai báo trước các kiểu widget vì header chỉ dùng con trỏ đến chúng.
class QLineEdit;
class QPushButton;
class QComboBox;
class QLabel;
class QSpinBox;
class QTableWidget;
class QHBoxLayout;
class MemoryMapWidget;
class MainWindow : public QMainWindow {
    // Cho phép moc sinh mã hỗ trợ các tín hiệu của MainWindow.
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    enum class UiState { Empty, Ready, Running, Paused, Finished };
    // Phan tich hop goi sau khi da noi cac signal voi Controller/CSV.
    void setBackendReady(bool ready);
    void setUiState(UiState state);
    void setScenarioStrategy(Strategy strategy);
    // Chi nhan ban sao ket qua de hien thi, khong chay thuat toan.
    void setRuns(const std::vector<RunResult>& runs);
    void showCsvError(const CsvError& error);

// Các yêu cầu từ người dùng mà phần kết nối trong main.cpp có thể nhận.
signals:
    void loadRequested(const QString& configPath, const QString& eventsPath);
    void nextRequested();
    void runRequested(int intervalMs);
    void pauseRequested();
    void resetRequested();
    void exportRequested(const QString& directory);
    void strategySelected(Strategy strategy);
private:
    // Nhóm các widget hiển thị kết quả của một chiến lược.
    struct Panel {
        Strategy strategy;
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
    // Bản sao kết quả dùng để vẽ và xem lại lịch sử.
    std::vector<RunResult> runs_;
    bool backendReady_ = false;
    UiState uiState_ = UiState::Empty;
    Strategy scenarioStrategy_ = Strategy::All;
    void rebuildPanels();
    void updateControls();
    void renderRuns();
    void showSnapshot(int index);
};

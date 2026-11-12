#include "main_window.h"
#include "memory_map_widget.h"
#include <QComboBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QMessageBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>
namespace {
QTableWidget* table(const QStringList& headers, QWidget* parent) {
    auto* result = new QTableWidget(0, headers.size(), parent);
    result->setHorizontalHeaderLabels(headers);
    result->setEditTriggers(QAbstractItemView::NoEditTriggers);
    result->setSelectionBehavior(QAbstractItemView::SelectRows);
    result->setSelectionMode(QAbstractItemView::SingleSelection);
    result->setAlternatingRowColors(true);
    result->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    result->horizontalHeader()->setStretchLastSection(true);
    result->verticalHeader()->hide();
    return result;
}

}
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("OS_MEM_11 - Memory Allocation Simulator");
    resize(1280, 850);
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* layout = new QVBoxLayout(central);
    auto* title = new QLabel(tr("Mô phỏng cấp phát bộ nhớ liên tục"), central);
    title->setStyleSheet("font-size:22px; font-weight:600; color:#0f766e; padding:6px;");
    layout->addWidget(title);
    auto* input = new QGroupBox(tr("Dữ liệu đầu vào"), central);
    auto* inputLayout = new QGridLayout(input);
    configPath_ = new QLineEdit(input); configPath_->setObjectName("configPath");
    eventsPath_ = new QLineEdit(input); eventsPath_->setObjectName("eventsPath");
    configPath_->setPlaceholderText(tr("Chọn config.csv"));
    eventsPath_->setPlaceholderText(tr("Chọn events.csv"));
    configBrowse_ = new QPushButton(tr("Chọn config…"), input);
    eventsBrowse_ = new QPushButton(tr("Chọn events…"), input);
    load_ = new QPushButton(tr("Nạp CSV"), input); load_->setObjectName("load");
    inputLayout->addWidget(configPath_,0,0); inputLayout->addWidget(configBrowse_,0,1);
    inputLayout->addWidget(eventsPath_,1,0); inputLayout->addWidget(eventsBrowse_,1,1);
    inputLayout->addWidget(load_,0,2,2,1);
    layout->addWidget(input);
    auto* toolbar = new QHBoxLayout;
    strategy_ = new QComboBox(central); strategy_->setObjectName("strategy");
    strategy_->addItems({"First-Fit", "Best-Fit", "Worst-Fit", "Compare All"});
    strategy_->setToolTip(tr("Chọn bố cục một chiến lược hoặc ba cột Compare All."));
    toolbar->addWidget(new QLabel(tr("Chiến lược:"), central)); toolbar->addWidget(strategy_);
    next_ = new QPushButton("Next", central); next_->setObjectName("next");
    run_ = new QPushButton("Run", central); run_->setObjectName("run");
    pause_ = new QPushButton("Pause", central); pause_->setObjectName("pause");
    reset_ = new QPushButton("Reset", central); reset_->setObjectName("reset");
    export_ = new QPushButton("Export", central); export_->setObjectName("export");
    for (auto* button : {next_, run_, pause_, reset_, export_}) toolbar->addWidget(button);
    speed_ = new QSpinBox(central); speed_->setObjectName("speed");
    speed_->setRange(50, 5000); speed_->setValue(500); speed_->setSuffix(" ms");
    toolbar->addWidget(new QLabel(tr("Mỗi bước:"), central)); toolbar->addWidget(speed_); toolbar->addStretch();
    layout->addLayout(toolbar);
    info_ = new QLabel(tr("Bản dựng giao diện — chưa nối Core, mô phỏng hoặc đọc/ghi CSV."), central);
    info_->setWordWrap(true); layout->addWidget(info_);
    auto* historyBar = new QHBoxLayout;
    state_ = new QLabel(central); state_->setObjectName("state");
    history_ = new QSpinBox(central); history_->setObjectName("history"); history_->setRange(0,0);
    historyBar->addWidget(state_); historyBar->addStretch();
    historyBar->addWidget(new QLabel(tr("Xem snapshot (0 = ban đầu):"), central)); historyBar->addWidget(history_);
    layout->addLayout(historyBar);
    auto* scroll = new QScrollArea(central); scroll->setWidgetResizable(true);
    panelsWidget_ = new QWidget(scroll); panelsLayout_ = new QHBoxLayout(panelsWidget_);
    scroll->setWidget(panelsWidget_);
    layout->addWidget(scroll,1);
    // Chon duong dan chi la viec cua GUI, khong doc noi dung CSV.
    connect(configBrowse_, &QPushButton::clicked, this, [this] {
        const auto path = QFileDialog::getOpenFileName(this, tr("Chọn config.csv"), configPath_->text(), "CSV (*.csv)");
        if (!path.isEmpty()) configPath_->setText(path);
    });
    connect(eventsBrowse_, &QPushButton::clicked, this, [this] {
        const auto path = QFileDialog::getOpenFileName(this, tr("Chọn events.csv"), eventsPath_->text(), "CSV (*.csv)");
        if (!path.isEmpty()) eventsPath_->setText(path);
    });
    connect(configPath_, &QLineEdit::textChanged, this, [this] { updateControls(); });
    connect(eventsPath_, &QLineEdit::textChanged, this, [this] { updateControls(); });
    connect(load_, &QPushButton::clicked, this, [this] {
        emit loadRequested(configPath_->text().trimmed(), eventsPath_->text().trimmed());
    });
    connect(next_, &QPushButton::clicked, this, &MainWindow::nextRequested);
    connect(run_, &QPushButton::clicked, this, [this] { emit runRequested(speed_->value()); });
    connect(pause_, &QPushButton::clicked, this, &MainWindow::pauseRequested);
    connect(reset_, &QPushButton::clicked, this, &MainWindow::resetRequested);
    connect(export_, &QPushButton::clicked, this, [this] {
        const auto directory = QFileDialog::getExistingDirectory(this, tr("Chọn thư mục xuất CSV"));
        if (!directory.isEmpty()) emit exportRequested(directory);
    });
    connect(history_, &QSpinBox::valueChanged, this, &MainWindow::showSnapshot);
    strategy_->setCurrentIndex(3);
    connect(strategy_, &QComboBox::currentIndexChanged, this, [this](int index) {
        rebuildPanels();
        renderRuns();
        emit strategySelected(static_cast<Strategy>(index));
    });
    rebuildPanels();
    setBackendReady(false);
}
void MainWindow::rebuildPanels() {
    while (auto* item = panelsLayout_->takeAt(0)) { delete item->widget(); delete item; }
    panels_.clear();
    const QStringList names = {"First-Fit", "Best-Fit", "Worst-Fit"};
    const int selected = strategy_->currentIndex();
    for (int i = 0; i < names.size(); ++i) {
        if (selected != 3 && selected != i) continue;
        auto* box = new QGroupBox(names[i], panelsWidget_);
        box->setMinimumWidth(380);
        auto* layout = new QVBoxLayout(box);
        auto* map = new MemoryMapWidget(box);
        auto* metrics = new QLabel(tr("Total: — KB · Unit: — KB\nFree: — KB · Largest hole: — KB · Holes: —\nInternal: — KB · External: — · ALLOC thành công/thất bại: —"), box); metrics->setWordWrap(true);
        auto* blocks = table({"Start KB","End KB)","Size KB","PID","Requested KB","Internal KB"},box);
        auto* trace = table({"Step","Op","PID","Req KB","Alloc KB","Start KB",tr("Kết quả")},box);
        trace->setObjectName("trace_" + names[i]);
        trace->setWordWrap(false);
        trace->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
        trace->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        trace->verticalHeader()->setDefaultSectionSize(28);
        trace->setToolTip(tr("Req: yêu cầu · Alloc: thực cấp · Start: địa chỉ cấp phát. Dấu —: không có địa chỉ. Chọn dòng để xem snapshot và Message."));
        auto* message = new QLabel(tr("Message: chưa có dữ liệu"),box);
        message->setWordWrap(true);
        message->setFixedHeight(38);
        auto* details = new QSplitter(Qt::Vertical,box);
        details->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Ignored);
        details->setMinimumHeight(230);
        details->addWidget(blocks);
        auto* traceSection = new QWidget(details);
        auto* traceLayout = new QVBoxLayout(traceSection);
        traceLayout->setContentsMargins(0,0,0,0);
        traceLayout->addWidget(new QLabel(tr("Lịch sử sự kiện"),traceSection));
        traceLayout->addWidget(trace,1);
        traceLayout->addWidget(message);
        details->addWidget(traceSection);
        details->setSizes({180,220});
        layout->addWidget(map); layout->addWidget(metrics); layout->addWidget(details,1);
        panelsLayout_->addWidget(box,1); panels_.push_back({static_cast<Strategy>(i),map,blocks,metrics,trace,message});
        connect(trace, &QTableWidget::cellClicked, this, [this](int row, int) { history_->setValue(row + 1); });
        connect(trace->verticalScrollBar(), &QScrollBar::valueChanged, this, [this,trace](int value) {
            for (const auto& panel : panels_) if (panel.trace != trace) {
                QSignalBlocker blocker(panel.trace->verticalScrollBar());
                panel.trace->verticalScrollBar()->setValue(value);
            }
        });
    }
}

// Cac ham duoi day la diem noi GUI; khong goi Core hay parser.
void MainWindow::setBackendReady(bool ready) {
    backendReady_ = ready;
    info_->setText(ready ? tr("Đã kết nối phần xử lý.")
                        : tr("Chọn được đường dẫn; chưa kết nối Controller/CSV nên chưa nạp hoặc chạy."));
    updateControls();
}

void MainWindow::setUiState(UiState state) {
    uiState_ = state;
    updateControls();
}

void MainWindow::setScenarioStrategy(Strategy strategy) {
    scenarioStrategy_ = strategy;
    strategy_->setCurrentIndex(static_cast<int>(strategy));
    strategy_->setToolTip(strategy == Strategy::All
        ? tr("Cấu hình ALL: xem riêng một chiến lược hoặc cả ba. Export gồm cả ba chiến lược.")
        : tr("Phiên này chạy chiến lược được chọn trong config.csv."));
    updateControls();
}

void MainWindow::updateControls() {
    const bool running = uiState_ == UiState::Running;
    const bool canStep = backendReady_ && (uiState_ == UiState::Ready || uiState_ == UiState::Paused);
    configPath_->setEnabled(!running);
    eventsPath_->setEnabled(!running);
    configBrowse_->setEnabled(!running);
    eventsBrowse_->setEnabled(!running);
    load_->setEnabled(backendReady_ && !running && !configPath_->text().trimmed().isEmpty()
                      && !eventsPath_->text().trimmed().isEmpty());
    load_->setToolTip(backendReady_ ? tr("Chọn đủ hai đường dẫn trước khi nạp.") : tr("Chờ kết nối CSV và Controller."));
    next_->setEnabled(canStep);
    run_->setEnabled(canStep);
    pause_->setEnabled(backendReady_ && running);
    reset_->setEnabled(backendReady_ && uiState_ != UiState::Empty);
    export_->setEnabled(backendReady_ && (uiState_ == UiState::Paused || uiState_ == UiState::Finished));
    strategy_->setEnabled(!running && scenarioStrategy_ == Strategy::All);
    speed_->setEnabled(backendReady_ && !running);
    history_->setEnabled(!running && history_->maximum() > 0);
    const QStringList states = {tr("Chưa nạp dữ liệu"), tr("Sẵn sàng"), tr("Đang chạy"), tr("Tạm dừng"), tr("Hoàn tất")};
    state_->setText(states[static_cast<int>(uiState_)]);
}

void MainWindow::showCsvError(const CsvError& error) {
    // Chi hien loi do CSV tra ve, khong tu validate lai.
    QMessageBox::warning(this, tr("Lỗi CSV"), tr("File: %1\nDòng: %2 · Cột: %3\n%4")
        .arg(QString::fromStdString(error.filePath)).arg(error.line).arg(error.column)
        .arg(QString::fromStdString(error.message)));
}

void MainWindow::setRuns(const std::vector<RunResult>& runs) {
    runs_ = runs;
    renderRuns();
}

void MainWindow::renderRuns() {
    int commonSteps = -1;
    for (auto& panel : panels_) {
        QSignalBlocker blocker(panel.trace);
        panel.trace->setRowCount(0);
        const RunResult* run = nullptr;
        for (const auto& candidate : runs_)
            if (candidate.strategy == panel.strategy) { run = &candidate; break; }
        const int count = run ? static_cast<int>(run->results.size()) : 0;
        if (commonSteps < 0 || count < commonSteps) commonSteps = count;
        if (!run) continue;
        for (const auto& result : run->results) {
            const int row = panel.trace->rowCount();
            panel.trace->insertRow(row);
            const QStringList cells = {QString::number(result.step),
                result.operation == OperationType::Allocate ? "ALLOC" : "FREE",
                QString::fromStdString(result.processId), QString::number(result.requestedSizeKb),
                QString::number(result.allocatedSizeKb),
                result.startAddressKb ? QString::number(*result.startAddressKb) : QStringLiteral("—"),
                result.success ? tr("Thành công") : tr("Thất bại")};
            for (int col = 0; col < cells.size(); ++col) {
                auto* item = new QTableWidgetItem(cells[col]);
                item->setToolTip(QString::fromStdString(result.message));
                panel.trace->setItem(row, col, item);
            }
        }
    }
    // Compare All chi xem cac snapshot co cung chi so su kien.
    QSignalBlocker blocker(history_);
    history_->setRange(0, qMax(0, commonSteps));
    history_->setValue(history_->maximum());
    showSnapshot(history_->value());
    updateControls();
}

void MainWindow::showSnapshot(int index) {
    for (auto& panel : panels_) {
        const RunResult* run = nullptr;
        for (const auto& candidate : runs_)
            if (candidate.strategy == panel.strategy) { run = &candidate; break; }
        panel.blocks->setRowCount(0);
        if (!run) {
            panel.map->setBlocks(0, {});
            panel.metrics->setText(tr("Chưa có dữ liệu cho chiến lược này."));
            panel.message->setText(tr("Message: chưa có dữ liệu"));
            continue;
        }
        const StepResult* result = index > 0 && index <= static_cast<int>(run->results.size())
            ? &run->results[index - 1] : nullptr;
        const auto& blocks = result ? result->blocks : run->initialState.blocks;
        const auto& metrics = result ? result->metrics : run->initialState.metrics;
        panel.map->setBlocks(run->config.totalMemoryKb, blocks);
        panel.metrics->setText(tr("Total: %1 KB · Unit: %2 KB\nFree: %3 KB · Largest hole: %4 KB · Holes: %5\nInternal: %6 KB · External: %7 KB · Ratio: %8\nToàn lượt chạy: ALLOC thành công %9 / thất bại %10")
            .arg(run->config.totalMemoryKb).arg(run->config.allocationUnitKb)
            .arg(metrics.totalFreeKb).arg(metrics.largestFreeBlockKb).arg(metrics.holeCount)
            .arg(metrics.internalFragmentationKb).arg(metrics.externalFragmentationKb)
            .arg(metrics.externalFragmentationRatio, 0, 'f', 3)
            .arg(run->summary.allocSuccesses).arg(run->summary.allocFailures));
        for (const auto& block : blocks) {
            const int row = panel.blocks->rowCount();
            panel.blocks->insertRow(row);
            const QStringList cells = {QString::number(block.startAddressKb),
                QString::number(block.startAddressKb + block.allocatedSizeKb),
                QString::number(block.allocatedSizeKb), block.isFree ? "FREE" : QString::fromStdString(block.processId),
                QString::number(block.requestedSizeKb),
                QString::number(block.isFree ? 0 : block.allocatedSizeKb - block.requestedSizeKb)};
            for (int col = 0; col < cells.size(); ++col)
                panel.blocks->setItem(row, col, new QTableWidgetItem(cells[col]));
        }
        panel.trace->clearSelection();
        if (result) panel.trace->selectRow(index - 1);
        panel.message->setText(result ? tr("Bước %1 · %2").arg(result->step).arg(QString::fromStdString(result->message))
                                     : tr("Trạng thái ban đầu (bước 0)"));
    }
}

#include "main_window.h"
#include "memory_map_widget.h"
#include <QComboBox>
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
    for (auto* widget : std::initializer_list<QWidget*>{load_, configBrowse_, eventsBrowse_, configPath_, eventsPath_, next_, run_, pause_, reset_, export_, speed_, history_}) {
        widget->setEnabled(false);
        widget->setToolTip(tr("Chưa tích hợp chức năng; hiện chỉ dựng giao diện."));
    }
    state_->setText(tr("Chế độ giao diện · chưa có dữ liệu"));
    strategy_->setCurrentIndex(3);
    connect(strategy_, &QComboBox::currentIndexChanged, this, [this] { rebuildPanels(); });
    rebuildPanels();
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
        panelsLayout_->addWidget(box,1); panels_.push_back({map,blocks,metrics,trace,message});
        connect(trace->verticalScrollBar(), &QScrollBar::valueChanged, this, [this,trace](int value) {
            for (const auto& panel : panels_) if (panel.trace != trace) {
                QSignalBlocker blocker(panel.trace->verticalScrollBar());
                panel.trace->verticalScrollBar()->setValue(value);
            }
        });
    }
}

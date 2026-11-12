#include "csv_handler.h"
#include "main_window.h"
#include "simulation_controller.h"

#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QStatusBar>
#include <QStringList>
#include <QTimer>

namespace {
using UiState = MainWindow::UiState;

void exportResults(MainWindow& window, const SimulationController& controller,
                   const QString& directory) {
    std::vector<StepResult> results;
    for (const auto& run : controller.runs()) {
        results.insert(results.end(), run.results.begin(), run.results.end());
    }

    const auto outputDir = directory.toStdString();
    const bool traceWritten = CSVHandler::exportTraceCSV("trace.csv", results, outputDir);
    const bool blocksWritten = CSVHandler::exportBlocksCSV("blocks.csv", results, outputDir);
    if (!traceWritten || !blocksWritten) {
        QStringList failedFiles;
        if (!traceWritten) failedFiles << QStringLiteral("trace.csv");
        if (!blocksWritten) failedFiles << QStringLiteral("blocks.csv");
        QMessageBox::warning(&window, MainWindow::tr("Lỗi xuất CSV"),
            MainWindow::tr("Không ghi được %1 trong thư mục %2.")
                .arg(failedFiles.join(QStringLiteral(", ")), directory));
        return;
    }
    window.statusBar()->showMessage(MainWindow::tr("Đã xuất trace.csv và blocks.csv."), 5000);
}

void connectSimulation(MainWindow& window, SimulationController& controller, QTimer& timer) {
    const auto showResults = [&] {
        window.setRuns(controller.runs());
        if (controller.isFinished()) {
            window.setUiState(UiState::Finished);
        } else if (timer.isActive()) {
            window.setUiState(UiState::Running);
        } else {
            window.setUiState(controller.processedCount() == 0 ? UiState::Ready : UiState::Paused);
        }
    };

    const auto advance = [&, showResults] {
        const bool advanced = controller.stepForward();
        if (!advanced || controller.isFinished()) timer.stop();
        if (advanced) showResults();
    };

    QObject::connect(&window, &MainWindow::loadRequested, &window,
        [&, showResults](const QString& configPath, const QString& eventsPath) {
            const auto config = CSVHandler::readConfig(QFileInfo(configPath).absoluteFilePath().toStdString(), {});
            if (config.error) {
                window.showCsvError(*config.error);
                return;
            }
            const auto events = CSVHandler::readEvents(QFileInfo(eventsPath).absoluteFilePath().toStdString(), {});
            if (events.error) {
                window.showCsvError(*events.error);
                return;
            }
            if (!controller.loadScenario(*config.value, *events.value)) {
                QMessageBox::warning(&window, MainWindow::tr("Không nạp được mô phỏng"),
                    MainWindow::tr("%1\n%2").arg(eventsPath, QString::fromStdString(controller.lastError())));
                return;
            }
            timer.stop();
            window.setScenarioStrategy(controller.config().strategy);
            showResults();
        });

    QObject::connect(&window, &MainWindow::nextRequested, &window, advance);
    QObject::connect(&timer, &QTimer::timeout, &window, [&, advance] {
        if (timer.isActive()) advance();
    });
    QObject::connect(&window, &MainWindow::runRequested, &window, [&](int intervalMs) {
        if (!controller.hasScenario() || controller.isFinished()) return;
        timer.start(intervalMs);
        window.setUiState(UiState::Running);
    });
    QObject::connect(&window, &MainWindow::pauseRequested, &window, [&] {
        timer.stop();
        window.setUiState(UiState::Paused);
    });
    QObject::connect(&window, &MainWindow::resetRequested, &window, [&, showResults] {
        timer.stop();
        controller.reset();
        showResults();
        if (!controller.lastError().empty()) {
            QMessageBox::warning(&window, MainWindow::tr("Không đặt lại được mô phỏng"),
                QString::fromStdString(controller.lastError()));
        }
    });
    QObject::connect(&window, &MainWindow::exportRequested, &window, [&](const QString& directory) {
        exportResults(window, controller, directory);
    });

    window.setBackendReady(true);
}
} // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("OS_MEM_11");
    SimulationController controller;
    QTimer timer;
    MainWindow window;
    connectSimulation(window, controller, timer);
    window.show();
    return app.exec();
}

#include "memory_map_widget.h"
#include <QPainter>
#include <QHelpEvent>
#include <QToolTip>
namespace {
QColor color(const MemoryBlock& block) {
    if (block.isFree) return QColor("#e2e8f0");
    unsigned hash = 2166136261u;
    for (unsigned char c : block.processId) hash = (hash ^ c) * 16777619u;
    return QColor::fromHsv(int(hash % 360), 125, 210);
}
}
MemoryMapWidget::MemoryMapWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(110);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}
void MemoryMapWidget::setBlocks(int total, const std::vector<MemoryBlock>& blocks) {
    total_ = total; blocks_ = blocks; update();
}
QRectF MemoryMapWidget::blockRect(const MemoryBlock& block) const {
    const double scale = double(qMax(0, width() - 16)) / total_;
    return {8 + block.startAddressKb * scale, 25, block.allocatedSizeKb * scale, 52};
}
void MemoryMapWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    if (total_ <= 0) { p.drawText(rect(), Qt::AlignCenter, tr("Chưa có dữ liệu")); return; }
    p.setPen(QColor("#475569")); p.drawText(8, 17, "0 KB");
    p.drawText(QRect(8, 0, width()-16, 20), Qt::AlignRight, QString::number(total_) + " KB");
    for (const auto& block : blocks_) {
        const auto r = blockRect(block);
        p.setPen(Qt::white); p.setBrush(color(block)); p.drawRect(r);
        const QString label = block.isFree ? "FREE" : QString::fromStdString(block.processId);
        p.setPen(block.isFree ? QColor("#334155") : Qt::white);
        if (r.width() > p.fontMetrics().horizontalAdvance(label) + 8) p.drawText(r, Qt::AlignCenter, label);
    }
    p.setPen(QColor("#64748b"));
    p.drawText(QRect(8, 82, width()-16, 25), Qt::AlignLeft, tr("Xám: FREE · Di chuột xem [start, end)"));
}
bool MemoryMapWidget::event(QEvent* event) {
    if (event->type() == QEvent::ToolTip && total_ > 0) {
        auto* help = static_cast<QHelpEvent*>(event);
        for (const auto& block : blocks_) if (blockRect(block).contains(help->pos())) {
            QToolTip::showText(help->globalPos(), QString("%1 · [%2, %3) KB\nSize: %4 KB · Requested: %5 KB")
                .arg(block.isFree ? "FREE" : QString::fromStdString(block.processId))
                .arg(block.startAddressKb).arg(block.startAddressKb + block.allocatedSizeKb)
                .arg(block.allocatedSizeKb).arg(block.requestedSizeKb), this);
            return true;
        }
        QToolTip::hideText(); return true;
    }
    return QWidget::event(event);
}

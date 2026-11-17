#include "memory_map_widget.h"
#include <QPainter>
#include <QHelpEvent>
#include <QToolTip>
namespace {
QColor color(const MemoryBlock& block) {
    if (block.isFree) return QColor("#e2e8f0");
    // Băm PID để cùng một tiến trình giữ màu qua các bước và các panel.
    unsigned hash = 2166136261u;
    for (unsigned char c : block.processId) hash = (hash ^ c) * 16777619u;
    // Đổi mã băm thành góc màu HSV từ 0 đến 359.
    return QColor::fromHsv(int(hash % 360), 125, 210);
}
}
MemoryMapWidget::MemoryMapWidget(QWidget* parent) : QWidget(parent) {
    // Chừa chỗ cho nhãn và chú thích. Layout có thể mở rộng thanh theo chiều ngang.
    setMinimumHeight(110);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}
void MemoryMapWidget::setBlocks(int total, const std::vector<MemoryBlock>& blocks) {
    // Lưu bản sao snapshot rồi yêu cầu Qt vẽ lại widget.
    total_ = total; blocks_ = blocks; update();
}
QRectF MemoryMapWidget::blockRect(const MemoryBlock& block) const {
    // Chừa lề 8 px mỗi bên rồi tính số pixel tương ứng với 1 KB.
    const double scale = double(qMax(0, width() - 16)) / total_;
    // Địa chỉ đầu quyết định vị trí ngang, dung lượng quyết định chiều rộng.
    return {8 + block.startAddressKb * scale, 25, block.allocatedSizeKb * scale, 52};
}
void MemoryMapWidget::paintEvent(QPaintEvent*) {
    // Vẽ lên chính widget này. Qt gọi hàm khi có yêu cầu cập nhật hoặc đổi kích thước.
    QPainter p(this);
    if (total_ <= 0) { p.drawText(rect(), Qt::AlignCenter, tr("Chưa có dữ liệu")); return; }
    p.setPen(QColor("#475569")); p.drawText(8, 17, "0 KB");
    p.drawText(QRect(8, 0, width()-16, 20), Qt::AlignRight, QString::number(total_) + " KB");
    // Vẽ từng block vào đúng vị trí trên thanh bộ nhớ.
    for (const auto& block : blocks_) {
        const auto r = blockRect(block);
        p.setPen(Qt::white); p.setBrush(color(block)); p.drawRect(r);
        const QString label = block.isFree ? "FREE" : QString::fromStdString(block.processId);
        p.setPen(block.isFree ? QColor("#334155") : Qt::white);
        // Chỉ vẽ nhãn nếu block đủ rộng để chứa chữ và khoảng đệm.
        if (r.width() > p.fontMetrics().horizontalAdvance(label) + 8) p.drawText(r, Qt::AlignCenter, label);
    }
    p.setPen(QColor("#64748b"));
    p.drawText(QRect(8, 82, width()-16, 25), Qt::AlignLeft, tr("Xám: FREE · Di chuột xem [start, end)"));
}
bool MemoryMapWidget::event(QEvent* event) {
    if (event->type() == QEvent::ToolTip && total_ > 0) {
        // Dùng tọa độ trong widget để tìm block và tọa độ màn hình để đặt tooltip.
        auto* help = static_cast<QHelpEvent*>(event);
        for (const auto& block : blocks_) if (blockRect(block).contains(help->pos())) {
            // Địa chỉ kết thúc bằng địa chỉ đầu cộng dung lượng và nằm ngoài block.
            QToolTip::showText(help->globalPos(), QString("%1 · [%2, %3) KB\nSize: %4 KB · Requested: %5 KB")
                .arg(block.isFree ? "FREE" : QString::fromStdString(block.processId))
                .arg(block.startAddressKb).arg(block.startAddressKb + block.allocatedSizeKb)
                .arg(block.allocatedSizeKb).arg(block.requestedSizeKb), this);
            return true;
        }
        // Nếu chuột ở ngoài các block, ẩn tooltip còn hiện từ lần trước.
        QToolTip::hideText(); return true;
    }
    // Chuyển các sự kiện khác cho lớp cha xử lý theo mặc định.
    return QWidget::event(event);
}

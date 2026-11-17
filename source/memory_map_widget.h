#pragma once
#include "models.h"
#include <QWidget>
// Widget tự vẽ thanh bộ nhớ từ các block của snapshot hiện tại.
class MemoryMapWidget : public QWidget {
public:
    explicit MemoryMapWidget(QWidget* parent = nullptr);
    // Nhận tổng dung lượng và lưu bản sao block để vẽ lại.
    void setBlocks(int total, const std::vector<MemoryBlock>& blocks);
protected:
    // Qt gọi paintEvent để vẽ và event để chuyển các sự kiện đến widget.
    void paintEvent(QPaintEvent*) override;
    bool event(QEvent* event) override;
private:
    int total_ = 0;
    std::vector<MemoryBlock> blocks_;
    // Dùng chung phép đổi từ KB sang pixel khi vẽ và khi tìm block dưới chuột.
    QRectF blockRect(const MemoryBlock& block) const;
};

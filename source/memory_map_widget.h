#pragma once
#include "models.h"
#include <QWidget>
class MemoryMapWidget : public QWidget {
public:
    explicit MemoryMapWidget(QWidget* parent = nullptr);
    void setBlocks(int total, const std::vector<MemoryBlock>& blocks);
protected:
    void paintEvent(QPaintEvent*) override;
    bool event(QEvent* event) override;
private:
    int total_ = 0;
    std::vector<MemoryBlock> blocks_;
    QRectF blockRect(const MemoryBlock& block) const;
};

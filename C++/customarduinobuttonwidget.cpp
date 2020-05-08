#include "customarduinobuttonwidget.h"

CustomArduinoButtonWidget::CustomArduinoButtonWidget(int index):
    buttonIndex_(this),
    minValue_(this),
    maxValue_(this)
{
    buttonIndex_.setText(QString::fromStdString("Button " + std::to_string(index)));
    minValue_.setText(QString::number(0));
    maxValue_.setText(QString::number(1023));

    //SetCenter
    buttonIndex_.setAlignment(Qt::AlignCenter);
    minValue_.setAlignment(Qt::AlignCenter);
    maxValue_.setAlignment(Qt::AlignCenter);

    //Color
    buttonIndex_.setStyleSheet("QLineEdit { background: blue;}");

    //Layout
    hLayout_.addWidget(&minValue_);
    hLayout_.addWidget(&buttonIndex_);
    hLayout_.addWidget(&maxValue_);
    this->setLayout(&hLayout_);
}

QLineEdit& CustomArduinoButtonWidget::getMinValueLineEdit(){
    return minValue_;
}

QLineEdit& CustomArduinoButtonWidget::getMaxValueLineEdit(){
    return maxValue_;
}

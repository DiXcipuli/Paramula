#ifndef CUSTOMARDUINOBUTTONWIDGET_H
#define CUSTOMARDUINOBUTTONWIDGET_H

#include <QFrame>
#include <QLineEdit>
#include <QHBoxLayout>

class CustomArduinoButtonWidget : public QFrame{

public:
    CustomArduinoButtonWidget(int index);

    //getters
    QLineEdit& getMinValueLineEdit();
    QLineEdit& getMaxValueLineEdit();

private:
    QLineEdit minValue_;
    QLineEdit maxValue_;
    QLineEdit buttonIndex_;
    QHBoxLayout hLayout_;
};

#endif

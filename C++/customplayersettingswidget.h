#ifndef CUSTOMPLAYERSETTINGSWIDGET_H
#define CUSTOMPLAYERSETTINGSWIDGET_H

#include <QFrame>
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QString>
#include <QStringList>
#include <QHBoxLayout>
#include <QLineEdit>

class CustomPlayerSettingsWidget: public QFrame{

public:
    CustomPlayerSettingsWidget(QWidget *parent, QString index);
    ~CustomPlayerSettingsWidget();

    //getters
    QString getPlayerName() const;
    QString getPlayerSkill() const;


private:
    QLineEdit leSkills_;
    QComboBox cbSkills_;
    QLineEdit leNameQ_;
    QLineEdit leNameA_;
    QStringList slSkillList_;
    QLabel lImg_;
    QHBoxLayout hLayout_;
};

#endif

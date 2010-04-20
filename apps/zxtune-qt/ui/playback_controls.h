/********************************************************************************
** Form generated from reading UI file 'playback_controls.ui'
**
** Created: Tue Apr 20 09:02:04 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PLAYBACK_CONTROLS_H
#define PLAYBACK_CONTROLS_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PlaybackControls
{
public:
    QHBoxLayout *horizontalLayout_2;
    QToolButton *prevButton;
    QToolButton *playButton;
    QToolButton *pauseButton;
    QToolButton *stopButton;
    QToolButton *nextButton;

    void setupUi(QWidget *PlaybackControls)
    {
        if (PlaybackControls->objectName().isEmpty())
            PlaybackControls->setObjectName(QString::fromUtf8("PlaybackControls"));
        PlaybackControls->setEnabled(true);
        PlaybackControls->resize(189, 37);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(PlaybackControls->sizePolicy().hasHeightForWidth());
        PlaybackControls->setSizePolicy(sizePolicy);
        PlaybackControls->setMinimumSize(QSize(0, 0));
        PlaybackControls->setContextMenuPolicy(Qt::NoContextMenu);
        PlaybackControls->setWindowTitle(QString::fromUtf8("Form"));
        horizontalLayout_2 = new QHBoxLayout(PlaybackControls);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        prevButton = new QToolButton(PlaybackControls);
        prevButton->setObjectName(QString::fromUtf8("prevButton"));
        sizePolicy.setHeightForWidth(prevButton->sizePolicy().hasHeightForWidth());
        prevButton->setSizePolicy(sizePolicy);
        prevButton->setFocusPolicy(Qt::NoFocus);
        prevButton->setContextMenuPolicy(Qt::NoContextMenu);

        horizontalLayout_2->addWidget(prevButton);

        playButton = new QToolButton(PlaybackControls);
        playButton->setObjectName(QString::fromUtf8("playButton"));
        sizePolicy.setHeightForWidth(playButton->sizePolicy().hasHeightForWidth());
        playButton->setSizePolicy(sizePolicy);
        playButton->setFocusPolicy(Qt::NoFocus);
        playButton->setContextMenuPolicy(Qt::NoContextMenu);

        horizontalLayout_2->addWidget(playButton);

        pauseButton = new QToolButton(PlaybackControls);
        pauseButton->setObjectName(QString::fromUtf8("pauseButton"));
        sizePolicy.setHeightForWidth(pauseButton->sizePolicy().hasHeightForWidth());
        pauseButton->setSizePolicy(sizePolicy);
        pauseButton->setFocusPolicy(Qt::NoFocus);
        pauseButton->setContextMenuPolicy(Qt::NoContextMenu);

        horizontalLayout_2->addWidget(pauseButton);

        stopButton = new QToolButton(PlaybackControls);
        stopButton->setObjectName(QString::fromUtf8("stopButton"));
        sizePolicy.setHeightForWidth(stopButton->sizePolicy().hasHeightForWidth());
        stopButton->setSizePolicy(sizePolicy);
        stopButton->setFocusPolicy(Qt::NoFocus);
        stopButton->setContextMenuPolicy(Qt::NoContextMenu);

        horizontalLayout_2->addWidget(stopButton);

        nextButton = new QToolButton(PlaybackControls);
        nextButton->setObjectName(QString::fromUtf8("nextButton"));
        sizePolicy.setHeightForWidth(nextButton->sizePolicy().hasHeightForWidth());
        nextButton->setSizePolicy(sizePolicy);
        nextButton->setFocusPolicy(Qt::NoFocus);
        nextButton->setContextMenuPolicy(Qt::NoContextMenu);

        horizontalLayout_2->addWidget(nextButton);


        retranslateUi(PlaybackControls);

        QMetaObject::connectSlotsByName(PlaybackControls);
    } // setupUi

    void retranslateUi(QWidget *PlaybackControls)
    {
        prevButton->setText(QApplication::translate("PlaybackControls", "<<", 0, QApplication::UnicodeUTF8));
        playButton->setText(QApplication::translate("PlaybackControls", ">", 0, QApplication::UnicodeUTF8));
        pauseButton->setText(QApplication::translate("PlaybackControls", "||", 0, QApplication::UnicodeUTF8));
        stopButton->setText(QApplication::translate("PlaybackControls", "x", 0, QApplication::UnicodeUTF8));
        nextButton->setText(QApplication::translate("PlaybackControls", ">>", 0, QApplication::UnicodeUTF8));
        Q_UNUSED(PlaybackControls);
    } // retranslateUi

};

namespace Ui {
    class PlaybackControls: public Ui_PlaybackControls {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PLAYBACK_CONTROLS_H

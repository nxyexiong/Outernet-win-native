#pragma once

#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QWidget>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <qevent.h>
#include <qthread.h>
#include <qmessagebox.h>
#include "control.h"


class MainWindow: public QMainWindow {
    Q_OBJECT

public:
	MainWindow(QWidget *parent = Q_NULLPTR);
	~MainWindow();

private:
	Control* control;

	QWidget* mainWidget;
	QGraphicsDropShadowEffect* shadow;
	QPushButton* closeBtn;
	QPushButton* minBtn;
	QLabel* titleLabel;
	QLabel* addrLabel;
	QLabel* portLabel;
	QLabel* userLabel;
	QLabel* secretLabel;
	QLabel* rxRateLabel;
	QLabel* txRateLabel;
	QLabel* rxTotalLabel;
	QLabel* txTotalLabel;
	QLineEdit* addrEdit;
	QLineEdit* portEdit;
	QLineEdit* userEdit;
	QLineEdit* secretEdit;
	QLabel* rxRateNumberLabel;
	QLabel* txRateNumberLabel;
	QLabel* rxTotalNumberLabel;
	QLabel* txTotalNumberLabel;
	QPushButton* toggleConnectBtn;
	QPushButton* toggleFilterBtn;
	QPushButton* viewMainPageBtn;
	QTimer* trafficTimer;

	ControlState controlState;
	// dragging
	bool isDragging;
	QPoint dragPos;
public:
	bool event(QEvent *qevent) override;

	void onControlStateChanged(ControlState state);

	void onClose();
	void onConnectBtnClicked();
};

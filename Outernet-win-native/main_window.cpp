#include "main_window.h"
#include "control.h"

#define VERSION_CODE "5.0"

const char* styleSheetStr = R""""(
#MainWidget{
    background-color: #312F30;
    border-top-left-radius:7px;
    border-top-right-radius:7px;
    border-bottom-left-radius:7px;
    border-bottom-right-radius:7px;
}
#CloseButton{
    background-color: #EE544A;
    border-radius: 5px;
}
#CloseButton:disabled{
    background-color: #666465;
    border-radius: 5px;
}
#MinButton{
    background-color: #FDBD3F;
    border-radius: 5px;
}
#MinButton:disabled{
    background-color: #666465;
    border-radius: 5px;
}
#TitleLabel{
    font-family: "Consolas";
    color: #CCCCCC;
    font-size: 12px;
}
#NormalLabel{
    font-family: "Consolas";
    color: #DDDDDD;
    font-size: 14px;
}
QLineEdit{
    background-color: #505050;
    border: 0px;
    border-radius: 5px;
    color: #DDDDDD;
    font-family: "Consolas";
    font-size: 14px;
    padding-left: 5px;
    padding-right: 5px;
}
QLineEdit:focus{
    background-color: #BBBBBB;
    border: 0px;
    border-radius: 5px;
    color: #505050;
    font-family: "Consolas";
    font-size: 14px;
    padding-left: 5px;
    padding-right: 5px;
}
QPushButton{
    background-color: #1583F6;
    border-radius: 3px;
    color: #DDDDDD;
    padding: 5px 5px;
    text-align: center;
    font-size: 14px;
    font-family: "Consolas";
}
QPushButton:hover{
    background-color: #2593FF;
    border-radius: 3px;
    color: #DDDDDD;
    padding: 5px 5px;
    text-align: center;
    font-size: 14px;
    font-family: "Consolas";
}
QPushButton:pressed{
    background-color: #0573E6;
    border-radius: 3px;
    color: #DDDDDD;
    padding: 5px 5px;
    text-align: center;
    font-size: 14px;
    font-family: "Consolas";
}
QPushButton:disabled{
    background-color: #666465;
    border-radius: 3px;
    color: #DDDDDD;
    padding: 5px 5px;
    text-align: center;
    font-size: 14px;
    font-family: "Consolas";
}
)"""";

class MainThreadEvent: public QEvent {
	std::function<void(void*)> func;
	void* args;
public:
	MainThreadEvent(const std::function<void(void*)>& func, void* args): QEvent(QEvent::User) {
		this->func = func;
		this->args = args;
	}
	void exec() {
		func(args);
	}
};

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {
	isDragging = false;
	control = new Control([this](ControlState state) {
		ControlState* newState = new ControlState(state);
		QApplication::instance()->postEvent(this, new MainThreadEvent([this](void* args) {
			ControlState* state = (ControlState*)args;
			this->onControlStateChanged(*state);
			delete state;
		}, newState));
	});
	Configs configs = control->load_configs();

	resize(350, 333);
	setWindowTitle("Outernet");
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setWindowIcon(QIcon("res/icon.png"));
	setStyleSheet(QString::fromUtf8(styleSheetStr));

	mainWidget = new QWidget(this);
	mainWidget->setGeometry(7, 7, width() - 14, height() - 14);
	mainWidget->setObjectName("MainWidget");

	shadow = new QGraphicsDropShadowEffect(mainWidget);
	shadow->setBlurRadius(10);
	shadow->setOffset(0, 0);
	shadow->setColor(Qt::black);
	mainWidget->setGraphicsEffect(shadow);

	closeBtn = new QPushButton(this);
	closeBtn->resize(10, 10);
	connect(closeBtn, &QPushButton::clicked, this, &MainWindow::onClose);
	closeBtn->setObjectName("CloseButton");
	closeBtn->move(14, 14);

	minBtn = new QPushButton(this);
	minBtn->resize(10, 10);
	connect(minBtn, &QPushButton::clicked, this, &MainWindow::hide);
	minBtn->setObjectName("MinButton");
	minBtn->move(32, 14);

	titleLabel = new QLabel(QString("Outernet - ") + QString(VERSION_CODE), this);
	titleLabel->setObjectName("TitleLabel");
	titleLabel->adjustSize();
	titleLabel->move(350 / 2 - titleLabel->width() / 2, 14);

	addrLabel = new QLabel("Server address", this);
	addrLabel->resize(112, 20);
	addrLabel->setObjectName("NormalLabel");
	addrLabel->move(30, 50);

	portLabel = new QLabel("Server port", this);
	portLabel->resize(112, 20);
	portLabel->setObjectName("NormalLabel");
	portLabel->move(30, 77);

	userLabel = new QLabel("User name", this);
	userLabel->resize(112, 20);;
	userLabel->setObjectName("NormalLabel");
	userLabel->move(30, 104);

	secretLabel = new QLabel("Secret", this);
	secretLabel->resize(112, 20);
	secretLabel->setObjectName("NormalLabel");
	secretLabel->move(30, 131);

	rxRateLabel = new QLabel("Down rate", this);
	rxRateLabel->resize(112, 20);
	rxRateLabel->setObjectName("NormalLabel");
	rxRateLabel->move(30, 158);

	txRateLabel = new QLabel("Up rate", this);
	txRateLabel->resize(112, 20);
	txRateLabel->setObjectName("NormalLabel");
	txRateLabel->move(30, 185);

	rxTotalLabel = new QLabel("Down total", this);
	rxTotalLabel->resize(112, 20);
	rxTotalLabel->setObjectName("NormalLabel");
	rxTotalLabel->move(30, 212);

	txTotalLabel = new QLabel("Up total", this);
	txTotalLabel->resize(112, 20);
	txTotalLabel->setObjectName("NormalLabel");
	txTotalLabel->move(30, 239);

	addrEdit = new QLineEdit(configs.server_ip.c_str(), this);
	addrEdit->setAlignment(Qt::AlignRight);
	addrEdit->resize(160, 22);
	addrEdit->move(160, 49);

	QString portStr = configs.server_port == 0 ? "" : QString::number(configs.server_port);
	portEdit = new QLineEdit(portStr, this);
	portEdit->setAlignment(Qt::AlignRight);
	portEdit->resize(160, 22);
	portEdit->move(160, 76);

	userEdit = new QLineEdit(configs.username.c_str(), this);
	userEdit->setAlignment(Qt::AlignRight);
	userEdit->resize(160, 22);
	userEdit->move(160, 103);

	secretEdit = new QLineEdit(configs.secret.c_str(), this);
	secretEdit->setAlignment(Qt::AlignRight);
	secretEdit->resize(160, 22);
	secretEdit->move(160, 130);

	rxRateNumberLabel = new QLabel("0.0 MB/s", this);
	rxRateNumberLabel->resize(160, 20);
	rxRateNumberLabel->setAlignment(Qt::AlignRight);
	rxRateNumberLabel->setObjectName("NormalLabel");
	rxRateNumberLabel->move(160, 158);

	txRateNumberLabel = new QLabel("0.0 MB/s", this);
	txRateNumberLabel->resize(160, 20);
	txRateNumberLabel->setAlignment(Qt::AlignRight);
	txRateNumberLabel->setObjectName("NormalLabel");
	txRateNumberLabel->move(160, 185);

	rxTotalNumberLabel = new QLabel("0.0 MB", this);
	rxTotalNumberLabel->resize(160, 20);
	rxTotalNumberLabel->setAlignment(Qt::AlignRight);
	rxTotalNumberLabel->setObjectName("NormalLabel");
	rxTotalNumberLabel->move(160, 212);

	txTotalNumberLabel = new QLabel("0.0 MB", this);
	txTotalNumberLabel->resize(160, 20);
	txTotalNumberLabel->setAlignment(Qt::AlignRight);
	txTotalNumberLabel->setObjectName("NormalLabel");
	txTotalNumberLabel->move(160, 239);

	toggleConnectBtn = new QPushButton("Connect", this);
	connect(toggleConnectBtn, &QPushButton::clicked, this, &MainWindow::onConnectBtnClicked);
	toggleConnectBtn->resize(230, 25);
	toggleConnectBtn->move(29, 278);

	toggleFilterBtn = new QPushButton(this);
	toggleFilterBtn->setIcon(QIcon("res/filter.png"));
	//toggleFilterBtn->clicked.connect(self.toggleFilter)
	toggleFilterBtn->resize(25, 25);
	toggleFilterBtn->move(264, 278);

	viewMainPageBtn = new QPushButton(this);
	viewMainPageBtn->setIcon(QIcon("res/home.png"));
	//viewMainPageBtn->clicked.connect(self.handleViewMainPage)
	viewMainPageBtn->resize(25, 25);
	viewMainPageBtn->move(294, 278);

	trafficTimer = new QTimer();
	//trafficTimer->timeout.connect(self.handleTraffic)
	trafficTimer->start(1000);
}

MainWindow::~MainWindow() {
	delete control;
}

bool MainWindow::event(QEvent *qevent) {
	// handle main thread event
	if (qevent->type() == QEvent::User) {
		MainThreadEvent* mainThreadEvent = (MainThreadEvent*)qevent;
		mainThreadEvent->exec();
		return true;
	}
	// handle window dragging
	else if (qevent->type() == QEvent::MouseButtonPress) {
		QMouseEvent* mouseEvent = (QMouseEvent*)qevent;
		if (mouseEvent->button() == Qt::LeftButton) {
			isDragging = true;
			dragPos = mouseEvent->globalPos() - pos();
			mouseEvent->accept();
			setCursor(QCursor(Qt::OpenHandCursor));
		}
	}
	else if (qevent->type() == QEvent::MouseMove) {
		QMouseEvent* mouseEvent = (QMouseEvent*)qevent;
		if (isDragging) {
			move(mouseEvent->globalPos() - dragPos);
			mouseEvent->accept();
		}
	}
	else if (qevent->type() == QEvent::MouseButtonRelease) {
		QMouseEvent* mouseEvent = (QMouseEvent*)qevent;
		if (mouseEvent->button() == Qt::LeftButton) {
			isDragging = false;
			setCursor(QCursor(Qt::ArrowCursor));
		}
	}
	return QWidget::event(qevent);
}

void MainWindow::onControlStateChanged(ControlState state) {
	controlState = state;
	if (state.type == CONTROL_STATE_DISCONNECTED) {
		closeBtn->setDisabled(false);
		minBtn->setDisabled(false);
		addrEdit->setDisabled(false);
		portEdit->setDisabled(false);
		userEdit->setDisabled(false);
		secretEdit->setDisabled(false);
		toggleConnectBtn->setDisabled(false);
		toggleFilterBtn->setDisabled(false);
		viewMainPageBtn->setDisabled(false);

		toggleConnectBtn->setText("Connect");
	}
	else if (state.type == CONTROL_STATE_CONNECTED) {
		closeBtn->setDisabled(true);
		minBtn->setDisabled(true);
		addrEdit->setDisabled(true);
		portEdit->setDisabled(true);
		userEdit->setDisabled(true);
		secretEdit->setDisabled(true);
		toggleConnectBtn->setDisabled(false);
		toggleFilterBtn->setDisabled(true);
		viewMainPageBtn->setDisabled(false);

		toggleConnectBtn->setText("Disconnect");
	}
	else if (state.type == CONTROL_STATE_CONNECTING) {
		closeBtn->setDisabled(true);
		minBtn->setDisabled(true);
		addrEdit->setDisabled(true);
		portEdit->setDisabled(true);
		userEdit->setDisabled(true);
		secretEdit->setDisabled(true);
		toggleConnectBtn->setDisabled(true);
		toggleFilterBtn->setDisabled(true);
		viewMainPageBtn->setDisabled(false);

		toggleConnectBtn->setText("Connecting...");
	}
	else if (state.type == CONTROL_STATE_SETTING_UP) {
		closeBtn->setDisabled(true);
		minBtn->setDisabled(true);
		addrEdit->setDisabled(true);
		portEdit->setDisabled(true);
		userEdit->setDisabled(true);
		secretEdit->setDisabled(true);
		toggleConnectBtn->setDisabled(true);
		toggleFilterBtn->setDisabled(true);
		viewMainPageBtn->setDisabled(false);

		toggleConnectBtn->setText("Setting up network...");
	}
	else if (state.type == CONTROL_STATE_ADDING_ROUTE) {
		closeBtn->setDisabled(true);
		minBtn->setDisabled(true);
		addrEdit->setDisabled(true);
		portEdit->setDisabled(true);
		userEdit->setDisabled(true);
		secretEdit->setDisabled(true);
		toggleConnectBtn->setDisabled(true);
		toggleFilterBtn->setDisabled(true);
		viewMainPageBtn->setDisabled(false);

		toggleConnectBtn->setText("Adding routes...");
	}
	else if (state.type == CONTROL_STATE_DISCONNECTING) {
		closeBtn->setDisabled(true);
		minBtn->setDisabled(true);
		addrEdit->setDisabled(true);
		portEdit->setDisabled(true);
		userEdit->setDisabled(true);
		secretEdit->setDisabled(true);
		toggleConnectBtn->setDisabled(true);
		toggleFilterBtn->setDisabled(true);
		viewMainPageBtn->setDisabled(false);

		toggleConnectBtn->setText("Disconnecting...");
	}
	else if (state.type == CONTROL_STATE_DELETING_ROUTE) {
		closeBtn->setDisabled(true);
		minBtn->setDisabled(true);
		addrEdit->setDisabled(true);
		portEdit->setDisabled(true);
		userEdit->setDisabled(true);
		secretEdit->setDisabled(true);
		toggleConnectBtn->setDisabled(true);
		toggleFilterBtn->setDisabled(true);
		viewMainPageBtn->setDisabled(false);

		toggleConnectBtn->setText("Deleting routes...");
	}
	else if (state.type == CONTROL_STATE_TEARING_DOWN) {
		closeBtn->setDisabled(true);
		minBtn->setDisabled(true);
		addrEdit->setDisabled(true);
		portEdit->setDisabled(true);
		userEdit->setDisabled(true);
		secretEdit->setDisabled(true);
		toggleConnectBtn->setDisabled(true);
		toggleFilterBtn->setDisabled(true);
		viewMainPageBtn->setDisabled(false);

		toggleConnectBtn->setText("Tearing down...");
	}
	else if (state.type == CONTROL_STATE_ERROR) {
		QMessageBox::warning(this, "error", state.msg.c_str(), nullptr, nullptr);
	}
}

void MainWindow::onClose() {
	show();
	if (controlState.type != CONTROL_STATE_DISCONNECTED) {
		QMessageBox::warning(this, "warning", "cannot exit during connection, please disconnect first.", nullptr, nullptr);
		return;
	}
	close();
}

void MainWindow::onConnectBtnClicked() {
	if (controlState.type == CONTROL_STATE_DISCONNECTED) {
		Configs configs;
		configs.server_ip = addrEdit->text().toStdString();
		int port = 0;
		try {
			std::string portStr = portEdit->text().toStdString();
			configs.server_port = std::stoi(portStr);
		}
		catch (std::exception const & e) {
			QMessageBox::warning(this, "warning", "invalid port.", nullptr, nullptr);
			return;
		}
		configs.username = userEdit->text().toStdString();
		configs.secret = secretEdit->text().toStdString();
		// save configs
		control->save_configs(configs);
		// start connecting
		control->start(configs);
	}
	else if (controlState.type == CONTROL_STATE_CONNECTED) {
		control->stop();
	}
}

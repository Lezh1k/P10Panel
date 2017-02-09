#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDesktopWidget>

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <QStandardItem>
#include <QStandardItemModel>
#include <QThread>
#include <QtConcurrent/QtConcurrent>

static const char* g_time_format = "mm:ss";
//todo move files to resources.
static const char* g_alarm_file = "qrc:/wav/files/alarm.wav";
static const char* g_start_file = "qrc:/wav/files/start.wav";
static const char* g_stop_file  = "qrc:/wav/files/stop.wav";
static const int TX_BUFFER_SIZE = 5;
static char g_tx_buffer[TX_BUFFER_SIZE] = {0};

QMediaPlayer MainWindow::m_player;

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  m_state(AS_IDLE),
  m_time_full(240*1000),
  m_time_alarm(60*1000),
  m_time_rotate(15*1000),
  m_serial_port(nullptr),
  m_model_ports(nullptr) {

  ui->setupUi(this);

  ui->m_te_full_time->setTime(QTime::fromMSecsSinceStartOfDay(m_time_full));
  ui->m_te_ring_time->setTime(QTime::fromMSecsSinceStartOfDay(m_time_alarm));
  ui->m_te_rotate_time->setTime(QTime::fromMSecsSinceStartOfDay(m_time_rotate));

  m_state_callbacks[AS_IDLE] = &MainWindow::Start;
  m_state_callbacks[AS_RUN] = &MainWindow::Stop;
  m_btn_text[AS_IDLE] = "Start";
  m_btn_text[AS_RUN] = "Stop";

  m_main_timer.setInterval(MAIN_TIMER_INTERVAL);
  m_rotate_timer.setInterval(m_time_rotate);

  ui->m_lbl_current_time->setText(ui->m_te_full_time->time().toString(g_time_format));
  m_player.setVolume(100);  

  m_model_ports = new QStandardItemModel;
  for (int i = 0; i < QSerialPortInfo::availablePorts().size(); ++i) {
    QStandardItem* item = new QStandardItem(QSerialPortInfo::availablePorts().at(i).portName());
    m_model_ports->appendRow(item);
  }
  ui->m_cb_serial_port->setModel(m_model_ports);

  connect(ui->m_chk_loop, SIGNAL(toggled(bool)),
          this, SLOT(ChkLoop_Changed(bool)));
  connect(ui->m_btn_start_stop, SIGNAL(pressed()),
          this, SLOT(BtnStartStop_Clicked()));
  connect(&m_main_timer, SIGNAL(timeout()),
          this, SLOT(MainTimer_Timeout()));
  connect(ui->m_te_full_time, SIGNAL(timeChanged(QTime)),
          this, SLOT(TimeEdit_TimeChanged(QTime)));
  connect(&m_rotate_timer, SIGNAL(timeout()),
          this, SLOT(RotateTimer_Timeout()));
  connect(ui->m_cb_serial_port, SIGNAL(currentIndexChanged(int)),
          this, SLOT(CbPorts_IndexChanged(int)));

  if (QSerialPortInfo::availablePorts().size() > 0) {
    CbPorts_IndexChanged(0);
  }

  g_tx_buffer[TX_BUFFER_SIZE-1] = 1;
  set_current_time_text();
  ui->m_lbl_error->setVisible(false);

  //UGLY HACK N1. I don't know how to adjust font size.
  //so I'm waiting here 200ms for constuction finishing and
  //then label is resized by layout. So I need to adjust font
  //size in that moment. Whoever knows how to adjust font size
  //right before form is shown - please send me this info to
  //lezh1k.vohrer@gmail.com.
  QtConcurrent::run([&](){
    QThread::msleep(200);
    adjust_font_size();
  });
}
/////////////////////////////////////////////////////////////////////////

MainWindow::~MainWindow() {
  delete ui;
  if (m_model_ports) delete m_model_ports;
  if (m_serial_port) delete m_serial_port;
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  adjust_font_size();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::Start() {
  adjust_font_size();
  m_player.stop();
  m_rotate_timer.stop();
  m_time_full = ui->m_te_full_time->time().msecsSinceStartOfDay();
  m_time_alarm = ui->m_te_ring_time->time().msecsSinceStartOfDay();
  m_time_rotate = ui->m_te_rotate_time->time().msecsSinceStartOfDay();
  m_rotate_timer.setInterval(m_time_rotate);
  set_current_time_text();
  m_main_timer.start();
  m_player.setMedia(QUrl(g_start_file));
  m_player.play();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::Stop() {
  m_player.stop();
  m_rotate_timer.stop();
  m_main_timer.stop();
  set_current_time_text();
}
//////////////////////////////////////////////////////////////

void
MainWindow::change_controls_enabled_state() {
  bool ce = m_state == AS_IDLE; //controls enabled
  ui->m_te_full_time->setEnabled(ce);
  ui->m_te_ring_time->setEnabled(ce);
  ui->m_te_rotate_time->setEnabled(ce &&
                                   ui->m_chk_loop->isChecked());
  ui->m_chk_loop->setEnabled(ce);
  ui->m_cb_serial_port->setEnabled(ce);
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::BtnStartStop_Clicked() {
  AS_STATE st = m_state;
  m_state = (AS_STATE)((st+1)%(AS_LAST));
  ui->m_btn_start_stop->setText(m_btn_text[m_state]);
  change_controls_enabled_state();
  (this->*m_state_callbacks[st])();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::time_elapsed() {
  m_player.stop();
  m_player.setMedia(QUrl(g_stop_file));
  m_player.play();
  m_main_timer.stop();
  if (!ui->m_chk_loop->isChecked()) {
    BtnStartStop_Clicked();
    return;
  }
  m_rotate_timer.start();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::set_current_time_text() {
  QString time_str =
      QTime::fromMSecsSinceStartOfDay(m_time_full).toString(g_time_format);
  static const int indexes[] = {0, 1, 3, 4};
  for (int i = 0; i < 4; ++i)
    g_tx_buffer[i] = time_str[indexes[i]].digitValue();
  ui->m_lbl_current_time->setText(time_str);
  send_to_p10();
}
//////////////////////////////////////////////////////////////

void MainWindow::send_to_p10() {
  if (!m_serial_port->isOpen()) return;
  m_serial_port->write(g_tx_buffer, TX_BUFFER_SIZE);
  m_serial_port->flush();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::ChkLoop_Changed(bool checked) {
  ui->m_te_rotate_time->setEnabled(checked);
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::MainTimer_Timeout() {
  m_time_full -= MAIN_TIMER_INTERVAL;

  if (m_time_full == m_time_alarm) {
    m_player.setMedia(QUrl(g_alarm_file));
    m_player.play();
  }
  if (m_time_full <= 0) time_elapsed();
  set_current_time_text();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::RotateTimer_Timeout() {
  Start();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::TimeEdit_TimeChanged(const QTime& ) {
  m_time_full = ui->m_te_full_time->time().msecsSinceStartOfDay();
  set_current_time_text();
}
//////////////////////////////////////////////////////////////

void MainWindow::CbPorts_IndexChanged(int ix) {
  if (m_serial_port) delete m_serial_port;
  ui->m_lbl_error->setVisible(false);
  m_serial_port = new QSerialPort(QSerialPortInfo::availablePorts().at(ix));
  m_serial_port->setBaudRate(9600);
  m_serial_port->setParity(QSerialPort::NoParity);
  m_serial_port->setDataBits(QSerialPort::Data8);
  m_serial_port->setStopBits(QSerialPort::OneStop);
  m_serial_port->setFlowControl(QSerialPort::NoFlowControl);
  if (!m_serial_port->open(QSerialPort::ReadWrite)) {
    ui->m_lbl_error->setVisible(true);
    ui->m_lbl_error->setText(m_serial_port->errorString());
  }
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::adjust_font_size() {
  QString str = ui->m_lbl_current_time->text();
  QFont font = ui->m_lbl_current_time->font();
  QFontMetrics fm(font);
  int fontSize = -1;
  do {
    ++fontSize;
    font.setPointSize(fontSize + 1);
    fm = QFontMetrics(font);
  } while(fm.height() <= ui->m_lbl_current_time->height() &&
          fm.width(str) < ui->m_lbl_current_time->width());

  font.setPointSize(fontSize);
  ui->m_lbl_current_time->setFont(font);  
}
/////////////////////////////////////////////////////////////////////////

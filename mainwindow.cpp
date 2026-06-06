#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QMenuBar>
#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QKeyEvent>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    sessionTimer = new QTimer(this);
    connect(sessionTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);

    setupUI();
    loadLessonsList();
    loadSettings();
}

void MainWindow::setupUI() {
    setWindowTitle("TypingTrainer");
    resize(900, 650);

    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *exitAction = fileMenu->addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *settingsMenu = menuBar()->addMenu("Settings");
    QAction *resetAction = settingsMenu->addAction("Reset Session");
    connect(resetAction, &QAction::triggered, this, &MainWindow::restartTraining);

    QMenu *helpMenu = menuBar()->addMenu("Help");
    QAction *aboutAction = helpMenu->addAction("About");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About", "TypingTrainer\nSimple keyboard training application.");
    });

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    stackedPages = new QStackedWidget(this);
    mainLayout->addWidget(stackedPages);

    stackedPages->addWidget(createStartPage());
    stackedPages->addWidget(createTrainingPage());
    stackedPages->addWidget(createResultsPage());
}

QWidget* MainWindow::createStartPage() {
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(50, 50, 50, 50);
    layout->setSpacing(18);

    QLabel *title = new QLabel("Typing Trainer", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 24px; font-weight: bold;");

    QLabel *lessonLabel = new QLabel("Select lesson:", this);
    lessonLabel->setAlignment(Qt::AlignCenter);

    comboLesson = new QComboBox(this);
    comboLesson->setMinimumHeight(36);

    QLabel *speedLabel = new QLabel("Speed metric:", this);
    speedLabel->setAlignment(Qt::AlignCenter);

    comboSpeedMode = new QComboBox(this);
    comboSpeedMode->addItems({"CPM", "WPM"});
    comboSpeedMode->setMinimumHeight(36);

    labelLessonDescription = new QLabel("Lesson information will be shown here.", this);
    labelLessonDescription->setAlignment(Qt::AlignCenter);
    labelLessonDescription->setFrameStyle(QFrame::StyledPanel);
    labelLessonDescription->setMinimumHeight(90);

    buttonStartTraining = new QPushButton("Start Training", this);
    buttonStartTraining->setMinimumHeight(45);

    connect(comboLesson, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        currentLessonIndex = index;
        if (lessons.contains(index)) {
            labelLessonDescription->setText(
                QString("Selected lesson: %1\nCharacters: %2")
                    .arg(lessons[index].title)
                    .arg(lessons[index].fullText.length())
                );
        }
        saveSettings();
    });

    connect(comboSpeedMode, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        updateMetrics();
        saveSettings();
    });

    connect(buttonStartTraining, &QPushButton::clicked, this, &MainWindow::startTraining);

    layout->addStretch();
    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(lessonLabel);
    layout->addWidget(comboLesson);
    layout->addWidget(speedLabel);
    layout->addWidget(comboSpeedMode);
    layout->addWidget(labelLessonDescription);
    layout->addSpacing(20);
    layout->addWidget(buttonStartTraining);
    layout->addStretch();

    return page;
}

QWidget* MainWindow::createTrainingPage() {
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    QHBoxLayout *topPanel = new QHBoxLayout();

    labelTimeValue = new QLabel("00:00", this);
    labelSpeedValue = new QLabel("0 CPM", this);
    labelAccuracyValue = new QLabel("100%", this);

    topPanel->addStretch();
    topPanel->addWidget(new QLabel("Time:", this));
    topPanel->addWidget(labelTimeValue);
    topPanel->addSpacing(30);
    topPanel->addWidget(new QLabel("Speed:", this));
    topPanel->addWidget(labelSpeedValue);
    topPanel->addSpacing(30);
    topPanel->addWidget(new QLabel("Accuracy:", this));
    topPanel->addWidget(labelAccuracyValue);
    topPanel->addStretch();

    textDisplay = new QTextEdit(this);
    textDisplay->setReadOnly(true);
    textDisplay->setMinimumHeight(180);
    textDisplay->setFocusPolicy(Qt::NoFocus);

    QFrame *keyboardFrame = new QFrame(this);
    keyboardFrame->setFrameStyle(QFrame::StyledPanel);
    QVBoxLayout *keyboardLayout = new QVBoxLayout(keyboardFrame);

    QStringList rows = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"};

    for (const QString &row : rows) {
        QHBoxLayout *rowLayout = new QHBoxLayout();
        rowLayout->addStretch();

        for (QChar ch : row) {
            QPushButton *button = new QPushButton(QString(ch), this);
            button->setFixedSize(42, 36);
            button->setFocusPolicy(Qt::NoFocus);
            rowLayout->addWidget(button);
        }

        rowLayout->addStretch();
        keyboardLayout->addLayout(rowLayout);
    }

    QHBoxLayout *bottomButtons = new QHBoxLayout();

    buttonRestartTraining = new QPushButton("Restart Training", this);
    buttonReturnToMain = new QPushButton("Return to Main", this);

    connect(buttonRestartTraining, &QPushButton::clicked, this, &MainWindow::restartTraining);
    connect(buttonReturnToMain, &QPushButton::clicked, this, &MainWindow::returnToMain);

    bottomButtons->addStretch();
    bottomButtons->addWidget(buttonRestartTraining);
    bottomButtons->addWidget(buttonReturnToMain);
    bottomButtons->addStretch();

    layout->addLayout(topPanel);
    layout->addWidget(textDisplay);
    layout->addWidget(keyboardFrame);
    layout->addLayout(bottomButtons);

    return page;
}

QWidget* MainWindow::createResultsPage() {
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(50, 50, 50, 50);
    layout->setSpacing(20);

    QLabel *title = new QLabel("Session Results", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 24px; font-weight: bold;");

    labelFinalTime = new QLabel("00:00", this);
    labelFinalSpeed = new QLabel("0 CPM", this);
    labelFinalAccuracy = new QLabel("100%", this);

    labelFinalTime->setAlignment(Qt::AlignCenter);
    labelFinalSpeed->setAlignment(Qt::AlignCenter);
    labelFinalAccuracy->setAlignment(Qt::AlignCenter);

    QPushButton *restartButton = new QPushButton("Restart Training", this);
    QPushButton *mainButton = new QPushButton("Return to Main", this);

    connect(restartButton, &QPushButton::clicked, this, &MainWindow::restartTraining);
    connect(mainButton, &QPushButton::clicked, this, &MainWindow::returnToMain);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(restartButton);
    buttons->addWidget(mainButton);
    buttons->addStretch();

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(new QLabel("Time:", this));
    layout->addWidget(labelFinalTime);
    layout->addWidget(new QLabel("Speed:", this));
    layout->addWidget(labelFinalSpeed);
    layout->addWidget(new QLabel("Accuracy:", this));
    layout->addWidget(labelFinalAccuracy);
    layout->addSpacing(20);
    layout->addLayout(buttons);
    layout->addStretch();

    return page;
}

void MainWindow::loadLessonsList() {
    comboLesson->clear();
    lessons.clear();

    QString lessonsPath = qApp->applicationDirPath() + "/lessons";
    QDir dir(lessonsPath);

    if (dir.exists()) {
        QFileInfoList files = dir.entryInfoList(QStringList() << "*.txt", QDir::Files);

        int index = 0;
        for (const QFileInfo &fileInfo : files) {
            comboLesson->addItem(fileInfo.baseName(), fileInfo.absoluteFilePath());
            loadLessonFromFile(fileInfo.absoluteFilePath());
            index++;
        }
    }

    if (lessons.isEmpty()) {
        addDefaultLessons();
    }

    buttonStartTraining->setEnabled(!lessons.isEmpty());
}

void MainWindow::addDefaultLessons() {
    TypingLesson lesson1;
    lesson1.title = "Starter Text";
    lesson1.fullText = "fjf jjf ffjjj fjfjf\nasdf jkl asdf jkl\nquick brown fox";
    lesson1.lines = lesson1.fullText.split('\n', Qt::SkipEmptyParts);

    TypingLesson lesson2;
    lesson2.title = "Common Words";
    lesson2.fullText = "hello world typing practice\nsimple words and letters\ntraining is useful";
    lesson2.lines = lesson2.fullText.split('\n', Qt::SkipEmptyParts);

    TypingLesson lesson3;
    lesson3.title = "Numbers";
    lesson3.fullText = "123 456 789 0\n2026 100 55 42\n1 2 3 4 5";
    lesson3.lines = lesson3.fullText.split('\n', Qt::SkipEmptyParts);

    lessons[0] = lesson1;
    lessons[1] = lesson2;
    lessons[2] = lesson3;

    comboLesson->addItem(lesson1.title);
    comboLesson->addItem(lesson2.title);
    comboLesson->addItem(lesson3.title);

    comboLesson->setCurrentIndex(0);
    labelLessonDescription->setText("Built-in lessons are loaded.");
}

void MainWindow::loadLessonFromFile(const QString &filePath) {
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    QString text = in.readAll();

    TypingLesson lesson;
    lesson.title = QFileInfo(filePath).baseName();
    lesson.fullText = text;
    lesson.lines = text.split('\n', Qt::SkipEmptyParts);

    int index = comboLesson->count() - 1;
    lessons[index] = lesson;
}

void MainWindow::startTraining() {
    currentLessonIndex = comboLesson->currentIndex();

    if (!lessons.contains(currentLessonIndex)) {
        QMessageBox::warning(this, "TypingTrainer", "No lesson selected.");
        return;
    }

    resetSession();
    updateTrainingDisplay();
    updateMetrics();

    stackedPages->setCurrentIndex(1);
    sessionTimer->start(1000);
    setFocus();
    saveSettings();
}

void MainWindow::restartTraining() {
    sessionTimer->stop();
    resetSession();
    updateTrainingDisplay();
    updateMetrics();

    stackedPages->setCurrentIndex(1);
    sessionTimer->start(1000);
    setFocus();
}

void MainWindow::returnToMain() {
    sessionTimer->stop();
    resetSession();
    stackedPages->setCurrentIndex(0);
    saveSettings();
}

void MainWindow::resetSession() {
    secondsElapsed = 0;
    totalCharsTyped = 0;
    correctCharsTyped = 0;
    lineIndex = 0;
    charIndex = 0;

    labelTimeValue->setText("00:00");
    labelSpeedValue->setText(comboSpeedMode->currentText() == "WPM" ? "0 WPM" : "0 CPM");
    labelAccuracyValue->setText("100%");
}

void MainWindow::onTimerTick() {
    secondsElapsed++;
    labelTimeValue->setText(formatTime(secondsElapsed));
    updateMetrics();
}

void MainWindow::updateMetrics() {
    int speed = 0;

    if (secondsElapsed > 0) {
        if (comboSpeedMode->currentText() == "WPM") {
            speed = ((totalCharsTyped / 5) * 60) / secondsElapsed;
            labelSpeedValue->setText(QString("%1 WPM").arg(speed));
        } else {
            speed = (totalCharsTyped * 60) / secondsElapsed;
            labelSpeedValue->setText(QString("%1 CPM").arg(speed));
        }
    }

    int accuracy = 100;

    if (totalCharsTyped > 0) {
        accuracy = static_cast<int>((static_cast<double>(correctCharsTyped) / totalCharsTyped) * 100.0);
    }

    labelAccuracyValue->setText(QString("%1%").arg(accuracy));
}

void MainWindow::updateTrainingDisplay() {
    if (!lessons.contains(currentLessonIndex)) {
        return;
    }

    TypingLesson lesson = lessons[currentLessonIndex];

    if (lineIndex >= lesson.lines.count()) {
        finishTraining();
        return;
    }

    QString line = lesson.lines[lineIndex];

    QString typed = line.left(charIndex);
    QString current = charIndex < line.length() ? line.mid(charIndex, 1) : "↵";
    QString rest = charIndex < line.length() ? line.mid(charIndex + 1) : "";

    QString html =
        "<div style='font-family: monospace; font-size: 22px;'>"
        "<span style='color: #4CAF50;'>" + typed.toHtmlEscaped() + "</span>"
                                  "<span style='background-color: #FFD966; color: black;'>" + current.toHtmlEscaped() + "</span>"
                                    "<span>" + rest.toHtmlEscaped() + "</span>"
                                 "</div>";

    textDisplay->setHtml(html);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (stackedPages->currentIndex() != 1) {
        return;
    }

    if (!lessons.contains(currentLessonIndex)) {
        return;
    }

    TypingLesson lesson = lessons[currentLessonIndex];

    if (lineIndex >= lesson.lines.count()) {
        finishTraining();
        return;
    }

    QString line = lesson.lines[lineIndex];

    if (event->key() == Qt::Key_Backspace) {
        if (charIndex > 0) {
            charIndex--;
        }

        updateTrainingDisplay();
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (charIndex >= line.length()) {
            lineIndex++;
            charIndex = 0;
        }

        updateTrainingDisplay();
        return;
    }

    QString input = event->text();

    if (input.isEmpty()) {
        return;
    }

    totalCharsTyped++;

    if (charIndex < line.length() && input[0] == line[charIndex]) {
        correctCharsTyped++;
    }

    charIndex++;

    if (charIndex > line.length()) {
        lineIndex++;
        charIndex = 0;
    }

    updateMetrics();

    if (lineIndex >= lesson.lines.count()) {
        finishTraining();
    } else {
        updateTrainingDisplay();
    }
}

void MainWindow::finishTraining() {
    sessionTimer->stop();

    labelFinalTime->setText(labelTimeValue->text());
    labelFinalSpeed->setText(labelSpeedValue->text());
    labelFinalAccuracy->setText(labelAccuracyValue->text());

    stackedPages->setCurrentIndex(2);
}

QString MainWindow::formatTime(int seconds) const {
    int minutes = seconds / 60;
    int secs = seconds % 60;

    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}

void MainWindow::saveSettings() {
    QSettings settings("StudentProjects", "TypingTrainer");
    settings.setValue("lastLesson", comboLesson->currentText());
    settings.setValue("speedMode", comboSpeedMode->currentText());
}

void MainWindow::loadSettings() {
    QSettings settings("StudentProjects", "TypingTrainer");

    QString lastLesson = settings.value("lastLesson", "").toString();
    QString speedMode = settings.value("speedMode", "CPM").toString();

    int lessonIndex = comboLesson->findText(lastLesson);
    if (lessonIndex >= 0) {
        comboLesson->setCurrentIndex(lessonIndex);
    }

    int speedIndex = comboSpeedMode->findText(speedMode);
    if (speedIndex >= 0) {
        comboSpeedMode->setCurrentIndex(speedIndex);
    }
}